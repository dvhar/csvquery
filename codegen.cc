#include "interpretor.h"
#include "vmachine.h"

class cgen {

	int normal_read = 0;
	int agg_phase = 0; //0 is not grouping, 1 is first read, 2 is aggregate retrieval
	int select_count = 0;
	int joinFileIdx = 0;
	int prevJoinRead = 0;
	int wherenot = 0;
	bool headerdone = false;
	vector<int> valposTypes;
	vector<opcode>& v;
	jumpPositions jumps;
	varScoper vs;
	querySpecs* q;

	public:
	void addop(int code);
	void addop(int code, int p1);
	void addop(int code, int p1, int p2);
	void addop(int code, int p1, int p2, int p3);
	void generateCode();
	void genScanAndChain(astnode &n, int fileno);
	void genAndChainSet(astnode &n);
	void genSortAnds(astnode &n);
	void genJoinPredicates(astnode &n);
	void genJoinCompare(astnode &n);
	void genJoinSets(astnode &n);
	void genTraverseJoins(astnode &n);
	void genScanJoinFiles(astnode &n);
	void genScannedJoinExprs(astnode &n, int fileno);
	void genNormalOrderedQuery(astnode &n);
	void genNormalQuery(astnode &n);
	void genGroupingQuery(astnode &n);
	void genJoiningQuery(astnode &n);
	void genAggSortList(astnode &n);
	void genVars(astnode &n);
	void genWhere(astnode &n);
	void genDistinct(astnode &n, int gotoIfNot);
	void genGetGroup(astnode &n);
	void genSelect(astnode &n);
	void genExprAll(astnode &n);
	void genExprAdd(astnode &n);
	void genExprMult(astnode &n);
	void genExprNeg(astnode &n);
	void genExprCase(astnode &n);
	void genCPredList(astnode &n, int end);
	void genCWExprList(astnode &n, int end);
	void genNormalSortList(astnode &n);
	void genCPred(astnode &n, int end);
	void genCWExpr(astnode &n, int end);
	void genPredicates(astnode &n);
	void genPredCompare(astnode &n);
	void genValue(astnode &n);
	void genFunction(astnode &n);
	void genPrint();
	void genHeader();
	void genSelectAll();
	void genSelections(astnode &n);
	void genTypeConv(astnode &n);
	void genIterateGroups(astnode &n);
	void genUnsortedGroupRow(astnode &n, int nextgroup, int doneGroups);
	void genSortedGroupRow(astnode &n, int nextgroup);
	void genEndrun();
	void finish();

	cgen(querySpecs &qs): q{&qs}, v{qs.bytecode} {}
};
void cgen::addop(int code){ addop(code,0,0,0); }
void cgen::addop(int code, int p1){ addop(code,p1,0,0); }
void cgen::addop(int code, int p1, int p2){ addop(code,p1,p2,0); }
void cgen::addop(int code, int p1, int p2, int p3){
	debugAddop
	v.push_back({code, p1, p2, p3});
}
static const int funcTypes[]  = { 0,0,1,0,0,2 };

//for debugging
static int ident = 0;
//#define e //turn off debug printer
#ifndef e
#define e(A) { \
	string spc; \
	for (int i=0; i< ident; i++) spc += "    "; \
	perr(st(spc,  A )); \
	ident++; } \
	shared_ptr<void> _(nullptr, [&n](...){ \
		ident--; \
		string spc; \
		for (int i=0; i< ident; i++) spc += "    "; \
	perr(st(spc,"done ",A)); });
#endif

#define pushvars() for (auto &i : q->vars) addop(PUSH);
#define popvars() for (auto &i : q->vars) addop(POP);


void jumpPositions::updateBytecode(vector<opcode> &vec) {
	for (auto &v : vec)
		if (opDoesJump(v.code) && v.p1 < 0)
			v.p1 = jumps[v.p1];
};


void cgen::generateCode(){

	if (q->joining)
		genJoiningQuery(q->tree);
	else if (q->grouping)
		genGroupingQuery(q->tree);
	else if (q->sorting)
		genNormalOrderedQuery(q->tree);
	else
		genNormalQuery(q->tree);

	jumps.updateBytecode(v);
	finish();
}

void cgen::finish(){
	int i = 0;
	for (auto c : v){
		perr(st("ip: ",left,setw(4),i++,c.print()));
	}
}

void codeGen(querySpecs &q){
	cgen cg(q);
	cg.generateCode();
}

//generate bytecode for expression nodes
void cgen::genExprAll(astnode &n){
	if (n == nullptr) return;
	switch (n->label){
	case N_EXPRADD:         genExprAdd     (n); break;
	case N_EXPRNEG:         genExprNeg     (n); break;
	case N_EXPRMULT:        genExprMult    (n); break;
	case N_EXPRCASE:        genExprCase    (n); break;
	case N_PREDICATES:      genPredicates  (n); break;
	case N_PREDCOMP:        genPredCompare (n); break;
	case N_VALUE:           genValue       (n); break;
	case N_FUNCTION:        genFunction    (n); break;
	case N_TYPECONV:        genTypeConv    (n); break;
	}
}

void cgen::genEndrun(){
	addop(ENDRUN);
}

