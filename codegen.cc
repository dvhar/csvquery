#include "interpretor.h"
#include "vmachine.h"

static void genNormalQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genVars(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genWhere(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genDistinct(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genGroupOrNewRow(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genSelect(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genPrint(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);
static void genRepeatPhase1(vector<opcode> &v, querySpecs &q, int);
static void genReturnGroups(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q);

static void addop(vector<opcode> &v, byte code){
	v.push_back({code, 0});
}
static void addop(vector<opcode> &v, byte code, int p1){
	v.push_back({code, p1});
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
        //normal plain and grouping

		// 1 read line
		// 2 check where and distinct
		// 3 set torow (new or group)
		// 4 select (phase 1)
		// 5 print/append if not grouping
		// 6 if not done, goto 1
		// 7 return grouped rows
		//     select (phase 2)
    }

}

//given q.tree as node param
static void genNormalQuery(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
	int ip1 = v.size();
	addop(v, RDLINE);
	if (q.varsNeededForFilter)
		genVars(n->node1, v, q);
	genWhere(n->node4, v, q);
	genDistinct(n->node4, v, q);
	if (!q.varsNeededForFilter)
		genVars(n->node1, v, q);
	genGroupOrNewRow(n->node4, v, q);
	genSelect(n->node4, v, q);
	if (!q.grouping)
		genPrint(n->node4, v, q);
	genRepeatPhase1(v, q, ip1);
	genReturnGroups(n->node4, v, q); //more selecting/printing
}

static void genVars(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
}

static void genWhere(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
}

static void genDistinct(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
}

static void genGroupOrNewRow(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
}

static void genSelect(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
}

static void genPrint(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
}

static void genRepeatPhase1(vector<opcode> &v, querySpecs &q, int restart){
}

static void genReturnGroups(unique_ptr<node> &n, vector<opcode> &v, querySpecs &q){
}
