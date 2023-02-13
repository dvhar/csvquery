#include "interpretor.h"
#include <boost/filesystem.hpp>
#include <memory>
#include<set>
#include<algorithm>

class analyzer {
	int andChainSize;
	bool onlyAggregate = false; //TODO: why is default parameter overriding not working?
	public:
		querySpecs* q;
		analyzer(querySpecs& qs): q{&qs} {
			if (int o = q->options; (o & O_OH) || (o & O_NOH))
				shouldPrintHeader();
		}
		void varUsedInFilter(astnode &n);
		void setSubtreeVarFilter(astnode &n, int filter);
		void propogateVarFilter(string var, int filter);
		void selectAll();
		void selectAllGroupOrSubquery();
		pair<astnode,node*> expandSelectAll();
		void recordResultColumns(astnode &n);
		bool findAgrregates(astnode &n);
		bool findOnlyAgrregates(astnode &n);
		bool allAgrregates(astnode &n);
		void findMidrowTargets(astnode &n);
		void setVarPhase(astnode &n, int phase, int section);
		void setNodePhase(astnode &n, int phase);
		void findIndexableJoinValues(astnode &n, int fileno);
		set<int> whichFilesReferenced(astnode &n);
		void findJoinAndChains(astnode &n, int fileno);
		bool ischain(astnode &n, int &predno);
		void shouldPrintHeader();
		int phaser(astnode &n);
		void setAttributes(astnode& n);
		int addAlias(astnode& n);
		void organizeVars(astnode& n);
		bool findVarReferences(astnode&, string&);
};

void analyzer::propogateVarFilter(string var, int filter){
	if (auto& firstvar = findFirstNode(q->tree->node1, N_VARS); firstvar)
		for (auto n = firstvar.get(); n; n = n->nnextvar().get())
			if (n->varname() == var){
				setSubtreeVarFilter(n->nsubexpr(),  filter);
				return;
			}
}
void analyzer::setSubtreeVarFilter(astnode &n, int filter){
	if (n == nullptr) return;
	switch (n->label){
	case N_VALUE:
		if (n->valtype() == VARIABLE){
			q->var(n->val()).filter |= filter;
			propogateVarFilter(n->val(), filter);
		} else {
			setSubtreeVarFilter(n->nsubexpr(), filter);
		}
		break;
	default:
		setSubtreeVarFilter(n->node1, filter);
		setSubtreeVarFilter(n->node2, filter);
		setSubtreeVarFilter(n->node3, filter);
		setSubtreeVarFilter(n->node4, filter);
	}
}
void analyzer::varUsedInFilter(astnode &n){
	if (n == nullptr) return;
	string t1;
	switch (n->label){
	//vars case just adds files referenced info to variable for joins and finds max
	case N_VARS:
		{
			auto& var = q->var(n->varname());
			var.filesReferenced = whichFilesReferenced(n->nsubexpr());
			var.maxfileno = 0;
			for (auto r: var.filesReferenced)
				var.maxfileno = max(r, var.maxfileno);
			varUsedInFilter(n->nnextvar());
		}
		break;
	case N_SELECTIONS:
		setSubtreeVarFilter(n->nsubexpr(), SELECT_FILTER);
		varUsedInFilter(n->nnextselection());
		break;
	case N_WHERE:
		setSubtreeVarFilter(n->nsubexpr(), WHERE_FILTER);
		break;
	case N_ORDER:
		setSubtreeVarFilter(n->nsubexpr(), ORDER_FILTER);
		break;
	case N_GROUPBY:
		setSubtreeVarFilter(n->nsubexpr(), GROUPING_FILTER);
		break;
	case N_HAVING:
		setSubtreeVarFilter(n->nsubexpr(), HAVING_FILTER);
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
		for (auto t : f->types){
			++q->colspec.count;
			if (f->noheader)
				q->colspec.colnames.push_back(st("col",q->colspec.count));
			q->colspec.types.push_back(t);
		}
		if (!f->noheader){
			q->colspec.colnames.insert(q->colspec.colnames.end(), f->colnames.begin(), f->colnames.end());
			shouldPrintHeader();
		}
	}
}