//given q.tree as node param
void cgen::genJoiningQuery(astnode &n){
	e("basic join");
	pushvars();
	joinFileIdx = 0;
	if (q->grouping)
		agg_phase = 1;
	genScanJoinFiles(n->nfrom()->njoins());
	genHeader();
	joinFileIdx = 0;
	genTraverseJoins(n->nfrom());
	if (q->grouping){ // includes group sorting
		agg_phase = 2;
		genIterateGroups(n->nafterfrom()->ngroups());
	} else if (q->sorting){
		int reread = jumps.newPlaceholder();
		int endreread = jumps.newPlaceholder();
		addop(SORT);
		addop(PREP_REREAD);
		jumps.setPlace(reread, v.size());
		addop(RDLINE_ORDERED, endreread);
		vs.setscope(DISTINCT_FILTER, V_READ2_SCOPE);
		genVars(n->npreselect());
		genDistinct(n->nselect(), reread);
		vs.setscope(SELECT_FILTER, V_READ2_SCOPE);
		genVars(n->npreselect());
		genSelect(n->nselect());
		genPrint();
		addop((q->quantityLimit > 0 ? JMPCNT : JMP), reread);
		jumps.setPlace(endreread, v.size());
		addop(POP); //rereader used 2 stack spaces
		addop(POP);
	}
	popvars();
	genEndrun();
}
//given 'from' node
void cgen::genTraverseJoins(astnode &n){
	if (n == nullptr) return;
	//start with base file
	addop(START_MESSAGE, messager::readingfirst);
	int endfile1 = jumps.newPlaceholder();
	normal_read = v.size();
	prevJoinRead = normal_read;
	addop(RDLINE, endfile1, 0);
	genJoinSets(n->njoins());
	jumps.setPlace(endfile1, v.size());
	addop(STOP_MESSAGE);
}
//given 'join' node
void cgen::genJoinSets(astnode &n){
	if (n == nullptr) {
		if (q->grouping){ // includes group sorting
			genWhere(q->tree->nafterfrom());
			genGetGroup(q->tree->nafterfrom()->ngroups());
			vs.setscope(SELECT_FILTER|ORDER_FILTER|HAVING_FILTER, V_READ1_SCOPE);
			genVars(q->tree->npreselect());
			genSelect(q->tree->nselect());
			genAggSortList(q->tree->nafterfrom());
			genPredicates(q->tree->nafterfrom()->nhaving()); //having phase 1
			addop(JMP, prevJoinRead);
		} else if (q->sorting){
			genWhere(q->tree->nafterfrom());
			genNormalSortList(q->tree->nafterfrom());
			addop(SAVEPOS);
			addop(JMP, prevJoinRead);
		} else {
			genWhere(q->tree->nafterfrom());
			vs.setscope(DISTINCT_FILTER, V_READ1_SCOPE);
			genVars(q->tree->npreselect());
			genDistinct(q->tree->nselect(), wherenot);
			vs.setscope(SELECT_FILTER, V_READ1_SCOPE);
			genVars(q->tree->npreselect());
			genSelect(q->tree->nselect());
			genPrint();
			addop((q->quantityLimit > 0 ? JMPCNT : JMP), prevJoinRead);
		}
		return;
	}
	// could genwhere per file
	joinFileIdx++;
	vs.setscope(JCOMP_FILTER, V_READ1_SCOPE);
	genVars(q->tree->npreselect());
	genJoinPredicates(n->njoinconds());
	addop(JOINSET_INIT, (joinFileIdx-1)*2, n->tok3.lower() == "left");
	int goWhenDone = prevJoinRead;
	prevJoinRead = v.size();
	wherenot = prevJoinRead;
	addop(JOINSET_TRAV, goWhenDone, (joinFileIdx-1)*2, joinFileIdx);
	genJoinSets(n->nnextjoin());
}
void cgen::genHeader(){
	if (headerdone)
		return;
	headerdone = true;
	if (!globalSettings.termbox && q->outputcsv && q->outputcsvheader)
		addop(PRINTCSV_HEADER);
}
void cgen::genPrint(){
	if (q->outputjson)
		addop(PRINTJSON, q->outputcsv ? 0 : 1);
	if (q->outputcsv){
		if (globalSettings.termbox)
			addop(PRINTBOX);
		else 
			addop(PRINTCSV);
	}
	if (q->isSubquery == SQ_INLIST){
		q->thisSq->btreeIdx = addBtree(q->thisSq->singleDatatype, q);
		addop(PRINTBTREE, q->thisSq->btreeIdx);
	}
}
void cgen::genAndChainSet(astnode &n){
	int cz = n->info[CHAINSIZE];
	int ci = n->info[CHAINIDX];
	int fi = n->info[FILENO];
	auto& chain = q->getFileReader(fi)->andchains[ci];
	auto nn = n.get();
	for (int i=0; i<cz; ++i){
		auto& prednode = nn->npredcomp();
		if (prednode->relop() == KW_LIKE){
			addop(LDLIT, q->dataholder.size());
			q->dataholder.push_back(prepareLike(prednode));
		} else if (prednode->info[TOSCAN] == 1){
			genExprAll(prednode->npredexp2());
		}else if (prednode->info[TOSCAN] == 2){
			genExprAll(prednode->npredexp1());
		}
		chain.functionTypes.push_back(funcTypes[prednode->datatype]);
		chain.relops.push_back(vmachine::relopIdx[prednode->relop()]);
		chain.negations.push_back(prednode->negated());
		nn = nn->nnextpreds().get();
	}
	chain.relops[0] = 4; // 4 is index of eq, instruction already konws real first relop
	int orEquals = 0;
	switch (n->npredcomp()->relop()){
		case SP_EQ:
			addop(JOINSET_EQ_AND, fi, ci);
			break;
		case SP_LESSEQ:
			orEquals = 1;
		case SP_LESS:
			addop(PUSH_N, orEquals);
			addop(JOINSET_LESS_AND, fi, ci);
			break;
		case SP_GREATEQ:
			orEquals = 1;
		case SP_GREAT:
			addop(PUSH_N, orEquals);
			addop(JOINSET_GRT_AND, fi, ci);
			break;
		default:
			error("joins with '",n->tok1.val,"' operator in first of 'and' conditions not implemented");
	}
}
//given 'predicates' node
void cgen::genJoinPredicates(astnode &n){
	if (n == nullptr) return;
	if (n->info[ANDCHAIN]){
		genAndChainSet(n);
		return;
	}
	genJoinPredicates(n->nnextpreds());
	genJoinCompare(n->npredcomp());
	switch (n->logop()){
	case KW_AND:
		addop(AND_SET);
		break;
	case KW_OR:
		addop(OR_SET);
		break;
	}
}
//given predicate comparison node
void cgen::genJoinCompare(astnode &n){
	if (n == nullptr) return;
	if (n->relop() == SP_LPAREN){
		genJoinPredicates(n->nmorepreds());
		return;
	}
	//evaluate the one not scanned
	if (n->info[TOSCAN] == 1){
		genExprAll(n->npredexp2());
	} else if (n->info[TOSCAN] == 2){
		genExprAll(n->npredexp1());
	}
	if (n->datatype == T_STRING)
		addop(NUL_TO_STR);
	int orEquals = 0, vpidx = n->info[VALPOSIDX];
	switch (n->relop()){
		case SP_EQ:
			addop(JOINSET_EQ, joinFileIdx, vpidx, funcTypes[valposTypes[vpidx]]);
			break;
		case SP_LESSEQ:
			orEquals = 1;
		case SP_LESS:
			addop(PUSH_N, orEquals);
			addop(JOINSET_LESS, joinFileIdx, vpidx, funcTypes[valposTypes[vpidx]]);
			break;
		case SP_GREATEQ:
			orEquals = 1;
		case SP_GREAT:
			addop(PUSH_N, orEquals);
			addop(JOINSET_GRT, joinFileIdx, vpidx, funcTypes[valposTypes[vpidx]]);
			break;
		default:
			error("joins with '",n->tok1.val,"' operator not implemented");
	}
}
void cgen::genScanJoinFiles(astnode &n){
	e("scan joins");
	auto& joinNode = findFirstNode(n, N_JOIN);
	for (auto jnode = joinNode.get(); jnode; jnode = jnode->nnextjoin().get()){
		auto& f = q->filemap[jnode->nfile()->filealias()];
		int afterfile = jumps.newPlaceholder();
		addop(START_MESSAGE, messager::scanningjoin);
		normal_read = v.size();
		addop(RDLINE, afterfile, f->fileno);
		joinFileIdx++;
		f->vpTypes = move(valposTypes);
		valposTypes.clear();
		vs.setscope(JSCAN_FILTER, V_SCAN_SCOPE);
		genVars(q->tree->npreselect());
		genScannedJoinExprs(jnode->njoinconds(), f->fileno);
		if (valposTypes.size())
			addop(SAVEVALPOS, f->fileno, f->joinValpos.size());
		addop(JMP, normal_read);
		jumps.setPlace(afterfile, v.size());
		addop(START_MESSAGE, messager::indexing);
		genSortAnds(joinNode->njoinconds());
		for (u32 i=0; i<valposTypes.size(); i++)
			addop(SORTVALPOS, f->fileno, i, funcTypes[valposTypes[i]]);
	}

}
void cgen::genSortAnds(astnode &n){
	if (n == nullptr) return;
	e("sort ands");
	if (n->info[ANDCHAIN] == 1){
		addop(SORT_ANDCHAIN, n->info[FILENO],  n->info[CHAINIDX]);
		return;
	} else {
		genSortAnds(n->nnextpreds());
		if (n->npredcomp()->relop() == SP_LPAREN)
			genSortAnds(n->npredcomp()->nmorepreds());
	}
}
void cgen::genScanAndChain(astnode &n, int fileno){
	if (n == nullptr || n->info[ANDCHAIN] == 0) return;
	e("join ands");

	auto nn = n.get();
	for (int i=0; i<n->info[CHAINSIZE]; ++i){
		auto& prednode = nn->npredcomp();
		if (prednode->info[TOSCAN] == 1){
			genExprAll(prednode->npredexp1());
		}else if (prednode->info[TOSCAN] == 2){
			genExprAll(prednode->npredexp2());
		}
		nn = nn->nnextpreds().get();
	}
	addop(SAVEANDCHAIN, n->info[CHAINIDX], fileno);
}

