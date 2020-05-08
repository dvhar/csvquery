#include "interpretor.h"

static void varUsedInFilter(unique_ptr<node> &n, querySpecs &q){
	if (n == nullptr) return;
	static int filterBranch = NO_FILTER;
	string t1;
	switch (n->label){
	//skip irrelvent subtrees
	case N_PRESELECT:
	case N_FROM:
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
	case N_HAVING:
		filterBranch = HAVING_FILTER;
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

static bool findAgrregates(unique_ptr<node> &n, querySpecs &q){
	if (n == nullptr) return false;
	switch (n->label){
	case N_FUNCTION:
		if ((n->tok1.id & AGG_BIT) != 0){
			return true;
		}
		break;
	case N_VARS:
		if (findAgrregates(n->node1, q))
			for (auto &v : q.vars)
				if (n->tok1.val == v.name){
					v.phase = 2; //set node phase later for loose coupling
					return true;
				}
		return false;
	case N_VALUE:
		if (n->tok2.id == VARIABLE)
			for (auto &v : q.vars)
				if (n->tok1.val == v.name && v.phase == 2)
					return true;
		return findAgrregates(n->node1, q);
	default:
		// no shortcircuit evaluation because need to check semantics
		return
		findAgrregates(n->node1, q) |
		findAgrregates(n->node2, q) |
		findAgrregates(n->node3, q) |
		findAgrregates(n->node4, q);
	}
	return false;
}

static void setNodePhase(unique_ptr<node> &n, querySpecs &q, int phase){
	if (n == nullptr) return;
	switch (n->label){
	case N_SELECTIONS:
		if (findAgrregates(n->node1, q)){
			//upper nodes of aggregate query are phase 2
			n->phase = 2;
			setNodePhase(n->node1, q, 2);
		} else {
			//non-aggregate gets queried in phase 1, retrieved again in phase 2
			n->phase = 1|2;
			setNodePhase(n->node1, q, 1);
		}
		setNodePhase(n->node2, q, 2);
		break;
	case N_FUNCTION:
		if ((n->tok1.id & AGG_BIT) != 0){
			//nodes below aggregate are phase 1
			setNodePhase(n->node1, q, 1);
		} else {
			setNodePhase(n->node1, q, phase);
		}
		break;
	case N_VARS:
		if (findAgrregates(n->node1, q)){
			n->phase = 2;
			setNodePhase(n->node1, q, 2);
		} else {
			n->phase = 1;
			setNodePhase(n->node1, q, 1);
		}
		for (auto &v : q.vars)
			if (n->tok1.val == v.name)
				v.phase = n->phase;
		setNodePhase(n->node2, q, 0);
		break;
	case N_GROUPBY:
	case N_WHERE:
		n->phase = 1;
		setNodePhase(n->node1, q, 1);
		break;
	case N_HAVING:
	case N_ORDER:
		n->phase = 2;
		setNodePhase(n->node1, q, 2);
		break;
	default:
		n->phase = phase;
		setNodePhase(n->node1, q, phase);
		setNodePhase(n->node2, q, phase);
		setNodePhase(n->node3, q, phase);
		setNodePhase(n->node4, q, phase);
	}
}

static void findMidrowTargets(unique_ptr<node> &n, querySpecs &q){
	if (n == nullptr || !q.grouping) return;
	switch (n->label){
	case N_VARS:
		findAgrregates(n, q); //mark variables as aggregate with phase value before doing rest of agg stuff
		findMidrowTargets(n->node2, q);
		break;
	case N_SELECTIONS:
		if (findAgrregates(n->node1, q)){
			findMidrowTargets(n->node1, q);
		} else {
			q.midcount++;
			n->tok3.id = q.midcount;
		}
		findMidrowTargets(n->node2, q);
		break;
	case N_FUNCTION:
		if ((n->tok1.id & AGG_BIT) != 0){
			if (findAgrregates(n->node1, q))
				error("Cannot have aggregate function inside another aggregate");
			q.midcount++;
			n->tok6.id = q.midcount;
			return;
		} else {
			findAgrregates(n->node1, q);
		}
		break;
	case N_GROUPBY:
		if (findAgrregates(n->node1, q))
			error("Cannot have aggregate function in groupby clause");
		break;
	default:
		findMidrowTargets(n->node1, q);
		findMidrowTargets(n->node2, q);
		findMidrowTargets(n->node3, q);
		findMidrowTargets(n->node4, q);
	}
}

static void countMidrowVars(unique_ptr<node> &n, querySpecs &q){
	if (n == nullptr) return;
	switch (n->label){
	case N_VARS:
		for (auto &v : q.vars)
			if (n->tok1.val == v.name){
				if (n->node1->phase == 2)
					v.mrindex = q.midcount + q.midrowvars++;
			}
		countMidrowVars(n->node1, q);
		countMidrowVars(n->node2, q);
		break;
	case N_SELECTIONS:
		return; //don't need to go past var branch
	default:
		countMidrowVars(n->node1, q);
		countMidrowVars(n->node2, q);
		countMidrowVars(n->node3, q);
		countMidrowVars(n->node4, q);
	}
}

//typing done, still need semantics etc
void analyzeTree(querySpecs &q){
	varUsedInFilter(q.tree, q);
	recordResultColumns(q.tree, q);
	if (q.grouping){
		findMidrowTargets(q.tree, q);
		setNodePhase(q.tree, q, 0);
		countMidrowVars(q.tree, q);
	}
}
