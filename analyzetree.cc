#include "interpretor.h"
#include<set>
#include<algorithm>

class analyzer {
	int andChainSize;
	public:
		querySpecs* q;
		analyzer(querySpecs& qs): q{&qs} {}
		void varUsedInFilter(unique_ptr<node> &n);
		void setSubtreeVarFilter(unique_ptr<node> &n, int filter);
		void propogateVarFilter(string var, int filter);
		void selectAll();
		void recordResultColumns(unique_ptr<node> &n);
		bool findAgrregates(unique_ptr<node> &n);
		void findMidrowTargets(unique_ptr<node> &n);
		void setVarPhase(unique_ptr<node> &n, int phase, int section);
		void setNodePhase(unique_ptr<node> &n, int phase);
		void findIndexableJoinValues(unique_ptr<node> &n, int fileno);
		set<int> whichFilesReferenced(unique_ptr<node> &n);
		void findJoinAndChains(unique_ptr<node> &n, int fileno);
		bool ischain(unique_ptr<node> &n, int &predno);
};

void analyzer::propogateVarFilter(string var, int filter){
	if (auto& firstvar = findFirstNode(q->tree->node1, N_VARS); firstvar)
		for (auto n = firstvar.get(); n; n = n->node2.get())
			if (n->tok1.val == var){
				setSubtreeVarFilter(n->node1,  filter);
				return;
			}
}
void analyzer::setSubtreeVarFilter(unique_ptr<node> &n, int filter){
	if (n == nullptr) return;
	switch (n->label){
	case N_VALUE:
		if (n->tok2.id == VARIABLE){
			auto& var = q->var(n->tok1.val);
			var.filter |= filter;
			propogateVarFilter(n->tok1.val, filter);
		} else {
			setSubtreeVarFilter(n->node1, filter);
		}
		break;
	default:
		setSubtreeVarFilter(n->node1, filter);
		setSubtreeVarFilter(n->node2, filter);
		setSubtreeVarFilter(n->node3, filter);
		setSubtreeVarFilter(n->node4, filter);
	}
}
void analyzer::varUsedInFilter(unique_ptr<node> &n){
	if (n == nullptr) return;
	string t1;
	switch (n->label){
	//vars case just adds files referenced info to variable for joins and finds max
	case N_VARS:
		{
			auto& var = q->var(n->tok1.val);
			var.filesReferenced = whichFilesReferenced(n->node1);
			var.maxfileno = 0;
			for (auto r: var.filesReferenced)
				var.maxfileno = max(r, var.maxfileno);
			varUsedInFilter(n->node2);
		}
		break;
	case N_SELECTIONS:
		t1 = n->tok1.lower();
		if (t1 == "hidden"sv || t1 == "distinct"sv){
			setSubtreeVarFilter(n->node1, DISTINCT_FILTER);
			if (t1 != "hidden"sv)
				setSubtreeVarFilter(n->node1, SELECT_FILTER);
		} else {
			setSubtreeVarFilter(n->node1, SELECT_FILTER);
		}
		varUsedInFilter(n->node2);
		break;
	case N_WHERE:
		setSubtreeVarFilter(n->node1, WHERE_FILTER);
		break;
	case N_ORDER:
		setSubtreeVarFilter(n->node1, ORDER_FILTER);
		break;
	case N_GROUPBY:
		setSubtreeVarFilter(n->node1, AGG_FILTER);
		break;
	case N_HAVING:
		setSubtreeVarFilter(n->node1, HAVING_FILTER);
		break;
	//joins handled later to distinguish scan vs comp values
	default:
		varUsedInFilter(n->node1);
		varUsedInFilter(n->node2);
		varUsedInFilter(n->node3);
		varUsedInFilter(n->node4);
	}
}

void analyzer::selectAll(){
	for (auto& f: q->filevec){
		for (auto &c : f->types){
			++q->colspec.count;
			if (f->noheader)
				q->colspec.colnames.push_back(st("col",q->colspec.count));
			q->colspec.types.push_back(T_STRING);
		}
		if (!f->noheader)
			q->colspec.colnames.insert(q->colspec.colnames.end(), f->colnames.begin(), f->colnames.end());
	}
}