void cgen::genScannedJoinExprs(astnode &n, int fileno){
	if (n == nullptr) return;
	e("join exprs");
	bool gotExpr = false;
	switch (n->label){
		case N_PREDCOMP:
			if (n->info[ANDCHAIN]) //this is just for valpos joins
				return;
			if (n->relop() == SP_LPAREN)
				genScannedJoinExprs(n->nmorepreds(), fileno);
			else if (n->info[TOSCAN] == 1){
				genExprAll(n->npredexp1());
				gotExpr = true;
			}else if (n->info[TOSCAN] == 2){
				genExprAll(n->npredexp2());
				gotExpr = true;
			}else{
				error("invalid join comparision");
			}
			if (gotExpr){
				valposTypes.push_back(n->datatype);
				if (n->datatype == T_STRING)
					addop(NUL_TO_STR);
			}
			break;
		case N_PREDICATES:
			if (n->info[ANDCHAIN]){ //handle andchains separately
				genScanAndChain(n, fileno);
				return;
			}
		default:
			genScannedJoinExprs(n->node1, fileno);
			genScannedJoinExprs(n->node2, fileno);
			genScannedJoinExprs(n->node3, fileno);
			genScannedJoinExprs(n->node4, fileno);
			break;
	}
}

void cgen::genNormalQuery(astnode &n){
	e("normal");
	int message = (q->whereFiltering || q->distinctFiltering) ?
		messager::readingfiltered : messager::reading;
	int endfile = jumps.newPlaceholder(); //where to jump when done reading file
	pushvars();
	genHeader();
	addop(START_MESSAGE, message);
	normal_read = v.size();
	wherenot = normal_read;
	addop(RDLINE, endfile, 0);
	genWhere(n->nafterfrom());
	vs.setscope(DISTINCT_FILTER, V_READ1_SCOPE);
	genVars(n->npreselect());
	genDistinct(n->nselect(), normal_read);
	vs.setscope(SELECT_FILTER, V_READ1_SCOPE);
	genVars(n->npreselect());
	genSelect(n->nselect());
	genPrint();
	addop((q->quantityLimit > 0 ? JMPCNT : JMP), normal_read);
	jumps.setPlace(endfile, v.size());
	addop(STOP_MESSAGE);
	popvars();
	genEndrun();
}
void cgen::genNormalOrderedQuery(astnode &n){
	int sorter = jumps.newPlaceholder(); //where to jump when done scanning file
	int reread = jumps.newPlaceholder();
	int endreread = jumps.newPlaceholder();
	pushvars();
	addop(START_MESSAGE, messager::scanning);
	normal_read = v.size();
	wherenot = normal_read;
	addop(RDLINE, sorter, 0);
	genWhere(n->nafterfrom());
	genNormalSortList(n->nafterfrom());
	addop(SAVEPOS);
	addop(JMP, normal_read);
	jumps.setPlace(sorter, v.size());
	addop(START_MESSAGE, messager::sorting);
	addop(SORT);
	addop(PREP_REREAD);
	addop(START_MESSAGE, messager::retrieving);
	genHeader();
	jumps.setPlace(reread, v.size());
	addop(RDLINE_ORDERED, endreread);
	vs.setscope(DISTINCT_FILTER, V_READ2_SCOPE);
	genVars(n->npreselect());
	genDistinct(n->nselect(), reread);
	vs.setscope(SELECT_FILTER, V_READ2_SCOPE);
	genVars(n->npreselect());
	genSelect(n->nselect());
	genPrint();
	addop((q->quantityLimit > 0 ? JMPCNT : JMP), reread);
	jumps.setPlace(endreread, v.size());
	addop(STOP_MESSAGE);
	addop(POP); //rereader used 2 stack spaces
	addop(POP);
	popvars();
	genEndrun();
};

