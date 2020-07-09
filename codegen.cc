#include "interpretor.h"
#include "vmachine.h"

class cgen {

	int normal_read;
	int agg_phase; //0 is not grouping, 1 is first read, 2 is aggregate retrieval
	int select_count;
	querySpecs* q;

	public:
	void generateCode();
	void genNormalOrderedQuery(unique_ptr<node> &n);
	void genNormalQuery(unique_ptr<node> &n);
	void genBasicGroupingQuery(unique_ptr<node> &n);
	void genBasicJoiningQuery(unique_ptr<node> &n);
	void genAggSortList(unique_ptr<node> &n);
	void genVars(unique_ptr<node> &n, varScoper* vo);
	void genWhere(unique_ptr<node> &n);
	void genDistinct(unique_ptr<node> &n, int gotoIfNot);
	void genGetGroup(unique_ptr<node> &n);
	void genSelect(unique_ptr<node> &n);
	void genExprAll(unique_ptr<node> &n);
	void genExprAdd(unique_ptr<node> &n);
	void genExprMult(unique_ptr<node> &n);
	void genExprNeg(unique_ptr<node> &n);
	void genExprCase(unique_ptr<node> &n);
	void genCPredList(unique_ptr<node> &n, int end);
	void genCWExprList(unique_ptr<node> &n, int end);
	void genNormalSortList(unique_ptr<node> &n);
	void genCPred(unique_ptr<node> &n, int end);
	void genCWExpr(unique_ptr<node> &n, int end);
	void genPredicates(unique_ptr<node> &n);
	void genPredCompare(unique_ptr<node> &n);
	void genValue(unique_ptr<node> &n);
	void genFunction(unique_ptr<node> &n);
	void genSelectAll();
	void genSelections(unique_ptr<node> &n);
	void genTypeConv(unique_ptr<node> &n);
	void genIterateGroups(unique_ptr<node> &n, varScoper &vs);
	void genUnsortedGroupRow(unique_ptr<node> &n, varScoper &vs, int nextgroup, int doneGroups);
	void genSortedGroupRow(unique_ptr<node> &n, varScoper &vs, int nextgroup);
	void finish();

	vector<opcode> v;
	cgen(querySpecs &qs){
		q = &qs;
		select_count = 0;
		agg_phase = 0;
	}
};

//for debugging
static int ident = 0;
//#define e //turn off debug printer
#ifndef e
#define e(A) for (int i=0; i< ident; i++) cerr << "    "; \
	cerr << A << endl; ident++; \
	shared_ptr<void> _(nullptr, [&n](...){ \
	ident--; for (int i=0; i< ident; i++) cerr << "    "; \
	cerr << "done " << A << endl; });
#endif

#define pushvars() for (auto &i : q->vars) addop(v, PUSH);
#define popvars() for (auto &i : q->vars) addop(v, POP);


void jumpPositions::updateBytecode(vector<opcode> &vec) {
	for (auto &v : vec)
		switch (v.code){
		case JMP:
		case JMPCNT:
		case JMPTRUE:
		case JMPFALSE:
		case RDLINE:
		case RDLINE_ORDERED:
		case NULFALSE1:
		case NULFALSE2:
		case NDIST:
		case SDIST:
		case JMPNOTNULL_ELSEPOP:
		case NEXTMAP:
		case NEXTVEC:
		case READ_NEXT_GROUP:
			if (v.p1 < 0)
				v.p1 = jumps[v.p1];
		}
};


void cgen::generateCode(){

	int whatdo = 0;
	if (q->sorting)  whatdo = 1;
	if (q->grouping) whatdo|= 2;
	if (q->joining)  whatdo|= 4;

	switch (whatdo){
	case 0:
		genNormalQuery(q->tree);
		break;
	case 1:
		genNormalOrderedQuery(q->tree);
		break;
	case 1|2:
	case 2:
		genBasicGroupingQuery(q->tree);
		break;
	case 4:
		genBasicJoiningQuery(q->tree);
		break;
	case 1|4:
		break;
	case 1|2|4:
		break;
	case 2|4:
		break;
	}

	q->jumps.updateBytecode(v);
	finish();
}

void cgen::finish(){
	int i = 0;
	for (auto c : v){
		cerr << "ip: " << left << setw(4) << i++;
		c.print();
	}
	q->bytecode = move(v);
}

