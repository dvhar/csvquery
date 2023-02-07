#include "interpretor.h"

//what type results from operation with 2 expressions with various data types and column/literal source
//don't use for types where different operation results in different type
//null[c,l], int[c,l], float[c,l], date[c,l], duration[c,l], string[c,l] in both dimensions
static const u8 typeChart[12][12] = {
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

	typer typeInnerNodes(astnode &n);
	void typeFinalValues(astnode &n, int finaltype);
	void typeInitialValue(astnode &n, bool trivial);
	typer typeCaseInnerNodes(astnode &n);
	void typeCaseFinalNodes(astnode &n, int finaltype);
	typer typePredCompareInnerNodes(astnode &n);
	void typePredCompFinalNodes(astnode &n);
	typer typeValueInnerNodes(astnode &n);
	typer typeFunctionInnerNodes(astnode &n);
	void typeFunctionFinalNodes(astnode &n, int finaltype);
	void checkMathSemantics(astnode &n);
	void checkFuncSemantics(astnode &n);
	bool canBeString(astnode& n);
	void getToptypes();
	void setToptypes();
	void selectallToptypes();

	dataTyper(querySpecs& qs): q{&qs} {};
	vector<pair<int,bool>> topitypes;
	vector<int> topftypes;
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
bool dataTyper::canBeString(astnode &n){
	if (n == nullptr) return true;

	switch (n->label){
	case N_CWEXPR:
	case N_CPRED:
		return canBeString(n->ncaseresultexpr());
	case N_EXPRNEG:
	case N_EXPRADD:
	case N_EXPRMULT:
		if (n->mathop())
			return false;
		else
			return canBeString(n->nsubexpr());
		break;
	case N_SETLIST:
		if (n->hassubquery()){
			auto& tv = q->subqueries[n->subqidx()].topinnertypes.get();
			if (!tv.size()) error("Subquery has no datatypes");
			for (auto& t: tv)
				if (!t.second) return false;
			return true;
		}
	case N_EXPRESSIONS:
	case N_DEXPRESSIONS:
	case N_CWEXPRLIST:
	case N_CPREDLIST:
		return canBeString(n->node1) && canBeString(n->node2);
	case N_EXPRCASE:
		switch (n->casenodetype()){
		case KW_CASE:
			switch (n->casenodetype()){
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
		if (n->valtype())
			return canBeString(n->nsubexpr());
		return true;
	case N_FUNCTION:
		switch (n->funcid()){
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
		case FN_SIP:
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
		case FN_MAX:
		case FN_MIN:
			return canBeString(n->nsubexpr());
		}
	}
	error("canBeString() function malfunctioned at node ", n->nodelabel());
	return false;
}

//see if a node involves operation that requires specific type
static bool stillTrivial(astnode &n){
	switch (n->label){
	case N_EXPRADD:
	case N_EXPRMULT:
	case N_EXPRNEG:
		if (n->mathop() != 0)
			return false;
		break;
	case N_EXPRCASE:
		if (n->casenodetype() != WORD_TK)
			return false;
		break;
	case N_VALUE:
		if (n->valtype() != COLUMN && n->valtype() != LITERAL && n->valtype() != VARIABLE)
			return false;
		break;
	}
	return true;
}

//give value nodes their initial type and look out for variables
void dataTyper::typeInitialValue(astnode &n, bool trivial){
	if (n == nullptr)
		return;

	string thisVar;
	string r;
	if (n->label == N_VARS){
		thisVar = n->varname();
	}

	//only do leaf nodes
	if (n->label == N_VALUE && n->valtype() != FUNCTION){
		string val = n->val();
		int period;
		//see if variable
		for (auto &v : q->vars)
			if (val == v.name){
				n->valtype() = VARIABLE;
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
					n->colidx() = i;
					n->valtype() = COLUMN;
					n->dotsrc() = alias;
					n->datatype = f->types[i];
					goto donetyping;
				} else if (!n->tok1.quoted && regex_match(column, cInt)){
					i = stoi(column.substr(1))-1;
					if (i < f->numFields){
						//found column idx with file alias
						n->colidx() = i;
						n->valtype() = COLUMN;
						n->dotsrc() = alias;
						n->datatype = f->types[i];
						goto donetyping;
					}
				} else if (!n->tok1.quoted && q->numIsCol() && regex_match(column, posInt)){
					i = stoi(column)-1;
					if (i < f->numFields){
						//found column idx with file alias
						n->colidx() = i;
						n->valtype() = COLUMN;
						n->dotsrc() = alias;
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
				n->colidx() = i;
				n->valtype() = COLUMN;
				n->dotsrc() = f->id;
				n->datatype = f->types[i];
				goto donetyping;
			}
		}
		if (!n->tok1.quoted && regex_match(val, cInt)){
			i = stoi(val.substr(1))-1;
			auto& f = q->filevec.front();
			if (i < f->numFields){
				//found column idx without file alias
				n->colidx() = i;
				n->valtype() = COLUMN;
				n->dotsrc() = "_f0";
				n->datatype = f->types[i];
				goto donetyping;
			}
		} else if (!n->tok1.quoted && q->numIsCol() && regex_match(val, posInt)){
			i = stoi(val)-1;
			auto& f = q->filevec.front();
			if (i < f->numFields){
				//found column idx without file alias
				n->colidx() = i;
				n->valtype() = COLUMN;
				n->dotsrc() = "_f0";
				n->datatype = f->types[i];
				goto donetyping;
			}
		}
		//is not a var, function, or column, must be literal
		n->valtype() = LITERAL;
		n->datatype = getNarrowestType((char*)val.c_str(), 0);
	}
	donetyping:;
	//cerr << "typed " << n->tok1.val << " as " << n->datatype << endl;

	//see if selecting trivial value (just column or literal)
	if (n->label == N_SELECTIONS && !q->isSubquery) trivial = true;
	if (trivial && !stillTrivial(n)) trivial = false;
	if (trivial && n->label == N_VALUE) {
		if (n->valtype() == COLUMN) {
			n->datatype = T_STRING;
			n->trivialvalcol() = 1;
		} else if (n->valtype() == VARIABLE)
			n->trivialvalalias() = 1;
	}

	typeInitialValue(n->node1, trivial);
	//record variable after typing var leafnodes to avoid recursive definition
	if (n->label == N_VARS) q->addVar(thisVar);
	//reset trivial indicator for next selection
	if (n->label == N_SELECTIONS) trivial = false;
	typeInitialValue(n->node2, trivial);
	typeInitialValue(n->node3, trivial);
	typeInitialValue(n->node4, trivial);

}

typer dataTyper::typeCaseInnerNodes(astnode &n){
	if (n == nullptr) return {0,0};
	typer innerType, elseExpr, whenExpr, thenExpr, compExpr;
	switch (n->tok1.id){
	//case statement
	case KW_CASE:
		switch (n->casetype()){
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
			n->casewhentype() = compExpr.type;
			return innerType;
		}
		break;
	//expression
	case WORD_TK:
	case SP_LPAREN:
		return typeInnerNodes(n->nsubexpr());
	}
	error("bad case node");
	return {0,0};
}

typer dataTyper::typePredCompareInnerNodes(astnode &n){
	if (n == nullptr) return {0,0};
	typer n1, n2, n3, innerType;
	if (n->relop() == SP_LPAREN){
		typeInnerNodes(n->node1);
	} else if (n->relop() & RELOP) {
		n1 = typeInnerNodes(n->node1);
		n2 = typeInnerNodes(n->node2);
		n3 = typeInnerNodes(n->node3);
		innerType = typeCompute(n1,n2);
		innerType = typeCompute(innerType,n3);
		if (n->relop() == KW_LIKE) //keep raw string if using regex
			innerType.type = T_STRING;
	} else { error("bad comparision node"); }
	return innerType;
}

typer dataTyper::typeValueInnerNodes(astnode &n){
	if (n == nullptr) return {0,0};
	typer innerType = {0,0};
	switch (n->valtype()){
	case FUNCTION:
		innerType = typeInnerNodes(n->nsubexpr());
		break;
	case LITERAL:
		innerType = {n->datatype, 1};
		break;
	case COLUMN:
		innerType = {n->datatype, 0};
		break;
	case VARIABLE:
		{
			auto& v = q->var(n->val());
			innerType = {v.type, v.lit};
		}
		break;
	}
	return innerType;
}

typer dataTyper::typeFunctionInnerNodes(astnode &n){
	if (n == nullptr) return {0,0};
	typer innerType = {0}, paramType = {0};
	switch (n->funcid()){
	case FN_COUNT:
		n->keep = true;
		typeInnerNodes(n->nsubexpr());
		innerType = {T_FLOAT, false};
		break;
	case FN_FORMAT:
	case FN_MONTHNAME:
	case FN_WDAYNAME:
		n->keep = true;
		typeInnerNodes(n->nsubexpr());
		innerType = {T_STRING, false};
		n->funcreturntype() = innerType.type;
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
		paramType = typeInnerNodes(n->nsubexpr());
		innerType = {T_STRING, false};
		n->funcreturntype() = innerType.type;
		if (!canBeString(n->nsubexpr()))
			n->origparamtype() = paramType.type;
		break;
	case FN_POW:{
		auto n1 = typeInnerNodes(n->node1);
		auto n2 = typeInnerNodes(n->node2);
		innerType = typeCompute(n1, n2);
		n->funcreturntype() = innerType.type;
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
		typeInnerNodes(n->nsubexpr());
		innerType = {T_INT, true}; //TODO: reconsider these true/false values
		n->funcreturntype() = T_INT;
		break;
	case FN_STDEV:
	case FN_STDEVP:
	case FN_AVG:
		innerType = typeInnerNodes(n->nsubexpr());
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
		if (auto pt = typeInnerNodes(n->nsubexpr()).type; pt > T_FLOAT)
			n->origparamtype() = pt;
		innerType = {T_FLOAT, false};
		n->funcreturntype() = T_FLOAT;
		break;
	case FN_NOW:
	case FN_NOWGM:
		innerType = {T_DATE, false};
		n->funcreturntype() = T_DATE;
		break;
	case FN_LEN:
		innerType = {T_FLOAT, false};
		n->funcreturntype() = T_FLOAT;
		n->tok2.id = typeInnerNodes(n->nsubexpr()).type;
		if (canBeString(n->nsubexpr()))
			n->tok2.id = T_STRING;
		break;
	case FN_SIP:
		innerType = {T_INT, false};
		n->funcreturntype() = T_INT;
		n->tok2.id = typeInnerNodes(n->nsubexpr()).type;
		if (canBeString(n->nsubexpr()))
			n->tok2.id = T_STRING;
		break;
	case FN_INT:
		innerType = {T_INT, false};
		n->origparamtype() = typeInnerNodes(n->nsubexpr()).type;
		n->funcreturntype() = T_INT;
		break;
	case FN_FLOAT:
		innerType = {T_FLOAT, false};
		n->origparamtype() = typeInnerNodes(n->nsubexpr()).type;
		n->funcreturntype() = T_FLOAT;
		break;
	case FN_DATE:
		innerType = {T_DATE, false};
		n->origparamtype() = typeInnerNodes(n->nsubexpr()).type;
		n->funcreturntype() = T_DATE;
		break;
	case FN_DUR:
		innerType = {T_DURATION, false};
		n->origparamtype() = typeInnerNodes(n->nsubexpr()).type;
		n->funcreturntype() = T_DURATION;
		break;

	default:
		innerType = typeInnerNodes(n->nsubexpr());
	}
	return innerType;
}

//set datatype value of inner tree nodes where applicable
typer dataTyper::typeInnerNodes(astnode &n){
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
	case N_FILE:
	case N_WHERE:
	case N_HAVING:
		typeInnerNodes(n->node1);
		typeInnerNodes(n->node2);
		typeInnerNodes(n->node3);
		typeInnerNodes(n->node4);
		break;
	case N_ORDER:
		innerType = typeInnerNodes(n->norderlist());
		if (q->sorting)
			q->sorting = innerType.type;
		break;
	//things that may be list but have independant types
	case N_SETLIST:
		if (n->tok1.id){
			auto& tv = q->subqueries[n->subqidx()].topinnertypes.get();
			if (!tv.size()) error("Subquery has no datatypes");
			innerType = {tv[0].first, false};
			for (auto& t: tv)
				innerType = typeCompute({t.first,false}, {innerType.type,false});
			break;
		}
	case N_SELECTIONS:
	case N_GROUPBY:
	case N_EXPRESSIONS:
		innerType = typeInnerNodes(n->node1);
		typeInnerNodes(n->node2);
		break;
	case N_VARS:
		innerType = typeInnerNodes(n->nsubexpr());
		{
			auto& v = q->var(n->varname());
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
		innerType = typeInnerNodes(n->ncaseresultexpr()); //result
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
		error("missed a node type: ",n->nodelabel());
	}
	n->datatype = innerType.type;
	//cerr << "type node " << treeMap[n->label] << " as " << n->datatype << endl;
	return innerType;
}

void dataTyper::typeCaseFinalNodes(astnode &n, int finaltype){
	if (n == nullptr) return;
	int comptype;
	switch (n->casenodetype()){
	//case statement
	case KW_CASE:
		switch (n->casetype()){
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
		typeFinalValues(n->nsubexpr(), finaltype);
	}
}

void dataTyper::typePredCompFinalNodes(astnode &n){
	if (n == nullptr) return;
	if (n->tok1 == SP_LPAREN){
		typeFinalValues(n->nmorepreds(), -1);
	} else if (n->tok1.id & RELOP) {
		typeFinalValues(n->node1, n->datatype);
		typeFinalValues(n->node2, n->datatype);
		typeFinalValues(n->node3, n->datatype);
	}
}

void dataTyper::typeFunctionFinalNodes(astnode &n, int finaltype){
	if (n == nullptr) return;
	int oldret = finaltype;
	if (n->keep) finaltype = -1;
	node *convNode, *tempNode;
	bool needRetConvert = false;
	//TODO: find out why now() return is not using dynamic typing
	if (auto rt = n->funcreturntype(); rt > 0 && finaltype != rt){
		//need conversion from RETTYPE (can do) to original finaltype (need)
		n->datatype = finaltype = rt;
		needRetConvert = true;
	}
	switch (n->funcid()){
	case FN_SIP:
	case FN_LEN:{
		if (n->funcparamtype() == T_STRING){
			typeFinalValues(n->nsubexpr(), T_STRING);
			break;
		}
		finaltype = n->funcparamtype();
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
		if (auto pt = n->origparamtype(); pt){
			typeFinalValues(n->node1, pt);
			typeFinalValues(n->node2, pt);
			convNode = new node(N_TYPECONV);
			convNode->keep = true;
			convNode->datatype = finaltype;
			convNode->convfromtype() = pt;
			tempNode = n->nsubexpr().release();
			convNode->nsubexpr().reset(tempNode);
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
			convNode->convfromtype() = n->funcreturntype();
			tempNode = n.release();
			convNode->nsubexpr().reset(tempNode);
			n.reset(convNode);
	}
}

//use high-level nodes to give lower nodes their final types
void dataTyper::typeFinalValues(astnode &n, int finaltype){
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
	case N_FILE:
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
	case N_SETLIST:
		if (n->hassubquery()){
			auto& sq = q->subqueries[n->subqidx()];
			auto sz = sq.topinnertypes.get().size();
			sq.singleDatatype = finaltype;
			sq.topfinaltypesp.set_value(vector<int>(sz, finaltype));
			break;
		}
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
		if (n->mathop() && (finaltype == T_STRING || finaltype == T_DATE))
			error("Minus sign does not work with type ",gettypename(finaltype));
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
		typeFinalValues(n->node1, finaltype); //function
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

void dataTyper::checkMathSemantics(astnode &n){
	if (n->label != N_EXPRADD && n->label != N_EXPRMULT)
		return;
	auto n1 = n->node1 == nullptr ? T_NULL : n->node1->datatype;
	auto n2 = n->node2 == nullptr ? T_NULL : n->node2->datatype;
	auto combo = [&](int a, int b){ return (n1 == a && n2 == b) || (n1 == b && n2 == a); };
	auto is = [&](int t){ return n->datatype == t; };
	auto typestr = gettypename(n->datatype);

	switch (n->mathop()){
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
			error("Cannot multiply type ",typestr);
		if (combo(T_DURATION, T_DURATION))
			error("cannot multiply 2 durations");
		return;
	case SP_MOD:
		if (!is(T_INT) && !is(T_FLOAT))
			error("Cannot modulus type ",typestr);
		return;
	case SP_CARROT:
		if (!is(T_INT) && !is(T_FLOAT))
			error("Cannot exponentiate type ",typestr);
		return;
	case SP_DIV:
		if (is(T_STRING) || is(T_DATE))
			error("Cannot divide type ",typestr);
		if (n1 != T_DURATION && n2 == T_DURATION)
			error("Cannot divide number by duration");
		return;
	}
}

void dataTyper::checkFuncSemantics(astnode &n){
	auto n1 = n->node1 == nullptr ? T_NULL : n->node1->datatype;
	auto typestr = gettypename(n->datatype);
	char* e = NULL;

	switch (n->funcid()){
	case FN_SUM:
	case FN_AVG:
	case FN_STDEV:
	case FN_STDEVP:
		if (n1 == T_STRING || n1 == T_DATE || n1 == T_DURATION)
			error("Cannot use ",n->tok1," function with type ",typestr);
		//TODO: implement stdev and avg for date and duration
		break;
	case FN_ABS:
		if (n1 == T_STRING || n1 == T_DATE)
			error("Cannot use ",n->tok1," function with type ",typestr);
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
			error(n->tok1," function 2nd value must be integer from -9 to 9");
		if (n->tok3 == SP_MINUS)
			n->tok2.id *= -1;
		break;
	case FN_ENCRYPT:
	case FN_DECRYPT:
		break;
	}
}
void dataTyper::selectallToptypes(){
	for (auto &f : q->filevec)
		for (auto &c: f->colnames)
			topitypes.push_back({T_NULL,true}); //turn to string if finaltype still null
}
void dataTyper::getToptypes(){
	auto n = findFirstNode(q->tree, N_SELECTIONS).get();
	if (n == nullptr){
		selectallToptypes();
	} else while (n){
		if (n->startok() == SP_STAR){
			selectallToptypes();
		} else {
			topitypes.push_back({n->datatype,canBeString(n->nsubexpr())});
		}
		n = n->nnextselection().get();
	}
	q->thisSq->topinnertypesp.set_value(move(topitypes));
}
void dataTyper::setToptypes(){
	topftypes = q->thisSq->topfinaltypes.get();
	auto n = findFirstNode(q->tree, N_SELECTIONS).get();
	int i = 0;
	while (n){
		n->datatype = topftypes[i++];
		if (n->datatype == T_NULL)
			error("Subquery has null datatype");
		n = n->nnextselection().get();
	}
}

void applyTypes(querySpecs &q){
	dataTyper dt(q);
	dt.typeInitialValue(q.tree, false);
	dt.typeInnerNodes(q.tree);
	if (q.isSubquery){
		dt.getToptypes();
		dt.setToptypes();
	}
	dt.typeFinalValues(q.tree, -1);
}