//given afterfrom node
void cgen::genNormalSortList(astnode &n){
	e("normal sort list");
	if (n == nullptr) return;
	vs.setscope(ORDER_FILTER, V_READ1_SCOPE);
	genVars(q->tree->npreselect());
	auto& ordnode = findFirstNode(n, N_ORDER);
	int i = 0;
	for (auto x = ordnode->norderlist().get(); x; x = x->nnextlist().get()){
		genExprAll(x->nsubexpr());
		if (x->datatype == T_STRING)
			addop(NUL_TO_STR);
		addop(operations[OPSVSRT][x->datatype], i++);
		q->sortInfo.push_back({x->orderasc(), x->datatype});
	}
}
void cgen::genGroupingQuery(astnode &n){
	e("grouping");
	agg_phase = 1;
	int getgroups = jumps.newPlaceholder();
	pushvars();
	addop(START_MESSAGE, messager::scanning);
	genHeader();
	normal_read = v.size();
	wherenot = normal_read;
	addop(RDLINE, getgroups, 0);
	genWhere(n->nafterfrom());
	genGetGroup(n->nafterfrom()->ngroups());
	vs.setscope(SELECT_FILTER|HAVING_FILTER|ORDER_FILTER, V_READ1_SCOPE);
	genVars(n->npreselect());
	genPredicates(n->nafterfrom()->nhaving()); //having phase 1
	genSelect(n->nselect());
	genAggSortList(n);
	addop(JMP, normal_read);
	jumps.setPlace(getgroups, v.size());
	agg_phase = 2;
	addop(STOP_MESSAGE);
	genIterateGroups(n->nafterfrom()->ngroups());
	popvars();
	genEndrun();
}

void cgen::genAggSortList(astnode &n){
	if (n == nullptr) return;
	e("agg sort list");
	switch (n->label){
	case N_QUERY:     genAggSortList(n->nafterfrom()); break;
	case N_AFTERFROM: genAggSortList(n->norder()); break;
	case N_ORDER:     genAggSortList(n->norderlist()); break;
	case N_EXPRESSIONS: // sort list
		genExprAdd(n->nsubexpr());
		genAggSortList(n->nnextlist());
	}
}

void cgen::genVars(astnode &n){
	if (n == nullptr) return;
	e("gen vars");
	switch (n->label){
	case N_PRESELECT: //currently only has 'with' branch
	case N_WITH:
		genVars(n->node1);
		break;
	case N_VARS:
		{
			int i = q->getVarIdx(n->varname());
			auto& var = q->vars[i];
			if (vs.neededHere(i, var.filter, var.maxfileno)){
				genExprAll(n->nsubexpr());
				if (n->phase == (1|2)){
					//non-aggs in phase2
					if (agg_phase == 1){
						addop2(PUTVAR2, i, n->varmididx());
					} else {
						addop1(LDMID, n->varmididx());
						addop1(PUTVAR, i);
					}
				} else {
					addop1(PUTVAR, i);
					int vartype = q->getVarType(n->varname());
					if (agg_phase == 2 && q->sorting && vartype == T_STRING)
						addop1(HOLDVAR, i);
				}
			}
			genVars(n->nnextvar());
		}
		break;
	}
}

