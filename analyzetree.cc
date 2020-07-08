#include "interpretor.h"
#include<set>
#include<algorithm>

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
			q.var(n->tok1.val).filter |= filterBranch;
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
			n->tok4.id = q.colspec.count++;
			q.colspec.colnames.push_back(n->tok2.val);
			q.colspec.types.push_back(n->datatype);
		}
		recordResultColumns(n->node2, q);
		break;
	case N_FROM:
		if (q.colspec.count == 0)
			selectAll(q);
		break;
	case N_EXPRESSIONS:
		if (n->tok2.id){
			if (q.grouping){
				n->tok3.id = q.colspec.count + q.sortcount;
			} else {
				n->tok3.id = q.sortcount;
			}
			q.sortcount++;
		}
	default:
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
			if (n->tok3.lower() == "distinct"){
				if (n->node1->datatype == T_STRING){
					n->tok4.id = q.distinctSFuncs++;
				} else {
					n->tok4.id = q.distinctNFuncs++;
				}
			}
			return true;
		} else {
			return findAgrregates(n->node1, q);
		}
		break;
	case N_VARS:
		if (findAgrregates(n->node1, q)){
			return true;
		}
		return false;
	case N_VALUE:
		if (n->tok2.id == VARIABLE && (q.var(n->tok1.val).phase & 2))
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

