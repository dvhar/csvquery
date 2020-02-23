#include "interpretor.h"


//what type results from operation with 2 expressions with various data types and column/literal source
//don't use for types where different operation results in different type
//null[c,l], int[c,l], float[c,l], date[c,l], duration[c,l], string[c,l] in both dimensions
static int typeChart[12][12] = {
	{5,5, 5,5, 5,5, 5,5, 5,5, 5,5},
	{5,5, 1,1, 2,2, 3,3, 4,4, 5,5},
	{5,1, 1,1, 2,2, 3,1, 4,4, 5,1},
	{5,1, 1,1, 2,2, 3,1, 4,4, 5,5},
	{5,2, 2,2, 2,2, 3,2, 4,2, 5,2},
	{5,2, 2,2, 2,2, 3,2, 4,4, 5,5},
	{5,3, 3,3, 3,3, 3,3, 3,3, 3,3},
	{5,3, 1,1, 2,2, 3,3, 3,3, 5,5},
	{5,4, 4,4, 4,4, 3,3, 4,4, 5,4},
	{5,4, 4,4, 2,4, 3,3, 4,4, 5,5},
	{5,5, 5,5, 5,5, 3,5, 5,5, 5,5},
	{5,5, 1,5, 2,5, 3,5, 4,5, 5,5}
};

class typer {
	public:
	int type; //datatype
	int lit; //literal - int for quicker writing
};
class ktype {
	public:
	int type; //datatype
	bool keep; //keep subtree types
};
static typer typeInnerNodes(querySpecs &q, unique_ptr<node> &n);
static void typeFinalValues(querySpecs &q, unique_ptr<node> &n, int finaltype);

static typer typeCompute(typer n1, typer n2){
	if (!n2.type) return n1;
	if (!n1.type) return n2;
	int i1 = 2*n1.type;
	int i2 = 2*n2.type;
	if (n1.lit) { i1++;}
	if (n2.lit) { i2++;}
	return { typeChart[i1][i2], n1.lit & n2.lit };
}

static ktype keepSubtreeTypes(int t1, int t2, int op) {
	switch (op) {
	case SP_STAR:
	case SP_DIV:
		if ((t1 == T_DURATION && t2 == T_INT) ||
		   (t1 == T_DURATION && t2 == T_FLOAT) ||
		   (t2 == T_DURATION && t1 == T_INT) ||
		   (t2 == T_DURATION && t1 == T_FLOAT)) return { T_DURATION, true };
		   break;
	case SP_MINUS:
		if (t1 == T_DATE && t2 == T_DATE) return { T_DURATION, true };
		if (t1 == T_DATE && t2 == T_DURATION) return { T_DATE, true };
		break;
	case SP_PLUS:
		if ((t1 == T_DURATION && t2 == T_DATE) ||
		   (t2 == T_DURATION && t1 == T_DATE)) return { T_DATE, true };
		break;
	}
	return {0, false};
}

//see if using string type would interfere with operations
static bool canBeString(unique_ptr<node> &n){
	if (n == nullptr) return true;

	switch (n->label){
	case N_CWEXPR:
	case N_CPRED:
		return canBeString(n->node2);
	case N_EXPRNEG:
	case N_EXPRADD:
	case N_EXPRMULT:
		if (n->tok1.id)
			return false;
		else
			return canBeString(n->node1);
		break;
	case N_DEXPRESSIONS:
	case N_CWEXPRLIST:
	case N_CPREDLIST:
		return canBeString(n->node1) && canBeString(n->node2);
	case N_EXPRCASE:
		switch (n->tok1.id){
		case KW_CASE:
			switch (n->tok1.id){
			case KW_WHEN:
				return canBeString(n->node1) && canBeString(n->node3);
			default:
				return canBeString(n->node2) && canBeString(n->node3);
			}
		default:
			return canBeString(n->node1);
		}
		break;
	case N_VALUE:
		if (n->tok2.id)
			return canBeString(n->node1);
		return true;
	case N_FUNCTION:
		switch (n->tok1.id){
		case FN_COALESCE:
			return canBeString(n->node1);
		case FN_ENCRYPT:
		case FN_DECRYPT:
			return true;
		case FN_INC:
		case FN_COUNT:
			return false;
		}
	}
	error("canBeString() function malfunctioned");
	return false;
}

