#include "interpretor.h"
#include "vmachine.h"

class cgen {

	int normal_read =0;
	int agg_phase =0; //0 is not grouping, 1 is first read, 2 is aggregate retrieval
	int select_count =0;
	int joinFileIdx =0;
	int prevJoinRead =0;
	int wherenot =0;
	vector<int> valposTypes;
	vector<opcode> v;
	jumpPositions jumps;
	varScoper vs;
	querySpecs* q;

	public:
	void addop(int code);
	void addop(int code, int p1);
	void addop(int code, int p1, int p2);
	void addop(int code, int p1, int p2, int p3);
	void generateCode();
	void genScanAndChain(unique_ptr<node> &n, int fileno);
	void genAndChainSet(unique_ptr<node> &n);
	void genSortAnds(unique_ptr<node> &n);
	void genJoinPredicates(unique_ptr<node> &n);
	void genJoinCompare(unique_ptr<node> &n);
	void genJoinSets(unique_ptr<node> &n);
	void genTraverseJoins(unique_ptr<node> &n);
	void genScanJoinFiles(unique_ptr<node> &n);
	void genScannedJoinExprs(unique_ptr<node> &n, int fileno);
	void genNormalOrderedQuery(unique_ptr<node> &n);
	void genNormalQuery(unique_ptr<node> &n);
	void genGroupingQuery(unique_ptr<node> &n);
	void genJoiningQuery(unique_ptr<node> &n);
	void genAggSortList(unique_ptr<node> &n);
	void genVars(unique_ptr<node> &n);
	void genWhere(unique_ptr<node> &n);
	void genDistinct(unique_ptr<node> &n, int gotoIfNot);
	void genGetGroup(unique_ptr<node> &n);
	void genSelect(unique_ptr<node> &n);
	void genExprAll(unique_ptr<node> &n);
	void genExprAdd(unique_ptr<node> &n);
	void genExprMult(unique_ptr<node> &n);
	void genExprNeg(unique_ptr<node> &n);
	void genExprCase(unique_ptr<node> &n);
	void genCPredList(unique_ptr<node> &n, int end);
	void genCWExprList(unique_ptr<node> &n, int end);
	void genNormalSortList(unique_ptr<node> &n);
	void genCPred(unique_ptr<node> &n, int end);
	void genCWExpr(unique_ptr<node> &n, int end);
	void genPredicates(unique_ptr<node> &n);
	void genPredCompare(unique_ptr<node> &n);
	void genValue(unique_ptr<node> &n);
	void genFunction(unique_ptr<node> &n);
	void genPrint();
	void genSelectAll();
	void genSelections(unique_ptr<node> &n);
	void genTypeConv(unique_ptr<node> &n);
	void genIterateGroups(unique_ptr<node> &n);
	void genUnsortedGroupRow(unique_ptr<node> &n, int nextgroup, int doneGroups);
	void genSortedGroupRow(unique_ptr<node> &n, int nextgroup);
	void finish();

	cgen(querySpecs &qs): q{&qs} {}
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
#define e(A) for (int i=0; i< ident; i++) perr("    "); \
	perr(st( A , "\n")); ident++; \
	shared_ptr<void> _(nullptr, [&n](...){ \
	ident--; for (int i=0; i< ident; i++) perr("    "); \
	perr(st("done ",A,'\n')); });
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
		perr(st("ip: ",left,setw(4),i++));
		c.print();
	}
	q->bytecode = move(v);
}

void codeGen(querySpecs &q){
	cgen cg(q);
	cg.generateCode();
}