void codeGen(querySpecs &q){
	cgen cg(q);
	cg.generateCode();
}

//generate bytecode for expression nodes
void cgen::genExprAll(unique_ptr<node> &n){
	if (n == nullptr) return;
	switch (n->label){
	case N_EXPRADD:         genExprAdd     (n); break;
	case N_EXPRNEG:         genExprNeg     (n); break;
	case N_EXPRMULT:        genExprMult    (n); break;
	case N_EXPRCASE:        genExprCase    (n); break;
	case N_PREDICATES:      genPredicates  (n); break;
	case N_PREDCOMP:        genPredCompare (n); break;
	case N_VALUE:           genValue       (n); break;
	case N_FUNCTION:        genFunction    (n); break;
	case N_TYPECONV:        genTypeConv    (n); break;
	}
}


//given q.tree as node param
void cgen::genBasicJoiningQuery(unique_ptr<node> &n){
}
void cgen::genNormalQuery(unique_ptr<node> &n){
	e("normal");
	int endfile = q->jumps.newPlaceholder(); //where to jump when done reading file
	varScoper vs;
	pushvars();
	normal_read = v.size();
	addop(v, RDLINE, endfile, 0);
	genVars(n->node1, vs.setscope(WHERE_FILTER|DISTINCT_FILTER, V_EQUALS, V_SCOPE1));
	genVars(n->node1, vs.setscope(WHERE_FILTER, V_EQUALS, V_SCOPE1));
	genWhere(n->node4);
	genVars(n->node1, vs.setscope(DISTINCT_FILTER, V_EQUALS, V_SCOPE1));
	genDistinct(n->node2->node1, normal_read);
	genVars(n->node1, vs.setscope(NO_FILTER, V_ANY, V_SCOPE1));
	genSelect(n->node2);
	addop(v, PRINT);
	addop(v, (q->quantityLimit > 0 ? JMPCNT : JMP), normal_read);
	q->jumps.setPlace(endfile, v.size());
	popvars();
	addop(v, ENDRUN);
}
void cgen::genNormalOrderedQuery(unique_ptr<node> &n){
	int sorter = q->jumps.newPlaceholder(); //where to jump when done scanning file
	int reread = q->jumps.newPlaceholder();
	int endreread = q->jumps.newPlaceholder();
	varScoper vs;
	pushvars();
	normal_read = v.size();
	addop(v, RDLINE, sorter, 0);
	genVars(n->node1, vs.setscope(WHERE_FILTER|ORDER_FILTER, V_INCLUDES, V_SCOPE1));
	genWhere(n->node4);
	genNormalSortList(n);
	addop(v, SAVEPOS);
	addop(v, JMP, normal_read);
	q->jumps.setPlace(sorter, v.size());
	addop(v, SORT);
	addop(v, PREP_REREAD, 0, 0);
	q->jumps.setPlace(reread, v.size());
	addop(v, RDLINE_ORDERED, endreread, 0, 0);
	genVars(n->node1, vs.setscope(DISTINCT_FILTER, V_EQUALS, V_SCOPE2));
	genDistinct(n->node2->node1, reread);
	genVars(n->node1, vs.setscope(NO_FILTER, V_ANY, V_SCOPE2));
	genSelect(n->node2);
	addop(v, PRINT);
	addop(v, (q->quantityLimit > 0 ? JMPCNT : JMP), reread);
	q->jumps.setPlace(endreread, v.size());
	addop(v, POP); //rereader used 2 stack spaces
	addop(v, POP);
	popvars();
	addop(v, ENDRUN);
};
 //update sorter to work with list