//see if a node involves operation that requires specific type
static bool stillTrivial(unique_ptr<node> &n){
	switch (n->label){
	case N_EXPRADD:
	case N_EXPRMULT:
	case N_EXPRNEG:
		if (n->tok1.id != 0)
			return false;
		break;
	case N_EXPRCASE:
		if (n->tok1.id != WORD_TK)
			return false;
		break;
	case N_VALUE:
		if (n->tok2.id != COLUMN && n->tok2.id != LITERAL)
			return false;
		break;
	}
	return true;
}

//give value nodes their initial type and look out for variables
static void typeInitialValue(querySpecs &q, unique_ptr<node> &n, bool trivial){
	if (n == nullptr)
		return;

	string thisVar;
	string r;
	if (n->label == N_VARS){
		thisVar = n->tok1.val;
	}

	//only do leaf nodes
	if (n->label == N_VALUE && n->tok2.id != FUNCTION){
		string val = n->tok1.val;
		int period;
		//see if variable
		for (auto &v : q.vars)
			if (val == v.name){
				n->tok2.id = VARIABLE;
				goto donetyping;
			}
		//see if file alias
		period = val.find_first_of(".");
		int i;
		if (period != -1){
			string alias = val.substr(0, period);
			if (q.files.count(alias)){
				string column = val.substr(period+1, val.length()-period);
				shared_ptr<fileReader> f = q.files[alias];
				i = f->getColIdx(column);
				if (i != -1){
					//found column name with file alias
					n->tok1.id = i;
					n->tok2.id = COLUMN;
					n->tok3.val = alias;
					n->datatype = f->types[i];
					goto donetyping;
				} else if (!n->tok1.quoted && regex_match(column, cInt)){
					i = stoi(column.substr(1))-1;
					if (i <= f->numFields){
						//found column idx with file alias
						n->tok1.id = i;
						n->tok2.id = COLUMN;
						n->tok3.val = alias;
						n->datatype = f->types[i];
						goto donetyping;
					}
				} else if (!n->tok1.quoted && q.numIsCol() && regex_match(column, posInt)){
					i = stoi(column)-1;
					if (i <= f->numFields){
						//found column idx with file alias
						n->tok1.id = i;
						n->tok2.id = COLUMN;
						n->tok3.val = alias;
						n->datatype = f->types[i];
						goto donetyping;
					}
				}
			}
		}
		//no file alias
		for (auto &f : q.files){
			i = f.second->getColIdx(val);
			if (i != -1){
				//found column name without file alias
				n->tok1.id = i;
				n->tok2.id = COLUMN;
				n->tok3.val = f.second->id;
				n->datatype = f.second->types[i];
				goto donetyping;
			}
		}
		if (!n->tok1.quoted && regex_match(val, cInt)){
			i = stoi(val.substr(1))-1;
			auto f = q.files["_f1"];
			if (i <= f->numFields){
				//found column idx without file alias
				n->tok1.id = i;
				n->tok2.id = COLUMN;
				n->tok3.val = "_f1";
				n->datatype = f->types[i];
				goto donetyping;
			}
		} else if (!n->tok1.quoted && q.numIsCol() && regex_match(val, posInt)){
			i = stoi(val)-1;
			auto f = q.files["_f1"];
			if (i <= f->numFields){
				//found column idx without file alias
				n->tok1.id = i;
				n->tok2.id = COLUMN;
				n->tok3.val = "_f1";
				n->datatype = f->types[i];
				goto donetyping;
			}
		}
		//is not a var, function, or column, must be literal
		n->tok2.id = LITERAL;
		n->datatype = getNarrowestType((char*)val.c_str(), 0);
	}
	donetyping:;
	//cerr << "typed " << n->tok1.val << " as " << n->datatype << endl;

	//see if selecting trivial value (just column or literal)
	if (n->label == N_SELECTIONS)       trivial = true;
	if (trivial && !stillTrivial(n))    trivial = false;
	if (trivial && n->label == N_VALUE && n->tok2.id == COLUMN) {
		n->datatype = T_STRING;
		n->tok3.id = 1;
	}

	typeInitialValue(q, n->node1, trivial);
	//record variable after typing var leafnodes to avoid recursive definition
	if (n->label == N_VARS)             q.addVar(thisVar);
	//reset trivial indicator for next selection
	if (n->label == N_SELECTIONS)       trivial = false;
	typeInitialValue(q, n->node2, trivial);
	typeInitialValue(q, n->node3, trivial);
	typeInitialValue(q, n->node4, trivial);

}

