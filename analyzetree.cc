#include "interpretor.h"

static void varUsedInFilter(unique_ptr<node> &n, querySpecs &q){
	if (n == nullptr) return;
	static int filterBranch = NO_FILTER;
	string t1;
	switch (n->label){
	//skip irrelvent subtrees
	case N_PRESELECT:
	case N_FROM:
	case N_HAVING: //may use this one later
		break;
	case N_SELECTIONS:
		t1 = n->tok1.lower();
		if (t1 == "hidden" || t1 == "distinct"){
			filterBranch = DISTINCT_FILTER;
			varUsedInFilter(n->node1, q);
			filterBranch = NO_FILTER;
		}
		varUsedInFilter(n->node2, q);
		break;
	case N_WHERE:
		filterBranch = WHERE_FILTER;
		varUsedInFilter(n->node1, q);
		filterBranch = NO_FILTER;
		break;
	case N_VALUE:
		if (filterBranch && n->tok2.id == VARIABLE){
			for (auto &v : q.vars)
				if (n->tok1.val == v.name){
					v.filter |= filterBranch;
					cerr << n->tok1.val << " used in filter\n";
				}
		}
		break;
	case N_ORDER:
		filterBranch = ORDER_FILTER;
		varUsedInFilter(n->node1, q);
		filterBranch = NO_FILTER;
		break;
	case N_GROUPBY:
		filterBranch = GROUP_FILTER;
		varUsedInFilter(n->node1, q);
		filterBranch = NO_FILTER;
		break;
	default:
		varUsedInFilter(n->node1, q);
		varUsedInFilter(n->node2, q);
		varUsedInFilter(n->node3, q);
		varUsedInFilter(n->node4, q);
	}
}

static void selectAll(querySpecs &q){
	for (int i=1; i<=q.numFiles; ++i){
		auto f = q.files[str2("_f", i)];
		for (auto &c : f->types){
			q.colspec.colnames.push_back(str2("col",++q.colspec.count));
			q.colspec.types.push_back(T_STRING);
		}
	}
}

static void recordResultColumns(unique_ptr<node> &n, querySpecs &q){
	if (n == nullptr) return;
	string t1 = n->tok1.lower();
	switch (n->label){
	case N_SELECTIONS:
		if (t1 == "hidden") {
		} else if (t1 == "*"){
			selectAll(q);
		} else {
			q.colspec.count++;
			q.colspec.colnames.push_back(n->tok2.val);
			q.colspec.types.push_back(n->datatype);
		}
		recordResultColumns(n->node2, q);
		break;
	case N_FROM:
		if (q.colspec.count == 0)
			selectAll(q);
		break;
	default:
		def:
		recordResultColumns(n->node1, q);
		recordResultColumns(n->node2, q);
		recordResultColumns(n->node3, q);
		recordResultColumns(n->node4, q);
	}
};

static bool findAgrregates(unique_ptr<node> &n){
	if (n == nullptr) return false;
	switch (n->label){
	case N_FUNCTION:
		if ((n->tok1.id & AGG_BIT) != 0){
			return true;
		}
		break;
	default:
		// no shortcircuit evaluation because need to check semantics
		return
		findAgrregates(n->node1) |
		findAgrregates(n->node2) |
		findAgrregates(n->node3) |
		findAgrregates(n->node4);
	}
	return false;
}

//mark selections and function nodes with midrow index+1
static void markGroupMidrowTargets(unique_ptr<node> &n, querySpecs &q){
	if (n == nullptr && !q.grouping) return;
	switch (n->label){
	case N_SELECTIONS:
		if (!findAgrregates(n->node1)){
			n->tok3.id = ++q.midcount;
		}
		markGroupMidrowTargets(n->node2, q);
		break;
	case N_FUNCTION:
		if ((n->tok1.id & AGG_BIT) != 0){
			if (findAgrregates(n->node1))
				error("Cannot have aggregate function inside another aggregate");
			n->tok6.id = ++q.midcount;
			return;
		}
		break;
	case N_GROUPBY:
		if (findAgrregates(n->node1))
			error("Cannot have aggregate function in groupby clause");
		break;
	default:
		markGroupMidrowTargets(n->node1, q);
		markGroupMidrowTargets(n->node2, q);
		markGroupMidrowTargets(n->node3, q);
		markGroupMidrowTargets(n->node4, q);
	}
}

//typing done, still need semantics etc
void analyzeTree(querySpecs &q){
	varUsedInFilter(q.tree, q);
	recordResultColumns(q.tree, q);
}