void cgen::genNormalSortList(unique_ptr<node> &n){
	auto& ordnode = findFirstNode(q->tree->node4, N_ORDER);
	int i = 0;
	for (auto x = ordnode->node1.get(); x; x = x->node2.get()){
		genExprAll(x->node1);
		if (x->datatype == T_STRING)
			addop(v, NUL_TO_STR);
		addop(v, ops[OPSVSRT][x->datatype], i++);
		q->sortInfo.push_back({x->tok1.id, x->datatype});
	}
}
void cgen::genBasicGroupingQuery(unique_ptr<node> &n){
	e("grouping");
	agg_phase = 1;
	int getgroups = q->jumps.newPlaceholder();
	varScoper vs;
	pushvars();
	normal_read = v.size();
	addop(v, RDLINE, getgroups, 0);
	genVars(n->node1, vs.setscope(WHERE_FILTER|DISTINCT_FILTER, V_EQUALS, V_SCOPE1));
	genVars(n->node1, vs.setscope(WHERE_FILTER, V_EQUALS, V_SCOPE1));
	genWhere(n->node4);
	genVars(n->node1, vs.setscope(DISTINCT_FILTER, V_EQUALS, V_SCOPE1));
	genDistinct(n->node2->node1, normal_read); //is this needed here?
	genVars(n->node1, vs.setscope(GROUP_FILTER, V_INCLUDES, V_SCOPE1));
	genGetGroup(n->node4->node2);
	genVars(n->node1, vs.setscope(NO_FILTER, V_ANY, V_SCOPE1));
	genPredicates(n->node4->node3); //having phase 1
	genSelect(n->node2);
	genAggSortList(n); //figure out how to do phase 2
	addop(v, JMP, normal_read);
	q->jumps.setPlace(getgroups, v.size());
	agg_phase = 2;
	genIterateGroups(n->node4->node2, vs);
	popvars();
	addop(v, ENDRUN);
}

void cgen::genAggSortList(unique_ptr<node> &n){
	if (n == nullptr) return;
	switch (n->label){
	case N_QUERY:     genAggSortList(n->node4->node4); break;
	case N_ORDER:     genAggSortList(n->node1); break;
	case N_EXPRESSIONS:
		genExprAdd(n->node1);
		genAggSortList(n->node2);
	}
}

void cgen::genVars(unique_ptr<node> &n, varScoper* vs){
	if (n == nullptr) return;
	e("gen vars");
	int i;
	int match = 0;
	switch (n->label){
	case N_PRESELECT: //currently only has 'with' branch
	case N_WITH:
		genVars(n->node1, vs);
		break;
	case N_VARS:
		i = getVarIdx(n->tok1.val, q);
		if (vs->neededHere(i, q->vars[i].filter)){
			genExprAll(n->node1);
			if (n->phase == (1|2)){
				//non-aggs in phase2
				if (agg_phase == 1){
					addop2(v, PUTVAR2, i, n->tok3.id);
				} else {
					addop1(v, LDMID, n->tok3.id);
					addop1(v, PUTVAR, i);
				}
			} else {
				addop1(v, PUTVAR, i);
				int vartype = getVarType(n->tok1.val, q);
				if (agg_phase == 2 && q->sorting && vartype == T_STRING)
					addop1(v, HOLDVAR, i);
			}
		}
		genVars(n->node2, vs);
		break;
	}
}

void cgen::genExprAdd(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen add");
	genExprAll(n->node1);
	if (!n->tok1.id) return;
	genExprAll(n->node2);
	switch (n->tok1.id){
	case SP_PLUS:
		addop0(v, ops[OPADD][n->datatype]);
		break;
	case SP_MINUS:
		addop0(v, ops[OPSUB][n->datatype]);
		break;
	}
}

void cgen::genExprMult(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen mult");
	genExprAll(n->node1);
	if (!n->tok1.id) return;
	genExprAll(n->node2);
	switch (n->tok1.id){
	case SP_STAR:
		addop0(v, ops[OPMULT][n->datatype]);
		break;
	case SP_DIV:
		addop0(v, ops[OPDIV][n->datatype]);
		break;
	case SP_CARROT:
		addop0(v, ops[OPEXP][n->datatype]);
		break;
	case SP_MOD:
		addop0(v, ops[OPMOD][n->datatype]);
		break;
	}
}

void cgen::genExprNeg(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen neg");
	genExprAll(n->node1);
	if (!n->tok1.id) return;
	addop0(v, ops[OPNEG][n->datatype]);
}