pair<astnode,node*> analyzer::expandSelectAll(){
	auto newselections = pair<astnode,node*>(make_unique<node>(N_SELECTIONS),nullptr);
	node* selection = newselections.first.get();
	node* last;
	for (int filenum = 0; filenum < q->filevec.size(); ++filenum){
		for (int colnum = 1; colnum <= q->filevec[filenum]->numFields; ++colnum){
			auto& newval = selection->nsubexpr() = make_unique<node>(N_VALUE);
			newval->tok1.id = WORD_TK;
			newval->tok1.val = st("_f",filenum,".c",colnum);
			selection->nnextselection() = make_unique<node>(N_SELECTIONS);
			last = selection;
			selection = selection->nnextselection().get();
		}
	}
	last->nnextselection().reset(nullptr);
	newselections.second = last;
	return newselections;
}

void analyzer::selectAllGroupOrSubquery(){
	if (!q->isSubquery && !q->grouping) return;
	if (auto& selections = findFirstNode(q->tree, N_SELECTIONS); selections == nullptr){
		if (auto& select = findFirstNode(q->tree, N_SELECT); select){
			select->nselections() = expandSelectAll().first;
		} else {
			q->tree->nselect() = make_unique<node>(N_SELECT);
			q->tree->nselect()->nselections() = expandSelectAll().first;
		}
	} else {
		auto prev = findFirstNode(q->tree, N_SELECT).get();
		auto next = prev->nselections().get();
		do {
			if (next->startok() == SP_STAR){
				auto&& allcolumns = expandSelectAll();
				auto nextafter = next->nnextselection().release();
				prev->nnextselection().reset(allcolumns.first.release());
				allcolumns.second->nnextselection().reset(nextafter);
			}
			prev = next;
			next = next->nnextselection().get();
		} while (next && next->label == N_SELECTIONS);
	}
}

