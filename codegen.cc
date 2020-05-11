#include "interpretor.h"
#include "vmachine.h"

static void genNormalOrderedQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genNormalQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genBasicGroupingQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genVars(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, varScoper* vo);
static void genWhere(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genDistinct(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int gotoIfNot);
static void genGetGroup(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genSelect(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genExprAdd(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genExprMult(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genExprNeg(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genExprCase(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genCPredList(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end);
static void genCWExprList(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end);
static void genCPred(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end);
static void genCWExpr(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end);
static void genPredicates(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genPredCompare(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genValue(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genFunction(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genSelectAll(vector<opcode> &v, querySpecs &q);
static void genSelections(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genTypeConv(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genIterateGroups(unique_ptr<node> &n, varScoper &vs, vector<opcode> &v, querySpecs &q);

//for debugging
static int ident = 0;
//#define e //turn off debug printer
#ifndef e
#define e(A) for (int i=0; i< ident; i++) cerr << "    "; \
	cerr << A << endl; ident++; \
	shared_ptr<void> _(nullptr, [](...){ \
	ident--; for (int i=0; i< ident; i++) cerr << "    "; \
	cerr << "done " << A << endl; });
#endif

#define pushvars() for (auto &i : q.vars) addop(v, PUSH);
#define popvars() for (auto &i : q.vars) addop(v, POP);
//#define pushvars() for (auto &v : q.vars) { if (v.filter & GROUP_FILTER) addop(v, PUSH) };
//#define popvars() for (auto &v : q.vars) { if (v.filter & GROUP_FILTER) addop(v, POP) };

static int normal_read;
static int agg_phase; //0 is not grouping, 1 is first read, 2 is aggregate retrieval
static int select_count;

//type conversion opcodes - [from][to]
static int typeConv[6][6] = {
	{0, 0,  0,  0,  0,  0 },
	{0, CVNO, CVIF, CVNO, CVNO, CVIS },
	{0, CVFI, CVNO, CVFI, CVFI, CVFS },
	{0, CVNO, CVIF, CVNO, CVER, CVDTS},
	{0, CVNO, CVIF, CVER, CVNO, CVDRS},
	{0, CVSI, CVSF, CVSDT,CVSDR,CVNO},
};

static int maxops[] = { 0, MAXI, MAXF, MAXI, MAXI, MAXS };
static int minops[] = { 0, MINI, MINF, MINI, MINI, MINS };
static int sumops[] = { 0, SUMI, SUMF, 0, SUMI, 0 };
static int avgops[] = { 0, AVGI, AVGF, AVGI, AVGI, 0 };
static int stvops[] = { 0, STDVI, STDVF, 0, STDVI, 0 };
//aggregates that need to be finished
static int ldstdvops[] = { 0, LDSTDVI, LDSTDVF, 0, LDSTDVI, 0 };
static int ldavgops[] = { 0, LDAVGI, LDAVGF, LDAVGI, LDAVGI, 0 };

static int sortOps[] = { 0, SORTI, SORTF, SORTI, SORTI, SORTS };
static int savePosOps[] = { 0, SAVEPOSI_JMP, SAVEPOSF_JMP, SAVEPOSI_JMP, SAVEPOSI_JMP, SAVEPOSS_JMP };
static int distinctOps[] = { 0, NDIST, NDIST, NDIST, NDIST, SDIST };

static bool isTrivial(unique_ptr<node> &n){
	if (n == nullptr) return false;
	if (n->label == N_VALUE && n->tok3.id)
		return true;
	return isTrivial(n->node1);
}

//parse literals and check for errors
static dat parseIntDat(const char* s){
	char* end = NULL;
	dat idat = { { i: strtol(s, &end, 10) }, T_INT };
	if (*end != 0)
		error(str3("Could not parse ", s, " as a number."));
	return idat;
}
static dat parseFloatDat(const char* s){
	char* end = NULL;
	dat fdat = { { f: strtof(s, &end) }, T_FLOAT };
	if (*end != 0)
		error(str3("Could not parse ", s, " as a number."));
	return fdat;
}
static dat parseDurationDat(const char* s) {
	date_t dur;
	if (parseDuration((char*)s, &dur))
		error(str3("Could not parse ", s, " as duration."));
	if (dur < 0) dur *= -1;
	dat ddat = { { i: dur }, T_DURATION };
	return ddat;
}
static dat parseDateDat(const char* s) {
	date_t date;
	if (dateparse_2(s, &date))
		error(str3("Could not parse ", s, " as date."));
	dat ddat = { { i: date }, T_DATE };
	return ddat;
}
static dat parseStringDat(const char* s) {
	//may want to malloc
	dat ddat = { { s: (char*)s }, T_STRING, (uint) strlen(s) };
	return ddat;
}
static int addBtree(int type, querySpecs &q){
	//returns index of btree
	switch (type){
	case T_INT:
	case T_DATE:
	case T_DURATION:
	case T_FLOAT:
		return q.btn++;
	case T_STRING:
		return q.bts++;
	default:
		error("invalid btree type");
	}
	return 0;
}


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
			if (v.p1 < 0)
				v.p1 = jumps[v.p1];
		}
};

#define has(A,B) ((A)==(B) || ((A) & (B)))
#define incSelectCount()  if has(n->phase, agg_phase) select_count++;
#define addop0(V,A)       if has(n->phase, agg_phase) addop(V, A)
#define addop1(V,A,B)     if has(n->phase, agg_phase) addop(V, A, B)
#define addop2(V,A,B,C)   if has(n->phase, agg_phase) addop(V, A, B, C)
#define addop3(V,A,B,C,D) if has(n->phase, agg_phase) addop(V, A, B, C, D)
#define debugAddop cerr << "addop: " << opMap[code] << endl;
//#define debugAddop
static void addop(vector<opcode> &v, byte code){
	debugAddop
	v.push_back({code, 0, 0, 0});
}
static void addop(vector<opcode> &v, byte code, int p1){
	debugAddop
	v.push_back({code, p1, 0, 0});
}
static void addop(vector<opcode> &v, byte code, int p1, int p2){
	debugAddop
	v.push_back({code, p1, p2, 0});
}
static void addop(vector<opcode> &v, byte code, int p1, int p2, int p3){
	debugAddop
	v.push_back({code, p1, p2, p3});
}

static void determinePath(querySpecs &q){

	int whatdo = 0;
	if (q.sorting)  whatdo = 1;
	if (q.grouping) whatdo|= 2;
	if (q.joining)  whatdo|= 4;

	switch (whatdo){
	case 0:
		cerr << "normal\n";
		genNormalQuery(q.tree, q.bytecode, q);
		break;
	case 1:
		cerr << "sorting\n";
		genNormalOrderedQuery(q.tree, q.bytecode, q);
		break;
	case 2:
		cerr << "basic grouping\n";
		genBasicGroupingQuery(q.tree, q.bytecode, q);
		break;
	case 4:
		break;
	case 1|2:
		break;
	case 1|4:
		break;
	case 1|2|4:
		break;
	case 2|4:
		break;
	}

	q.jumps.updateBytecode(q.bytecode);
}

void codeGen(querySpecs &q){
	select_count = 0;
	agg_phase = 0;
	determinePath(q);
	int i = 0;
	for (auto c : q.bytecode){
		cerr << "ip: " << left << setw(4) << i++;
		c.print();
	}
}

//generate bytecode for expression nodes
static void genExprAll(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	switch (n->label){
	case N_DEXPRESSIONS:
	case N_EXPRNEG:         genExprNeg     (n, v, q); break;
	case N_EXPRADD:         genExprAdd     (n, v, q); break;
	case N_EXPRMULT:        genExprMult    (n, v, q); break;
	case N_EXPRCASE:        genExprCase    (n, v, q); break;
	case N_PREDICATES:      genPredicates  (n, v, q); break;
	case N_PREDCOMP:        genPredCompare (n, v, q); break;
	case N_VALUE:           genValue       (n, v, q); break;
	case N_FUNCTION:        genFunction    (n, v, q); break;
	case N_TYPECONV:        genTypeConv    (n, v, q); break;
	}
}


//given q.tree as node param
static void genNormalQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	e("normal");
	int endfile = q.jumps.newPlaceholder(); //where to jump when done reading file
	varScoper vs;
	pushvars();
	normal_read = v.size();
	addop(v, RDLINE, endfile, 0);
	genVars(n->node1, v, q, vs.setscope(WHERE_FILTER|DISTINCT_FILTER, V_EQUALS, V_SCOPE1));
	genVars(n->node1, v, q, vs.setscope(WHERE_FILTER, V_EQUALS, V_SCOPE1));
	genWhere(n->node4, v, q);
	genVars(n->node1, v, q, vs.setscope(DISTINCT_FILTER, V_EQUALS, V_SCOPE1));
	genDistinct(n->node2->node1, v, q, normal_read);
	genVars(n->node1, v, q, vs.setscope(NO_FILTER, V_ANY, V_SCOPE1));
	genSelect(n->node2, v, q);
	addop(v, PRINT);
	addop(v, (q.quantityLimit > 0 ? JMPCNT : JMP), normal_read);
	q.jumps.setPlace(endfile, v.size());
	popvars();
	addop(v, ENDRUN);
}
static void genNormalOrderedQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	int sorter = q.jumps.newPlaceholder(); //where to jump when done scanning file
	int reread = q.jumps.newPlaceholder();
	int endreread = q.jumps.newPlaceholder();
	varScoper vs;
	pushvars();
	normal_read = v.size();
	addop(v, RDLINE, sorter, 0);
	genVars(n->node1, v, q, vs.setscope(WHERE_FILTER|ORDER_FILTER, V_INCLUDES, V_SCOPE1));
	genWhere(n->node4, v, q);
	genExprAll(n->node4->node4->node1, v, q);
	addop(v, savePosOps[q.sorting], normal_read, 0, 0);
	q.posVecs = 1;
	q.jumps.setPlace(sorter, v.size());
	int asc = n->node4->node4->tok1.lower() == "asc" ? 1 : 0;
	addop(v, sortOps[q.sorting], 0, asc);
	addop(v, PREP_REREAD, 0, 0);
	q.jumps.setPlace(reread, v.size());
	addop(v, RDLINE_ORDERED, endreread, 0, 0);
	genVars(n->node1, v, q, vs.setscope(DISTINCT_FILTER, V_EQUALS, V_SCOPE2));
	genDistinct(n->node2->node1, v, q, reread);
	genVars(n->node1, v, q, vs.setscope(NO_FILTER, V_ANY, V_SCOPE2));
	genSelect(n->node2, v, q);
	addop(v, PRINT);
	addop(v, (q.quantityLimit > 0 ? JMPCNT : JMP), reread);
	q.jumps.setPlace(endreread, v.size());
	addop(v, POP); //rereader used 2 stack spaces
	addop(v, POP);
	popvars();
	addop(v, ENDRUN);
};
static void genBasicGroupingQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	e("grouping");
	agg_phase = 1;
	int getgroups = q.jumps.newPlaceholder();
	varScoper vs;
	pushvars();
	normal_read = v.size();
	addop(v, RDLINE, getgroups, 0);
	genVars(n->node1, v, q, vs.setscope(WHERE_FILTER|DISTINCT_FILTER, V_EQUALS, V_SCOPE1));
	genVars(n->node1, v, q, vs.setscope(WHERE_FILTER, V_EQUALS, V_SCOPE1));
	genWhere(n->node4, v, q);
	genVars(n->node1, v, q, vs.setscope(DISTINCT_FILTER, V_EQUALS, V_SCOPE1));
	genDistinct(n->node2->node1, v, q, normal_read);
	genVars(n->node1, v, q, vs.setscope(GROUP_FILTER, V_INCLUDES, V_SCOPE1));
	genGetGroup(n->node4->node2, v, q);
	genVars(n->node1, v, q, vs.setscope(NO_FILTER, V_ANY, V_SCOPE1));
	genSelect(n->node2, v, q);
	addop(v, JMP, normal_read);
	q.jumps.setPlace(getgroups, v.size());
	agg_phase = 2;
	select_count = 0; //used as midrow count in phase 1
	genIterateGroups(n->node4->node2, vs, v, q);
	popvars();
	addop(v, ENDRUN);
}

static void genVars(unique_ptr<node> &n, vector<opcode> &vec, querySpecs &q, varScoper* vs){
	if (n == nullptr) return;
	e("gen vars");
	int i;
	int match = 0;
	switch (n->label){
	case N_PRESELECT: //currently only has 'with' branch
	case N_WITH:
		genVars(n->node1, vec, q, vs);
		break;
	case N_VARS:
		i = getVarIdx(n->tok1.val, q);
		if (vs->neededHere(i, q.vars[i].filter)){
			genExprAll(n->node1, vec, q);
			addop1(vec, PUTVAR, i);
		}
		genVars(n->node2, vec, q, vs);
		break;
	}
}

static void genExprAdd(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen add");
	genExprAll(n->node1, v, q);
	if (!n->tok1.id) return;
	genExprAll(n->node2, v, q);
	switch (n->tok1.id){
	case SP_PLUS:
		addop0(v, ops[OPADD][n->datatype]);
		break;
	case SP_MINUS:
		addop0(v, ops[OPSUB][n->datatype]);
		break;
	}
}

static void genExprMult(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen mult");
	genExprAll(n->node1, v, q);
	if (!n->tok1.id) return;
	genExprAll(n->node2, v, q);
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

static void genExprNeg(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen neg");
	genExprAll(n->node1, v, q);
	if (!n->tok1.id) return;
	addop0(v, ops[OPNEG][n->datatype]);
}

static void genValue(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen value");
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
			addop1(v, LDLIT, q.dataholder.size());
			q.dataholder.push_back(lit);
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
		genExprAll(n->node1, v, q);
		break;
	}
}

static void genExprCase(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen case");
	int caseEnd = q.jumps.newPlaceholder();
	switch (n->tok1.id){
	case KW_CASE:
		switch (n->tok2.id){
		//when predicates are true
		case KW_WHEN:
			genCPredList(n->node1, v, q, caseEnd);
			genExprAll(n->node3, v, q);
			if (n->node3 == nullptr)
				addop0(v, LDNULL);
			q.jumps.setPlace(caseEnd, v.size());
			break;
		//expression matches expression list
		case WORD_TK:
		case SP_LPAREN:
			genExprAll(n->node1, v, q);
			genCWExprList(n->node2, v, q, caseEnd);
			addop0(v, POP); //don't need comparison value anymore
			genExprAll(n->node3, v, q);
			if (n->node3 == nullptr)
				addop0(v, LDNULL);
			q.jumps.setPlace(caseEnd, v.size());
			break;
		}
		break;
	case SP_LPAREN:
	case WORD_TK:
		genExprAll(n->node1, v, q);
	}
}

static void genCWExprList(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end){
	if (n == nullptr) return;
	e("gen case w list");
	genCWExpr(n->node1, v, q, end);
	genCWExprList(n->node2, v, q, end);
}

static void genCWExpr(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end){
	if (n == nullptr) return;
	e("gen case w expr");
	int next = q.jumps.newPlaceholder(); //get jump pos for next try
	genExprAll(n->node1, v, q); //evaluate comparision expression
	addop1(v, ops[OPEQ][n->tok1.id], 0); //leave '=' result where this comp value was
	addop2(v, JMPFALSE, next, 1);
	addop0(v, POP); //don't need comparison value anymore
	genExprAll(n->node2, v, q); //result value if eq
	addop1(v, JMP, end);
	q.jumps.setPlace(next, v.size()); //jump here for next try
}

static void genCPredList(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end){
	if (n == nullptr) return;
	e("gen case p list");
	genCPred(n->node1, v, q, end);
	genCPredList(n->node2, v, q, end);
}

static void genCPred(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end){
	if (n == nullptr) return;
	e("gen case p");
	int next = q.jumps.newPlaceholder(); //get jump pos for next try
	genPredicates(n->node1, v, q);
	addop2(v, JMPFALSE, next, 1);
	genExprAll(n->node2, v, q); //result value if true
	addop1(v, JMP, end);
	q.jumps.setPlace(next, v.size()); //jump here for next try
}

static void genSelect(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	genSelections(n->node1, v, q);
}

static void genSelections(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) {
		//reached end of selections section of query
		if (!select_count) genSelectAll(v, q);
		return;
	}
	e("gen selections");
	string t1 = n->tok1.lower();
	switch (n->label){
	case N_SELECTIONS:
		if (t1 == "hidden") {

		} else if (t1 == "distinct") {
			addop1(v, PUTDIST, select_count);
			incSelectCount();

		} else if (t1 == "*") {
			genSelectAll(v, q);

		} else if (isTrivial(n)) {
			switch (agg_phase){
			case 0:
				for (auto nn = n.get(); nn; nn = nn->node1.get()) if (nn->label == N_VALUE){
					addop3(v, LDPUT, select_count, nn->tok1.id, getFileNo(nn->tok3.val, q));
					break;
				} break;
			case 1:
				for (auto nn = n.get(); nn; nn = nn->node1.get()) if (nn->label == N_VALUE){
					addop3(v, LDPUTGRP, select_count, nn->tok1.id, getFileNo(nn->tok3.val, q));
					break;
				} break;
			case 2:
				addop2(v, LDPUTMID, select_count, n->tok3.id-1);
				break;
			}
			incSelectCount();

		} else {
			genExprAll(n->node1, v, q);
			addop2(v, PUT, select_count, agg_phase==1?1:0);
			incSelectCount();
		}
		break;
	default:
		error("selections generator error");
		return;
	}
	genSelections(n->node2, v, q);
}

static void genPredicates(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen preds");
	genPredCompare(n->node1, v, q);
	int doneAndOr = q.jumps.newPlaceholder();
	switch (n->tok1.id){
	case KW_AND:
		addop2(v, JMPFALSE, doneAndOr, 0);
		addop0(v, POP); //don't need old result
		genPredicates(n->node2, v, q);
		break;
	case KW_OR:
		addop2(v, JMPTRUE, doneAndOr, 0);
		addop0(v, POP); //don't need old result
		genPredicates(n->node2, v, q);
		break;
	case KW_XOR:
		break;
	}
	q.jumps.setPlace(doneAndOr, v.size());
	if (n->tok2.id == SP_NEGATE)
		addop0(v, PNEG); //can optimize be returning value to genwhere and add opposite jump code
}

static void genPredCompare(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen pred compare");
	dat reg;
	int negation = n->tok2.id;
	int endcomp, greaterThanExpr3;
	genExprAll(n->node1, v, q);
	switch (n->tok1.id){
	case SP_NOEQ: negation ^= 1;
	case SP_EQ:
		genExprAll(n->node2, v, q);
		addop2(v, ops[OPEQ][n->datatype], 1, negation);
		break;
	case SP_GREATEQ: negation ^= 1;
	case SP_LESS:
		genExprAll(n->node2, v, q);
		addop2(v, ops[OPLT][n->datatype], 1, negation);
		break;
	case SP_GREAT: negation ^= 1;
	case SP_LESSEQ:
		genExprAll(n->node2, v, q);
		addop2(v, ops[OPLEQ][n->datatype], 1, negation);
		break;
	case KW_BETWEEN:
		endcomp = q.jumps.newPlaceholder();
		greaterThanExpr3 = q.jumps.newPlaceholder();
		addop1(v, NULFALSE1, endcomp);
		genExprAll(n->node2, v, q);
		addop1(v, NULFALSE2, endcomp);
		addop2(v, ops[OPLT][n->datatype], 0, 1);
		addop2(v, JMPFALSE, greaterThanExpr3, 1);
		genExprAll(n->node3, v, q);
		addop1(v, NULFALSE2, endcomp);
		addop2(v, ops[OPLT][n->datatype], 1, negation);
		addop1(v, JMP, endcomp);
		q.jumps.setPlace(greaterThanExpr3, v.size());
		genExprAll(n->node3, v, q);
		addop1(v, NULFALSE2, endcomp);
		addop2(v, ops[OPLT][n->datatype], 1, negation^1);
		q.jumps.setPlace(endcomp, v.size());
		break;
	case KW_IN:
		endcomp = q.jumps.newPlaceholder();
		for (auto nn=n->node2.get(); nn; nn=nn->node2.get()){
			genExprAll(nn->node1, v, q);
			addop1(v, ops[OPEQ][n->node1->datatype], 0);
			addop2(v, JMPTRUE, endcomp, 0);
			if (nn->node2.get())
				addop0(v, POP);
		}
		q.jumps.setPlace(endcomp, v.size());
		if (negation)
			addop0(v, PNEG);
		addop0(v, POPCPY); //put result where 1st expr was
		break;
	case KW_LIKE:
		addop2(v, LIKE, q.dataholder.size(), negation);
		reg.u.r = new regex_t;
		reg.b = RMAL;
		boost::replace_all(n->tok3.val, "_", ".");
		boost::replace_all(n->tok3.val, "%", ".*");
		if (regcomp(reg.u.r, ("^"+n->tok3.val+"$").c_str(), REG_EXTENDED|REG_ICASE))
			error("Could not parse 'like' pattern");
		q.dataholder.push_back(reg);
		break;
	}
}

static void genSelectAll(vector<opcode> &v, querySpecs &q){
	addop(v, LDPUTALL, select_count);
	for (int i=1; i<=q.numFiles; ++i)
		select_count += q.files[str2("_f", i)]->numFields;
}

static void genWhere(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen where");
	switch (n->label){
	case N_QUERY:     genWhere(n->node4, v, q); break;
	case N_AFTERFROM: genWhere(n->node1, v, q); break;
	case N_WHERE:
		genPredicates(n->node1, v, q);
		addop2(v, JMPFALSE, normal_read, 1);
	}
}

static void genDistinct(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int gotoIfNot){
	if (n == nullptr) return;
	e("gen distinct");
	if (n->label != N_SELECTIONS) return;
	if (n->tok1.id == KW_DISTINCT){
		genExprAll(n->node1, v, q);
		addop3(v, distinctOps[n->datatype], gotoIfNot, addBtree(n->datatype, q),
			n->tok1.lower() == "hidden" ? 0 : 1);
	} else {
		//there can be only 1 distinct filter
		genDistinct(n->node2, v, q, gotoIfNot);
	}
}

static void genFunction(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen function");
	int funcDone = q.jumps.newPlaceholder();
	int idx;

	//stuff common to all aggregate functions
	if ((n->tok1.id & AGG_BIT) != 0 ) {
		genExprAll(n->node1, v, q);
		if (n->tok3.val == "distinct"){
			addop3(v, distinctOps[n->datatype], funcDone, addBtree(n->datatype, q), 1);
			addop0(v, LDDIST);
		}
	}

	switch (n->tok1.id){
	//non-aggregates
	case FN_COALESCE:
		for (auto nn = n->node1.get(); nn; nn = nn->node2.get()){
			genExprAll(nn->node1, v, q);
			addop1(v, JMPNOTNULL_ELSEPOP, funcDone);
		}
		addop0(v, LDNULL);
		break;
	case FN_INC:
		addop1(v, FINC, q.dataholder.size());
		q.dataholder.push_back(dat{ {.f = 0.0}, T_FLOAT});
		break;
	case FN_ENCRYPT:
		genExprAll(n->node1, v, q);
		if (n->tok3.val == "chacha"){
			idx = q.crypt.newChacha(n->tok4.val);
			addop1(v, ENCCHA, idx);
		} else /* aes */ {
		}
		break;
	case FN_DECRYPT:
		genExprAll(n->node1, v, q);
		if (n->tok3.val == "chacha"){
			idx = q.crypt.newChacha(n->tok4.val);
			addop1(v, DECCHA, idx);
		} else /* aes */ {
		}
		break;
	}

	//aggregates
	if (agg_phase == 1) {
		switch (n->tok1.id){
		case FN_SUM:
			addop(v, sumops[n->datatype], select_count);
			break;
		case FN_AVG:
			addop(v, avgops[n->datatype], select_count);
			break;
		case FN_STDEV:
		case FN_STDEVP:
			addop(v, stvops[n->datatype], select_count);
			break;
		case FN_MIN:
			addop(v, minops[n->datatype], select_count);
			break;
		case FN_MAX:
			addop(v, maxops[n->datatype], select_count);
			break;
		case FN_COUNT:
			addop(v, COUNT, select_count, n->tok2.id ? 1 : 0);
			break;
		}
		select_count++;
	} else if (agg_phase == 2) {
		switch (n->tok1.id){
		case FN_AVG:
			addop(v, ldavgops[n->datatype], n->tok6.id-1);
			break;
		case FN_STDEV:
			addop(v, ldstdvops[n->datatype], n->tok6.id-1, 1);
			break;
		case FN_STDEVP:
			addop(v, ldstdvops[n->datatype], n->tok6.id-1);
			break;
		case FN_MIN:
		case FN_MAX:
		case FN_SUM:
		case FN_COUNT:
			addop(v, LDMID, n->tok6.id-1);
			break;
		}
	}
	q.jumps.setPlace(funcDone, v.size());
}

static void genTypeConv(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("type convert");
	genExprAll(n->node1, v, q);
	addop0(v, typeConv[n->tok1.id][n->datatype]);
}

static void genGetGroup(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("get group");
	if (n->label == N_GROUPBY){
		int depth = -1;
		for (auto nn=n->node1.get(); nn; nn=nn->node2.get()){
			depth++;
			genExprAll(nn->node1, v, q);
		}
		addop1(v, GETGROUP, depth);
	}
}

static void genIterateGroups(unique_ptr<node> &n, varScoper &vs, vector<opcode> &v, querySpecs &q){
	e("iterate groups");
	if (n == nullptr) {
		addop(v, ONEGROUP);
		genVars(q.tree->node1, v, q, vs.setscope(NO_FILTER, V_ANY, V_SCOPE2));
		genSelect(q.tree->node2, v, q);
		// for debugging:
		addop(v, PRINT);
		return;
	}
	if (n->label == N_GROUPBY){
		int doneGroups = q.jumps.newPlaceholder();
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
				genVars(q.tree->node1, v, q, vs.setscope(NO_FILTER, V_ANY, V_SCOPE2));
				genSelect(q.tree->node2, v, q);
				// for debugging:
				addop(v, PRINT);
				addop(v, JMP, nextvec);
			}
		}
		q.jumps.setPlace(doneGroups, v.size());
	}
}