void cgen::genValue(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen value: "+n->tok1.val);
	dat lit;
	int vtype, op, aggvar;
	switch (n->tok2.id){
	case COLUMN:
		addop2(v, ops[OPLD][n->datatype], getFileNo(n->tok3.val, q), n->tok1.id);
		break;
	case LITERAL:
		switch (n->datatype){
		case T_INT:      lit = parseIntDat(n->tok1.val.c_str());      break;
		case T_FLOAT:    lit = parseFloatDat(n->tok1.val.c_str());    break;
		case T_DATE:     lit = parseDateDat(n->tok1.val.c_str());     break;
		case T_DURATION: lit = parseDurationDat(n->tok1.val.c_str()); break;
		case T_STRING:   lit = parseStringDat(n->tok1.val.c_str());   break;
		}
		if (n->tok1.lower() == "null"){
			addop0(v, LDNULL);
		} else {
			addop1(v, LDLIT, q->dataholder.size());
			q->dataholder.push_back(lit);
		}
		break;
	case VARIABLE:
		addop1(v, LDVAR, getVarIdx(n->tok1.val, q));
		//variable may be used in operations with different types
		vtype = getVarType(n->tok1.val, q);
		op = typeConv[vtype][n->datatype];
		if (op == CVER)
			error(ft("Error converting variable of type {} to new type {}", vtype, n->datatype));
		if (op != CVNO)
			addop0(v, op);
		break;
	case FUNCTION:
		genExprAll(n->node1);
		break;
	}
}

void cgen::genExprCase(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen case");
	int caseEnd = q->jumps.newPlaceholder();
	switch (n->tok1.id){
	case KW_CASE:
		switch (n->tok2.id){
		//when predicates are true
		case KW_WHEN:
			genCPredList(n->node1, caseEnd);
			genExprAll(n->node3);
			if (n->node3 == nullptr)
				addop0(v, LDNULL);
			q->jumps.setPlace(caseEnd, v.size());
			break;
		//expression matches expression list
		case WORD_TK:
		case SP_LPAREN:
			genExprAll(n->node1);
			genCWExprList(n->node2, caseEnd);
			addop0(v, POP); //don't need comparison value anymore
			genExprAll(n->node3);
			if (n->node3 == nullptr)
				addop0(v, LDNULL);
			q->jumps.setPlace(caseEnd, v.size());
			break;
		}
		break;
	case SP_LPAREN:
	case WORD_TK:
		genExprAll(n->node1);
	}
}

void cgen::genCWExprList(unique_ptr<node> &n, int end){
	if (n == nullptr) return;
	e("gen case w list");
	genCWExpr(n->node1, end);
	genCWExprList(n->node2, end);
}

void cgen::genCWExpr(unique_ptr<node> &n, int end){
	if (n == nullptr) return;
	e("gen case w expr");
	int next = q->jumps.newPlaceholder(); //get jump pos for next try
	genExprAll(n->node1); //evaluate comparision expression
	addop1(v, ops[OPEQ][n->tok1.id], 0); //leave '=' result where this comp value was
	addop2(v, JMPFALSE, next, 1);
	addop0(v, POP); //don't need comparison value anymore
	genExprAll(n->node2); //result value if eq
	addop1(v, JMP, end);
	q->jumps.setPlace(next, v.size()); //jump here for next try
}

void cgen::genCPredList(unique_ptr<node> &n, int end){
	if (n == nullptr) return;
	e("gen case p list");
	genCPred(n->node1, end);
	genCPredList(n->node2, end);
}

void cgen::genCPred(unique_ptr<node> &n, int end){
	if (n == nullptr) return;
	e("gen case p");
	int next = q->jumps.newPlaceholder(); //get jump pos for next try
	genPredicates(n->node1);
	addop2(v, JMPFALSE, next, 1);
	genExprAll(n->node2); //result value if true
	addop1(v, JMP, end);
	q->jumps.setPlace(next, v.size()); //jump here for next try
}

void cgen::genSelect(unique_ptr<node> &n){
	genSelections(n->node1);
}

