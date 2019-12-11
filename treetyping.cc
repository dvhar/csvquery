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
	int i1 = 2*n1.type;
	int i2 = 2*n2.type;
	if (n1.lit) { i1++;}
	if (n2.lit) { i2++;}
	return { typeChart[i1][i2], n1.lit|n2.lit };
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
	switch (n->tok1.id){
	//case statement
	case KW_CASE:
		switch (n->tok2.id){
		//when predicates are true
		case KW_WHEN:
			break;
		//expression matches expression list
		case WORD:
		case SP_LPAREN:
			auto predlist = n->node1.get();
		}
		break;
	//expression
	case WORD:
	case SP_LPAREN:
		return typeCheck(q,n->node1);
	}
	return {0,0};
}

//set datatype value of inner tree nodes where applicable
static typer typeCheck(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr) return {0,0};
	typer n1, n2, n3, n4, finaltype;
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
	//things that have a type but don't matter to upper nodes
	case N_VARS:
	case N_SELECTIONS:
		n->datatype = typeCheck(q, n->node1).type;
		typeCheck(q, n->node2);
		break;
	//basic math operators
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
		n->datatype = finaltype.type;
		break;
	case N_EXPRCASE:
		finaltype = typeCaseExpr(q,n);
		n->datatype = finaltype.type;
		break;
	case N_ORDER:
		break;
	case N_CPREDLIST:
		break;
	case N_CPRED:
		break;
	case N_CWEXPRLIST:
		break;
	case N_CWEXPR:
		break;
	case N_PREDICATES:
		break;
	case N_PREDCOMP:
		break;
	case N_VALUE:
		break;
	case N_FUNCTION:
		break;
	case N_GROUPBY:
		break;
	case N_EXPRESSIONS:
		break;
	case N_DEXPRESSIONS:
		break;
	}
	return finaltype;
}

//do all typing
void applyTypes(querySpecs &q, unique_ptr<node> &n){
	typeInitialValue(q,n);
}