//generate bytecode for expression nodes
void cgen::genExprAll(unique_ptr<node> &n){
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


//given q.tree as node param
void cgen::genJoiningQuery(unique_ptr<node> &n){
	e("basic join");
	pushvars();
	joinFileIdx = 0;
	if (q->grouping)
		agg_phase = 1;
	genScanJoinFiles(n->node3->node1);
	joinFileIdx = 0;
	genTraverseJoins(n->node3);
	if (q->grouping){ // includes group sorting
		agg_phase = 2;
		genIterateGroups(n->node4->node2);
	} else if (q->sorting){
		int reread = jumps.newPlaceholder();
		int endreread = jumps.newPlaceholder();
		addop(SORT);
		addop(PREP_REREAD);
		jumps.setPlace(reread, v.size());
		addop(RDLINE_ORDERED, endreread);
		vs.setscope(DISTINCT_FILTER, V_READ2_SCOPE);
		genVars(n->node1);
		genDistinct(n->node2->node1, reread);
		vs.setscope(SELECT_FILTER, V_READ2_SCOPE);
		genVars(n->node1);
		genSelect(n->node2);
		genPrint();
		addop((q->quantityLimit > 0 ? JMPCNT : JMP), reread);
		jumps.setPlace(endreread, v.size());
		addop(POP); //rereader used 2 stack spaces
		addop(POP);
	}
	popvars();
	addop(ENDRUN);
}
//given 'from' node
void cgen::genTraverseJoins(unique_ptr<node> &n){
	if (n == nullptr) return;
	//start with base file
	addop(START_MESSAGE, messager::readingfirst);
	int endfile1 = jumps.newPlaceholder();
	normal_read = v.size();
	prevJoinRead = normal_read;
	addop(RDLINE, endfile1, 0);
	genJoinSets(n->node1);
	jumps.setPlace(endfile1, v.size());
	addop(STOP_MESSAGE);
}
//given 'join' node
void cgen::genJoinSets(unique_ptr<node> &n){
	if (n == nullptr) {
		if (q->grouping){ // includes group sorting
			genWhere(q->tree->node4);
			genGetGroup(q->tree->node4->node2);
			vs.setscope(SELECT_FILTER|ORDER_FILTER|HAVING_FILTER, V_READ1_SCOPE);
			genVars(q->tree->node1);
			genSelect(q->tree->node2);
			genAggSortList(q->tree->node4);
			genPredicates(q->tree->node4->node3); //having phase 1
			addop(JMP, prevJoinRead);
		} else if (q->sorting){
			genWhere(q->tree->node4);
			genNormalSortList(q->tree);
			addop(SAVEPOS);
			addop(JMP, prevJoinRead);
		} else {
			genWhere(q->tree->node4);
			vs.setscope(DISTINCT_FILTER, V_READ1_SCOPE);
			genVars(q->tree->node1);
			genDistinct(q->tree->node2->node1, wherenot);
			vs.setscope(SELECT_FILTER, V_READ1_SCOPE);
			genVars(q->tree->node1);
			genSelect(q->tree->node2);
			genPrint();
			addop((q->quantityLimit > 0 ? JMPCNT : JMP), prevJoinRead);
		}
		return;
	}
	// could genwhere per file
	joinFileIdx++;
	vs.setscope(JCOMP_FILTER, V_READ1_SCOPE);
	genVars(q->tree->node1);
	genJoinPredicates(n->node1);
	addop(JOINSET_INIT, (joinFileIdx-1)*2, n->tok3.lower() == "left");
	int goWhenDone = prevJoinRead;
	prevJoinRead = v.size();
	wherenot = prevJoinRead;
	addop(JOINSET_TRAV, goWhenDone, (joinFileIdx-1)*2, joinFileIdx);
	genJoinSets(n->node2);
}
void cgen::genPrint(){
	if (q->outputjson)
		addop(PRINTJSON, q->outputcsv ? 0 : 1);
	if (q->outputcsv)
		addop(PRINTCSV);
}
void cgen::genAndChainSet(unique_ptr<node> &n){
	int cz = n->info[CHAINSIZE];
	int ci = n->info[CHAINIDX];
	int fi = n->info[FILENO];
	auto& chain = q->getFileReader(fi)->andchains[ci];
	auto nn = n.get();
	for (int i=0; i<cz; ++i){
		auto& prednode = nn->node1;
		if (prednode->tok1.id == KW_LIKE){
			addop(LDLIT, q->dataholder.size());
			q->dataholder.push_back(prepareLike(prednode));
		} else if (prednode->info[TOSCAN] == 1){
			genExprAll(prednode->node2);
		}else if (prednode->info[TOSCAN] == 2){
			genExprAll(prednode->node1);
		}
		chain.functionTypes.push_back(funcTypes[prednode->datatype]);
		chain.relops.push_back(vmachine::relopIdx[prednode->tok1.id]);
		chain.negations.push_back(prednode->tok2.id);
		nn = nn->node2.get();
	}
	chain.relops[0] = 4; // 4 is index of eq, instruction already konws real first relop
	int orEquals = 0;
	switch (n->node1->tok1.id){
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
			error("joins with '"+n->tok1.val+"' operator in first of 'and' conditions not implemented");
	}
}
//given 'predicates' node
void cgen::genJoinPredicates(unique_ptr<node> &n){
	if (n == nullptr) return;
	if (n->info[ANDCHAIN]){
		genAndChainSet(n);
		return;
	}
	genJoinPredicates(n->node2);
	genJoinCompare(n->node1);
	switch (n->tok1.id){
	case KW_AND:
		addop(AND_SET);
		break;
	case KW_OR:
		addop(OR_SET);
		break;
	}
}
//given predicate comparison node
void cgen::genJoinCompare(unique_ptr<node> &n){
	if (n == nullptr) return;
	if (n->tok1.id == SP_LPAREN){
		genJoinPredicates(n->node1);
		return;
	}
	//evaluate the one not scanned
	if (n->info[TOSCAN] == 1){
		genExprAll(n->node2);
	} else if (n->info[TOSCAN] == 2){
		genExprAll(n->node1);
	}
	if (n->datatype == T_STRING)
		addop(NUL_TO_STR);
	int orEquals = 0, vpidx = n->info[VALPOSIDX];
	switch (n->tok1.id){
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
			error("joins with '"+n->tok1.val+"' operator not implemented");
	}
}
void cgen::genScanJoinFiles(unique_ptr<node> &n){
	e("scan joins");
	auto& joinNode = findFirstNode(n, N_JOIN);
	for (auto jnode = joinNode.get(); jnode; jnode = jnode->node2.get()){
		auto& f = q->filemap[jnode->tok4.val];
		int afterfile = jumps.newPlaceholder();
		addop(START_MESSAGE, messager::scanningjoin);
		normal_read = v.size();
		addop(RDLINE, afterfile, f->fileno);
		joinFileIdx++;
		f->vpTypes = move(valposTypes);
		valposTypes.clear();
		vs.setscope(JSCAN_FILTER, V_SCAN_SCOPE);
		genVars(q->tree->node1);
		genScannedJoinExprs(jnode->node1, f->fileno);
		if (valposTypes.size())
			addop(SAVEVALPOS, f->fileno, f->joinValpos.size());
		addop(JMP, normal_read);
		jumps.setPlace(afterfile, v.size());
		addop(START_MESSAGE, messager::indexing);
		genSortAnds(joinNode->node1);
		for (u32 i=0; i<valposTypes.size(); i++)
			addop(SORTVALPOS, f->fileno, i, funcTypes[valposTypes[i]]);
	}

}
void cgen::genSortAnds(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("sort ands");
	if (n->info[ANDCHAIN] == 1){
		addop(SORT_ANDCHAIN, n->info[FILENO],  n->info[CHAINIDX]);
		return;
	} else {
		genSortAnds(n->node2);
		if (n->node1->tok1.id == SP_LPAREN)
			genSortAnds(n->node1->node1);
	}
}
void cgen::genScanAndChain(unique_ptr<node> &n, int fileno){
	if (n == nullptr || n->info[ANDCHAIN] == 0) return;
	e("join ands");

	auto nn = n.get();
	for (int i=0; i<n->info[CHAINSIZE]; ++i){
		auto& prednode = nn->node1;
		if (prednode->info[TOSCAN] == 1){
			genExprAll(prednode->node1);
		}else if (prednode->info[TOSCAN] == 2){
			genExprAll(prednode->node2);
		}
		nn = nn->node2.get();
	}
	addop(SAVEANDCHAIN, n->info[CHAINIDX], fileno);
}

void cgen::genScannedJoinExprs(unique_ptr<node> &n, int fileno){
	if (n == nullptr) return;
	e("join exprs");
	bool gotExpr = false;
	switch (n->label){
		case N_PREDCOMP:
			if (n->info[ANDCHAIN]) //this is just for valpos joins
				return;
			if (n->tok1.id == SP_LPAREN)
				genScannedJoinExprs(n->node1, fileno);
			else if (n->info[TOSCAN] == 1){
				genExprAll(n->node1);
				gotExpr = true;
			}else if (n->info[TOSCAN] == 2){
				genExprAll(n->node2);
				gotExpr = true;
			}else
				error("invalid join comparision");
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

void cgen::genNormalQuery(unique_ptr<node> &n){
	e("normal");
	int message = (q->whereFiltering || q->distinctFiltering) ?
		messager::readingfiltered : messager::reading;
	int endfile = jumps.newPlaceholder(); //where to jump when done reading file
	pushvars();
	addop(START_MESSAGE, message);
	normal_read = v.size();
	wherenot = normal_read;
	addop(RDLINE, endfile, 0);
	genWhere(n->node4);
	vs.setscope(DISTINCT_FILTER, V_READ1_SCOPE);
	genVars(n->node1);
	genDistinct(n->node2->node1, normal_read);
	vs.setscope(SELECT_FILTER, V_READ1_SCOPE);
	genVars(n->node1);
	genSelect(n->node2);
	genPrint();
	addop((q->quantityLimit > 0 ? JMPCNT : JMP), normal_read);
	jumps.setPlace(endfile, v.size());
	addop(STOP_MESSAGE);
	popvars();
	addop(ENDRUN);
}
void cgen::genNormalOrderedQuery(unique_ptr<node> &n){
	int sorter = jumps.newPlaceholder(); //where to jump when done scanning file
	int reread = jumps.newPlaceholder();
	int endreread = jumps.newPlaceholder();
	pushvars();
	addop(START_MESSAGE, messager::scanning);
	normal_read = v.size();
	wherenot = normal_read;
	addop(RDLINE, sorter, 0);
	genWhere(n->node4);
	genNormalSortList(n);
	addop(SAVEPOS);
	addop(JMP, normal_read);
	jumps.setPlace(sorter, v.size());
	addop(START_MESSAGE, messager::sorting);
	addop(SORT);
	addop(PREP_REREAD);
	addop(START_MESSAGE, messager::retrieving);
	jumps.setPlace(reread, v.size());
	addop(RDLINE_ORDERED, endreread);
	vs.setscope(DISTINCT_FILTER, V_READ2_SCOPE);
	genVars(n->node1);
	genDistinct(n->node2->node1, reread);
	vs.setscope(SELECT_FILTER, V_READ2_SCOPE);
	genVars(n->node1);
	genSelect(n->node2);
	genPrint();
	addop((q->quantityLimit > 0 ? JMPCNT : JMP), reread);
	jumps.setPlace(endreread, v.size());
	addop(STOP_MESSAGE);
	addop(POP); //rereader used 2 stack spaces
	addop(POP);
	popvars();
	addop(ENDRUN);
};
 //update sorter to work with list
void cgen::genNormalSortList(unique_ptr<node> &n){
	vs.setscope(ORDER_FILTER, V_READ1_SCOPE);
	genVars(q->tree->node1);
	auto& ordnode = findFirstNode(q->tree->node4, N_ORDER);
	int i = 0;
	for (auto x = ordnode->node1.get(); x; x = x->node2.get()){
		genExprAll(x->node1);
		if (x->datatype == T_STRING)
			addop(NUL_TO_STR);
		addop(operations[OPSVSRT][x->datatype], i++);
		q->sortInfo.push_back({x->tok1.id, x->datatype});
	}
}
void cgen::genGroupingQuery(unique_ptr<node> &n){
	e("grouping");
	agg_phase = 1;
	int getgroups = jumps.newPlaceholder();
	pushvars();
	addop(START_MESSAGE, messager::scanning);
	normal_read = v.size();
	wherenot = normal_read;
	addop(RDLINE, getgroups, 0);
	genWhere(n->node4);
	genGetGroup(n->node4->node2);
	vs.setscope(SELECT_FILTER|HAVING_FILTER|ORDER_FILTER, V_READ1_SCOPE);
	genVars(n->node1);
	genPredicates(n->node4->node3); //having phase 1
	genSelect(n->node2);
	genAggSortList(n);
	addop(JMP, normal_read);
	jumps.setPlace(getgroups, v.size());
	agg_phase = 2;
	addop(STOP_MESSAGE);
	genIterateGroups(n->node4->node2);
	popvars();
	addop(ENDRUN);
}

void cgen::genAggSortList(unique_ptr<node> &n){
	if (n == nullptr) return;
	switch (n->label){
	case N_QUERY:     genAggSortList(n->node4->node4); break;
	case N_ORDER:     genAggSortList(n->node1); break;
	case N_EXPRESSIONS:
		genExprAdd(n->node1);
		genAggSortList(n->node2);
	}
}

void cgen::genVars(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen vars");
	switch (n->label){
	case N_PRESELECT: //currently only has 'with' branch
	case N_WITH:
		genVars(n->node1);
		break;
	case N_VARS:
		{
			int i = getVarIdx(n->tok1.val, q);
			auto& var = q->vars[i];
			if (vs.neededHere(i, var.filter, var.maxfileno)){
				genExprAll(n->node1);
				if (n->phase == (1|2)){
					//non-aggs in phase2
					if (agg_phase == 1){
						addop2(PUTVAR2, i, n->tok3.id);
					} else {
						addop1(LDMID, n->tok3.id);
						addop1(PUTVAR, i);
					}
				} else {
					addop1(PUTVAR, i);
					int vartype = getVarType(n->tok1.val, q);
					if (agg_phase == 2 && q->sorting && vartype == T_STRING)
						addop1(HOLDVAR, i);
				}
			}
			genVars(n->node2);
		}
		break;
	}
}

void cgen::genExprAdd(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen add");
	genExprAll(n->node1);
	if (!n->tok1.id) return;
	genExprAll(n->node2);
	switch (n->tok1.id){
	case SP_PLUS:
		addop0(operations[OPADD][n->datatype]);
		break;
	case SP_MINUS:
		addop0(operations[OPSUB][n->datatype]);
		break;
	}
}

void cgen::genExprMult(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen mult");
	genExprAll(n->node1);
	if (!n->tok1.id) return;
	genExprAll(n->node2);
	switch (n->tok1.id){
	case SP_STAR:
		addop0(operations[OPMULT][n->datatype]);
		break;
	case SP_DIV:
		addop0(operations[OPDIV][n->datatype]);
		break;
	case SP_CARROT:
		addop0(operations[OPEXP][n->datatype]);
		break;
	case SP_MOD:
		addop0(operations[OPMOD][n->datatype]);
		break;
	}
}

void cgen::genExprNeg(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen neg");
	genExprAll(n->node1);
	if (!n->tok1.id) return;
	addop0(operations[OPNEG][n->datatype]);
}

void cgen::genValue(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen value: "+n->tok1.val);
	dat lit;
	int vtype, op;
	switch (n->tok2.id){
	case COLUMN:
		addop2(operations[OPLD][n->datatype], getFileNo(n->tok3.val, q), n->tok1.id);
		break;
	case LITERAL:
		if (n->tok1.lower() == "null"){
			addop0(PUSH);
		} else {
			switch (n->datatype){
			case T_INT:      lit = parseIntDat(n->tok1.val.c_str());      break;
			case T_FLOAT:    lit = parseFloatDat(n->tok1.val.c_str());    break;
			case T_DATE:     lit = parseDateDat(n->tok1.val.c_str());     break;
			case T_DURATION: lit = parseDurationDat(n->tok1.val.c_str()); break;
			case T_STRING:   lit = parseStringDat(n->tok1.val.c_str());   break;
			}
			addop1(LDLIT, q->dataholder.size());
			q->dataholder.push_back(lit);
		}
		break;
	case VARIABLE:
		addop1(LDVAR, getVarIdx(n->tok1.val, q));
		//variable may be used in operations with different types
		vtype = getVarType(n->tok1.val, q);
		op = typeConv[vtype][n->datatype];
		if (op == CVER)
			error(st("Cannot use alias '",n->tok1.val,"' of type ",nameMap.at(vtype)," with incompatible type ",nameMap.at(n->datatype)));
		if (op != CVNO)
			addop0(op);
		break;
	case FUNCTION:
		genExprAll(n->node1);
		break;
	}
}

void cgen::genExprCase(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen case");
	int caseEnd = jumps.newPlaceholder();
	switch (n->tok1.id){
	case KW_CASE:
		switch (n->tok2.id){
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
		genExprAll(n->node1);
	}
}

void cgen::genCWExprList(unique_ptr<node> &n, int end){
	if (n == nullptr) return;
	e("gen case w list");
	genCWExpr(n->node1, end);
	genCWExprList(n->node2, end);
}

void cgen::genCWExpr(unique_ptr<node> &n, int end){
	if (n == nullptr) return;
	e("gen case w expr");
	int nextCase = jumps.newPlaceholder(); //get jump pos for next try
	genExprAll(n->node1); //evaluate comparision expression
	addop1(operations[OPEQ][n->tok1.id], 0); //leave '=' result where this comp value was
	addop2(JMPFALSE, nextCase, 1);
	addop0(POP); //don't need comparison value anymore
	genExprAll(n->node2); //result value if eq
	addop1(JMP, end);
	jumps.setPlace(nextCase, v.size()); //jump here for next try
}

void cgen::genCPredList(unique_ptr<node> &n, int end){
	if (n == nullptr) return;
	e("gen case p list");
	genCPred(n->node1, end);
	genCPredList(n->node2, end);
}

void cgen::genCPred(unique_ptr<node> &n, int end){
	if (n == nullptr) return;
	e("gen case p");
	int nextCase = jumps.newPlaceholder(); //get jump pos for next try
	genPredicates(n->node1);
	addop2(JMPFALSE, nextCase, 1);
	genExprAll(n->node2); //result value if true
	addop1(JMP, end);
	jumps.setPlace(nextCase, v.size()); //jump here for next try
}

void cgen::genSelect(unique_ptr<node> &n){
	genSelections(n->node1);
}

void cgen::genSelections(unique_ptr<node> &n){
	if (n == nullptr) {
		//reached end of selections section of query
		if (!select_count) genSelectAll();
		return;
	}
	e("gen selections");
	string t1 = n->tok1.lower();
	switch (n->label){
	case N_SELECTIONS:
		if (t1 == "hidden"sv) {

		} else if (t1 == "distinct"sv) {
			if (agg_phase == 1){
				genExprAll(n->node1);
			} else {
				addop1(PUTDIST, n->tok4.id);
			}
			incSelectCount();

		} else if (t1 == "*"sv) {
			genSelectAll();

		} else if (isTrivial(n)) {
			switch (agg_phase){
			case 0:
				for (auto nn = n.get(); nn; nn = nn->node1.get()) if (nn->label == N_VALUE){
					addop3(LDPUT, n->tok4.id, nn->tok1.id, getFileNo(nn->tok3.val, q));
					break;
				} break;
			case 1:
				for (auto nn = n.get(); nn; nn = nn->node1.get()) if (nn->label == N_VALUE){
					addop3(LDPUTGRP, n->tok3.id, nn->tok1.id, getFileNo(nn->tok3.val, q));
					break;
				} break;
			case 2:
				addop2(LDPUTMID, n->tok4.id, n->tok3.id);
				break;
			}
			incSelectCount();

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

void cgen::genPredicates(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen preds");
	genPredCompare(n->node1);
	int doneAndOr = jumps.newPlaceholder();
	switch (n->tok1.id){
	case KW_AND:
		addop2(JMPFALSE, doneAndOr, 0);
		addop0(POP); //don't need old result
		genPredicates(n->node2);
		break;
	case KW_OR:
		addop2(JMPTRUE, doneAndOr, 0);
		addop0(POP); //don't need old result
		genPredicates(n->node2);
		break;
	case KW_XOR:
		break;
	}
	jumps.setPlace(doneAndOr, v.size());
	if (n->tok2.id == SP_NEGATE)
		addop0(PNEG);
}

void cgen::genPredCompare(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen pred compare");
	int negation = n->tok2.id;
	int endcomp, greaterThanExpr3;
	genExprAll(n->node1);
	switch (n->tok1.id){
	case SP_NOEQ: negation ^= 1;
	case SP_EQ:
		genExprAll(n->node2);
		addop2(operations[OPEQ][n->datatype], 1, negation);
		break;
	case SP_GREATEQ: negation ^= 1;
	case SP_LESS:
		genExprAll(n->node2);
		addop2(operations[OPLT][n->datatype], 1, negation);
		break;
	case SP_GREAT: negation ^= 1;
	case SP_LESSEQ:
		genExprAll(n->node2);
		addop2(operations[OPLEQ][n->datatype], 1, negation);
		break;
	case KW_BETWEEN:
		endcomp = jumps.newPlaceholder();
		greaterThanExpr3 = jumps.newPlaceholder();
		addop1(NULFALSE1, endcomp);
		genExprAll(n->node2);
		addop1(NULFALSE2, endcomp);
		addop2(operations[OPLT][n->datatype], 0, 1);
		addop2(JMPFALSE, greaterThanExpr3, 1);
		genExprAll(n->node3);
		addop1(NULFALSE2, endcomp);
		addop2(operations[OPLT][n->datatype], 1, negation);
		addop1(JMP, endcomp);
		jumps.setPlace(greaterThanExpr3, v.size());
		genExprAll(n->node3);
		addop1(NULFALSE2, endcomp);
		addop2(operations[OPLT][n->datatype], 1, negation^1);
		jumps.setPlace(endcomp, v.size());
		break;
	case KW_IN:
		endcomp = jumps.newPlaceholder();
		for (auto nn=n->node2.get(); nn; nn=nn->node2.get()){
			genExprAll(nn->node1);
			addop1(operations[OPEQ][n->node1->datatype], 0);
			addop2(JMPTRUE, endcomp, 0);
			if (nn->node2.get())
				addop0(POP);
		}
		jumps.setPlace(endcomp, v.size());
		if (negation)
			addop0(PNEG);
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

void cgen::genWhere(unique_ptr<node> &nn){
	vs.setscope(WHERE_FILTER, V_READ1_SCOPE);
	genVars(q->tree->node1);
	auto& n = findFirstNode(nn, N_WHERE);
	e("gen where");
	if (n == nullptr) return;
	genPredicates(n->node1);
	addop2(JMPFALSE, wherenot, 1);
}

void cgen::genDistinct(unique_ptr<node> &n, int gotoIfNot){
	if (n == nullptr) return;
	e("gen distinct");
	if (n->label != N_SELECTIONS) return;
	if (n->tok1.id == KW_DISTINCT){
		genExprAll(n->node1);
		addop2(operations[OPDIST][n->datatype], gotoIfNot, addBtree(n->datatype, q));
	} else {
		//there can be only 1 distinct filter
		genDistinct(n->node2, gotoIfNot);
	}
}

void cgen::genFunction(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("gen function");
	int funcDone = jumps.newPlaceholder();
	int idx;

	//stuff common to all aggregate functions
	if ((n->tok1.id & AGG_BIT) != 0 ) {
		genExprAll(n->node1);
		if (n->tok3.val == "distinct"sv && agg_phase == 1){
			int setIndex = n->tok4.id;
			int separateSets = 1;
			if (q->grouping == 1){ //when onegroup, btree not indexed by rowgroup
				setIndex = addBtree(n->node1->datatype, q);
				separateSets = 0;
			}
			addop(operations[OPDIST][n->node1->datatype], funcDone, setIndex, separateSets);
			addop(LDDIST);
		}
	}

	switch (n->tok1.id){
	//non-aggregates
	case FN_COALESCE:
		for (auto nn = n->node1.get(); nn; nn = nn->node2.get()){
			genExprAll(nn->node1);
			addop1(JMPNOTNULL_ELSEPOP, funcDone);
		}
		addop0(PUSH);
		break;
	case FN_ABS:
		genExprAll(n->node1);
		addop0(operations[OPABS][n->datatype]);
		break;
	case FN_INC:
		addop1(FINC, q->dataholder.size());
		q->dataholder.push_back(dat{ {.f = 0.0}, T_FLOAT});
		break;
	case FN_ENCRYPT:
		genExprAll(n->node1);
		if (n->tok3.val == "chacha"sv){
			idx = q->crypt.newChacha(n->tok4.val);
			addop1(ENCCHA, idx);
		} else /* aes */ {
		}
		break;
	case FN_DECRYPT:
		genExprAll(n->node1);
		if (n->tok3.val == "chacha"sv){
			idx = q->crypt.newChacha(n->tok4.val);
			addop1(DECCHA, idx);
		} else /* aes */ {
		}
		break;
	case FN_SUBSTR:
		genExprAll(n->node1);
		if (!n->tok2.quoted) {
			auto n1 = parseIntDat(n->tok2.val.c_str());
			auto n2 = parseIntDat(n->tok3.val.c_str());
			addop3(FUNC_SUBSTR, n1.u.i, n2.u.i, 0);
		} else {
			dat reg{{r: new regex_t}, RMAL};
			if (regcomp(reg.u.r, n->tok2.val.c_str(), REG_EXTENDED))
				error("Could not parse 'substr' pattern");
			addop3(FUNC_SUBSTR, q->dataholder.size(), 0, 1);
			q->dataholder.push_back(reg);
		}
		break;
	case FN_STRING:
	case FN_INT:
	case FN_FLOAT:
		genExprAll(n->node1); //has conv node from datatyper
		break;
	case FN_POW:
		genExprAll(n->node1);
		genExprAll(n->node2);
		addop0(operations[OPEXP][n->datatype]);
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
		genExprAll(n->node1);
		addop(functionCode[n->tok1.id], n->tok2.id);
		break;
	case FN_LEN:
		genExprAll(n->node1);
		if (n->node1->datatype != T_STRING)
			addop0(typeConv[n->node1->datatype][T_STRING]);
		addop(functionCode[n->tok1.id]);
		break;
	}

	//aggregates
	if (agg_phase == 1) {
		switch (n->tok1.id){
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
		switch (n->tok1.id){
		case FN_AVG:
			addop(operations[OPLAVG][n->datatype], n->info[MIDIDX]);
			break;
		case FN_STDEV:
			addop(operations[OPLSTV][n->datatype], n->info[MIDIDX], 1);
			break;
		case FN_STDEVP:
			addop(operations[OPLSTV][n->datatype], n->info[MIDIDX]);
			break;
		case FN_MIN:
		case FN_MAX:
		case FN_SUM:
		case FN_COUNT:
			addop(LDMID, n->info[MIDIDX]);
			break;
		}
	}
	jumps.setPlace(funcDone, v.size());
}

void cgen::genTypeConv(unique_ptr<node> &n){
	if (n == nullptr) return;
	e("type convert");
	genExprAll(n->node1);
	auto cnv = typeConv[n->tok1.id][n->datatype];
	if (cnv == CVER)
		error(st("Cannot use type ",nameMap.at(n->tok1.id)," with incompatible type ",nameMap.at(n->datatype)));
	if (cnv != CVNO)
		addop0(typeConv[n->tok1.id][n->datatype]);
}

void cgen::genGetGroup(unique_ptr<node> &n){
	vs.setscope(AGG_FILTER, V_READ1_SCOPE);
	genVars(q->tree->node1);
	if (n == nullptr) return;
	e("get group");
	if (n->label == N_GROUPBY){
		int depth = -1;
		for (auto nn=n->node1.get(); nn; nn=nn->node2.get()){
			depth++;
			genExprAll(nn->node1);
		}
		addop1(GETGROUP, depth);
	}
}

void cgen::genIterateGroups(unique_ptr<node> &n){
	e("iterate groups");
	if (n == nullptr) {
		addop(ONEGROUP);
		vs.setscope(SELECT_FILTER, V_GROUP_SCOPE);
		genVars(q->tree->node1);
		genSelect(q->tree->node2);
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
		for (auto nn=n->node1.get(); nn; nn=nn->node2.get()){
			if (depth == 0) {
				goWhenDone = doneGroups;
			} else {
				goWhenDone = prevmap;
			}
			if (nn->node2.get()){
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
			auto& ordnode1 = findFirstNode(q->tree->node4, N_ORDER)->node1;
			int doneReadGroups = jumps.newPlaceholder();
			addop(GSORT, ordnode1->tok3.id);
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

void cgen::genUnsortedGroupRow(unique_ptr<node> &n, int nextgroup, int doneGroups){
	e("gen unsorted group row");
	vs.setscope(HAVING_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->node1);
	genPredicates(q->tree->node4->node3);
	if (q->havingFiltering)
		addop(JMPFALSE, nextgroup, 1);
	vs.setscope(DISTINCT_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->node1);
	genDistinct(q->tree->node2->node1, nextgroup);
	vs.setscope(SELECT_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->node1);
	genSelect(q->tree->node2);
	genPrint();
	addop((q->quantityLimit > 0 ? JMPCNT : JMP), nextgroup);
	addop(JMP, doneGroups);
}
void cgen::genSortedGroupRow(unique_ptr<node> &n, int nextgroup){
	e("gen sorted group row");
	vs.setscope(HAVING_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->node1);
	genPredicates(q->tree->node4->node3);
	if (q->havingFiltering)
		addop(JMPFALSE, nextgroup, 1);
	vs.setscope(DISTINCT_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->node1);
	genDistinct(q->tree->node2->node1, nextgroup);
	addop(ADD_GROUPSORT_ROW);
	vs.setscope(SELECT_FILTER, V_GROUP_SCOPE);
	genVars(q->tree->node1);
	genSelect(q->tree->node2);
	auto& ordnode = findFirstNode(q->tree->node4, N_ORDER);
	for (auto x = ordnode->node1.get(); x; x = x->node2.get()){
		genExprAll(x->node1);
		if (x->datatype == T_STRING)
			addop(NUL_TO_STR);
		addop(PUT, x->tok3.id);
		q->sortInfo.push_back({x->tok1.id, x->datatype});
	}
	addop(FREEMIDROW);
	addop(JMP, nextgroup);
}