void cgen::genExprAdd(astnode &n){
	if (n == nullptr) return;
	e("gen add");
	genExprAll(n->node1);
	if (!n->mathop()) return;
	genExprAll(n->node2);
	switch (n->mathop()){
	case SP_PLUS:
		addop0(operations[OPADD][n->datatype]);
		break;
	case SP_MINUS:
		addop0(operations[OPSUB][n->datatype]);
		break;
	}
}

void cgen::genExprMult(astnode &n){
	if (n == nullptr) return;
	e("gen mult");
	genExprAll(n->node1);
	if (!n->mathop()) return;
	genExprAll(n->node2);
	switch (n->mathop()){
	case SP_STAR:
		addop0(operations[OPMULT][n->datatype]);
		break;
	case SP_DIV:
		addop0(operations[OPDIV][n->datatype]);
		break;
	case SP_CARROT:
		addop0(operations[OPPOW][n->datatype]);
		break;
	case SP_MOD:
		addop0(operations[OPMOD][n->datatype]);
		break;
	}
}

void cgen::genExprNeg(astnode &n){
	if (n == nullptr) return;
	e("gen neg");
	genExprAll(n->node1);
	if (!n->mathop()) return;
	addop0(operations[OPNEG][n->datatype]);
}

void cgen::genValue(astnode &n){
	if (n == nullptr) return;
	e("gen value: "+n->tok1.val);
	dat lit;
	int vtype, op;
	switch (n->valtype()){
	case COLUMN:
		addop2(operations[OPLD][n->datatype], q->getFileNo(n->dotsrc()), n->colidx());
		break;
	case LITERAL:
		if (n->tok1.lower() == "null"){
			addop0(PUSH);
		} else {
			switch (n->datatype){
			case T_INT:      lit = parseIntDat(n->nval().c_str());      break;
			case T_FLOAT:    lit = parseFloatDat(n->nval().c_str());    break;
			case T_DATE:     lit = parseDateDat(n->nval().c_str());     break;
			case T_DURATION: lit = parseDurationDat(n->nval().c_str()); break;
			case T_STRING:   lit = parseStringDat(n->nval().c_str());   break;
			}
			addop1(LDLIT, q->dataholder.size());
			q->dataholder.push_back(lit);
		}
		break;
	case VARIABLE:
		addop1(LDVAR, q->getVarIdx(n->varname()));
		//variable may be used in operations with different types
		vtype = q->getVarType(n->varname());
		op = typeConv[vtype][n->datatype];
		if (op == CVER)
			error("Cannot use alias '",n->nval(),"' of type ",gettypename(vtype)," with incompatible type ",gettypename(n->datatype));
		if (op != CVNO)
			addop0(op);
		break;
	case FUNCTION:
		genExprAll(n->nsubexpr());
		break;
	}
}

void cgen::genExprCase(astnode &n){
	if (n == nullptr) return;
	e("gen case");
	int caseEnd = jumps.newPlaceholder();
	switch (n->casenodetype()){
	case KW_CASE:
		switch (n->casetype()){
		//when predicates are true
		case KW_WHEN:
			genCPredList(n->node1, caseEnd);
			genExprAll(n->node3);
			if (n->node3 == nullptr)
				addop0(PUSH);
			jumps.setPlace(caseEnd, v.size());
			break;
		//expression matches expression list
		case WORD_TK:
		case SP_LPAREN:
			genExprAll(n->node1);
			genCWExprList(n->node2, caseEnd);
			addop0(POP); //don't need comparison value anymore
			genExprAll(n->node3);
			if (n->node3 == nullptr)
				addop0(PUSH);
			jumps.setPlace(caseEnd, v.size());
			break;
		}
		break;
	case SP_LPAREN:
	case WORD_TK:
		genExprAll(n->nsubexpr());
	}
}

void cgen::genCWExprList(astnode &n, int end){
	if (n == nullptr) return;
	e("gen case w list");
	genCWExpr(n->node1, end);
	genCWExprList(n->node2, end);
}

void cgen::genCWExpr(astnode &n, int end){
	if (n == nullptr) return;
	e("gen case w expr");
	int nextCase = jumps.newPlaceholder(); //get jump pos for next try
	genExprAll(n->node1); //evaluate comparision expression
	addop1(operations[OPEQ][n->tok1.id], 0); //leave '=' result where this comp value was
	addop2(JMPFALSE, nextCase, 1);
	addop0(POP); //don't need comparison value anymore
	genExprAll(n->ncaseresultexpr()); //result value if eq
	addop1(JMP, end);
	jumps.setPlace(nextCase, v.size()); //jump here for next try
}

void cgen::genCPredList(astnode &n, int end){
	if (n == nullptr) return;
	e("gen case p list");
	genCPred(n->node1, end);
	genCPredList(n->node2, end);
}

void cgen::genCPred(astnode &n, int end){
	if (n == nullptr) return;
	e("gen case p");
	int nextCase = jumps.newPlaceholder(); //get jump pos for next try
	genPredicates(n->node1);
	addop2(JMPFALSE, nextCase, 1);
	genExprAll(n->node2); //result value if true
	addop1(JMP, end);
	jumps.setPlace(nextCase, v.size()); //jump here for next try
}

//given select node
void cgen::genSelect(astnode &n){
	if (n == nullptr) {
		//no selection branch
		genSelectAll();
		return;
	}
	genSelections(n->nselections());
}