static typer typeCaseInnerNodes(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer innerType, elseExpr, whenExpr, thenExpr, compExpr;
	switch (n->tok1.id){
	//case statement
	case KW_CASE:
		switch (n->tok2.id){
		//when predicates are true
		case KW_WHEN:
			thenExpr  = typeInnerNodes(q,n->node1);
			elseExpr  = typeInnerNodes(q,n->node3);
			innerType = typeCompute(thenExpr, elseExpr);
			return innerType;
		//expression matches expression list
		case WORD_TK:
		case SP_LPAREN:
			compExpr  = typeInnerNodes(q, n->node1);
			thenExpr  = typeInnerNodes(q, n->node2);
			elseExpr  = typeInnerNodes(q, n->node3);
			innerType = typeCompute(thenExpr, elseExpr);
			node* list = n->node2.get();
			for (auto nn=list; nn; nn=nn->node2.get()){
				whenExpr = typeInnerNodes(q, nn->node1->node1);
				compExpr = typeCompute(compExpr, whenExpr);
			}
			n->datatype = innerType.type;
			n->tok3.id = compExpr.type;
			return innerType;
		}
		break;
	//expression
	case WORD_TK:
	case SP_LPAREN:
		return typeInnerNodes(q,n->node1);
	}
	error("bad case node");
	return {0,0};
}

static typer typePredCompareInnerNodes(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer n1, n2, n3, innerType;
	if (n->tok1.id == SP_LPAREN){
		typeInnerNodes(q, n->node1);
	} else if (n->tok1.id & RELOP) {
		n1 = typeInnerNodes(q, n->node1);
		n2 = typeInnerNodes(q, n->node2);
		n3 = typeInnerNodes(q, n->node3);
		innerType = typeCompute(n1,n2);
		innerType = typeCompute(innerType,n3);
		if (n->tok1.id == KW_LIKE) //keep raw string if using regex
			innerType.type = T_STRING;
	} else { n->print(); error("bad comparision node"); }
	return innerType;
}

static typer typeValueInnerNodes(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer innerType = {0,0};
	switch (n->tok2.id){
	case FUNCTION:
		innerType = typeInnerNodes(q, n->node1);
		break;
	case LITERAL:
		innerType = {n->datatype, 1};
		break;
	case COLUMN:
		innerType = {n->datatype, 0};
		break;
	case VARIABLE:
		for (auto &v : q.vars)
			if (n->tok1.val == v.name){
				innerType = {v.type, v.lit};
				break;
			}
		break;
	}
	return innerType;
}

static typer typeFunctionInnerNodes(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer innerType = {0}, paramType = {0};
	switch (n->tok1.id){
	case FN_COUNT:
		n->keep = true;
	case FN_INC:
		innerType = {T_FLOAT, true};
		break;
	case FN_MONTHNAME:
	case FN_WDAYNAME:
		n->keep = true;
		typeInnerNodes(q, n->node1);
		innerType = {T_STRING, false};
		break;
	case FN_ENCRYPT:
	case FN_DECRYPT:
		paramType = typeInnerNodes(q, n->node1);
		innerType = {T_STRING, false};
		if (!canBeString(n->node1)) {
			n->tok5.id = paramType.type;
		}
		break;
	case FN_YEAR:
	case FN_MONTH:
	case FN_WEEK:
	case FN_YDAY:
	case FN_MDAY:
	case FN_WDAY:
	case FN_HOUR:
		typeInnerNodes(q, n->node1);
		innerType = {T_DATE, true};
		break;
	default:
		innerType = typeInnerNodes(q, n->node1);
	}
	return innerType;
}