void cgen::genSelections(unique_ptr<node> &n){
	if (n == nullptr) {
		//reached end of selections section of query
		if (!select_count) genSelectAll();
		return;
	}
	e("gen selections");
	string t1 = n->tok1.lower();
	switch (n->label){
	case N_SELECTIONS:
		if (t1 == "hidden") {

		} else if (t1 == "distinct") {
			addop1(v, PUTDIST, n->tok4.id);
			incSelectCount();

		} else if (t1 == "*") {
			genSelectAll();

		} else if (isTrivial(n)) {
			switch (agg_phase){
			case 0:
				for (auto nn = n.get(); nn; nn = nn->node1.get()) if (nn->label == N_VALUE){
					addop3(v, LDPUT, n->tok4.id, nn->tok1.id, getFileNo(nn->tok3.val, q));
					break;
				} break;
			case 1:
				for (auto nn = n.get(); nn; nn = nn->node1.get()) if (nn->label == N_VALUE){
					addop3(v, LDPUTGRP, n->tok3.id, nn->tok1.id, getFileNo(nn->tok3.val, q));
					break;
				} break;
			case 2:
				addop2(v, LDPUTMID, n->tok4.id, n->tok3.id);
				break;
			}
			incSelectCount();

		} else {
			genExprAll(n->node1);
			addop2(v, PUT, n->tok4.id, agg_phase==1?1:0);
			incSelectCount();
		}
		break;
	default:
		error("selections generator error");
		return;
	}
	genSelections(n->node2);
}

void cgen::genPredicates(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen preds");
	genPredCompare(n->node1);
	int doneAndOr = q->jumps.newPlaceholder();
	switch (n->tok1.id){
	case KW_AND:
		addop2(v, JMPFALSE, doneAndOr, 0);
		addop0(v, POP); //don't need old result
		genPredicates(n->node2);
		break;
	case KW_OR:
		addop2(v, JMPTRUE, doneAndOr, 0);
		addop0(v, POP); //don't need old result
		genPredicates(n->node2);
		break;
	case KW_XOR:
		break;
	}
	q->jumps.setPlace(doneAndOr, v.size());
	if (n->tok2.id == SP_NEGATE)
		addop0(v, PNEG); //can optimize be returning value to genwhere and add opposite jump code
}

void cgen::genPredCompare(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen pred compare");
	dat reg;
	int negation = n->tok2.id;
	int endcomp, greaterThanExpr3;
	genExprAll(n->node1);
	switch (n->tok1.id){
	case SP_NOEQ: negation ^= 1;
	case SP_EQ:
		genExprAll(n->node2);
		addop2(v, ops[OPEQ][n->datatype], 1, negation);
		break;
	case SP_GREATEQ: negation ^= 1;
	case SP_LESS:
		genExprAll(n->node2);
		addop2(v, ops[OPLT][n->datatype], 1, negation);
		break;
	case SP_GREAT: negation ^= 1;
	case SP_LESSEQ:
		genExprAll(n->node2);
		addop2(v, ops[OPLEQ][n->datatype], 1, negation);
		break;
	case KW_BETWEEN:
		endcomp = q->jumps.newPlaceholder();
		greaterThanExpr3 = q->jumps.newPlaceholder();
		addop1(v, NULFALSE1, endcomp);
		genExprAll(n->node2);
		addop1(v, NULFALSE2, endcomp);
		addop2(v, ops[OPLT][n->datatype], 0, 1);
		addop2(v, JMPFALSE, greaterThanExpr3, 1);
		genExprAll(n->node3);
		addop1(v, NULFALSE2, endcomp);
		addop2(v, ops[OPLT][n->datatype], 1, negation);
		addop1(v, JMP, endcomp);
		q->jumps.setPlace(greaterThanExpr3, v.size());
		genExprAll(n->node3);
		addop1(v, NULFALSE2, endcomp);
		addop2(v, ops[OPLT][n->datatype], 1, negation^1);
		q->jumps.setPlace(endcomp, v.size());
		break;
	case KW_IN:
		endcomp = q->jumps.newPlaceholder();
		for (auto nn=n->node2.get(); nn; nn=nn->node2.get()){
			genExprAll(nn->node1);
			addop1(v, ops[OPEQ][n->node1->datatype], 0);
			addop2(v, JMPTRUE, endcomp, 0);
			if (nn->node2.get())
				addop0(v, POP);
		}
		q->jumps.setPlace(endcomp, v.size());
		if (negation)
			addop0(v, PNEG);
		addop0(v, POPCPY); //put result where 1st expr was
		break;
	case KW_LIKE:
		addop2(v, LIKE, q->dataholder.size(), negation);
		reg.u.r = new regex_t;
		reg.b = RMAL;
		boost::replace_all(n->tok3.val, "_", ".");
		boost::replace_all(n->tok3.val, "%", ".*");
		if (regcomp(reg.u.r, ("^"+n->tok3.val+"$").c_str(), REG_EXTENDED|REG_ICASE))
			error("Could not parse 'like' pattern");
		q->dataholder.push_back(reg);
		break;
	}
}