//given selections node
void cgen::genSelections(astnode &n){
	if (n == nullptr) {
		//reached end of selections section of query
		if (!select_count) genSelectAll();
		return;
	}
	e("gen selections");
	string t1 = n->diststartok().lower();
	switch (n->label){
	case N_SELECTIONS:
		if (t1 == "hidden") {

		} else if (t1 == "distinct") {
			if (agg_phase == 1){
				genExprAll(n->nsubexpr());
			} else {
				addop1(PUTDIST, n->selectiondistnum());
			}
			incSelectCount();

		} else if (t1 == "*") {
			genSelectAll();

		} else if (isTrivialColumn(n)) {
			switch (agg_phase){
			case 0:
				for (auto nn = n.get(); nn; nn = nn->node1.get()) if (nn->label == N_VALUE){
					addop3(LDPUT, n->selectiondestidx(), nn->colidx(), q->getFileNo(nn->dotsrc()));
					break;
				} break;
			case 1:
				for (auto nn = n.get(); nn; nn = nn->node1.get()) if (nn->label == N_VALUE){
					addop3(LDPUTGRP, n->selectionmididx(), nn->colidx(), q->getFileNo(nn->dotsrc()));
					break;
				} break;
			case 2:
				addop2(LDPUTMID, n->selectiondestidx(), n->selectionmididx());
				break;
			}
			incSelectCount();

		} else if (agg_phase == 2 && n->info[LPMID]) {
			addop2(LDPUTMID, n->selectiondestidx(), n->selectionmididx());

		} else {
			genExprAll(n->node1);
			addop2(PUT, n->tok4.id, agg_phase==1?1:0);
			incSelectCount();
		}
		break;
	default:
		error("selections generator error");
		return;
	}
	genSelections(n->node2);
}

void cgen::genPredicates(astnode &n){
	if (n == nullptr) return;
	e("gen preds");
	genPredCompare(n->npredcomp());
	int doneAndOr = jumps.newPlaceholder();
	int xor1true;
	switch (n->logop()){
	case KW_AND:
		addop2(JMPFALSE, doneAndOr, 0);
		addop0(POP); //don't need old result
		genPredicates(n->nnextpreds());
		break;
	case KW_OR:
		addop2(JMPTRUE, doneAndOr, 0);
		addop0(POP); //don't need old result
		genPredicates(n->nnextpreds());
		break;
	case KW_XOR:
		xor1true = jumps.newPlaceholder();
		genPredicates(n->nnextpreds());
		addop2(JMPTRUE, xor1true, 0);
		addop0(POP);
		addop1(JMP, doneAndOr);
		jumps.setPlace(xor1true, v.size());
		addop0(POP);
		addop0(PNEG);
		break;
	}
	jumps.setPlace(doneAndOr, v.size());
	if (n->tok2.id == SP_NEGATE)
		addop0(PNEG);
}

void cgen::genPredCompare(astnode &n){
	if (n == nullptr) return;
	e("gen pred compare");
	int negation = n->negated();
	int endcomp, greaterThanExpr3, subq=0;
	genExprAll(n->npredexp1());
	switch (n->relop()){
	case SP_NOEQ: negation ^= 1;
	case SP_EQ:
		genExprAll(n->npredexp2());
		addop2(operations[OPEQ][n->datatype], 1, negation);
		break;
	case SP_GREATEQ: negation ^= 1;
	case SP_LESS:
		genExprAll(n->npredexp2());
		addop2(operations[OPLT][n->datatype], 1, negation);
		break;
	case SP_GREAT: negation ^= 1;
	case SP_LESSEQ:
		genExprAll(n->npredexp2());
		addop2(operations[OPLEQ][n->datatype], 1, negation);
		break;
	case KW_BETWEEN:
		endcomp = jumps.newPlaceholder();
		greaterThanExpr3 = jumps.newPlaceholder();
		addop2(NULFALSE, endcomp, 0);
		genExprAll(n->node2);
		addop2(NULFALSE, endcomp, 1);
		genExprAll(n->node3);
		addop2(NULFALSE, endcomp, 2);
		addop2(BETWEEN, funcTypes[n->datatype], negation);
		jumps.setPlace(endcomp, v.size());
		break;
	case KW_IN:
		if (n->nsetlist()->hassubquery()){
			addop(INSUBQUERY, n->nsetlist()->subqidx());
			subq = 1;
		} else {
			endcomp = jumps.newPlaceholder();
			for (auto nn=n->nsetlist()->node1.get(); nn; nn=nn->nnextlist().get()){
				genExprAll(nn->nsubexpr());
				addop1(operations[OPEQ][n->npredexp1()->datatype], 0);
				addop2(JMPTRUE, endcomp, 0);
				if (nn->nnextlist().get())
					addop0(POP);
			}
			jumps.setPlace(endcomp, v.size());
		}
		if (negation)
			addop0(PNEG);
		if (!subq)
			addop0(POPCPY); //put result where 1st expr was
		break;
	case KW_LIKE:
		addop2(LIKE, q->dataholder.size(), negation);
		q->dataholder.push_back(prepareLike(n));
		break;
	}
}

void cgen::genSelectAll(){
	addop(LDPUTALL, select_count);
	for (auto& f : q->filevec)
		select_count += f->numFields;
}

//given afterfrom node
void cgen::genWhere(astnode &nn){
	auto& n = findFirstNode(nn, N_WHERE);
	e("gen where");
	if (n == nullptr) return;
	vs.setscope(WHERE_FILTER, V_READ1_SCOPE);
	genVars(q->tree->npreselect());
	genPredicates(n->nwhere());
	addop2(JMPFALSE, wherenot, 1);
}