void analyzer::recordResultColumns(astnode &n){
	if (n == nullptr) return;
	switch (n->label){
	case N_SELECTIONS:
		if (n->startok() == "*"){
			selectAll();
		} else {
			n->selectiondestidx() = q->colspec.count++;
			auto name = n->selectionalias();
			bool tcol = isTrivialColumn(n);
			if (name.empty() && (tcol || isTrivialAlias(n)))
				name = nodeName(n->nsubexpr(), q);
			if (!name.empty())
				shouldPrintHeader();
			if (name.empty())
				name = st("col",q->colspec.count);
			q->colspec.colnames.push_back(name);
			if (tcol)
				q->colspec.types.push_back(q->trivialColumnType(n));
			else
				q->colspec.types.push_back(n->datatype);
		}
		recordResultColumns(n->nnextselection());
		break;
	case N_FROM:
		if (q->colspec.count == 0)
			selectAll();
		break;
	case N_EXPRESSIONS:  //sort list
		if (n->ordersize()){
			if (q->grouping){
				n->aggorderdestidx() = q->colspec.count + q->sortcount;
			} else {
				n->aggorderdestidx() = q->sortcount;
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

bool analyzer::allAgrregates(astnode &n){
	for (auto nn = n.get(); nn; nn = nn->node2.get())
		if (!findAgrregates(nn->node1))
			return false;
	return true;
}
bool analyzer::findOnlyAgrregates(astnode &n){
	bool temp = onlyAggregate;
	onlyAggregate = true;
	bool ret = findAgrregates(n);
	onlyAggregate = temp;
	return ret;
}
bool analyzer::findAgrregates(astnode &n){
	if (n == nullptr) return false;
	switch (n->label){
	case N_FUNCTION:
		if ((n->funcid() & AGG_BIT) != 0){
			if (n->funcdisttok() == "distinct"){
				n->funcdistnum() = q->settypes.size();
				q->settypes.push_back(n->nsubexpr()->datatype == T_STRING);
				q->distinctFuncs = 1;
			}
			return true;
		} else {
			return findAgrregates(n->nsubexpr());
		}
		break;
	case N_VARS:
		if (findAgrregates(n->nsubexpr())){
			return true;
		}
		return false;
	case N_VALUE:
		if (n->valtype() == VARIABLE){
			if (onlyAggregate) {
				if (q->var(n->varname()).phase == 2)
					return true;
			} else {
				if (q->var(n->varname()).phase & 2)
					return true;
			}
		}
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

void analyzer::setVarPhase(astnode &n, int phase, int section){
	if (n == nullptr) return;
	switch (n->label){
	case N_VARS:
		if (findAgrregates(n)){
			q->var(n->varname()).phase |= 2;
			setVarPhase(n->nnextvar(), 2, 1);
		} else {
			q->var(n->varname()).phase |= 1;
		}
		setVarPhase(n->nnextvar(), 1, 0);
		break;
	case N_SELECTIONS:
		if (!findAgrregates(n->nsubexpr()))
			setVarPhase(n->nsubexpr(), 1|2, 2);
		else
			setVarPhase(n->nsubexpr(), 2, 2);
		setVarPhase(n->nnextselection(), 1, 0);
		break;
	case N_FUNCTION:
		if ((n->funcid() & AGG_BIT) != 0){
			setVarPhase(n->nsubexpr(), 1, section);
		} else {
			setVarPhase(n->nsubexpr(), phase, section);
		}
		break;
	case N_VALUE:
		if (n->valtype() == VARIABLE){
			switch (section){
			case 0: 
				error("invalid variable found");
			case 1: //var declarations
			case 2: //selections
			case 3: //join conditions
			case 4: //having conditions
			case 5: //where conditions
			// case 6: //order by ?
			case 7: //group by
				q->var(n->val()).phase |= phase;
				break;
			}
		} else {
			setVarPhase(n->nsubexpr(), phase, section);
		}
		break;
	case N_JOIN:
		if (findOnlyAgrregates(n->njoinconds()))
			error("cannot have aggregates in 'join' clause");
		setVarPhase(n->njoinconds(), 1, 3);
		break;
	case N_HAVING:
		if (!findAgrregates(n->npredconds()))
			error("values in 'having' clause must be aggregates");
		setVarPhase(n->npredconds(), 2, 4);
		break;
	case N_WHERE:
		if (findOnlyAgrregates(n->npredconds()))
			error("cannot have aggregates in 'where' clause");
		setVarPhase(n->npredconds(), 1, 5);
		break;
	case N_ORDER:
		if (!allAgrregates(n->norderlist()))
			error("cannot sort aggregate query by non-aggregate value");
		setVarPhase(n->norderlist(), 2, 6);
		break;
	case N_GROUPBY:
		if (findOnlyAgrregates(n->ngrouplist()))
			error("cannot have aggregate in group by clause");
		setVarPhase(n->ngrouplist(), 1, 7);
		break;
	default:
		setVarPhase(n->node1, phase, section);
		setVarPhase(n->node2, phase, section);
		setVarPhase(n->node3, phase, section);
		setVarPhase(n->node4, phase, section);
	}
}

void analyzer::setNodePhase(astnode &n, int phase){
	if (n == nullptr) return;
	switch (n->label){
	case N_SELECTIONS:
		if (findAgrregates(n->nsubexpr())){
			//upper nodes of aggregate query are phase 2
			n->phase = 2;
			setNodePhase(n->nsubexpr(), 2);
		} else {
			//non-aggregate gets queried in phase 1, retrieved again in phase 2
			n->phase = 1|2;
			setNodePhase(n->nsubexpr(), 1);
		}
		setNodePhase(n->nnextselection(), 2);
		phaser(n);
		break;
	case N_FUNCTION:
		n->phase = phase;
		if ((n->funcid() & AGG_BIT) != 0){
			//nodes below aggregate are phase 1
			setNodePhase(n->nsubexpr(), 1);
			setNodePhase(n->node2, 1);
		} else {
			setNodePhase(n->nsubexpr(), phase);
			setNodePhase(n->node2, phase);
		}
		break;
	case N_VARS:
		if (findAgrregates(n->nsubexpr())){
			n->phase = 2;
			setNodePhase(n->nsubexpr(), 2);
		} else {
			n->phase = q->var(n->varname()).phase;
			setNodePhase(n->nsubexpr(), 1);
		}
		q->var(n->varname()).phase |= n->phase;
		setNodePhase(n->nnextvar(), 0);
		break;
	case N_GROUPBY:
	case N_WHERE:
		n->phase = 1;
		setNodePhase(n->npredconds(), 1);
		break;
	case N_HAVING:
		n->phase = 2;
		setNodePhase(n->npredconds(), 2);
		break;
	case N_ORDER:
		n->phase = 2;
		setNodePhase(n->norderlist(), 2);
		break;
	default:
		n->phase = phase;
		setNodePhase(n->node1, phase);
		setNodePhase(n->node2, phase);
		setNodePhase(n->node3, phase);
		setNodePhase(n->node4, phase);
	}
}

int analyzer::phaser(astnode &n){
	if (n == nullptr || !q->grouping) return 0;
	switch (n->label){
		case N_SELECTIONS:
			if (n->phase > 1){
				if (phaser(n->nsubexpr()) < n->phase) {
					n->selectionlpmid() = 1;
				}
			}
	}
	return n->phase;
}
void analyzer::findMidrowTargets(astnode &n){
	if (n == nullptr || !q->grouping) return;
	switch (n->label){
	case N_VARS:
		if (findAgrregates(n)) {
			findMidrowTargets(n->nsubexpr());
		} else {
			n->varmididx() = q->midcount;
			q->midcount++;
		}
		findMidrowTargets(n->nnextvar());
		break;
	case N_SELECTIONS:
		if (findAgrregates(n->nsubexpr())){
			findMidrowTargets(n->nsubexpr());
		} else {
			n->selectionmididx() = q->midcount;
			q->midcount++;
		}
		findMidrowTargets(n->nnextselection());
		break;
	case N_FUNCTION:
		if ((n->funcid() & AGG_BIT) != 0){
			//TODO:re-enable nested aggregate check
			n->funcmididx() = q->midcount;
			q->midcount++;
			return;
		} else {
			findMidrowTargets(n->nsubexpr());
		}
		break;
	case N_GROUPBY:
		if (findOnlyAgrregates(n->ngrouplist()))
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
set<int> analyzer::whichFilesReferenced(astnode &n){
	if (n == nullptr) return {};
	switch (n->label){
		case N_VALUE:
			if (n->valtype() == COLUMN)
				return {q->filemap[n->dotsrc()]->fileno };
			else if (n->valtype() == VARIABLE)
				return q->var(n->varname()).filesReferenced;
			else
				return whichFilesReferenced(n->nsubexpr());
		default: 
			{
				auto f1 = whichFilesReferenced(n->node1);
				auto f2 = whichFilesReferenced(n->node2);
				auto f3 = whichFilesReferenced(n->node3);
				auto f4 = whichFilesReferenced(n->node4);
				f1.insert(f2.begin(), f2.end());
				f1.insert(f3.begin(), f3.end());
				f1.insert(f4.begin(), f4.end());
				return f1;
			}
	}
}

//only called on predicates nodes
bool analyzer::ischain(astnode &n, int &predno){
	if (n == nullptr) return predno >= 2;
	if (n->logop() == KW_OR || n->logop() == KW_XOR) return false;
	bool simpleCompare = n->npredcomp()->relop() != SP_LPAREN;
	if (simpleCompare){
		++predno;
		if (ischain(n->nnextpreds(), predno)){
			n->andChain() = 1;
			n->node1->andChain() = 1;
			return true;
		}
	}
	return false;
}

void analyzer::findJoinAndChains(astnode &n, int fileno){
	if (n == nullptr) return;
	auto& chainvec = q->getFileReader(fileno)->andchains;
	switch (n->label){
		case N_JOIN:
			findJoinAndChains(n->njoinconds(), fileno);
			findJoinAndChains(n->nnextjoin(), fileno+1);
			break;
		case N_PREDICATES:
			if (n->logop() == KW_AND){
				bool first = n->andChain() != 1;
				int chainsize = 0;
				if (ischain(n, chainsize) && first){
					n->chainSize() = chainsize;
					n->chainIdx() = chainvec.size();
					n->predFileNum() = fileno;
					chainvec.push_back(andchain(n->chainSize()));
				}
			}
			if (n->andChain()){
				if (n->chainSize() == 0){ //not first
					n->node1->andChain() = 2;
				}
			}
			if (n->npredcomp()->relop() == SP_LPAREN)
				findJoinAndChains(n->npredcomp()->nmorepreds(), fileno);
			findJoinAndChains(n->nnextpreds(), fileno);
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

void analyzer::findIndexableJoinValues(astnode &n, int fileno){
	if (n == nullptr || !q->joining) return;
	switch (n->label){
	case N_PREDCOMP:
		switch (n->relop()){
		case SP_LPAREN:
			findIndexableJoinValues(n->nmorepreds(), fileno);
			return;
		default: //found a comparison
			{
			auto e1 = whichFilesReferenced(n->npredexp1());
			auto e2 = whichFilesReferenced(n->npredexp2());
			set<int> intersection;
			set_intersection(e1.begin(), e1.end(), e2.begin(), e2.end(),
					inserter(intersection, intersection.begin()));
			if (intersection.size())
				error("Join condition cannot reference same file on both sides of '",n->tok1,"'");
			if (e1.size() == 1 && *e1.begin() == fileno){
				for (auto i : e2)
					if (i > fileno)
						error("Join condition cannot compare to a file that appears later in the query, only earlier");
				n->scannedExpr() = 1;
				setSubtreeVarFilter(n->npredexp2(), JCOMP_FILTER);
				setSubtreeVarFilter(n->npredexp1(), JSCAN_FILTER);
			}else if (e2.size() == 1 && *e2.begin() == fileno){
				for (auto i : e1)
					if (i > fileno)
						error("Join condition cannot compare to a file that appears later in the query, only earlier");
				n->scannedExpr() = 2;
				n->relop() = flipOp(n->relop());
				setSubtreeVarFilter(n->npredexp2(), JSCAN_FILTER);
				setSubtreeVarFilter(n->npredexp1(), JCOMP_FILTER);
			}else{
				error("One side of join condition must be the joined file and only the joined file");
			}

			//add valpos vector only if not part of and chain
			if (n->andChain() == 0){
				auto& vpv = q->getFileReader(fileno)->joinValpos;
				n->predValposIdx() = vpv.size();
				vpv.emplace_back();
			}
			return;
			}
		}
		break;
	case N_JOIN:
		{
			auto& f = q->filemap[n->nfile()->filealias()];
			if (!f)
				error("Could not find file matching join alias ",n->nfile()->filealias());
			fileno = f->fileno;
		}
	default:
		findIndexableJoinValues(n->node1, fileno);
		findIndexableJoinValues(n->node2, fileno);
		findIndexableJoinValues(n->node3, fileno);
		findIndexableJoinValues(n->node4, fileno);
	}
}
void analyzer::shouldPrintHeader(){
	if (q->options & O_OH){
		q->outputcsvheader = true;
		return;
	} else if (q->options & O_NOH){
		q->outputcsvheader = false;
		return;
	}
	q->outputcsvheader = true;
}
void analyzer::setAttributes(astnode& n){
	if (n == nullptr) return;
	switch (n->label){
	case N_PRESELECT:
		q->options = n->optionbits();
		break;
	case N_SELECT:
		if (n->quantlimit())
			q->quantityLimit = n->quantlimit();
		break;
	case N_AFTERFROM:
		if (n->quantlimit())
			q->quantityLimit = n->quantlimit();
		break;
	case N_WHERE:
		q->whereFiltering = 1;
		break;
	case N_HAVING:
		q->havingFiltering = 1;
		break;
	case N_ORDER:
		q->sorting = 1;
		return;
	case N_JOIN:
		q->joining = 1;
		return;
	case N_SETLIST:
		if (n->hassubquery()){
			n->subqidx() = q->addSubquery(n->node1, SQ_INLIST);
			return;
		}
		break;
	case N_GROUPBY:
		q->grouping = max(q->grouping, 2);
		return;
	case N_FUNCTION:
		if ((n->funcid() & AGG_BIT) != 0) {
			q->grouping = max(q->grouping,1);
		}
		if (n->funcid() == FN_ENCRYPT || n->funcid() == FN_DECRYPT){
			q->needPass = true;
			q->password = n->password();
		}
	}
	setAttributes(n->node1);
	setAttributes(n->node2);
	setAttributes(n->node3);
	setAttributes(n->node4);
}
int analyzer::addAlias(astnode& n){
	if (n->tok1 == N_HANDLEALIAS){
		auto& aliasnode = n->node1;
		auto& action = aliasnode->tok2;
		//add alias
		if (action == "add") {
			auto& filenode = aliasnode->node1;
			string& alias = aliasnode->tok1.val;
			string& fpath = filenode->tok1.val;
			int opts = filenode->optionbits();
			if (regex_match(alias,filelike))
				error("File alias cannot have dots or slashes");
			string aliasfile = st(globalSettings.configdir,"/alias-",alias,".txt");
			if (boost::filesystem::exists(aliasfile))
				error(alias," alias already exists");
			findExtension(fpath);
			ofstream afile(aliasfile);
			afile << boost::filesystem::canonical(fpath).string() << endl << opts << endl;
			return CMD_SHOWTABLES;
		//drop alias
		} else if (action == "drop") {
			string& alias = aliasnode->tok1.val;
			string aliasfile = st(globalSettings.configdir,"/alias-",alias,".txt");
			if (!boost::filesystem::exists(aliasfile))
				error(alias," alias does not exist");
			boost::filesystem::remove(aliasfile);
			return CMD_DROPALIAS;
		//show aliases - maybe do this in vm
		} else if (action == "show") {
			return CMD_SHOWTABLES;
		}
	}
	return 0;
}

// use renamed selections as vars
void analyzer::organizeVars(astnode& n){
	if (!n || n->label != N_SELECTIONS)
		return;
	auto& vartok = n->tok2;
	if (!vartok.id)
		return;
	if (!(findVarReferences(n->nnextselection(), vartok.val)
			|| findVarReferences(q->tree->nfrom(), vartok.val)
			|| findVarReferences(q->tree->nafterfrom(), vartok.val)))
		return;
	node* varnode;
	if (auto& vars = findFirstNode(q->tree, N_VARS)){
		varnode = vars.get();
		while (varnode->node2)
			varnode = varnode->node2.get();
		varnode->node2 = make_unique<node>(N_VARS);
		varnode = varnode->node2.get();
	} else {
		auto& preselect = findFirstNode(q->tree, N_PRESELECT);
		auto& with = preselect->node1 = make_unique<node>(N_WITH);
		auto& newvars = with->node1 = make_unique<node>(N_VARS);
		varnode = newvars.get();
	}
	varnode->tok1 = vartok;
	varnode->node1 = move(n->node1);
	auto& refnode = n->node1 = make_unique<node>(N_VALUE);
	refnode->val() = vartok.val;
	refnode->valtype() = VARIABLE;
	organizeVars(n->node2);
}

bool analyzer::findVarReferences(astnode &n, string& name){
	if (!n)
		return false;
	if (n->label == N_VALUE && n->val() == name)
			return true;
	return findVarReferences(n->node1, name)
		?: findVarReferences(n->node2, name)
		?: findVarReferences(n->node3, name)
		?: findVarReferences(n->node4, name);
}

int earlyAnalyze(querySpecs &q){
	analyzer an(q);
	if (int nonquery = an.addAlias(q.tree); nonquery)
		return nonquery;
	an.setAttributes(q.tree);
	an.organizeVars(findFirstNode(q.tree, N_SELECTIONS));
	return 0;
}
void midAnalyze(querySpecs &q){
	analyzer an(q);
	an.selectAllGroupOrSubquery();
}
void lateAnalyze(querySpecs &q){
	analyzer an(q);
	an.varUsedInFilter(q.tree);
	an.recordResultColumns(q.tree);
	if (q.grouping){
		an.setVarPhase(q.tree, 1, 0);
		an.findMidrowTargets(q.tree);
		an.setNodePhase(q.tree, 1);
	}
	if (q.joining){
		an.findJoinAndChains(q.tree->nfrom()->njoins(), 1);
		an.findIndexableJoinValues(q.tree->nfrom()->njoins(), 0);
	}
}