//set datatype value of inner tree nodes where applicable
static typer typeInnerNodes(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer n1, n2, n3, n4, innerType = {0,0};
	ktype k;
	switch (n->label){
	//not applicable - move on to subtrees
	case N_SELECT:
	case N_QUERY:
	case N_AFTERFROM:
	case N_PRESELECT:
	case N_WITH:
	case N_FROM:
	case N_JOIN:
	case N_WHERE:
	case N_HAVING:
		typeInnerNodes(q, n->node1);
		typeInnerNodes(q, n->node2);
		typeInnerNodes(q, n->node3);
		typeInnerNodes(q, n->node4);
		break;
	case N_ORDER:
		innerType = typeInnerNodes(q, n->node1);
		if (q.sorting)
			q.sorting = innerType.type;
		break;
	//things that may be list but have independant types
	case N_SELECTIONS:
	case N_GROUPBY:
	case N_EXPRESSIONS:
		innerType = typeInnerNodes(q, n->node1);
		typeInnerNodes(q, n->node2);
		break;
	case N_VARS:
		innerType = typeInnerNodes(q, n->node1);
		for (auto &v : q.vars)
			if (n->tok1.val == v.name){
				v.type = innerType.type;
				v.lit = innerType.lit;
				break;
			}
		typeInnerNodes(q, n->node2);
		break;
	case N_EXPRNEG:
	case N_EXPRADD:
	case N_EXPRMULT:
		n1 = typeInnerNodes(q, n->node1);
		n2 = typeInnerNodes(q, n->node2);
		innerType = typeCompute(n1, n2);
		k = keepSubtreeTypes(n1.type, n2.type, n->tok1.id);
		if (k.keep){
			innerType.type = k.type;
			n->keep = true;
		}
		break;
	case N_EXPRCASE:
		innerType = typeCaseInnerNodes(q,n);
		break;
	//interdependant lists
	case N_CPREDLIST:
	case N_CWEXPRLIST:
	case N_DEXPRESSIONS:
		n1 = typeInnerNodes(q, n->node1);
		n2 = typeInnerNodes(q, n->node2);
		innerType = typeCompute(n1, n2);
		break;
	case N_CPRED:
	case N_CWEXPR:
		typeInnerNodes(q, n->node1); //conditions or comparision
		innerType = typeInnerNodes(q, n->node2); //result
		break;
	case N_PREDICATES:
		//both just boolean
		typeInnerNodes(q, n->node1);
		typeInnerNodes(q, n->node2);
		break;
	case N_PREDCOMP:
		innerType = typePredCompareInnerNodes(q,n);
		break;
	case N_VALUE:
		innerType = typeValueInnerNodes(q,n);
		//will add type conversion node for variables
		break;
	case N_FUNCTION:
		innerType = typeFunctionInnerNodes(q,n);
		break;
	default:
		error("missed a node type: "+treeMap[n->label]);
	}
	n->datatype = innerType.type;
	//cerr << "type node " << treeMap[n->label] << " as " << n->datatype << endl;
	return innerType;
}

static void typeCaseFinalNodes(querySpecs &q, unique_ptr<node> &n, int finaltype){
	if (n == nullptr) return;
	int comptype;
	switch (n->tok1.id){
	//case statement
	case KW_CASE:
		switch (n->tok2.id){
		//when predicates are true
		case KW_WHEN:
			typeFinalValues(q, n->node1, finaltype);
			typeFinalValues(q, n->node3, finaltype);
			break;
		//expression matches expression list
		case WORD_TK:
		case SP_LPAREN:
			comptype = n->tok3.id;
			typeFinalValues(q, n->node1, comptype);
			typeFinalValues(q, n->node2, finaltype);
			typeFinalValues(q, n->node3, finaltype);
			node* list = n->node2.get();
			for (auto nn=list; nn; nn=nn->node2.get()){
				nn->node1->tok1.id = comptype;
				typeFinalValues(q, nn->node1->node1, comptype);
			}
			break;
		}
		break;
	//expression
	case WORD_TK:
	case SP_LPAREN:
		typeFinalValues(q, n->node1, finaltype);
	}
}

static void typePredCompFinalNodes(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return;
	if (n->tok1.id == SP_LPAREN){
		typeFinalValues(q, n->node1, -1);
	} else if (n->tok1.id & RELOP) {
		if (n->tok1.id == KW_LIKE)
			typeFinalValues(q, n->node2, n->datatype);
		else {
			typeFinalValues(q, n->node1, n->datatype);
			typeFinalValues(q, n->node2, n->datatype);
			typeFinalValues(q, n->node3, n->datatype);
		}
	}
}