//given select or selections node
void cgen::genDistinct(astnode &n, int gotoIfNot){
	if (n == nullptr) return;
	e("gen distinct");
	if (n->label == N_SELECT){
		genDistinct(n->nselections(), gotoIfNot);
		return;
	} else if (n->label != N_SELECTIONS)
		return;
	if (n->diststartok().id == KW_DISTINCT){
		genExprAll(n->nsubexpr());
		addop2(DIST, gotoIfNot, addBtree(n->datatype, q));
	} else {
		//there can be only 1 distinct filter
		genDistinct(n->nnextselection(), gotoIfNot);
	}
}

void cgen::genFunction(astnode &n){
	if (n == nullptr) return;
	e("gen function");
	int funcDone = jumps.newPlaceholder();
	int idx;

	//stuff common to all aggregate functions
	if ((n->funcid() & AGG_BIT) != 0 ) {
		genExprAll(n->nsubexpr());
		if (n->tok3.val == "distinct" && agg_phase == 1){
			int setIndex = n->funcdistnum();
			int separateSets = 1;
			if (q->grouping == 1){ //when onegroup, btree not indexed by rowgroup
				setIndex = addBtree(n->nsubexpr()->datatype, q);
				separateSets = 0;
			}
			addop(DIST, funcDone, setIndex, separateSets);
			addop(LDDIST);
		}
	}

	switch (n->tok1.id){
	//non-aggregates
	case FN_COALESCE:
		for (auto nn = n->node1.get(); nn; nn = nn->nnextlist().get()){
			genExprAll(nn->nsubexpr());
			addop1(JMPNOTNULL_ELSEPOP, funcDone);
		}
		addop0(PUSH);
		break;
	case FN_ABS:
		genExprAll(n->nsubexpr());
		addop0(operations[OPABS][n->datatype]);
		break;
	case FN_INC:
		addop1(FINC, q->dataholder.size());
		q->dataholder.push_back(dat{ {.f = 0.0}, T_FLOAT});
		break;
	case FN_ENCRYPT:
		genExprAll(n->nsubexpr());
		//TODO: if add block cipher, check here
		idx = q->crypt.newChacha(n->password());
		addop1(ENCCHA, idx);
		break;
	case FN_DECRYPT:
		genExprAll(n->nsubexpr());
		//TODO: if add block cipher, check here
		idx = q->crypt.newChacha(n->password());
		addop1(DECCHA, idx);
		break;
	case FN_SUBSTR:
		genExprAll(n->nsubexpr());
		if (!n->tok2.quoted) {
			auto n1 = parseIntDat(n->tok2.val.c_str());
			auto n2 = parseIntDat(n->tok3.val.c_str());
			addop3(FUNC_SUBSTR, n1.u.i, n2.u.i, 0);
		} else {
			q->dataholder.push_back(dat{{ .r = new regex_t}, RMAL});
			if (regcomp(q->dataholder.back().u.r, n->tok2.val.c_str(), REG_EXTENDED))
				error("Could not parse 'substr' pattern");
			addop3(FUNC_SUBSTR, q->dataholder.size()-1, 0, 1);
		}
		break;
	case FN_STRING:
	case FN_INT:
	case FN_FLOAT:
	case FN_DATE:
	case FN_DUR:
		genExprAll(n->nsubexpr()); //has conv node from datatyper
		break;
	case FN_POW:
		genExprAll(n->node1);
		genExprAll(n->node2);
		addop0(operations[OPPOW][n->datatype]);
		break;
	case FN_FORMAT:
		genExprAll(n->nsubexpr());
		addop1(functionCode[n->funcid()], q->dataholder.size());
		{ auto fmt = dateFormatCode(n->tok2.val);
		q->dataholder.push_back(dat{ { .s = strdup(fmt) }, T_STRING|MAL, (u32)strlen(fmt) }); }
		break;
	case FN_YEAR:
	case FN_MONTH:
	case FN_MONTHNAME:
	case FN_WEEK:
	case FN_WDAYNAME:
	case FN_YDAY:
	case FN_MDAY:
	case FN_WDAY:
	case FN_HOUR:
	case FN_MINUTE:
	case FN_SECOND:
	case FN_CEIL:
	case FN_FLOOR:
	case FN_ACOS:
	case FN_ASIN:
	case FN_ATAN:
	case FN_COS:
	case FN_SIN:
	case FN_TAN:
	case FN_EXP:
	case FN_LOG:
	case FN_LOG2:
	case FN_LOG10:
	case FN_SQRT:
	case FN_CBRT:
	case FN_RAND:
	case FN_UPPER:
	case FN_LOWER:
	case FN_BASE64_ENCODE:
	case FN_BASE64_DECODE:
	case FN_HEX_ENCODE:
	case FN_HEX_DECODE:
	case FN_MD5:
	case FN_SHA1:
	case FN_SHA256:
	case FN_ROUND:
	case FN_NOW:
	case FN_NOWGM:
		genExprAll(n->nsubexpr());
		//p1 is rounding places, p2 is use nan vs null
		addop2(functionCode[n->funcid()], n->tok2.id, ((q->options & O_NAN) == 0));
		break;
	case FN_LEN:
	case FN_SIP:
		genExprAll(n->nsubexpr());
		if (n->node1->datatype != T_STRING)
			addop0(typeConv[n->node1->datatype][T_STRING]);
		addop0(functionCode[n->funcid()]);
		break;
	}

	//aggregates
	if (agg_phase == 1) {
		switch (n->funcid()){
		case FN_SUM:
			addop(operations[OPSUM][n->datatype], n->info[MIDIDX]);
			break;
		case FN_AVG:
			addop(operations[OPAVG][n->datatype], n->info[MIDIDX]);
			break;
		case FN_STDEV:
		case FN_STDEVP:
			addop(operations[OPSTV][n->datatype], n->info[MIDIDX]);
			break;
		case FN_MIN:
			addop(operations[OPMIN][n->datatype], n->info[MIDIDX]);
			break;
		case FN_MAX:
			addop(operations[OPMAX][n->datatype], n->info[MIDIDX]);
			break;
		case FN_COUNT:
			addop(COUNT, n->info[MIDIDX], n->tok2.id ? 1 : 0);
			break;
		}
		select_count++;
	} else if (agg_phase == 2) {
		switch (n->funcid()){
		case FN_AVG:
			addop(operations[OPLAVG][n->datatype], n->info[MIDIDX]);
			break;
		case FN_STDEV:
			addop(operations[OPLSTV][n->datatype], n->info[MIDIDX], 1);
			break;
		case FN_STDEVP:
			addop(operations[OPLSTV][n->datatype], n->info[MIDIDX]);
			break;
		case FN_COUNT:
			addop(LDCOUNT, n->info[MIDIDX]);
			break;
		case FN_MIN:
		case FN_MAX:
		case FN_SUM:
			addop(LDMID, n->info[MIDIDX]);
			break;
		}
	}
	jumps.setPlace(funcDone, v.size());
}