void cgen::genSelectAll(){
	addop(v, LDPUTALL, select_count);
	for (int i=1; i<=q->numFiles; ++i)
		select_count += q->files[str2("_f", i)]->numFields;
}

void cgen::genWhere(unique_ptr<node> &nn){
	auto& n = findFirstNode(nn, N_WHERE);
	e("gen where");
	if (n == nullptr) return;
	genPredicates(n->node1);
	addop2(v, JMPFALSE, normal_read, 1);
}

void cgen::genDistinct(unique_ptr<node> &n, int gotoIfNot){
	if (n == nullptr) return;
	e("gen distinct");
	if (n->label != N_SELECTIONS) return;
	if (n->tok1.id == KW_DISTINCT){
		genExprAll(n->node1);
		addop2(v, ops[OPDIST][n->datatype], gotoIfNot, addBtree(n->datatype, q));
	} else {
		//there can be only 1 distinct filter
		genDistinct(n->node2, gotoIfNot);
	}
}

void cgen::genFunction(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen function");
	int funcDone = q->jumps.newPlaceholder();
	int idx;

	//stuff common to all aggregate functions
	if ((n->tok1.id & AGG_BIT) != 0 ) {
		genExprAll(n->node1);
		if (n->tok3.val == "distinct" && agg_phase == 1){
			int setIndex = n->tok4.id;
			int separateSets = 1;
			if (q->grouping == 1){ //when onegroup, btree not indexed by rowgroup
				setIndex = addBtree(n->node1->datatype, q);
				separateSets = 0;
			}
			addop(v, ops[OPDIST][n->node1->datatype], funcDone, setIndex, separateSets);
			addop(v, LDDIST);
		}
	}

	switch (n->tok1.id){
	//non-aggregates
	case FN_COALESCE:
		for (auto nn = n->node1.get(); nn; nn = nn->node2.get()){
			genExprAll(nn->node1);
			addop1(v, JMPNOTNULL_ELSEPOP, funcDone);
		}
		addop0(v, LDNULL);
		break;
	case FN_INC:
		addop1(v, FINC, q->dataholder.size());
		q->dataholder.push_back(dat{ {.f = 0.0}, T_FLOAT});
		break;
	case FN_ENCRYPT:
		genExprAll(n->node1);
		if (n->tok3.val == "chacha"){
			idx = q->crypt.newChacha(n->tok4.val);
			addop1(v, ENCCHA, idx);
		} else /* aes */ {
		}
		break;
	case FN_DECRYPT:
		genExprAll(n->node1);
		if (n->tok3.val == "chacha"){
			idx = q->crypt.newChacha(n->tok4.val);
			addop1(v, DECCHA, idx);
		} else /* aes */ {
		}
		break;
	}

	//aggregates
	if (agg_phase == 1) {
		switch (n->tok1.id){
		case FN_SUM:
			addop(v, ops[OPSUM][n->datatype], n->tok6.id);
			break;
		case FN_AVG:
			addop(v, ops[OPAVG][n->datatype], n->tok6.id);
			break;
		case FN_STDEV:
		case FN_STDEVP:
			addop(v, ops[OPSTV][n->datatype], n->tok6.id);
			break;
		case FN_MIN:
			addop(v, ops[OPMIN][n->datatype], n->tok6.id);
			break;
		case FN_MAX:
			addop(v, ops[OPMAX][n->datatype], n->tok6.id);
			break;
		case FN_COUNT:
			addop(v, COUNT, n->tok6.id, n->tok2.id ? 1 : 0);
			break;
		}
		select_count++;
	} else if (agg_phase == 2) {
		switch (n->tok1.id){
		case FN_AVG:
			addop(v, ops[OPLAVG][n->datatype], n->tok6.id);
			break;
		case FN_STDEV:
			addop(v, ops[OPLSTV][n->datatype], n->tok6.id, 1);
			break;
		case FN_STDEVP:
			addop(v, ops[OPLSTV][n->datatype], n->tok6.id);
			break;
		case FN_MIN:
		case FN_MAX:
		case FN_SUM:
		case FN_COUNT:
			addop(v, LDMID, n->tok6.id);
			break;
		}
	}
	q->jumps.setPlace(funcDone, v.size());
}