static void setVarPhase(unique_ptr<node> &n, querySpecs &q, int phase, int section){
	if (n == nullptr) return;
	switch (n->label){
	case N_VARS:
		if (findAgrregates(n, q)){
			q.var(n->tok1.val).phase |= 2;
			setVarPhase(n->node2, q, 2, 1);
		} else {
			q.var(n->tok1.val).phase |= 1;
		}
		setVarPhase(n->node2, q, 1, 0);
		break;
	case N_SELECTIONS:
		if (!findAgrregates(n->node1, q))
			setVarPhase(n->node1, q, 1|2, 2);
		else
			setVarPhase(n->node1, q, 2, 2);
		setVarPhase(n->node2, q, 1, 0);
		break;
	case N_FUNCTION:
		if ((n->tok1.id & AGG_BIT) != 0){
			setVarPhase(n->node1, q, 1, section);
		} else {
			setVarPhase(n->node1, q, phase, section);
		}
		break;
	case N_VALUE:
		if (n->tok2.id == VARIABLE){
			switch (section){
			case 0: 
				error("invalid variable found");
			case 1: //var declarations
			case 2: //selections
			case 3: //join conditions
			case 4: //having conditions
			case 5: //where conditions
				q.var(n->tok1.val).phase |= phase;
				break;
			}
		} else {
			setVarPhase(n->node1, q, phase, section);
		}
		break;
	case N_JOIN:
		if (findAgrregates(n->node1, q))
			error("cannot have aggregates in 'join' clause");
		setVarPhase(n->node1, q, 1, 3);
		break;
	case N_HAVING:
		if (!findAgrregates(n->node1, q))
			error("values in 'having' clause must be aggregates");
		setVarPhase(n->node1, q, 2, 4);
		break;
	case N_WHERE:
		if (findAgrregates(n->node1, q))
			error("cannot have aggregates in 'where' clause");
		setVarPhase(n->node1, q, 1, 5);
		break;
	case N_ORDER:
		if (!findAgrregates(n->node1, q))
			error("cannot sort aggregate query by non-aggregate value");
		setVarPhase(n->node1, q, 2, 6);
		break;
	default:
		setVarPhase(n->node1, q, phase, section);
		setVarPhase(n->node2, q, phase, section);
		setVarPhase(n->node3, q, phase, section);
		setVarPhase(n->node4, q, phase, section);
	}
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
		n->phase = phase;
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
			n->phase = q.var(n->tok1.val).phase;
			setNodePhase(n->node1, q, 1);
		}
		q.var(n->tok1.val).phase |= n->phase;
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
		if (findAgrregates(n, q)) {
			findMidrowTargets(n->node1, q);
		} else {
			n->tok3.id = q.midcount;
			q.midcount++;
		}
		findMidrowTargets(n->node2, q);
		break;
	case N_SELECTIONS:
		if (findAgrregates(n->node1, q)){
			findMidrowTargets(n->node1, q);
		} else {
			n->tok3.id = q.midcount;
			q.midcount++;
		}
		findMidrowTargets(n->node2, q);
		break;
	case N_FUNCTION:
		if ((n->tok1.id & AGG_BIT) != 0){
			//TODO:re-enable nested aggregate check
			n->tok6.id = q.midcount;
			q.midcount++;
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

//return file number referenced in expression, or -1 if not one file
static set<int> whichFilesReferenced(unique_ptr<node> &n, querySpecs &q){
	if (n == nullptr) return {};
	switch (n->label){
		case N_VALUE:
			if (n->tok2.id == COLUMN)
				return {q.files[n->tok3.val]->fileno };
			else
				return whichFilesReferenced(n->node1, q);
		default: 
			{
				auto fileSet = whichFilesReferenced(n->node1, q);
				fileSet.merge(whichFilesReferenced(n->node2, q));
				fileSet.merge(whichFilesReferenced(n->node3, q));
				fileSet.merge(whichFilesReferenced(n->node4, q));
				return fileSet;
			}
	}
}

static void findIndexableJoinValues(unique_ptr<node> &n, querySpecs &q, int fileno){
	if (n == nullptr || !q.joining) return;
	switch (n->label){
	case N_PREDCOMP: {
			auto e1 = whichFilesReferenced(n->node1, q);
			auto e2 = whichFilesReferenced(n->node2, q);
			if (q.strictJoin){
				if (n->tok1.id != SP_EQ)
					error("Join condition must use '='");
				if (e1.size() != 1 && e2.size() != 1)
					error("Join condition must reference one file on each side of '='");
			}
			set<int> intersection;
			set_intersection(e1.begin(), e1.end(), e2.begin(), e2.end(), inserter(intersection, intersection.begin()));
			if (intersection.size())
				error("Join condition cannot reference same file on both sides of '"+n->tok1.val+"'");
			if (e1.size() == 1 && *e1.begin() == fileno){
				for (auto i : e2)
					if (i > fileno)
						error("Join condition cannot compare to a file that appears later in the query, only earlier");
				n->tok4.id = 1;
			}else if (e2.size() == 1 && *e2.begin() == fileno){
				for (auto i : e1)
					if (i > fileno)
						error("Join condition cannot compare to a file that appears later in the query, only earlier");
				n->tok4.id = 2;
			}else
				error("One side of join condition must be the joined file and only the joined file");

			switch (n->tok1.id){
			case SP_LPAREN:
				findIndexableJoinValues(n->node1, q, fileno);
				break;
			case SP_EQ:
				break;
			}
		}
		break;
	case N_JOIN:
		{
			auto& f = q.files[n->tok4.val];
			if (!f)
				error("Could not find file matching join alias "+n->tok4.val);
			fileno = f->fileno;
		}
	default:
		findIndexableJoinValues(n->node1, q, fileno);
		findIndexableJoinValues(n->node2, q, fileno);
		findIndexableJoinValues(n->node3, q, fileno);
		findIndexableJoinValues(n->node4, q, fileno);
	}
}

//typing done, still need semantics etc
void analyzeTree(querySpecs &q){
	varUsedInFilter(q.tree, q);
	recordResultColumns(q.tree, q);
	if (q.grouping){
		setVarPhase(q.tree, q, 1, 0);
		findMidrowTargets(q.tree, q);
		setNodePhase(q.tree, q, 1);
	}
	if (q.joining){
		findIndexableJoinValues(q.tree->node3->node1, q, 0);
	}
}
