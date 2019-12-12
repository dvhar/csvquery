#include "interpretor.h"

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
static typer typeCheck(querySpecs &q, unique_ptr<node> &n);

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
		break;
	case SP_PLUS:
		if ((t1 == T_DURATION && t2 == T_DATE) ||
		   (t2 == T_DURATION && t1 == T_DATE)) return { T_DATE, true };
		break;
	}
	return {0, false};
}

//give value nodes their initial type and look out for variables
static void typeInitialValue(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr)
		return;

	string thisVar;
	if (n->label == N_VARS){
		thisVar = n->tok1.val;
	}

	//only do leaf nodes
	if (n->label == N_VALUE && n->tok2.id != N_FUNCTION){
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
					i = stoi(column.substr(1))+1;
					if (i <= f->numFields){
						//found column idx with file alias
						n->tok1.id = i;
						n->tok2.id = COLUMN;
						n->tok3.val = alias;
						n->datatype = f->types[i];
						goto donetyping;
					}
				} else if (!n->tok1.quoted && q.numIsCol() && regex_match(column, posInt)){
					i = stoi(column)+1;
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
			i = stoi(val.substr(1))+1;
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
			i = stoi(val)+1;
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
	//put label after } if not printing
	donetyping:
	cerr << "typed " << n->tok1.val << " as " << n->datatype << endl;
	}

	typeInitialValue(q, n->node1);
	//record variable after typing var leafnodes to avoid recursive definition
	if (n->label == N_VARS)
		q.addVar(thisVar);
	typeInitialValue(q, n->node2);
	typeInitialValue(q, n->node3);
	typeInitialValue(q, n->node4);

}

static typer typeCaseExpr(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer result, elseExpr, whenExpr, thenExpr, compExpr;
	switch (n->tok1.id){
	//case statement
	case KW_CASE:
		switch (n->tok2.id){
		//when predicates are true
		case KW_WHEN:
			thenExpr = typeCheck(q,n->node1);
			elseExpr = typeCheck(q,n->node3);
			result   = typeCompute(thenExpr, elseExpr);
			return result;
		//expression matches expression list
		case WORD:
		case SP_LPAREN:
			compExpr = typeCheck(q, n->node1);
			thenExpr = typeCheck(q, n->node2);
			elseExpr = typeCheck(q, n->node3);
			result   = typeCompute(thenExpr, elseExpr);
			node* list = n->node2.get();
			for (auto n=list; n; n=n->node2.get()){
				whenExpr = typeCheck(q, n->node1->node1);
				compExpr = typeCompute(compExpr, whenExpr);
			}
			n->datatype = result.type;
			n->tok3.id = compExpr.type;
			return result;
		}
		break;
	//expression
	case WORD:
	case SP_LPAREN:
		return typeCheck(q,n->node1);
	}
	error("bad case node");
	return {0,0};
}

static void typePredCompare(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return;
	typer n1, n2, n3, finaltype;
	if (n->tok1.id == SP_LPAREN){
		typeCheck(q, n->node1);
	} else if (n->tok1.id & RELOP) {
		n1 = typeCheck(q, n->node1);
		n2 = typeCheck(q, n->node2);
		n3 = typeCheck(q, n->node3);
		finaltype = typeCompute(n1,n2);
		finaltype = typeCompute(finaltype,n3);
		if (n->tok1.id == KW_LIKE) //keep raw string if using regex
			finaltype.type = T_STRING;
		n->datatype = finaltype.type;
	} else { n->print(); error("bad comparision node"); }
}

static typer typeValue(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer finaltype = {0,0};
	switch (n->tok2.id){
	case FUNCTION:
		finaltype = typeCheck(q, n->node1);
		break;
	case LITERAL:
		finaltype = {n->datatype, 1};
		break;
	case COLUMN:
		finaltype = {n->datatype, 0};
		break;
	case VARIABLE:
		for (auto &v : q.vars)
			if (n->tok1.val == v.name){
				finaltype = {v.type, v.lit};
				break;
			}
		break;
	}
	return finaltype;
}

static typer typeFunction(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer finaltype = {0,0};
	switch (n->tok1.id){
	case FN_COUNT:
		n->keep = true;
	case FN_INC:
		finaltype = {T_FLOAT, true};
		break;
	case FN_MONTHNAME:
	case FN_WDAYNAME:
	case FN_ENCRYPT:
	case FN_DECRYPT:
		n->keep = true;
		typeCheck(q, n->node1);
		finaltype = {T_STRING, true};
		break;
	case FN_YEAR:
	case FN_MONTH:
	case FN_WEEK:
	case FN_YDAY:
	case FN_MDAY:
	case FN_WDAY:
	case FN_HOUR:
		typeCheck(q, n->node1);
		finaltype = {T_DATE, true};
		break;
	default:
		finaltype = typeCheck(q, n->node1);
	}
	return finaltype;
}

//set datatype value of inner tree nodes where applicable
static typer typeCheck(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	cerr << "typecheck at node " << treeMap[n->label] << endl;
	typer n1, n2, n3, n4, finaltype = {0,0};
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
		typeCheck(q, n->node1);
		typeCheck(q, n->node2);
		typeCheck(q, n->node3);
		typeCheck(q, n->node4);
		break;
	//things that may be list but have independant types
	case N_SELECTIONS:
	case N_ORDER:
	case N_GROUPBY:
	case N_EXPRESSIONS:
		finaltype = typeCheck(q, n->node1);
		typeCheck(q, n->node2);
		break;
	case N_VARS:
		finaltype = typeCheck(q, n->node1);
		for (auto &v : q.vars)
			if (n->tok1.val == v.name){
				v.type = finaltype.type;
				v.lit = finaltype.lit;
				break;
			}
		typeCheck(q, n->node2);
		break;
	case N_EXPRNEG:
	case N_EXPRADD:
	case N_EXPRMULT:
		n1 = typeCheck(q, n->node1);
		n2 = typeCheck(q, n->node2);
		finaltype = typeCompute(n1, n2);
		k = keepSubtreeTypes(n1.type, n2.type, n->tok1.id);
		if (k.keep){
			finaltype.type = k.type;
			n->keep = true;
		}
		break;
	case N_EXPRCASE:
		finaltype = typeCaseExpr(q,n);
		break;
	//interdependant lists
	case N_CPREDLIST:
	case N_CWEXPRLIST:
	case N_DEXPRESSIONS:
		n1 = typeCheck(q, n->node1);
		n2 = typeCheck(q, n->node2);
		finaltype = typeCompute(n1, n2);
		break;
	case N_CPRED:
	case N_CWEXPR:
		typeCheck(q, n->node1); //conditions or comparision
		finaltype = typeCheck(q, n->node2); //result
		break;
	case N_PREDICATES:
		//both just boolean
		typeCheck(q, n->node1);
		typeCheck(q, n->node2);
		break;
	case N_PREDCOMP:
		typePredCompare(q,n);
		break;
	case N_VALUE:
		finaltype = typeValue(q,n);
		break;
	case N_FUNCTION:
		finaltype = typeFunction(q,n);
		break;
	default:
		error("missed a node type: "+treeMap[n->label]);
	}
	cerr << "typecheck returning from node " << treeMap[n->label]
	<< " with finaltype " << finaltype.type << endl;
	n->datatype = finaltype.type;
	return finaltype;
}

//do all typing
void applyTypes(querySpecs &q, unique_ptr<node> &n){
	typeInitialValue(q,n);
	typeCheck(q,n);
}