static void typeFunctionFinalNodes(querySpecs &q, unique_ptr<node> &n, int finaltype){
	if (n == nullptr) return;
	if (n->keep) finaltype = -1;
	node *convNode, *tempNode;
	switch (n->tok1.id){
	case FN_COUNT:
	case FN_INC:
	case FN_ENCRYPT:
	case FN_DECRYPT:
		//add type conversion node if needed
		if (n->tok5.id){
			typeFinalValues(q, n->node1, n->tok5.id);
			convNode = new node;
			*convNode = {0};
			convNode->label = N_TYPECONV;
			convNode->keep = true;
			convNode->datatype = finaltype;
			convNode->tok1.id = n->tok5.id;
			tempNode = n->node1.release();
			convNode->node1.reset(tempNode);
			n->node1.reset(convNode);
		} else {
			typeFinalValues(q, n->node1, finaltype);
		}
		break;
	case FN_MONTHNAME:
	case FN_WDAYNAME:
	case FN_YEAR:
	case FN_MONTH:
	case FN_WEEK:
	case FN_YDAY:
	case FN_MDAY:
	case FN_WDAY:
	case FN_HOUR:
		typeFinalValues(q, n->node1, T_DATE);
		break;
	default:
		typeFinalValues(q, n->node1, finaltype);
	}
}

//use high-level nodes to give lower nodes their final types
static void typeFinalValues(querySpecs &q, unique_ptr<node> &n, int finaltype){
	if (n == nullptr) return;

	//if using own datatype instead of parent node's
	if (finaltype < 0) finaltype = n->datatype;
	n->datatype = finaltype;

	switch (n->label){
	//not applicable - move on to subtrees
	case N_SELECT:
	case N_QUERY:
	case N_AFTERFROM:
	case N_PRESELECT:
	case N_FROM:
	case N_JOIN:
	case N_PREDICATES:
	case N_WHERE:
	case N_HAVING:
	case N_WITH:
		typeFinalValues(q, n->node1, -1);
		typeFinalValues(q, n->node2, -1);
		typeFinalValues(q, n->node3, -1);
		typeFinalValues(q, n->node4, -1);
		break;
	//straightforward stuff
	case N_CWEXPRLIST:
	case N_CPREDLIST:
	case N_DEXPRESSIONS:
		typeFinalValues(q, n->node1, finaltype);
		typeFinalValues(q, n->node2, finaltype);
		break;
	//things that may be list but have independant types
	case N_SELECTIONS:
	case N_ORDER:
	case N_GROUPBY:
	case N_EXPRESSIONS:
		typeFinalValues(q, n->node1, n->datatype);
		typeFinalValues(q, n->node2, -1);
		break;
	//may or may not preserve subtree types
	case N_EXPRNEG:
	case N_EXPRADD:
	case N_EXPRMULT:
		if (n->keep) finaltype = -1;
		typeFinalValues(q, n->node1, finaltype);
		typeFinalValues(q, n->node2, finaltype);
		break;
	//first node is independant, second is final value
	case N_CPRED:
		typeFinalValues(q, n->node1, -1);
		typeFinalValues(q, n->node2, finaltype);
		break;
	case N_EXPRCASE:
		typeCaseFinalNodes(q, n, finaltype);
		break;
	case N_CWEXPR:
		//node1 is already typed in case function
		typeFinalValues(q, n->node2, finaltype);
		break;
	case N_PREDCOMP:
		typePredCompFinalNodes(q, n);
		break;
	case N_VALUE:
		typeFinalValues(q, n->node1, finaltype); //functioin
		break;
	case N_FUNCTION:
		typeFunctionFinalNodes(q, n, finaltype);
		break;
	case N_VARS:
		for (auto &v : q.vars)
			if (n->tok1.val == v.name){
				v.type = finaltype;
				break;
			}
		typeFinalValues(q, n->node1, finaltype);
		typeFinalValues(q, n->node2, -1);
		break;
	}
}

//do all typing
void applyTypes(querySpecs &q){
	typeInitialValue(q, q.tree, false);
	typeInnerNodes(q, q.tree);
	typeFinalValues(q, q.tree, -1);
	cerr << "final type tree:\n";
	//printTree(q.tree, 0);
}

