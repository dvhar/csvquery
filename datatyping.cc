#include "interpretor.h"

//what type results from operation with 2 expressions with various data types and column/literal source
//don't use for types where different operation results in different type
//null[c,l], int[c,l], float[c,l], date[c,l], duration[c,l], string[c,l] in both dimensions
static const int typeChart[12][12] = {
	{5,5, 5,5, 5,5, 5,5, 5,5, 5,5},
	{5,5, 1,1, 2,2, 3,3, 4,4, 5,5},
	{5,1, 1,1, 2,2, 3,1, 4,4, 5,1},
	{5,1, 1,1, 2,2, 3,1, 4,4, 5,5},
	{5,2, 2,2, 2,2, 3,2, 4,2, 5,2},
	{5,2, 2,2, 2,2, 3,2, 4,4, 5,5},
	{5,3, 3,3, 3,3, 3,3, 3,3, 5,3},
	{5,3, 1,1, 2,2, 3,3, 3,3, 5,5},
	{5,4, 4,4, 4,4, 3,3, 4,4, 5,4},
	{5,4, 4,4, 2,4, 3,3, 4,4, 5,5},
	{5,5, 5,5, 5,5, 5,5, 5,5, 5,5},
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

class dataTyper {
	querySpecs* q;
	public:

	typer typeInnerNodes(unique_ptr<node> &n);
	void typeFinalValues(unique_ptr<node> &n, int finaltype);
	void typeInitialValue(unique_ptr<node> &n, bool trivial);
	typer typeCaseInnerNodes(unique_ptr<node> &n);
	void typeCaseFinalNodes(unique_ptr<node> &n, int finaltype);
	typer typePredCompareInnerNodes(unique_ptr<node> &n);
	void typePredCompFinalNodes(unique_ptr<node> &n);
	typer typeValueInnerNodes(unique_ptr<node> &n);
	typer typeFunctionInnerNodes(unique_ptr<node> &n);
	void typeFunctionFinalNodes(unique_ptr<node> &n, int finaltype);
	void checkMathSemantics(unique_ptr<node> &n);
	void checkFuncSemantics(unique_ptr<node> &n);

	dataTyper(querySpecs& qs): q{&qs} {};
};

//special cases handled later by keepSubtreeTypes()
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
		if ((t1 == T_DURATION && t2 == T_INT) || (t1 == T_DURATION && t2 == T_FLOAT) ||
		   (t2 == T_DURATION && t1 == T_INT) || (t2 == T_DURATION && t1 == T_FLOAT))
			return { T_DURATION, true };
		if (t1 == T_DURATION && t2 == T_DURATION)  //only div
		    return { T_FLOAT, true };
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
		case FN_FORMAT:
		case FN_MONTHNAME:
		case FN_WDAYNAME:
		case FN_UPPER:
		case FN_LOWER:
		case FN_BASE64_ENCODE:
		case FN_BASE64_DECODE:
		case FN_HEX_ENCODE:
		case FN_HEX_DECODE:
		case FN_SUBSTR:
		case FN_MD5:
		case FN_SHA1:
		case FN_SHA256:
		case FN_STRING:
			return true;
		case FN_INC:
		case FN_COUNT:
		case FN_ABS:
		case FN_STDEV:
		case FN_STDEVP:
		case FN_AVG:
		case FN_YEAR:
		case FN_MONTH:
		case FN_WEEK:
		case FN_YDAY:
		case FN_MDAY:
		case FN_WDAY:
		case FN_HOUR:
		case FN_MINUTE:
		case FN_SECOND:
		case FN_CEIL:
		case FN_FLOOR:
		case FN_ACOS:
		case FN_ASIN:
		case FN_ATAN:
		case FN_COS:
		case FN_SIN:
		case FN_TAN:
		case FN_EXP:
		case FN_LOG:
		case FN_LOG2:
		case FN_LOG10:
		case FN_SQRT:
		case FN_CBRT:
		case FN_RAND:
		case FN_LEN:
		case FN_INT:
		case FN_FLOAT:
		case FN_DATE:
		case FN_DUR:
		case FN_ROUND:
		case FN_POW:
		case FN_NOW:
		case FN_NOWGM:
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
void dataTyper::typeInitialValue(unique_ptr<node> &n, bool trivial){
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
		for (auto &v : q->vars)
			if (val == v.name){
				n->tok2.id = VARIABLE;
				goto donetyping;
			}
		//see if file alias
		period = val.find_first_of(".");
		int i;
		if (period != -1){
			string alias = val.substr(0, period);
			if (q->filemap.count(alias)){
				string column = val.substr(period+1, val.length()-period);
				shared_ptr<fileReader> f = q->filemap[alias];
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
					if (i < f->numFields){
						//found column idx with file alias
						n->tok1.id = i;
						n->tok2.id = COLUMN;
						n->tok3.val = alias;
						n->datatype = f->types[i];
						goto donetyping;
					}
				} else if (!n->tok1.quoted && q->numIsCol() && regex_match(column, posInt)){
					i = stoi(column)-1;
					if (i < f->numFields){
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
		for (auto &f : q->filevec){
			i = f->getColIdx(val);
			if (i != -1){
				//found column name without file alias
				n->tok1.id = i;
				n->tok2.id = COLUMN;
				n->tok3.val = f->id;
				n->datatype = f->types[i];
				goto donetyping;
			}
		}
		if (!n->tok1.quoted && regex_match(val, cInt)){
			i = stoi(val.substr(1))-1;
			auto& f = q->filevec.front();
			if (i < f->numFields){
				//found column idx without file alias
				n->tok1.id = i;
				n->tok2.id = COLUMN;
				n->tok3.val = "_f0";
				n->datatype = f->types[i];
				goto donetyping;
			}
		} else if (!n->tok1.quoted && q->numIsCol() && regex_match(val, posInt)){
			i = stoi(val)-1;
			auto& f = q->filevec.front();
			if (i < f->numFields){
				//found column idx without file alias
				n->tok1.id = i;
				n->tok2.id = COLUMN;
				n->tok3.val = "_f0";
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
	if (trivial && n->label == N_VALUE) {
		if (n->tok2.id == COLUMN) {
			n->datatype = T_STRING;
			n->tok3.id = 1;
		} else if (n->tok2.id == VARIABLE)
			n->tok4.id = 1;
	}

	typeInitialValue(n->node1, trivial);
	//record variable after typing var leafnodes to avoid recursive definition
	if (n->label == N_VARS)             q->addVar(thisVar);
	//reset trivial indicator for next selection
	if (n->label == N_SELECTIONS)       trivial = false;
	typeInitialValue(n->node2, trivial);
	typeInitialValue(n->node3, trivial);
	typeInitialValue(n->node4, trivial);

}

typer dataTyper::typeCaseInnerNodes(unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer innerType, elseExpr, whenExpr, thenExpr, compExpr;
	switch (n->tok1.id){
	//case statement
	case KW_CASE:
		switch (n->tok2.id){
		//when predicates are true
		case KW_WHEN:
			thenExpr  = typeInnerNodes(n->node1);
			elseExpr  = typeInnerNodes(n->node3);
			innerType = typeCompute(thenExpr, elseExpr);
			return innerType;
		//expression matches expression list
		case WORD_TK:
		case SP_LPAREN:
			compExpr  = typeInnerNodes(n->node1);
			thenExpr  = typeInnerNodes(n->node2);
			elseExpr  = typeInnerNodes(n->node3);
			innerType = typeCompute(thenExpr, elseExpr);
			node* list = n->node2.get();
			for (auto nn=list; nn; nn=nn->node2.get()){
				whenExpr = typeInnerNodes(nn->node1->node1);
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
		return typeInnerNodes(n->node1);
	}
	error("bad case node");
	return {0,0};
}

typer dataTyper::typePredCompareInnerNodes(unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer n1, n2, n3, innerType;
	if (n->tok1.id == SP_LPAREN){
		typeInnerNodes(n->node1);
	} else if (n->tok1.id & RELOP) {
		n1 = typeInnerNodes(n->node1);
		n2 = typeInnerNodes(n->node2);
		n3 = typeInnerNodes(n->node3);
		innerType = typeCompute(n1,n2);
		innerType = typeCompute(innerType,n3);
		if (n->tok1.id == KW_LIKE) //keep raw string if using regex
			innerType.type = T_STRING;
	} else { error("bad comparision node"); }
	return innerType;
}

typer dataTyper::typeValueInnerNodes(unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer innerType = {0,0};
	switch (n->tok2.id){
	case FUNCTION:
		innerType = typeInnerNodes(n->node1);
		break;
	case LITERAL:
		innerType = {n->datatype, 1};
		break;
	case COLUMN:
		innerType = {n->datatype, 0};
		break;
	case VARIABLE:
		{
			auto& v = q->var(n->tok1.val);
			innerType = {v.type, v.lit};
		}
		break;
	}
	return innerType;
}

typer dataTyper::typeFunctionInnerNodes(unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer innerType = {0}, paramType = {0};
	switch (n->tok1.id){
	case FN_COUNT:
		n->keep = true;
		typeInnerNodes(n->node1);
		innerType = {T_FLOAT, false};
		break;
	case FN_FORMAT:
	case FN_MONTHNAME:
	case FN_WDAYNAME:
		n->keep = true;
		typeInnerNodes(n->node1);
		innerType = {T_STRING, false};
		n->info[RETTYPE] = innerType.type;
		break;
	case FN_ENCRYPT:
	case FN_DECRYPT:
	case FN_BASE64_ENCODE:
	case FN_HEX_ENCODE:
	case FN_MD5:
	case FN_SHA1:
	case FN_SHA256:
	case FN_STRING:
	case FN_UPPER:
	case FN_LOWER:
	case FN_SUBSTR:
	case FN_BASE64_DECODE:
	case FN_HEX_DECODE:
		paramType = typeInnerNodes(n->node1);
		innerType = {T_STRING, false};
		n->info[RETTYPE] = innerType.type;
		if (!canBeString(n->node1))
			n->info[PARAMTYPE] = paramType.type;
		break;
	case FN_POW:{
		auto n1 = typeInnerNodes(n->node1);
		auto n2 = typeInnerNodes(n->node2);
		innerType = typeCompute(n1, n2);
		n->info[RETTYPE] = innerType.type;
		}
		break;
	case FN_YEAR:
	case FN_MONTH:
	case FN_WEEK:
	case FN_YDAY:
	case FN_MDAY:
	case FN_WDAY:
	case FN_HOUR:
	case FN_MINUTE:
	case FN_SECOND:
		typeInnerNodes(n->node1);
		innerType = {T_INT, true}; //TODO: reconsider these true/false values
		n->info[RETTYPE] = T_INT;
		break;
	case FN_STDEV:
	case FN_STDEVP:
	case FN_AVG:
		innerType = typeInnerNodes(n->node1);
		if (innerType.type == T_INT)
			innerType.type = T_FLOAT;
		break;
	case FN_CEIL:
	case FN_FLOOR:
	case FN_ACOS:
	case FN_ASIN:
	case FN_ATAN:
	case FN_COS:
	case FN_SIN:
	case FN_TAN:
	case FN_EXP:
	case FN_LOG:
	case FN_LOG2:
	case FN_LOG10:
	case FN_SQRT:
	case FN_CBRT:
	case FN_RAND:
	case FN_ROUND:
	case FN_INC:
		if (auto pt = typeInnerNodes(n->node1).type; pt > T_FLOAT)
			n->info[PARAMTYPE] = pt;
		innerType = {T_FLOAT, false};
		n->info[RETTYPE] = T_FLOAT;
		break;
	case FN_NOW:
	case FN_NOWGM:
		innerType = {T_DATE, false};
		n->info[RETTYPE] = T_DATE;
		break;
	case FN_LEN:
		innerType = {T_FLOAT, false};
		n->info[RETTYPE] = T_FLOAT;
		n->tok2.id = typeInnerNodes(n->node1).type;
		if (canBeString(n->node1))
			n->tok2.id = T_STRING;
		break;
	case FN_INT:
		innerType = {T_INT, false};
		n->info[PARAMTYPE] = typeInnerNodes(n->node1).type;
		n->info[RETTYPE] = T_INT;
		break;
	case FN_FLOAT:
		innerType = {T_FLOAT, false};
		n->info[PARAMTYPE] = typeInnerNodes(n->node1).type;
		n->info[RETTYPE] = T_FLOAT;
		break;
	case FN_DATE:
		innerType = {T_DATE, false};
		n->info[PARAMTYPE] = typeInnerNodes(n->node1).type;
		n->info[RETTYPE] = T_DATE;
		break;
	case FN_DUR:
		innerType = {T_DURATION, false};
		n->info[PARAMTYPE] = typeInnerNodes(n->node1).type;
		n->info[RETTYPE] = T_DURATION;
		break;

	default:
		innerType = typeInnerNodes(n->node1);
	}
	return innerType;
}

//set datatype value of inner tree nodes where applicable
typer dataTyper::typeInnerNodes(unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer n1, n2, innerType = {0,0};
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
		typeInnerNodes(n->node1);
		typeInnerNodes(n->node2);
		typeInnerNodes(n->node3);
		typeInnerNodes(n->node4);
		break;
	case N_ORDER:
		innerType = typeInnerNodes(n->node1);
		if (q->sorting)
			q->sorting = innerType.type;
		break;
	//things that may be list but have independant types
	case N_SELECTIONS:
	case N_GROUPBY:
	case N_EXPRESSIONS:
		innerType = typeInnerNodes(n->node1);
		typeInnerNodes(n->node2);
		break;
	case N_VARS:
		innerType = typeInnerNodes(n->node1);
		{
			auto& v = q->var(n->tok1.val);
			v.type = innerType.type;
			v.lit = innerType.lit;
		}
		typeInnerNodes(n->node2);
		break;
	case N_EXPRNEG:
	case N_EXPRADD:
	case N_EXPRMULT:
		n1 = typeInnerNodes(n->node1);
		n2 = typeInnerNodes(n->node2);
		innerType = typeCompute(n1, n2);
		k = keepSubtreeTypes(n1.type, n2.type, n->tok1.id);
		if (k.keep){
			innerType.type = k.type;
			n->keep = true;
		}
		break;
	case N_EXPRCASE:
		innerType = typeCaseInnerNodes(n);
		break;
	//interdependant lists
	case N_CPREDLIST:
	case N_CWEXPRLIST:
	case N_DEXPRESSIONS:
		n1 = typeInnerNodes(n->node1);
		n2 = typeInnerNodes(n->node2);
		innerType = typeCompute(n1, n2);
		break;
	case N_CPRED:
	case N_CWEXPR:
		typeInnerNodes(n->node1); //conditions or comparision
		innerType = typeInnerNodes(n->node2); //result
		break;
	case N_PREDICATES:
		//both just boolean
		typeInnerNodes(n->node1);
		typeInnerNodes(n->node2);
		break;
	case N_PREDCOMP:
		innerType = typePredCompareInnerNodes(n);
		break;
	case N_VALUE:
		innerType = typeValueInnerNodes(n);
		break;
	case N_FUNCTION:
		innerType = typeFunctionInnerNodes(n);
		break;
	default:
		error(st("missed a node type: ",getnodename(n->label)));
	}
	n->datatype = innerType.type;
	//cerr << "type node " << treeMap[n->label] << " as " << n->datatype << endl;
	return innerType;
}

void dataTyper::typeCaseFinalNodes(unique_ptr<node> &n, int finaltype){
	if (n == nullptr) return;
	int comptype;
	switch (n->tok1.id){
	//case statement
	case KW_CASE:
		switch (n->tok2.id){
		//when predicates are true
		case KW_WHEN:
			typeFinalValues(n->node1, finaltype);
			typeFinalValues(n->node3, finaltype);
			break;
		//expression matches expression list
		case WORD_TK:
		case SP_LPAREN:
			comptype = n->tok3.id;
			typeFinalValues(n->node1, comptype);
			typeFinalValues(n->node2, finaltype);
			typeFinalValues(n->node3, finaltype);
			node* list = n->node2.get();
			for (auto nn=list; nn; nn=nn->node2.get()){
				nn->node1->tok1.id = comptype;
				typeFinalValues(nn->node1->node1, comptype);
			}
			break;
		}
		break;
	//expression
	case WORD_TK:
	case SP_LPAREN:
		typeFinalValues(n->node1, finaltype);
	}
}

void dataTyper::typePredCompFinalNodes(unique_ptr<node> &n){
	if (n == nullptr) return;
	if (n->tok1.id == SP_LPAREN){
		typeFinalValues(n->node1, -1);
	} else if (n->tok1.id & RELOP) {
		typeFinalValues(n->node1, n->datatype);
		typeFinalValues(n->node2, n->datatype);
		typeFinalValues(n->node3, n->datatype);
	}
}

void dataTyper::typeFunctionFinalNodes(unique_ptr<node> &n, int finaltype){
	if (n == nullptr) return;
	int oldret = finaltype;
	if (n->keep) finaltype = -1;
	node *convNode, *tempNode;
	bool needRetConvert = false;
	//TODO: find out why now() return is not using dynamic typing
	if (auto rt = n->info[RETTYPE]; rt > 0 && finaltype != rt){
		//need conversion from RETTYPE (can do) to original finaltype (need)
		n->datatype = finaltype = rt;
		needRetConvert = true;
	}
	switch (n->tok1.id){
	case FN_LEN:{
		if (n->tok2.id == T_STRING){
			typeFinalValues(n->node1, T_STRING);
			break;
		}
		finaltype = n->tok2.id;
	}
	case FN_COUNT:
	case FN_INC:
	case FN_ENCRYPT:
	case FN_DECRYPT:
	case FN_BASE64_ENCODE:
	case FN_HEX_ENCODE:
	case FN_BASE64_DECODE:
	case FN_HEX_DECODE:
	case FN_MD5:
	case FN_SHA1:
	case FN_SHA256:
	case FN_UPPER:
	case FN_LOWER:
	case FN_SUBSTR:
	case FN_CEIL:
	case FN_FLOOR:
	case FN_ACOS:
	case FN_ASIN:
	case FN_ATAN:
	case FN_COS:
	case FN_SIN:
	case FN_TAN:
	case FN_EXP:
	case FN_LOG:
	case FN_LOG2:
	case FN_LOG10:
	case FN_SQRT:
	case FN_CBRT:
	case FN_RAND:
	case FN_INT:
	case FN_FLOAT:
	case FN_DATE:
	case FN_DUR:
	case FN_STRING:
	case FN_ROUND:
	case FN_POW:
	case FN_NOW:
	case FN_NOWGM:
		//add type conversion node for parameter if needed
		if (auto pt = n->info[PARAMTYPE]; pt){
			typeFinalValues(n->node1, pt);
			typeFinalValues(n->node2, pt);
			convNode = new node(N_TYPECONV);
			convNode->keep = true;
			convNode->datatype = finaltype;
			convNode->tok1.id = pt;
			tempNode = n->node1.release();
			convNode->node1.reset(tempNode);
			n->node1.reset(convNode);
		} else {
			typeFinalValues(n->node1, finaltype);
			typeFinalValues(n->node2, finaltype);
		}
		break;
	case FN_FORMAT:
	case FN_MONTHNAME:
	case FN_WDAYNAME:
	case FN_YEAR:
	case FN_MONTH:
	case FN_WEEK:
	case FN_YDAY:
	case FN_MDAY:
	case FN_WDAY:
	case FN_HOUR:
	case FN_MINUTE:
	case FN_SECOND:
		typeFinalValues(n->node1, T_DATE);
		break;
	default:
		typeFinalValues(n->node1, finaltype);
	}
	checkFuncSemantics(n);
	if (needRetConvert){
			convNode = new node(N_TYPECONV);
			convNode->keep = true;
			convNode->datatype = oldret;
			convNode->tok1.id = n->info[RETTYPE];
			tempNode = n.release();
			convNode->node1.reset(tempNode);
			n.reset(convNode);
	}
}

//use high-level nodes to give lower nodes their final types
void dataTyper::typeFinalValues(unique_ptr<node> &n, int finaltype){
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
		typeFinalValues(n->node1, -1);
		typeFinalValues(n->node2, -1);
		typeFinalValues(n->node3, -1);
		typeFinalValues(n->node4, -1);
		break;
	//straightforward stuff
	case N_CWEXPRLIST:
	case N_CPREDLIST:
	case N_DEXPRESSIONS:
		typeFinalValues(n->node1, finaltype);
		typeFinalValues(n->node2, finaltype);
		break;
	//things that may be list but have independant types
	case N_SELECTIONS:
	case N_ORDER:
	case N_GROUPBY:
	case N_EXPRESSIONS:
		typeFinalValues(n->node1, n->datatype);
		typeFinalValues(n->node2, -1);
		break;
	//may or may not preserve subtree types
	case N_EXPRNEG:
		if (n->tok1.id && (finaltype == T_STRING || finaltype == T_DATE))
			error(st("Minus sign does not work with type ",gettypename(finaltype)));
	case N_EXPRADD:
	case N_EXPRMULT:
		if (n->keep) finaltype = -1;
		typeFinalValues(n->node1, finaltype);
		typeFinalValues(n->node2, finaltype);
		checkMathSemantics(n);
		break;
	//first node is independant, second is final value
	case N_CPRED:
		typeFinalValues(n->node1, -1);
		typeFinalValues(n->node2, finaltype);
		break;
	case N_EXPRCASE:
		typeCaseFinalNodes(n, finaltype);
		break;
	case N_CWEXPR:
		//node1 is already typed in case function
		typeFinalValues(n->node2, finaltype);
		break;
	case N_PREDCOMP:
		typePredCompFinalNodes(n);
		break;
	case N_VALUE:
		typeFinalValues(n->node1, finaltype); //functioin
		break;
	case N_FUNCTION:
		typeFunctionFinalNodes(n, finaltype);
		break;
	case N_VARS:
		q->var(n->tok1.val).type = finaltype;
		typeFinalValues(n->node1, finaltype);
		typeFinalValues(n->node2, -1);
		break;
	}
}

void dataTyper::checkMathSemantics(unique_ptr<node> &n){
	if (n->label != N_EXPRADD && n->label != N_EXPRMULT)
		return;
	auto n1 = n->node1 == nullptr ? T_NULL : n->node1->datatype;
	auto n2 = n->node2 == nullptr ? T_NULL : n->node2->datatype;
	auto combo = [&](int a, int b){ return (n1 == a && n2 == b) || (n1 == b && n2 == a); };
	auto is = [&](int t){ return n->datatype == t; };
	auto typestr = gettypename(n->datatype);

	switch (n->tok1.id){
	case SP_PLUS:
		if (combo(T_DATE, T_DATE))
			error("Cannot add 2 dates");
		if (combo(T_DURATION, T_INT) || combo(T_DURATION, T_FLOAT))
			error("Cannot add duration and number");
		return;
	case SP_MINUS:
		if (n1 == T_DURATION && n2 == T_DATE)
			error("Cannot subtract date from duration");
		if (is(T_STRING))
			error("Cannot subtract text");
		if (combo(T_DURATION, T_INT) || combo(T_DURATION, T_FLOAT))
			error("Cannot subtract duration and number");
		return;
	case SP_STAR:
		if (is(T_STRING) || is(T_DATE))
			error(st("Cannot multiply type ",typestr));
		if (combo(T_DURATION, T_DURATION))
			error("cannot multiply 2 durations");
		return;
	case SP_MOD:
		if (!is(T_INT) && !is(T_FLOAT))
			error(st("Cannot modulus type ",typestr));
		return;
	case SP_CARROT:
		if (!is(T_INT) && !is(T_FLOAT))
			error(st("Cannot exponentiate type ",typestr));
		return;
	case SP_DIV:
		if (is(T_STRING) || is(T_DATE))
			error(st("Cannot divide type ",typestr));
		if (n1 != T_DURATION && n2 == T_DURATION)
			error("Cannot divide number by duration");
		return;
	}
}

void dataTyper::checkFuncSemantics(unique_ptr<node> &n){
	auto n1 = n->node1 == nullptr ? T_NULL : n->node1->datatype;
	auto typestr = gettypename(n->datatype);
	char* e = NULL;

	switch (n->tok1.id){
	case FN_SUM:
	case FN_AVG:
	case FN_STDEV:
	case FN_STDEVP:
		if (n1 == T_STRING || n1 == T_DATE || n1 == T_DURATION)
			error(st("Cannot use ",n->tok1.val," function with type ",typestr));
		//TODO: implement stdev and avg for date and duration
		break;
	case FN_ABS:
		if (n1 == T_STRING || n1 == T_DATE)
			error(st("Cannot use ",n->tok1.val," function with type ",typestr));
		break;
	case FN_SUBSTR:
		if (n->tok2.quoted){
			if (n->tok3.id)
				error("SUBSTR function requires 2 numbers or 1 pattern");
		} else if (!isInt(n->tok2.val.c_str()) || !isInt(n->tok3.val.c_str()))
				error("SUBSTR function requires 2 numbers or 1 pattern");
		break;
	case FN_ROUND:
	case FN_CEIL:
	case FN_FLOOR:
		if (!n->tok2.id) return;
		n->tok2.id = strtol(n->tok2.val.c_str(), &e, 10);
		if (*e != 0 || n->tok2.id < 0 || n->tok2.id > 9)
			error(st(n->tok1.val," function 2nd value must be integer from -9 to 9"));
		if (n->tok3.id == SP_MINUS)
			n->tok2.id *= -1;
		break;
	case FN_ENCRYPT:
	case FN_DECRYPT:
		if (n->tok4.val.empty())
			n->tok4.val = q->password;
		break;
	}
}

void applyTypes(querySpecs &q){
	dataTyper dt(q);
	dt.typeInitialValue(q.tree, false);
	dt.typeInnerNodes(q.tree);
	dt.typeFinalValues(q.tree, -1);
}