void cgen::genTypeConv(astnode &n){
	if (n == nullptr) return;
	e("type convert");
	genExprAll(n->nsubexpr());
	auto cnv = typeConv[n->convfromtype()][n->datatype];
	if (cnv == CVER)
		error("Cannot use type ",gettypename(n->convfromtype())," with incompatible type ",gettypename(n->datatype));
	if (cnv != CVNO)
		addop0(cnv);
}

void cgen::genGetGroup(astnode &n){
	vs.setscope(AGG_FILTER, V_READ1_SCOPE);
	genVars(q->tree->npreselect());
	if (n == nullptr) return;
	e("get group");
	if (n->label == N_GROUPBY){
		int depth = -1;
		for (auto nn=n->node1.get(); nn; nn=nn->nnextlist().get()){
			depth++;
			genExprAll(nn->nsubexpr());
		}
		addop1(GETGROUP, depth);
	}
}

void cgen::genIterateGroups(astnode &n){
	e("iterate groups");
	if (n == nullptr) {
		addop(ONEGROUP);
		vs.setscope(SELECT_FILTER, V_GROUP_SCOPE);
		genVars(q->tree->npreselect());
		genSelect(q->tree->nselect());
		// for debugging:
		genPrint();
		return;
	}
	if (n->label == N_GROUPBY){
		int doneGroups = jumps.newPlaceholder();
		int depth = 0;
		int goWhenDone;
		int prevmap;
		addop(ROOTMAP, depth);
		for (auto nn=n->node1.get(); nn; nn=nn->nnextlist().get()){
			if (depth == 0) {
				goWhenDone = doneGroups;
			} else {
				goWhenDone = prevmap;
			}
			if (nn->nnextlist().get()){
				depth += 2;
				prevmap = v.size();
				addop(NEXTMAP, goWhenDone, depth);
			} else {
				int nextvec = v.size();
				addop(NEXTVEC, goWhenDone, depth);
				if (q->sorting){
					genSortedGroupRow(n, nextvec);
				} else {
					genUnsortedGroupRow(n, nextvec, doneGroups);
				}
			}
		}
		jumps.setPlace(doneGroups, v.size());

		if (q->sorting){
			auto& ordnode1 = q->tree->nafterfrom()->norder()->norderlist();
			int doneReadGroups = jumps.newPlaceholder();
			addop(GSORT, ordnode1->aggorderdestidx());
			addop(PUSH_N, 0);
			int readNext = v.size();
			addop(READ_NEXT_GROUP, doneReadGroups);
			genPrint();
			addop((q->quantityLimit > 0 ? JMPCNT : JMP), readNext);
			jumps.setPlace(doneReadGroups, v.size());
			addop(POP);
		}
	}
}

void cgen::genUnsortedGroupRow(astnode &n, int nextgroup, int doneGroups){
	e("gen unsorted group row");
	vs.setscope(HAVING_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->npreselect());
	genPredicates(q->tree->nafterfrom()->nhaving());
	if (q->havingFiltering)
		addop(JMPFALSE, nextgroup, 1);
	vs.setscope(DISTINCT_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->npreselect());
	genDistinct(q->tree->nselect(), nextgroup);
	vs.setscope(SELECT_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->npreselect());
	genSelect(q->tree->nselect());
	genPrint();
	addop((q->quantityLimit > 0 ? JMPCNT : JMP), nextgroup);
	addop(JMP, doneGroups);
}
void cgen::genSortedGroupRow(astnode &n, int nextgroup){
	e("gen sorted group row");
	vs.setscope(HAVING_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->npreselect());
	genPredicates(q->tree->nafterfrom()->nhaving());
	if (q->havingFiltering)
		addop(JMPFALSE, nextgroup, 1);
	vs.setscope(DISTINCT_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->npreselect());
	genDistinct(q->tree->nselect(), nextgroup);
	addop(ADD_GROUPSORT_ROW);
	vs.setscope(SELECT_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->npreselect());
	genSelect(q->tree->nselect());
	auto& ordnode = q->tree->nafterfrom()->norder();
	for (auto x = ordnode->norderlist().get(); x; x = x->nnextlist().get()){
		genExprAll(x->nsubexpr());
		if (x->datatype == T_STRING)
			addop(NUL_TO_STR);
		addop(PUT, x->aggorderdestidx());
		q->sortInfo.push_back({x->orderasc(), x->datatype});
	}
	addop(FREEMIDROW);
	addop(JMP, nextgroup);
}
