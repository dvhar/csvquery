#include "interpretor.h"

static void varUsedInWhere(unique_ptr<node> &n, querySpecs &q);

//typing done, still need semantics etc
void analyzeTree(querySpecs &q){
	varUsedInWhere(q.tree, q);
}

static void varUsedInWhere(unique_ptr<node> &n, querySpecs &q){
	if (n == nullptr) return;
	static int filterBranch = 0;
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
		filterBranch = 1;
		varUsedInWhere(n->node1, q);
		filterBranch = 0;
		break;
	case N_VALUE:
		if (filterBranch && n->tok2.id == VARIABLE){
			cerr << "checking " << n->tok1.val << endl;
			for (auto &v : q.vars)
				if (n->tok1.val == v.name){
					v.filter = filterBranch;
					cerr << n->tok1.val << " used in filter\n";
				}
		}
		break;
	default:
		varUsedInWhere(n->node1, q);
		varUsedInWhere(n->node2, q);
		varUsedInWhere(n->node3, q);
		varUsedInWhere(n->node4, q);
	}
}