void analyzer::recordResultColumns(unique_ptr<node> &n){
	if (n == nullptr) return;
	string t1 = n->tok1.lower();
	switch (n->label){
	case N_SELECTIONS:
		if (t1 == "hidden"sv) {
		} else if (t1 == "*"sv){
			selectAll();
		} else {
			n->tok4.id = q->colspec.count++;
			auto name = n->tok2.val;
			if (name.empty())
				name = nodeName(n->node1, q);
			if (name.empty())
				name = st("col",q->colspec.count);
			q->colspec.colnames.push_back(name); //TODO: names for non-aliased columns
			q->colspec.types.push_back(n->datatype);
		}
		recordResultColumns(n->node2);
		break;
	case N_FROM:
		if (q->colspec.count == 0)
			selectAll();
		break;
	case N_EXPRESSIONS:
		if (n->tok2.id){
			if (q->grouping){
				n->tok3.id = q->colspec.count + q->sortcount;
			} else {
				n->tok3.id = q->sortcount;
			}
			q->sortcount++;
		}
	default:
		recordResultColumns(n->node1);
		recordResultColumns(n->node2);
		recordResultColumns(n->node3);
		recordResultColumns(n->node4);
	}
};

bool analyzer::findAgrregates(unique_ptr<node> &n){
	if (n == nullptr) return false;
	switch (n->label){
	case N_FUNCTION:
		if ((n->tok1.id & AGG_BIT) != 0){
			if (n->tok3.lower() == "distinct"sv){
				if (n->node1->datatype == T_STRING){
					n->tok4.id = q->distinctSFuncs++;
				} else {
					n->tok4.id = q->distinctNFuncs++;
				}
			}
			return true;
		} else {
			return findAgrregates(n->node1);
		}
		break;
	case N_VARS:
		if (findAgrregates(n->node1)){
			return true;
		}
		return false;
	case N_VALUE:
		if (n->tok2.id == VARIABLE && (q->var(n->tok1.val).phase & 2))
			return true;
		return findAgrregates(n->node1);
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

void analyzer::setVarPhase(unique_ptr<node> &n, int phase, int section){
	if (n == nullptr) return;
	switch (n->label){
	case N_VARS:
		if (findAgrregates(n)){
			q->var(n->tok1.val).phase |= 2;
			setVarPhase(n->node2, 2, 1);
		} else {
			q->var(n->tok1.val).phase |= 1;
		}
		setVarPhase(n->node2, 1, 0);
		break;
	case N_SELECTIONS:
		if (!findAgrregates(n->node1))
			setVarPhase(n->node1, 1|2, 2);
		else
			setVarPhase(n->node1, 2, 2);
		setVarPhase(n->node2, 1, 0);
		break;
	case N_FUNCTION:
		if ((n->tok1.id & AGG_BIT) != 0){
			setVarPhase(n->node1, 1, section);
		} else {
			setVarPhase(n->node1, phase, section);
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
				q->var(n->tok1.val).phase |= phase;
				break;
			}
		} else {
			setVarPhase(n->node1, phase, section);
		}
		break;
	case N_JOIN:
		if (findAgrregates(n->node1))
			error("cannot have aggregates in 'join' clause");
		setVarPhase(n->node1, 1, 3);
		break;
	case N_HAVING:
		if (!findAgrregates(n->node1))
			error("values in 'having' clause must be aggregates");
		setVarPhase(n->node1, 2, 4);
		break;
	case N_WHERE:
		if (findAgrregates(n->node1))
			error("cannot have aggregates in 'where' clause");
		setVarPhase(n->node1, 1, 5);
		break;
	case N_ORDER:
		if (!findAgrregates(n->node1))
			error("cannot sort aggregate query by non-aggregate value");
		setVarPhase(n->node1, 2, 6);
		break;
	default:
		setVarPhase(n->node1, phase, section);
		setVarPhase(n->node2, phase, section);
		setVarPhase(n->node3, phase, section);
		setVarPhase(n->node4, phase, section);
	}
}

void analyzer::setNodePhase(unique_ptr<node> &n, int phase){
	if (n == nullptr) return;
	switch (n->label){
	case N_SELECTIONS:
		if (findAgrregates(n->node1)){
			//upper nodes of aggregate query are phase 2
			n->phase = 2;
			setNodePhase(n->node1, 2);
		} else {
			//non-aggregate gets queried in phase 1, retrieved again in phase 2
			n->phase = 1|2;
			setNodePhase(n->node1, 1);
		}
		setNodePhase(n->node2, 2);
		break;
	case N_FUNCTION:
		n->phase = phase;
		if ((n->tok1.id & AGG_BIT) != 0){
			//nodes below aggregate are phase 1
			setNodePhase(n->node1, 1);
		} else {
			setNodePhase(n->node1, phase);
		}
		break;
	case N_VARS:
		if (findAgrregates(n->node1)){
			n->phase = 2;
			setNodePhase(n->node1, 2);
		} else {
			n->phase = q->var(n->tok1.val).phase;
			setNodePhase(n->node1, 1);
		}
		q->var(n->tok1.val).phase |= n->phase;
		setNodePhase(n->node2, 0);
		break;
	case N_GROUPBY:
	case N_WHERE:
		n->phase = 1;
		setNodePhase(n->node1, 1);
		break;
	case N_HAVING:
	case N_ORDER:
		n->phase = 2;
		setNodePhase(n->node1, 2);
		break;
	default:
		n->phase = phase;
		setNodePhase(n->node1, phase);
		setNodePhase(n->node2, phase);
		setNodePhase(n->node3, phase);
		setNodePhase(n->node4, phase);
	}
}

void analyzer::findMidrowTargets(unique_ptr<node> &n){
	if (n == nullptr || !q->grouping) return;
	switch (n->label){
	case N_VARS:
		if (findAgrregates(n)) {
			findMidrowTargets(n->node1);
		} else {
			n->tok3.id = q->midcount;
			q->midcount++;
		}
		findMidrowTargets(n->node2);
		break;
	case N_SELECTIONS:
		if (findAgrregates(n->node1)){
			findMidrowTargets(n->node1);
		} else {
			n->tok3.id = q->midcount;
			q->midcount++;
		}
		findMidrowTargets(n->node2);
		break;
	case N_FUNCTION:
		if ((n->tok1.id & AGG_BIT) != 0){
			//TODO:re-enable nested aggregate check
			n->info[MIDIDX] = q->midcount;
			q->midcount++;
			return;
		} else {
			findAgrregates(n->node1);
		}
		break;
	case N_GROUPBY:
		if (findAgrregates(n->node1))
			error("Cannot have aggregate function in groupby clause");
		break;
	default:
		findMidrowTargets(n->node1);
		findMidrowTargets(n->node2);
		findMidrowTargets(n->node3);
		findMidrowTargets(n->node4);
	}
}

//return file numbers referenced in expression
set<int> analyzer::whichFilesReferenced(unique_ptr<node> &n){
	if (n == nullptr) return {};
	switch (n->label){
		case N_VALUE:
			if (n->tok2.id == COLUMN)
				return {q->filemap[n->tok3.val]->fileno };
			else if (n->tok2.id == VARIABLE)
				return q->var(n->tok1.val).filesReferenced;
			else
				return whichFilesReferenced(n->node1);
		default: 
			{
				auto fileSet = whichFilesReferenced(n->node1);
				fileSet.merge(whichFilesReferenced(n->node2));
				fileSet.merge(whichFilesReferenced(n->node3));
				fileSet.merge(whichFilesReferenced(n->node4));
				return fileSet;
			}
	}
}

//only called on predicates nodes
bool analyzer::ischain(unique_ptr<node> &n, int &predno){
	if (n == nullptr) return predno >= 2;
	if (n->label == KW_OR) return false;
	bool simpleCompare = n->node1->tok1.id != SP_LPAREN;
	if (simpleCompare){
		++predno;
		if (ischain(n->node2, predno)){
			n->info[ANDCHAIN] = 1;
			n->node1->info[ANDCHAIN] = 1;
			return true;
		}
	}
	return false;
}

void analyzer::findJoinAndChains(unique_ptr<node> &n, int fileno){
	if (n == nullptr) return;
	auto& chainvec = q->getFileReader(fileno)->andchains;
	switch (n->label){
		case N_JOIN:
			findJoinAndChains(n->node1, fileno);
			findJoinAndChains(n->node2, fileno+1);
			break;
		case N_PREDICATES:
			if (n->tok1.id == KW_AND){
				bool first = n->info[ANDCHAIN] != 1;
				int chainsize = 0;
				if (ischain(n, chainsize) && first){
					n->info[CHAINSIZE] = chainsize;
					n->info[CHAINIDX] = chainvec.size();
					n->info[FILENO] = fileno;
					chainvec.push_back(andchain(n->info[CHAINSIZE]));
				}
			}
			if (n->info[ANDCHAIN]){
				if (n->info[CHAINSIZE] == 0){ //not first
					n->node1->info[ANDCHAIN] = 2;
				}
			}
			if (n->node1->tok1.id == SP_LPAREN)
				findJoinAndChains(n->node1->node1, fileno);
			findJoinAndChains(n->node2, fileno);
			break;
	}
}

// flip operator when scanned op comes last
int flipOp(int op){
	static flatmap<int,int> flipOperatorMap = {
		{SP_GREAT, SP_LESS},
		{SP_GREATEQ, SP_LESSEQ},
		{SP_LESS, SP_GREAT},
		{SP_LESSEQ, SP_GREATEQ},
	};
	return flipOperatorMap[op] ?: op;
}

void analyzer::findIndexableJoinValues(unique_ptr<node> &n, int fileno){
	if (n == nullptr || !q->joining) return;
	switch (n->label){
	case N_PREDCOMP:
		switch (n->tok1.id){
		case SP_LPAREN:
			findIndexableJoinValues(n->node1, fileno);
			return;
		default: //found a comparison
			{
			auto e1 = whichFilesReferenced(n->node1);
			auto e2 = whichFilesReferenced(n->node2);
			set<int> intersection;
			set_intersection(e1.begin(), e1.end(), e2.begin(), e2.end(),
					inserter(intersection, intersection.begin()));
			if (intersection.size())
				error("Join condition cannot reference same file on both sides of '"+n->tok1.val+"'");
			if (e1.size() == 1 && *e1.begin() == fileno){
				for (auto i : e2)
					if (i > fileno)
						error("Join condition cannot compare to a file that appears later in the query, only earlier");
				n->info[TOSCAN] = 1;
				setSubtreeVarFilter(n->node2, JCOMP_FILTER);
				setSubtreeVarFilter(n->node1, JSCAN_FILTER);
			}else if (e2.size() == 1 && *e2.begin() == fileno){
				for (auto i : e1)
					if (i > fileno)
						error("Join condition cannot compare to a file that appears later in the query, only earlier");
				n->info[TOSCAN] = 2;
				n->tok1.id = flipOp(n->tok1.id);
				setSubtreeVarFilter(n->node2, JSCAN_FILTER);
				setSubtreeVarFilter(n->node1, JCOMP_FILTER);
			}else{
				error("One side of join condition must be the joined file and only the joined file");
			}

			//add valpos vector only if not part of and chain
			if (n->info[ANDCHAIN] == 0){
				auto& vpv = q->getFileReader(fileno)->joinValpos;
				n->tok5.id = vpv.size();
				vpv.push_back(vector<valpos>());
			}
			return;
			}
		}
		break;
	case N_JOIN:
		{
			auto& f = q->filemap[n->tok4.val];
			if (!f)
				error("Could not find file matching join alias "+n->tok4.val);
			fileno = f->fileno;
		}
	default:
		findIndexableJoinValues(n->node1, fileno);
		findIndexableJoinValues(n->node2, fileno);
		findIndexableJoinValues(n->node3, fileno);
		findIndexableJoinValues(n->node4, fileno);
	}
}

//typing done, still need semantics etc
void analyzeTree(querySpecs &q){
	analyzer an(q);
	an.varUsedInFilter(q.tree);
	an.recordResultColumns(q.tree);
	if (q.grouping){
		an.setVarPhase(q.tree, 1, 0);
		an.findMidrowTargets(q.tree);
		an.setNodePhase(q.tree, 1);
	}
	if (q.joining){
		an.findJoinAndChains(q.tree->node3->node1, 1);
		an.findIndexableJoinValues(q.tree->node3->node1, 0);
	}
}
