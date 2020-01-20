#include "interpretor.h"
#include "vmachine.h"

static void genNormalQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genVars(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int filter);
static void genWhere(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genDistinct(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genGroupOrNewRow(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genSelect(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genPrint(vector<opcode> &v, querySpecs &q);
static void genReturnGroups(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
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
static void genSelectAll(vector<opcode> &v, querySpecs &q, int &count);
static void genSelections(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);

//for debugging
static int ident = 0;
#define e //turn off debug printer
#ifndef e
#define e(A) for (int i=0; i< ident; i++) cerr << "    "; \
	cerr << A << endl; ident++; \
	shared_ptr<void> _(nullptr, [](...){ \
	ident--; for (int i=0; i< ident; i++) cerr << "    "; \
	cerr << "done " << A << endl; });
#endif

//global bytecode position
static int NORMAL_READ;

//type conversion opcodes - [from][to]
static int typeConv[6][6] = {
	{0, 0,  0,  0,  0,  0 },
	{0, CVNO, CVIF, CVNO, CVNO, CVIS },
	{0, CVFI, CVNO, CVFI, CVFI, CVFS },
	{0, CVNO, CVIF, CVNO, CVER, CVDTS},
	{0, CVNO, CVIF, CVER, CVNO, CVDRS},
	{0, CVSI, CVSF, CVSDT,CVSDR,CVNO},
};

static bool isTrivial(unique_ptr<node> &n){
	if (n == nullptr) return false;
	if (n->label == N_VALUE && n->tok3.id)
		return true;
	return isTrivial(n->node1);
}

//parse literals and check for errors
static dat parseIntDat(const char* s){
	char* end = NULL;
	dat idat = { { .i = strtol(s, &end, 10) }, I };
	if (*end != 0)
		error(str3("Could not parse ", s, " as a number."));
	return idat;
}
static dat parseFloatDat(const char* s){
	char* end = NULL;
	dat fdat = { { .f = strtof(s, &end) }, F };
	if (*end != 0)
		error(str3("Could not parse ", s, " as a number."));
	return fdat;
}
static dat parseDurationDat(const char* s) {
	date_t dur;
	if (parseDuration((char*)s, &dur))
		error(str3("Could not parse ", s, " as duration."));
	if (dur < 0) dur *= -1;
	dat ddat = { { .i = dur }, DR };
	return ddat;
}
static dat parseDateDat(const char* s) { //need to finish dateparse library
	date_t date;
	if (dateparse64_2(s, &date))
		error(str3("Could not parse ", s, " as date."));
	dat ddat = { { .i = date }, DT };
	return ddat;
}
static dat parseStringDat(const char* s) {
	//may want to malloc
	dat ddat = { { .s = (char*)s }, T, (short)strlen(s) };
	return ddat;
}


void jumpPositions::updateBytecode(vector<opcode> &vec) {
	for (auto &v : vec)
		switch (v.code){
		case JMP:
		case JMPTRUE:
		case JMPFALSE:
		case RDLINE:
		case NULFALSE1:
		case NULFALSE2:
			if (v.p1 < 0)
				v.p1 = jumps[v.p1];
		}
};

static void addop(vector<opcode> &v, byte code){
	//cerr << "adding op " << opMap[code] << endl;
	v.push_back({code, 0, 0, 0});
}
static void addop(vector<opcode> &v, byte code, int p1){
	//cerr << "adding op " << opMap[code] << "  p1 " << p1 << endl;
	v.push_back({code, p1, 0, 0});
}
static void addop(vector<opcode> &v, byte code, int p1, int p2){
	//cerr << "adding op " << opMap[code] << "  p1 " << p1 << "  p2 " << p2 << endl;
	v.push_back({code, p1, p2, 0});
}
static void addop(vector<opcode> &v, byte code, int p1, int p2, int p3){
	//cerr << "adding op " << opMap[code]
		//<< "  p1 " << p1 << "  p2 " << p2 << "  p3 " << p3 << endl;
	v.push_back({code, p1, p2, p3});
}

static void determinePath(querySpecs &q){

	if (q.sorting && !q.grouping && q.joining) {
		//order join
	} else if (q.sorting && !q.grouping) {
		//ordered plain

		// 1 read line
		// 2 check where
		// 3 retrieve and append sort expr
		// 4 if not done, goto 1
		// 5 sort
		// 6 read line at index
		// 7 check distinct
		// 8 set torow (new or group)
		// 9 select
		//10 print/append if not grouping
		//11 if not done, goto 6


	} else if (q.joining) {
		//normal join and grouping
	} else {
		genNormalQuery(q.tree, q.bytecode, q);
		//normal plain and grouping

		// 1 read line
		// 2 eval vars used for filter
		// 3 check where and distinct
		// 4 eval vars not used for filter
		// 5 set torow (new or group)
		// 6 select (phase 1)
		// 7 print/append if not grouping
		// 8 if not done, goto 1
		// 9 return grouped rows
		//	 select (phase 2)
	}
	q.jumps.updateBytecode(q.bytecode);

}

void codeGen(querySpecs &q){
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
	}
}

//given q.tree as node param
static void genNormalQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	e("normal");
	NORMAL_READ = v.size();
	int endfile = q.jumps.newPlaceholder(); //where to jump when done reading file
	addop(v, RDLINE, endfile, 0);
	genVars(n->node1, v, q, 1);
	genWhere(n->node4, v, q);
	genDistinct(n->node2, v, q);
	genVars(n->node1, v, q, 0);
	genGroupOrNewRow(n->node4, v, q);
	genSelect(n->node2, v, q);
	if (!q.grouping)
		genPrint(v, q);
	addop(v, (q.quantityLimit > 0 && !q.grouping ? JMPCNT : JMP), NORMAL_READ);
	q.jumps.setPlace(endfile, v.size());
	genReturnGroups(n->node4, v, q); //more selecting/printing if grouping
	addop(v, ENDRUN);
}

static void genVars(unique_ptr<node> &n, vector<opcode> &vec, querySpecs &q, int filter){
	if (n == nullptr) return;
	e("gen vars");
	int i;
	switch (n->label){
	case N_PRESELECT: //currently only has 'with' branch
	case N_WITH:
		genVars(n->node1, vec, q, filter);
		break;
	case N_VARS:
		i = getVarIdx(n->tok1.val, q);
		if (q.vars[i].filter == filter){
			genExprAll(n->node1, vec, q);
			addop(vec, PUTVAR, i);
		}
		genVars(n->node2, vec, q, filter);
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
		addop(v, ops[OPADD][n->datatype]);
		break;
	case SP_MINUS:
		addop(v, ops[OPSUB][n->datatype]);
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
		addop(v, ops[OPMULT][n->datatype]);
		break;
	case SP_DIV:
		addop(v, ops[OPDIV][n->datatype]);
		break;
	case SP_CARROT:
		addop(v, ops[OPEXP][n->datatype]);
		break;
	case SP_MOD:
		addop(v, IMOD); //only integer
		break;
	}
}

static void genExprNeg(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen neg");
	genExprAll(n->node1, v, q);
	if (!n->tok1.id) return;
	addop(v, ops[OPNEG][n->datatype]);
}

static void genValue(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	dat lit;
	int vtype, op;
	if (n == nullptr) return;
	e("gen value");
	switch (n->tok2.id){
	case COLUMN:
		addop(v, ops[OPLD][n->datatype], getFileNo(n->tok3.val, q), n->tok1.id);
		break;
	case LITERAL:
		switch (n->datatype){
		case T_INT:      lit = parseIntDat(n->tok1.val.c_str());      break;
		case T_FLOAT:    lit = parseFloatDat(n->tok1.val.c_str());    break;
		case T_DATE:     lit = parseDateDat(n->tok1.val.c_str());     break;
		case T_DURATION: lit = parseDurationDat(n->tok1.val.c_str()); break;
		case T_STRING:   lit = parseStringDat(n->tok1.val.c_str());   break;
		}
		addop(v, LDLIT, q.literals.size());
		q.literals.push_back(lit);
		break;
	case VARIABLE:
		addop(v, LDVAR, getVarIdx(n->tok1.val, q));
		//variable may be used in operations with different types
		vtype = getVarType(n->tok1.val, q);
		op = typeConv[vtype][n->datatype];
		if (op == CVER)
			error(ft("Error converting variable of type {} to new type {}", vtype, n->datatype));
		if (op != CVNO)
			addop(v, op, vtype, n->datatype);
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
				addop(v, LDNULL);
			q.jumps.setPlace(caseEnd, v.size());
			break;
		//expression matches expression list
		case WORD:
		case SP_LPAREN:
			genExprAll(n->node1, v, q);
			genCWExprList(n->node2, v, q, caseEnd);
			addop(v, POP); //don't need comparison value anymore
			genExprAll(n->node3, v, q);
			if (n->node3 == nullptr)
				addop(v, LDNULL);
			q.jumps.setPlace(caseEnd, v.size());
			break;
		}
		break;
	case SP_LPAREN:
	case WORD:
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
	addop(v, ops[OPEQ][n->tok1.id], 0); //leave '=' result where this comp value was
	addop(v, JMPFALSE, next, 1);
	addop(v, POP); //don't need comparison value anymore
	genExprAll(n->node2, v, q); //result value if eq
	addop(v, JMP, end);
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
	addop(v, JMPFALSE, next, 1);
	genExprAll(n->node2, v, q); //result value if true
	addop(v, JMP, end);
	q.jumps.setPlace(next, v.size()); //jump here for next try
}

static void genSelect(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	genSelections(n->node1, v, q);
}

static void genSelections(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	static int count = 0;
	if (n == nullptr) {
		//reached end of selections section of query
		if (!count) genSelectAll(v, q, count);
		return;
	}
	e("gen selections");
	string t1 = n->tok1.lower();
	switch (n->label){
	case N_SELECTIONS:
		if (t1 == "hidden") {
		} else if (t1 == "distinct") {
			count++; //retrieve value from earlier
		} else if (t1 == "*") {
			genSelectAll(v, q, count);
		} else if (isTrivial(n)) {
			for (auto nn = n.get(); nn; nn = nn->node1.get()) if (nn->label == N_VALUE){
				addop(v, LDPUT, count, nn->tok1.id, getFileNo(nn->tok3.val, q));
				break;
			}
			count++;
		} else {
			genExprAll(n->node1, v, q);
			addop(v, PUT, count);
			count++;
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
	int done = q.jumps.newPlaceholder();
	switch (n->tok1.id){
	case KW_AND:
		addop(v, JMPFALSE, done, 0);
		addop(v, POP); //don't need old result
		genPredicates(n->node2, v, q);
		break;
	case KW_OR:
		addop(v, JMPTRUE, done, 0);
		addop(v, POP); //don't need old result
		genPredicates(n->node2, v, q);
		break;
	case KW_XOR:
		break;
	}
	q.jumps.setPlace(done, v.size());
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
		addop(v, ops[OPEQ][n->datatype], 1, negation);
		break;
	case SP_GREATEQ: negation ^= 1;
	case SP_LESS:
		genExprAll(n->node2, v, q);
		addop(v, ops[OPLT][n->datatype], 1, negation);
		break;
	case SP_GREAT: negation ^= 1;
	case SP_LESSEQ:
		genExprAll(n->node2, v, q);
		addop(v, ops[OPLEQ][n->datatype], 1, negation);
		break;
	case KW_BETWEEN:
		endcomp = q.jumps.newPlaceholder();
		greaterThanExpr3 = q.jumps.newPlaceholder();
		addop(v, NULFALSE1, endcomp);
		genExprAll(n->node2, v, q);
		addop(v, NULFALSE2, endcomp);
		addop(v, ops[OPLT][n->datatype], 0, 1);
		addop(v, JMPFALSE, greaterThanExpr3, 1);
		genExprAll(n->node3, v, q);
		addop(v, NULFALSE2, endcomp);
		addop(v, ops[OPLT][n->datatype], 1, negation);
		addop(v, JMP, endcomp);
		q.jumps.setPlace(greaterThanExpr3, v.size());
		genExprAll(n->node3, v, q);
		addop(v, NULFALSE2, endcomp);
		addop(v, ops[OPLT][n->datatype], 1, negation^1);
		q.jumps.setPlace(endcomp, v.size());
		break;
	case KW_IN:
		endcomp = q.jumps.newPlaceholder();
		for (auto nn=n->node2.get(); nn; nn=nn->node2.get()){
			genExprAll(nn->node1, v, q);
			addop(v, ops[OPEQ][n->node1->datatype], 0);
			addop(v, JMPTRUE, endcomp, 0);
			if (nn->node2.get())
				addop(v, POP);
		}
		q.jumps.setPlace(endcomp, v.size());
		if (negation)
			addop(v, PNEG);
		addop(v, POPCPY); //put result where 1st expr was
		break;
	case KW_LIKE:
		addop(v, LIKE, q.literals.size(), negation);
		reg.u.r = new regex_t;
		reg.b = R|RMAL;
		boost::replace_all(n->tok3.val, "_", ".");
		boost::replace_all(n->tok3.val, "%", ".*");
		if (regcomp(reg.u.r, ("^"+n->tok3.val+"$").c_str(), REG_EXTENDED|REG_ICASE))
			error("Could not parse 'like' pattern");
		q.literals.push_back(reg);
		break;
	}
}

static void genSelectAll(vector<opcode> &v, querySpecs &q, int &count){
	addop(v, LDPUTALL, count);
	for (int i=1; i<=q.numFiles; ++i)
		count += q.files[str2("_f", i)]->numFields;
}

static void genWhere(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	e("gen where");
	switch (n->label){
	case N_QUERY:     genWhere(n->node4, v, q); break;
	case N_AFTERFROM: genWhere(n->node1, v, q); break;
	case N_WHERE:
		genPredicates(n->node1, v, q);
		addop(v, JMPFALSE, NORMAL_READ, 1);
	}
}

static void genFunction(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genDistinct(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genPrint(vector<opcode> &v, querySpecs &q){
	addop(v, PRINT);
}

static void genReturnGroups(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genGroupOrNewRow(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}
