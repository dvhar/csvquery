#include "interpretor.h"
#include "vmachine.h"

static void genNormalQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genVars(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int filter);
static void genWhere(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genDistinct(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genGroupOrNewRow(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genSelect(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genPrint(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genRepeatPhase1(vector<opcode> &v, querySpecs &q, int);
static void genReturnGroups(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genExprAdd(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genExprMult(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genExprNeg(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genExprList(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genExprCase(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genCPredList(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genCWExprList(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end);
static void genCPred(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genCWExpr(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end);
static void genPredicates(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genPredCompare(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genValue(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genFunction(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genSelectAll(vector<opcode> &v, querySpecs &q);
static void genSelections(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);

void jumpPositions::updateBytecode(vector<opcode> &vec) {
	for (auto &v : vec)
		if ((v.code == JMP || v.code == JMPFALSE || v.code == JMPTRUE) && v.p1 < 0)
			v.p1 = places[v.p1];
};

static void addop(vector<opcode> &v, byte code){
	cerr << "adding op " << opMap[code] << endl;
	v.push_back({code, 0});
}
static void addop(vector<opcode> &v, byte code, int p1){
	cerr << "adding op " << opMap[code] << "  p1 " << p1 << endl;
	v.push_back({code, p1});
}

static void determinePath(querySpecs &q){

	if (q.sorting && !q.grouping && q.joining) {
		cerr << "ordered join\n";
        //order join
    } else if (q.sorting && !q.grouping) {
		cerr << "ordered plain\n";
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
		cerr << "normal join\n";
        //normal join and grouping
    } else {
		cerr << "normal plain\n";
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
		//     select (phase 2)
    }

}

void codeGen(querySpecs &q){
	determinePath(q);
	for (auto c : q.bytecode)
		c.print();
}

//generate bytecode for any node in an expression
static void genExprAll(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	cerr << "calling gen func for node " << treeMap[n->label] << endl;
	switch (n->label){
	case N_DEXPRESSIONS:
	case N_EXPRESSIONS:     genExprList    (n, v, q); break;
	case N_EXPRNEG:         genExprNeg     (n, v, q); break;
	case N_EXPRADD:         genExprAdd     (n, v, q); break;
	case N_EXPRMULT:        genExprMult    (n, v, q); break;
	case N_EXPRCASE:        genExprCase    (n, v, q); break;
	case N_CPREDLIST:       genCPredList   (n, v, q); break;
	case N_CPRED:           genCPred       (n, v, q); break;
	case N_PREDICATES:      genPredicates  (n, v, q); break;
	case N_PREDCOMP:        genPredCompare (n, v, q); break;
	case N_VALUE:           genValue       (n, v, q); break;
	case N_FUNCTION:        genFunction    (n, v, q); break;
	}
	cerr << "call completed for node " << treeMap[n->label] << endl;
}

//given q.tree as node param
static void genNormalQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	int ip1 = v.size();
	addop(v, RDLINE);
	genVars(n->node1, v, q, 1);
	genWhere(n->node4, v, q);
	genDistinct(n->node4, v, q);
	genVars(n->node1, v, q, 0);
	genGroupOrNewRow(n->node4, v, q);
	genSelect(n->node4, v, q);
	if (!q.grouping)
		genPrint(n->node4, v, q);
	genRepeatPhase1(v, q, ip1);
	genReturnGroups(n->node4, v, q); //more selecting/printing if grouping
}

static void genVars(unique_ptr<node> &n, vector<opcode> &vec, querySpecs &q, int filter){
	if (n == nullptr) return;
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
	genExprAll(n->node1, v, q);
	if (!n->tok1.id) return;
	addop(v, ops[OPNEG][n->datatype]);
}

static void genValue(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	switch (n->tok2.id){
	case COLUMN:
		addop(v, ops[OPLD][n->datatype], n->tok1.id);
		break;
	case LITERAL:
		//parse values here
		break;
	case VARIABLE:
		addop(v, LDVAR, getVarIdx(n->tok1.val, q));
		break;
	case FUNCTION:
		genExprAll(n->node1, v, q);
		break;
	}
}

static void genExprCase(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
	int caseEnd;
	switch (n->tok1.id){
	case KW_CASE:
		switch (n->tok2.id){
		//when predicates are true
		case KW_WHEN:
			break;
		//expression matches expression list
		case WORD:
		case SP_LPAREN:
			caseEnd = q.places.newPlaceholder();
			genExprAll(n->node1, v, q);
			genCWExprList(n->node2, v, q, caseEnd);
			genExprAll(n->node3, v, q);
			if (n->node3 == nullptr)
				addop(v, LDNULL);
			q.places.setPlace(caseEnd, v.size());
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
	genCWExpr(n->node1, v, q, end);
	genCWExprList(n->node2, v, q, end);
}

static void genCWExpr(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q, int end){
	if (n == nullptr) return;
	int next = q.places.newPlaceholder(); //get jump pos for next try
	genExprAll(n->node1, v, q); //evaluate comparision expression
	addop(v, ops[OPEQ][n->tok1.id], 0); //leave '=' result where this comp value was
	addop(v, JMPFALSE, next);
	addop(v, POP, 1); //don't need comparison value anymore
	genExprAll(n->node2, v, q); //result value if eq
	addop(v, JMP, end);
	q.places.setPlace(next, v.size()); //jump here for next try
}

static void genSelect(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n->node1->label != N_SELECTIONS)
		genSelectAll(v, q);
	else
		genSelections(n->node1, v, q);
}

static void genSelections(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genSelectAll(vector<opcode> &v, querySpecs &q){
}

static void genGroupOrNewRow(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genWhere(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genExprList(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genCPredList(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genCPred(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genPredicates(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genPredCompare(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genFunction(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genDistinct(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genPrint(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}

static void genRepeatPhase1(vector<opcode> &v, querySpecs &q, int restart){
}

static void genReturnGroups(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	if (n == nullptr) return;
}
