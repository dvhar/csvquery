#include "interpretor.h"

static void varUsedInWhere(string var, unique_ptr<node> &n, querySpecs &q);

//typing done, still need semantics etc
void analyzeTree(querySpecs &q){
	for (auto &v : q.vars)
		varUsedInWhere(v.name, q.tree, q);
}

static void varUsedInWhere(string var, unique_ptr<node> &n, querySpecs &q){
	if (n == nullptr) return;
	switch (n->label){
	//skip irrelvent subtrees
	case N_PRESELECT:
	case N_SELECT:
	case N_FROM:
	case N_GROUPBY:
	case N_ORDER:
	case N_HAVING: //may use this one later
		break;
	case N_WHERE:
		varUsedInWhere(var, n->node1, q);
		break;
	case N_VALUE:
		if (n->tok2.id == VARIABLE && n->tok1.val == var){
			cerr << "checking " << n->tok1.val << endl;
			for (auto &v : q.vars)
				if (var == v.name){
					v.filter = 1;
					cerr << var << " used in filter\n";
				}
		}
		break;
	default:
		varUsedInWhere(var, n->node1, q);
		varUsedInWhere(var, n->node2, q);
		varUsedInWhere(var, n->node3, q);
		varUsedInWhere(var, n->node4, q);
	}
}
