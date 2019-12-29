#include "interpretor.h"


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

static void selectAll(querySpecs &q){
	for (int i=0; i<q.numFiles; ++i){
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
		if (t1 == "hidden")
			return;
		else if (t1 == "*"){
			selectAll(q);
		} else
			q.colspec.count++;
			q.colspec.colnames.push_back(n->tok2.val);
			q.colspec.types.push_back(n->datatype);
		break;
	case N_FROM:
		if (q.colspec.count == 0)
			selectAll(q);
		break;
	default:
		recordResultColumns(n->node1, q);
		recordResultColumns(n->node2, q);
		recordResultColumns(n->node3, q);
		recordResultColumns(n->node4, q);
	}
};

//typing done, still need semantics etc
void analyzeTree(querySpecs &q){
	varUsedInWhere(q.tree, q);
	recordResultColumns(q.tree, q);
}