void cgen::genTypeConv(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("type convert");
	genExprAll(n->node1);
	addop0(v, typeConv[n->tok1.id][n->datatype]);
}

void cgen::genGetGroup(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("get group");
	if (n->label == N_GROUPBY){
		int depth = -1;
		for (auto nn=n->node1.get(); nn; nn=nn->node2.get()){
			depth++;
			genExprAll(nn->node1);
		}
		addop1(v, GETGROUP, depth);
	}
}

void cgen::genIterateGroups(unique_ptr<node> &n, varScoper &vs){
	e("iterate groups");
	if (n == nullptr) {
		addop(v, ONEGROUP);
		genVars(q->tree->node1, vs.setscope(NO_FILTER, V_ANY, V_SCOPE2));
		genSelect(q->tree->node2);
		// for debugging:
		addop(v, PRINT);
		return;
	}
	if (n->label == N_GROUPBY){
		int doneGroups = q->jumps.newPlaceholder();
		int depth = 0;
		int goWhenDone;
		int prevmap;
		addop(v, ROOTMAP, depth);
		for (auto nn=n->node1.get(); nn; nn=nn->node2.get()){
			if (depth == 0) {
				goWhenDone = doneGroups;
			} else {
				goWhenDone = prevmap;
			}
			if (nn->node2.get()){
				depth += 2;
				prevmap = v.size();
				addop(v, NEXTMAP, goWhenDone, depth);
			} else {
				int nextvec = v.size();
				addop(v, NEXTVEC, goWhenDone, depth);
				if (q->sorting){
					genSortedGroupRow(n, vs, nextvec);
				} else {
					genUnsortedGroupRow(n, vs, nextvec, doneGroups);
				}
			}
		}
		q->jumps.setPlace(doneGroups, v.size());

		//maybe put this part in higher level function?
		if (q->sorting){
			auto& ordnode1 = findFirstNode(q->tree->node4, N_ORDER)->node1;
			int doneReadGroups = q->jumps.newPlaceholder();
			addop(v, GSORT, ordnode1->tok3.id);
			addop(v, PUSH_0);
			int readNext = v.size();
			addop(v, READ_NEXT_GROUP, doneReadGroups);
			addop(v, PRINT);
			addop(v, (q->quantityLimit > 0 ? JMPCNT : JMP), readNext);
			q->jumps.setPlace(doneReadGroups, v.size());
			addop(v, POP);
		}
	}
}

void cgen::genUnsortedGroupRow(unique_ptr<node> &n, varScoper &vs, int nextgroup, int doneGroups){
	e("gen unsorted group row");
	genVars(q->tree->node1, vs.setscope(NO_FILTER, V_ANY, V_SCOPE2));
	genPredicates(q->tree->node4->node3);
	if (q->havingFiltering)
		addop(v, JMPFALSE, nextgroup, 1);
	genSelect(q->tree->node2);
	// for debugging:
	addop(v, PRINT);
	addop(v, (q->quantityLimit > 0 ? JMPCNT : JMP), nextgroup);
	addop(v, JMP, doneGroups);
}
void cgen::genSortedGroupRow(unique_ptr<node> &n, varScoper &vs, int nextgroup){
	e("gen sorted group row");
	genVars(q->tree->node1, vs.setscope(NO_FILTER, V_ANY, V_SCOPE2));
	genPredicates(q->tree->node4->node3);
	if (q->havingFiltering)
		addop(v, JMPFALSE, nextgroup, 1);
	addop(v, ADD_GROUPSORT_ROW);
	genSelect(q->tree->node2);
	auto& ordnode = findFirstNode(q->tree->node4, N_ORDER);
	for (auto x = ordnode->node1.get(); x; x = x->node2.get()){
		genExprAll(x->node1);
		if (x->datatype == T_STRING)
			addop(v, NUL_TO_STR);
		addop(v, PUT, x->tok3.id);
		q->sortInfo.push_back({x->tok1.id, x->datatype});
	}
	addop(v, FREEMIDROW);
	addop(v, JMP, nextgroup);
}
