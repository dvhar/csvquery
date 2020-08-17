#include "interpretor.h"
#include "vmachine.h"
#include <cmath>
#include <numeric>

#include <execution>
#define parallel() execution::par_unseq,

//work with stack data
#define stk0 (*stacktop)
#define stk1 (*(stacktop-1))
#define stkt(N) (*(stacktop-N))
#define stkb(N) (*(stackbot+N))
#define push() ++stacktop
#define pop() --stacktop

#define debugOpcode
#ifndef debugOpcode
#define debugOpcode  cerr << ft("\nip %1% opcode %2% stack %3%")% ip% opMap[op->code]% (stacktop-stack.data());
#endif
//jump to next operation
#define next() \
	op = ops + ip; \
	debugOpcode \
	goto *(labels[op->code]);

void vmachine::run(){
	ios::sync_with_stdio(false);
	constexpr void* labels[] = { &&CVER_, &&CVNO_, &&CVIF_, &&CVIS_, &&CVFI_, &&CVFS_, &&CVDRS_, &&CVDTS_, &&CVSI_, &&CVSF_, &&CVSDR_, &&CVSDT_, &&IADD_, &&FADD_, &&TADD_, &&DTADD_, &&DRADD_, &&ISUB_, &&FSUB_, &&DTSUB_, &&DRSUB_, &&IMULT_, &&FMULT_, &&DRMULT_, &&IDIV_, &&FDIV_, &&DRDIV_, &&INEG_, &&FNEG_, &&PNEG_, &&IMOD_, &&FMOD_, &&IEXP_, &&FEXP_, &&JMP_, &&JMPCNT_, &&JMPTRUE_, &&JMPFALSE_, &&JMPNOTNULL_ELSEPOP_, &&RDLINE_, &&RDLINE_ORDERED_, &&PREP_REREAD_, &&PUT_, &&LDPUT_, &&LDPUTALL_, &&PUTVAR_, &&PUTVAR2_, &&LDINT_, &&LDFLOAT_, &&LDTEXT_, &&LDDATE_, &&LDDUR_, &&LDNULL_, &&LDLIT_, &&LDVAR_, &&HOLDVAR_, &&IEQ_, &&FEQ_, &&TEQ_, &&LIKE_, &&ILEQ_, &&FLEQ_, &&TLEQ_, &&ILT_, &&FLT_, &&TLT_, &&PRINT_, &&PUSH_, &&PUSH_N_, &&POP_, &&POPCPY_, &&ENDRUN_, &&NULFALSE1_, &&NULFALSE2_, &&NDIST_, &&SDIST_, &&PUTDIST_, &&LDDIST_, &&FINC_, &&ENCCHA_, &&DECCHA_, &&SAVESORTN_, &&SAVESORTS_, &&SAVEVALPOS_, &&SAVEPOS_, &&SORT_, &&GETGROUP_, &&ONEGROUP_, &&SUMI_, &&SUMF_, &&AVGI_, &&AVGF_, &&STDVI_, &&STDVF_, &&COUNT_, &&MINI_, &&MINF_, &&MINS_, &&MAXI_, &&MAXF_, &&MAXS_, &&NEXTMAP_, &&NEXTVEC_, &&ROOTMAP_, &&LDMID_, &&LDPUTMID_, &&LDPUTGRP_, &&LDSTDVI_, &&LDSTDVF_, &&LDAVGI_, &&LDAVGF_, &&ADD_GROUPSORT_ROW_, &&FREE_MIDROW_, &&GSORT_, &&READ_NEXT_GROUP_, &&NUL_TO_STR_, &&SORTVALPOS_, &&GET_SET_ANDS_, &&GET_SET_EQ_, &&GET_SET_LESS_, &&GET_SET_GRT_, &&JOINSET_INIT_, &&JOINSET_TRAV_, &&AND_SET_, &&OR_SET_, &&SAVEANDCHAIN_, &&SORT_ANDCHAIN_ };

	//vars for data
	int64 i64Temp;
	int iTemp1, iTemp2;
	double fTemp;
	char* cstrTemp;
	char bufTemp[40];
	bool boolTemp;
	dat datTemp;
	dat *datpTemp;
	csvEntry csvTemp;
	unsigned int sizeTemp;

	//vars for vm operations
	static int funcTypes[]  = { 0,0,1,0,0,2 };
	int numPrinted = 0;
	dat* stacktop = stack.data();
	dat* stackbot = stack.data();
	decltype(groupTree->getMap().begin()) groupItstk[20];
	typedef decltype(joinSetStack.front().begin()) jnit ;
	vector<jnit> setItstk((q->numFiles-1)*2);
	dat* midrow;
	int ip = 0;
	opcode *op;
	rowgroup *groupTemp;

	function<bool (const valpos&, const dat&)> vpLessFuncs[] = {
		[](const valpos& v, const dat& d){ return v.val.i < d.u.i; },
		[](const valpos& v, const dat& d){ return v.val.f < d.u.f; },
		[](const valpos& v, const dat& d){ return strcmp(v.val.s, d.u.s)<0; },
	};
	function<bool (const valpos&, const dat&)> vpGrtFuncs[] = {
		[](const valpos& v, const dat& d){ return v.val.i > d.u.i; },
		[](const valpos& v, const dat& d){ return v.val.f > d.u.f; },
		[](const valpos& v, const dat& d){ return strcmp(v.val.s, d.u.s)>0; },
	};
	function<bool (const valpos&, const dat&)> vpEqFuncs[] = {
		[](const valpos& v, const dat& d){ return v.val.i == d.u.i; },
		[](const valpos& v, const dat& d){ return v.val.f == d.u.f; },
		[](const valpos& v, const dat& d){ return !strcmp(v.val.s, d.u.s); },
	};

	//file writer
	ostream output(cout.rdbuf());
	string outbuf;

	next();

//put data from stack into torow
PUT_:
	freedat(torow[op->p1]);
	torow[op->p1] = stk0;
	disown(stk0);
	pop();
	++ip;
	next();
//put data from filereader directly into torow
LDPUT_:
	csvTemp = files[op->p3]->entries[op->p2];
	freedat(torow[op->p1]);
	torow[op->p1] = dat{ { s: csvTemp.val }, T_STRING, valSize(csvTemp) };
	++ip;
	next();
LDPUTGRP_:
	csvTemp = files[op->p3]->entries[op->p2];
	sizeTemp = valSize(csvTemp);
	if (sizeTemp && isnull(torow[op->p1])){
		torow[op->p1] = { { s:newStr(csvTemp.val, sizeTemp) }, T_STRING|MAL, sizeTemp };
	}
	++ip;
	next();
LDPUTALL_:
	iTemp1 = op->p1;
	for (auto &f : files){
		for (auto e=f->entries, end=f->entries+f->numFields; e<end; ++e){
			freedat(torow[iTemp1]);
			torow[iTemp1++] = dat{ { s: e->val }, T_STRING, valSize((*e)) };
		}
	}
	++ip;
	next();
//put data from midrow to torow
LDPUTMID_:
	freedat(torow[op->p1]);
	torow[op->p1] = midrow[op->p2];
	disown(midrow[op->p2]);
	++ip;
	next();
LDMID_:
	push();
	freedat(stk0);
	stk0 = midrow[op->p1];
	disown(midrow[op->p1]);
	++ip;
	next();

PUTDIST_:
	freedat(torow[op->p1]);
	torow[op->p1] = distinctVal;
	disown(distinctVal);
	++ip;
	next();
//put variable from stack into stackbot
PUTVAR_:
	freedat(stkb(op->p1));
	stkb(op->p1) = stk0;
	disown(stk0);
	pop();
	++ip;
	next();
//put variable from stack into midrow and stackbot
PUTVAR2_:
	datpTemp = &torow[op->p2];
	if (isnull(*datpTemp) && !isnull(stk0)){
		freedat(*datpTemp);
		*datpTemp = stk0.heap();
	}
	freedat(stkb(op->p1));
	stkb(op->p1) = stk0;
	pop();
	++ip;
	next();

HOLDVAR_:
	if (ismal(stkb(op->p1))){
		groupSortVars.emplace_front(stkb(op->p1).u.s);
		disown(stkb(op->p1));
	}
	++ip;
	next();
LDVAR_:
	push();
	freedat(stk0);
	stk0 = stkb(op->p1);
	disown(stk0); //var source still owns c string
	++ip;
	next();
//load data from filereader to the stack
LDDUR_:
	push();
	iTemp1 = parseDuration(files[op->p1]->entries[op->p2].val, &i64Temp);
	if (iTemp1) { setnull(stk0); }
	else stk0 = dat{ { i: i64Temp}, T_DURATION};
	++ip;
	next();
LDDATE_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	iTemp1 = dateparse(csvTemp.val, &i64Temp, &iTemp2, valSize(csvTemp));
	if (iTemp1) { setnull(stk0); }
	else stk0 = dat{ { i: i64Temp}, T_DATE, (unsigned int) iTemp2 };
	++ip;
	next();
LDTEXT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	sizeTemp = valSize(csvTemp);
	if (!sizeTemp) { setnull(stk0); }
	else stk0 = dat{ { s: csvTemp.val }, T_STRING, sizeTemp };
	++ip;
	next();
LDFLOAT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.f = strtof(csvTemp.val, &cstrTemp);
	stk0.b = T_FLOAT;
	if (!valSize(csvTemp) || *cstrTemp){ setnull(stk0); }
	++ip;
	next();
LDINT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.i = strtol(csvTemp.val, &cstrTemp, 10);
	stk0.b = T_INT;
	if (!valSize(csvTemp) || *cstrTemp) { setnull(stk0); }
	++ip;
	next();
LDNULL_:
	push();
	{ setnull(stk0); }
	++ip;
	next();
LDLIT_:
	push();
	stk0 = q->dataholder[op->p1];
	++ip;
	next();

//read a new line from a file
RDLINE_:
	ip = files[op->p2]->readline() ? op->p1 : ip+1;
	next();
RDLINE_ORDERED_:
	//stk0 has current read index, stk1 has vector.size()
	if (stk0.u.i < stk1.u.i){
		for (auto &f : files)
			f->readlineat(f->positions[sortIdxs[stk0.u.i++]]);
		++ip;
	} else {
		ip = op->p1;
	}
	next();
PREP_REREAD_:
	push();
	stk0.u.i = sortIdxs.size();
	push();
	stk0.u.i = 0;
	++ip;
	next();
NUL_TO_STR_:
	if (isnull(stk0)){
		stk0 = dat{ { .s = (char*) calloc(1,1) }, T_STRING|MAL, 0 };
	}
	++ip;
	next();
SAVESORTN_:
	normalSortVals[op->p1].push_back(stk0.u);
	pop();
	++ip;
	next();
SAVESORTS_:
	normalSortVals[op->p1].push_back(stk0.heap().u);
	disown(stk0);
	pop();
	++ip;
	next();
SAVEANDCHAIN_:
	{
		auto& file = files[op->p2];
		auto& chain = file->andchains[op->p1];
		int size = chain.datatypes.size();
		chain.positiions.push_back(file->pos);
		for (int i=0; i<size; ++i)
			chain.values[i].push_back(stkt(size-i-1).heap().u);
	}
	++ip;
	next();
SAVEVALPOS_:
	{
		iTemp1 = op->p2-1;
		auto& f = files[op->p1];
		for (auto& vpv : f->joinValpos){
			vpv.push_back(valpos{stkt(iTemp1).heap().u, f->pos});
			--iTemp1;
		}
		stacktop -= op->p2;
	}
	++ip;
	next();
//=
GET_SET_ANDS_:
	{
		auto& chain = files[op->p1]->andchains[op->p2];
	}
	++ip;
	next();
GET_SET_GRT_:
	{
		auto& grtfunc = vpGrtFuncs[op->p3];
		auto& vpvector = files[op->p1]->joinValpos[op->p2];
		int r = vpvector.size()-1;
		int l = 0;
		int m;
		joinSetStack.push_front(bset<int64>());
		while (l < r){
			m = (l+r)/2;
			if (grtfunc(vpvector[m], stk1))
				r = m;
			else
				l = m + 1;
		}
		--r;
		if (vpEqFuncs[op->p3](vpvector[r], stk1)){
			if (stk0.u.i) // >=
				for (m = r; m >= 0 && vpEqFuncs[op->p3](vpvector[m], stk1); --m)
					joinSetStack.front().insert(vpvector[m].pos);
			++r;
		}
		while (r < vpvector.size())
			joinSetStack.front().insert(vpvector[r++].pos);
	}
	pop(); //stk0 tells if GTE
	pop(); //comp value in stk1
	++ip;
	next();
GET_SET_LESS_:
	{
		auto& lessfunc = vpLessFuncs[op->p3];
		auto& vpvector = files[op->p1]->joinValpos[op->p2];
		int r = vpvector.size()-1;
		int l = 0;
		int m;
		joinSetStack.push_front(bset<int64>());
		while (l < r){
			m = (l+r)/2;
			if (lessfunc(vpvector[m], stk1))
				l = m + 1;
			else
				r = m;
		}
		if (vpEqFuncs[op->p3](vpvector[l], stk1)){
			if (stk0.u.i) // <=
				for (m = l; m < vpvector.size() && vpEqFuncs[op->p3](vpvector[m], stk1); ++m)
					joinSetStack.front().insert(vpvector[m].pos);
			--l;
		}
		while (l >= 0)
			joinSetStack.front().insert(vpvector[l--].pos);
	}
	pop(); //stk0 tells if LTE
	pop(); //comp value in stk1
	++ip;
	next();
GET_SET_EQ_:
	{
		auto& lessfunc = vpLessFuncs[op->p3];
		auto& vpvector = files[op->p1]->joinValpos[op->p2];
		int r = vpvector.size()-1;
		int l = 0;
		int m;
		joinSetStack.push_front(bset<int64>());
		while (l < r){
			m = (l+r)/2;
			if (lessfunc(vpvector[m], stk0))
				l = m + 1;
			else
				r = m;
		}
		while (l < vpvector.size() && vpEqFuncs[op->p3](vpvector[l], stk0))
			joinSetStack.front().insert(vpvector[l++].pos);
	}
	pop();
	++ip;
	next();
AND_SET_:
	{
		;
		bset<int64> tempset1(move(joinSetStack.front()));
		joinSetStack.pop_front();
		auto& target = joinSetStack.front();
		bset<int64> tempset2(move(target));
		set_intersection(tempset1.begin(), tempset1.end(), tempset2.begin(), tempset2.end(),
				inserter(target, target.begin()));
	}
	++ip;
	next();
OR_SET_:
	{
		bset<int64> tempset(move(joinSetStack.front()));
		joinSetStack.pop_front();
		auto& target = joinSetStack.front();
		for (auto loc : tempset)
			target.insert(loc);
	}
	++ip;
	next();
JOINSET_INIT_:
	{
		auto& jset = joinSetStack.front();
		if (op->p2 && jset.empty())
			jset.insert(-1); //blank row for left join without match
		setItstk[op->p1] = jset.begin();
		setItstk[op->p1+1] = jset.end();
	}
	op = ops+ ++ip;
	debugOpcode;
JOINSET_TRAV_:
	if (setItstk[op->p2] == setItstk[op->p2+1]){
		ip = op->p1;
		joinSetStack.pop_front();
	} else {
		files[op->p3]->readlineat(*(setItstk[op->p2]++));
		++ip;
	}
	next();
SAVEPOS_:
	for (auto &f : files)
		f->positions.push_back(f->pos);
	++ip;
	next();
SORT_ANDCHAIN_:
	{
		auto& chain = files[op->p1]->andchains[op->p2];
		chain.indexes.resize(chain.positiions.size());
		iota(begin(chain.indexes), end(chain.indexes), 0);
		function<bool (const int,const int)> uComparers[] = {
			[&](const int a, const int b) { return chain.values[0][a].i > chain.values[0][b].i; },
			[&](const int a, const int b) { return chain.values[0][a].f > chain.values[0][b].f; },
			[&](const int a, const int b) { return strcmp(chain.values[0][a].s, chain.values[0][b].s) > 0; },
		};
		sort(parallel() chain.indexes.begin(), chain.indexes.end(), uComparers[funcTypes[chain.datatypes[0]]]);
	}
	++ip;
	next();
SORTVALPOS_:
	{
		auto& vpvector = files[op->p1]->joinValpos[op->p2];
		function<bool (const valpos&,const valpos&)> valposComparers[] = {
			[&](const valpos &a, const valpos &b) { return a.val.i < b.val.i; },
			[&](const valpos &a, const valpos &b) { return a.val.f < b.val.f; },
			[&](const valpos &a, const valpos &b) {
				return a.val.s && b.val.s && strcmp(a.val.s, b.val.s)<0;
				//find out how a null is getting in here
			},
		};
		sort(parallel() vpvector.begin(), vpvector.end(), valposComparers[op->p3]);
	}
	++ip;
	next();

//normal sorter
SORT_:
	{
		sortIdxs.resize(files[0]->positions.size());
		iota(begin(sortIdxs), end(sortIdxs), 0);
		int sortVal = 0, start = 0, end = 0, last = sortIdxs.size()-1, prevVal = -1;
		function<bool (const int,const int)> normalComparers[] = {
			[&](const int a, const int b) { return normalSortVals[sortVal][a].i > normalSortVals[sortVal][b].i; },
			[&](const int a, const int b) { return normalSortVals[sortVal][a].f > normalSortVals[sortVal][b].f; },
			[&](const int a, const int b) { return strcmp(normalSortVals[sortVal][a].s, normalSortVals[sortVal][b].s) > 0; },
			[&](const int a, const int b) { return normalSortVals[sortVal][a].i < normalSortVals[sortVal][b].i; },
			[&](const int a, const int b) { return normalSortVals[sortVal][a].f < normalSortVals[sortVal][b].f; },
			[&](const int a, const int b) { return strcmp(normalSortVals[sortVal][a].s, normalSortVals[sortVal][b].s) < 0; },
		};
		auto backcheck = [&](int i) -> bool {
			int backidx = 0;
			while (i > 0){
				if (q->sortInfo[i-1].second == T_STRING){
					if (strcmp(normalSortVals[prevVal-backidx][sortIdxs[start]].s, normalSortVals[prevVal-backidx][sortIdxs[end+1]].s))
						return false;
				} else {
					if (normalSortVals[prevVal-backidx][sortIdxs[start]].i != normalSortVals[prevVal-backidx][sortIdxs[end+1]].i)
						return false;
				}
				--i; ++backidx;
			}
			return true;
		};
		auto comp = normalComparers[getSortComparer(q, 0)];
		sort(parallel() sortIdxs.begin(), sortIdxs.end(), comp);
		for (int i=1; i<q->sortcount; i++) {
			++sortVal; ++prevVal;
			start = end = 0;
			auto comp = normalComparers[getSortComparer(q, i)];
			while (start < last){
				while (end < last && backcheck(i))
					++end;
				if (end > start){
					sort(parallel() sortIdxs.begin()+start, sortIdxs.begin()+end+1, comp);
				}
				start = end + 1;
			}
		}
	}
	++ip;
	next();
//group sorter - p1 is sort index
GSORT_:
	{
		int sortVal = op->p1;
		int prevVal = sortVal - 1;
		int start = 0, end = 0, last = groupSorter.size()-1;
		function<bool (const dat*,const dat*)> groupComparers[] = {
			[&](const dat*a, const dat*b) { return a[sortVal].u.i > b[sortVal].u.i; },
			[&](const dat*a, const dat*b) { return a[sortVal].u.f > b[sortVal].u.f; },
			[&](const dat*a, const dat*b) { return strcmp(a[sortVal].u.s, b[sortVal].u.s) > 0; },
			[&](const dat*a, const dat*b) { return a[sortVal].u.i < b[sortVal].u.i; },
			[&](const dat*a, const dat*b) { return a[sortVal].u.f < b[sortVal].u.f; },
			[&](const dat*a, const dat*b) { return strcmp(a[sortVal].u.s, b[sortVal].u.s) < 0; },
		};
		auto backcheck = [&](int i) -> bool {
			int backidx = 0;
			while (i > 0){
				if (q->sortInfo[i-1].second == T_STRING){
					if (strcmp(groupSorter[start][prevVal-backidx].u.s, groupSorter[end+1][prevVal-backidx].u.s))
						return false;
				} else {
					if (groupSorter[start][prevVal-backidx].u.i != groupSorter[end+1][prevVal-backidx].u.i)
						return false;
				}
				--i; ++backidx;
			}
			return true;
		};
		auto comp = groupComparers[getSortComparer(q, 0)];
		sort(parallel() groupSorter.begin(), groupSorter.end(), comp);
		for (int i=1; i<q->sortcount; i++) {
			++sortVal; ++prevVal;
			start = end = 0;
			auto comp = groupComparers[getSortComparer(q, i)];
			while (start < last){
				while (end < last && backcheck(i))
					++end;
				if (end > start){
					sort(parallel() groupSorter.begin()+start, groupSorter.begin()+end+1, comp);
				}
				start = end + 1;
			}
		}
	}
	++ip;
	next();

//math operations
IADD_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else stk1.u.i += stk0.u.i;
	pop();
	++ip;
	next();
FADD_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else stk1.u.f += stk0.u.f;
	pop();
	++ip;
	next();
TADD_:
	strplus(stk1, stk0);
	pop();
	++ip;
	next();
DRADD_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else { stk1.u.i += stk0.u.i; stk1.b = T_DURATION; }
	pop();
	++ip;
	next();
DTADD_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else { stk1.u.i += stk0.u.i; stk1.b = T_DATE; }
	pop();
	++ip;
	next();
ISUB_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else stk1.u.i -= stk0.u.i;
	pop();
	++ip;
	next();
FSUB_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else stk1.u.f -= stk0.u.f;
	pop();
	++ip;
	next();
DTSUB_:
	//TODO: handle date-date
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else { stk1.u.i -= stk0.u.i; stk1.b = T_DATE; }
	pop();
	++ip;
	next();
DRSUB_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else { stk1.u.i -= stk0.u.i; stk1.b = T_DURATION; }
	pop();
	if (stk0.u.i < 0) stk0.u.i *= -1;
	++ip;
	next();
IMULT_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else stk1.u.i *= stk0.u.i;
	pop();
	++ip;
	next();
FMULT_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else stk1.u.f *= stk0.u.f;
	pop();
	++ip;
	next();
DRMULT_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else {
		iTemp1 = stk1.b; iTemp2 = stk0.b;
		if (iTemp1 == T_DATE){
			if (iTemp2 == T_INT){ // date * int
				stk1.u.i = stk1.u.i * stk0.u.i;
			} else { // date * float
				stk1.u.i = stk1.u.i * stk0.u.f;
			}
		} else {
			if (iTemp2 == T_INT){ // int * date
				stk1.u.i = stk1.u.i * stk0.u.i;
			} else { // float * date
				stk1.u.i = stk1.u.f * stk0.u.i;
			}
			stk1.b = T_DURATION;
		}
	}
	pop();
	++ip;
	next();
IDIV_:
	if (isnull(stk0) || isnull(stk1) || stk0.u.i==0) { setnull(stk1); }
	else stk1.u.i /= stk0.u.i;
	pop();
	++ip;
	next();
FDIV_:
	if (isnull(stk0) || isnull(stk1) || stk0.u.f==0) { setnull(stk1); }
	else stk1.u.f /= stk0.u.f;
	pop();
	++ip;
	next();
DRDIV_:
	if (isnull(stk0) || isnull(stk1) || stk0.u.f==0) { setnull(stk1); }
	else {
		if (stk0.b == T_INT)
			stk1.u.i /= stk0.u.i;
		else
			stk1.u.i /= stk0.u.f;
	}
	pop();
	++ip;
	next();
INEG_:
	if (isnull(stk0)) { setnull(stk0); }
	else stk0.u.i *= -1;
	++ip;
	next();
FNEG_:
	if (isnull(stk0)) { setnull(stk0); }
	else stk0.u.f *= -1;
	++ip;
	next();
PNEG_:
	stk0.u.p ^= true;
	++ip;
	next();
FEXP_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else stk1.u.f = pow(stk1.u.f, stk0.u.f);
	pop();
	++ip;
	next();
IEXP_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else stk1.u.i = pow(stk1.u.i, stk0.u.i);
	pop();
	++ip;
	next();
IMOD_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else stk1.u.i = stk1.u.i % stk0.u.i;
	pop();
	++ip;
	next();
FMOD_:
	if (isnull(stk0) || isnull(stk1)) { setnull(stk1); }
	else stk1.u.f = static_cast<int64>(stk1.u.f) % static_cast<int64>(stk0.u.f);
	pop();
	++ip;
	next();

//comparisions - p1 determines how far down the stack to leave the result
// p2 is negator
IEQ_:
	iTemp1 = isnull(stk0);
	iTemp2 = isnull(stk1);
	if (iTemp1 ^ iTemp2) stkt(op->p1).u.p = false;
	else if (iTemp1 & iTemp2) stkt(op->p1).u.p = true;
	else stkt(op->p1).u.p = (stk1.u.i == stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FEQ_:
	iTemp1 = isnull(stk0);
	iTemp2 = isnull(stk1);
	if (iTemp1 ^ iTemp2) stkt(op->p1).u.p = false;
	else if (iTemp1 & iTemp2) stkt(op->p1).u.p = true;
	else stkt(op->p1).u.p = (stk1.u.f == stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TEQ_:
	iTemp1 = isnull(stk0);
	iTemp2 = isnull(stk1);
	if (!(iTemp1|iTemp2)){ //none null
		boolTemp = (strcmp(stk1.u.s, stk0.u.s) == 0)^op->p2;
		freedat(stk0);
		freedat(stkt(op->p1));
		stkt(op->p1).u.p = boolTemp;
	} else if (iTemp1 & iTemp2) { //both null
		stkt(op->p1).u.p = true^op->p2;
	} else { //one null
		freedat(stk0);
		freedat(stkt(op->p1));
		stkt(op->p1).u.p = false^op->p2;
	}
	stacktop -= op->p1;
	++ip;
	next();
ILEQ_:
	if (isnull(stk0) || isnull(stk1)) stkt(op->p1).u.p = false;
	else stkt(op->p1).u.p = (stk1.u.i <= stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FLEQ_:
	if (isnull(stk0) || isnull(stk1)) stkt(op->p1).u.p = false;
	else stkt(op->p1).u.p = (stk1.u.f <= stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TLEQ_:
	if (isnull(stk0) || isnull(stk1)) {
		freedat(stkt(op->p1));
		stkt(op->p1).u.p = false;
	} else {
		boolTemp = (strcmp(stk1.u.s, stk0.u.s) <= 0)^op->p2;
		freedat(stk0);
		freedat(stkt(op->p1));
		stkt(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	++ip;
	next();
ILT_:
	if (isnull(stk0) || isnull(stk1)) stkt(op->p1).u.p = false;
	else stkt(op->p1).u.p = (stk1.u.i < stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FLT_:
	if (isnull(stk0) || isnull(stk1)) stkt(op->p1).u.p = false;
	else stkt(op->p1).u.p = (stk1.u.f < stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TLT_:
	if (isnull(stk0) || isnull(stk1)){
		freedat(stkt(op->p1));
		stkt(op->p1).u.p = false;
	} else {
		boolTemp = (strcmp(stk1.u.s, stk0.u.s) < 0)^op->p2;
		freedat(stk0);
		freedat(stkt(op->p1));
		stkt(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	++ip;
	next();
LIKE_:
	if (isnull(stk0)){
		stk0.u.p = op->p2;
	} else {
		iTemp1 = !regexec(q->dataholder[op->p1].u.r, stk0.u.s, 0, 0, 0)^op->p2;
		freedat(stk0);
		stk0.u.p = iTemp1;
	}
	++ip;
	next();

PUSH_N_:
	push();
	freedat(stk0);
	stk0.u.i = op->p1;;
	++ip;
	next();
PUSH_:
	push();	
	++ip;
	next();
POP_:
	pop();
	++ip;
	next();
POPCPY_:
	freedat(stk1);
	stk1 = stk0;
	disown(stk0);
	pop();
	++ip;
	next();
NULFALSE1_:
	if (isnull(stk0)){
		stk0.u.p = false;
		ip = op->p1;
	} else ++ip;
	next();
NULFALSE2_:
	if (isnull(stk0)){
		pop();
		freedat(stk0);
		stk0.u.p = false;
		ip = op->p1;
	} else ++ip;
	next();

//type conversions
CVIS_:
	if (!isnull(stk0)){
		stk0.z = sprintf(bufTemp, "%lld", stk0.u.i);
		stk0.u.s = (char*) malloc(stk0.z+1);
		memcpy(stk0.u.s, bufTemp, stk0.z+1);
		stk0.b = T_STRING|MAL;
	}
	++ip;
	next();
CVFS_:
	if (!isnull(stk0)){
		stk0.z = sprintf(bufTemp, "%.10g", stk0.u.f);
		stk0.u.s = (char*) malloc(stk0.z+1);
		memcpy(stk0.u.s, bufTemp, stk0.z+1);
		stk0.b = T_STRING|MAL;
	}
	++ip;
	next();
CVFI_:
	if (!isnull(stk0)){
		stk0.u.i = stk0.u.f;
		stk0.b = T_INT;
	}
	++ip;
	next();
CVIF_:
	if (!isnull(stk0)){
		stk0.u.f = stk0.u.i;
		stk0.b = T_FLOAT;
	}
	++ip;
	next();
CVSI_:
	if (!isnull(stk0)){
		iTemp1 = strtol(stk0.u.s, &cstrTemp, 10);
		freedat(stk0);
		stk0.u.i = iTemp1;
		stk0.b = T_INT;
		if (*cstrTemp){ setnull(stk0); }
	}
	++ip;
	next();
CVSF_:
	if (!isnull(stk0)){
		fTemp = strtof(stk0.u.s, &cstrTemp);
		freedat(stk0);
		stk0.u.f = fTemp;
		stk0.b = T_FLOAT;
		if (*cstrTemp){ setnull(stk0); }
	}
	++ip;
	next();
CVSDT_:
	if (!isnull(stk0)){
		iTemp1 = dateparse(stk0.u.s, &i64Temp, &iTemp2, stk0.z);
		freedat(stk0);
		stk0.u.i = i64Temp;
		stk0.b = T_DATE;
		stk0.z = iTemp2;
		if (iTemp1) { setnull(stk0); }
	}
	++ip;
	next();
CVSDR_:
	if (!isnull(stk0)){
		iTemp1 = parseDuration(stk0.u.s, &i64Temp);
		freedat(stk0);
		stk0.u.i = i64Temp;
		stk0.b = T_DURATION;
		if (iTemp1) { setnull(stk0); }
	}
	++ip;
	next();
CVDRS_:
	if (!isnull(stk0)){
		stk0.u.s = (char*) malloc(24);
		durstring(stk0.u.i, stk0.u.s);
		stk0.b = T_STRING|MAL;
		stk0.z = strlen(stk0.u.s);
	}
	++ip;
	next();
CVDTS_:
	if (!isnull(stk0)){
		//make version of datestring that writes directly to arg buf
		cstrTemp = datestring(stk0.u.i);
		stk0.u.s = (char*) malloc(20);
		strncpy(stk0.u.s, cstrTemp, 19);
		stk0.b = T_STRING|MAL;
		stk0.z = 19;
	}
	++ip;
	next();

//jump instructions
JMP_:
	ip = op->p1;
	next();
JMPCNT_:
	ip = (numPrinted < quantityLimit) ? op->p1 : ip+1;
	next();
JMPFALSE_:
	ip = !stk0.u.p ? op->p1 : ip+1;
	if (op->p2 == 1){
		pop();
	}
	next();
JMPTRUE_:
	ip = stk0.u.p ? op->p1 : ip+1;
	if (op->p2 == 1){
		pop();
	}
	next();
JMPNOTNULL_ELSEPOP_:
	if (isnull(stk0)){
		freedat(stk0);
		pop();
		++ip;
	} else {
		ip = op->p1;
	}
	next();

PRINT_:
	iTemp1 = 0;	
	printfield:
	torow[iTemp1].appendToBuffer(outbuf);
	if (outbuf.size() > 900){
		output << outbuf;
		outbuf.clear();
	}
	if (++iTemp1 < torowSize){
		outbuf += ',';
		goto printfield;
	}
	outbuf += '\n';
	++numPrinted;
	++ip;
	next();

//distinct checkers
LDDIST_:
	push();
	stk0 = distinctVal;
	++ip;
	next();
NDIST_:
	iTemp1 = op->p3 ? groupTemp->meta.distinctNSetIdx : 0;
	i64Temp = isnull(stk0) ? SMALLEST : stk0.u.i;
	boolTemp = bt_nums[op->p2+iTemp1].insert(i64Temp).second;
	if (boolTemp) {
		distinctVal = stk0;
		++ip;
	} else {
		ip = op->p1;
	}
	pop();
	next();
SDIST_:
	{
		iTemp1 = op->p3 ? groupTemp->meta.distinctSSetIdx : 0;
		treeCString tsc(stk0);
		boolTemp = bt_strings[op->p2+iTemp1].insert(tsc).second;
		if (boolTemp) {
			distinctVal = stk0;
			++ip;
		} else {
			free(tsc.s);
			ip = op->p1;
		}
	}
	pop();
	next();

//functions
FINC_:
	q->dataholder[op->p1].u.f++;
	push();
	stk0 = q->dataholder[op->p1];
	++ip;
	next();
ENCCHA_:
	{
		auto&& pt1 = q->crypt.chachaEncrypt(op->p1, stk0.z, stk0.u.s);
		if (stk0.b & MAL) free(stk0.u.s);
		stk0.u.s = pt1.first;
		stk0.z = pt1.second;
		stk0.b = T_STRING|MAL;
	}
	++ip;
	next();
DECCHA_:
	{
		auto&& pt2 = q->crypt.chachaDecrypt(op->p1, stk0.z, stk0.u.s);
		if (stk0.b & MAL) free(stk0.u.s);
		stk0.u.s = pt2.first;
		stk0.z = pt2.second;
		stk0.b = T_STRING|MAL;
	}
	++ip;
	next();
//aggregates
LDSTDVI_:
	++ip;
	next()
LDSTDVF_:
	push();
	freedat(stk0);
	if (!isnull(midrow[op->p1]))
		stk0 = stdvs[midrow[op->p1].u.i].eval(op->p2);
	++ip;
	next()
LDAVGI_:
	push();
	freedat(stk0);
	datpTemp = &midrow[op->p1];
	if (!isnull((*datpTemp)) && datpTemp->z){
		stk0.u.i = datpTemp->u.i / datpTemp->z;
		stk0.b = T_INT;
	}
	++ip;
	next()
LDAVGF_:
	push();
	freedat(stk0);
	datpTemp = &midrow[op->p1];
	if (!isnull((*datpTemp)) && datpTemp->z){
		stk0.u.f = datpTemp->u.f / datpTemp->z;
		stk0.b = T_FLOAT;
	}
	++ip;
	next()
AVGI_:
	if (!isnull(stk0))
		torow[op->p1].z++;
SUMI_:
	if (!isnull(stk0)){
		torow[op->p1].u.i += stk0.u.i;
		torow[op->p1].b = stk0.b;
	}
	pop();
	++ip;
	next();
AVGF_:
	if (!isnull(stk0))
		torow[op->p1].z++;
SUMF_:
	if (!isnull(stk0)){
		torow[op->p1].u.f += stk0.u.f;
		torow[op->p1].b = stk0.b;
	}
	pop();
	++ip;
	next();
STDVI_:
	//TODO - duration
	pop();
	++ip;
	next();
STDVF_:
	{
		if (!isnull(stk0))
			if (isnull(torow[op->p1])){
				torow[op->p1] = {{i:static_cast<int64>(stdvs.size())},T_INT};
				stdvs.emplace_back(stddev(stk0.u.f));
			} else {
				stdvs[torow[op->p1].u.i].numbers.push_front(stk0.u.f);
			}
	}
	pop();
	++ip;
	next();
COUNT_:
	if (op->p2){ //count(*)
		torow[op->p1].b = T_FLOAT;
		torow[op->p1].u.f++;
	} else {
		if (!isnull(stk0)) {
			torow[op->p1].b = T_FLOAT;
			torow[op->p1].u.f++;
		}
		pop();
	}
	++ip;
	next();
MINI_:
	if (isnull(torow[op->p1]) || (!isnull(stk0) && torow[op->p1].u.i > stk0.u.i)){
		torow[op->p1] = stk0;
	}
	pop();
	++ip;
	next();
MINF_:
	if (isnull(torow[op->p1]) || (!isnull(stk0) && torow[op->p1].u.f > stk0.u.f))
		torow[op->p1] = stk0;
	pop();
	++ip;
	next();
MINS_:
	if (isnull(torow[op->p1]) || (!isnull(stk0) && strcmp(torow[op->p1].u.s, stk0.u.s) > 0)){
		freedat(torow[op->p1]);
		torow[op->p1] = stk0.heap();
		disown(stk0);
	}
	pop();
	++ip;
	next();
MAXI_:
	if (isnull(torow[op->p1]) || (!isnull(stk0) && torow[op->p1].u.i < stk0.u.i))
		torow[op->p1] = stk0;
	pop();
	++ip;
	next();
MAXF_:
	if (isnull(torow[op->p1]) || (!isnull(stk0) && torow[op->p1].u.f < stk0.u.f))
		torow[op->p1] = stk0;
	pop();
	++ip;
	next();
MAXS_:
	if (isnull(torow[op->p1]) || (!isnull(stk0) && strcmp(torow[op->p1].u.s, stk0.u.s) < 0)){
		freedat(torow[op->p1]);
		torow[op->p1] = stk0.heap();
		disown(stk0);
	}
	pop();
	++ip;
	next();
GETGROUP_:
	groupTemp = groupTree.get();
	for (int i=op->p1; i >= 0; --i){
		groupTemp = &groupTemp->nextGroup(stkt(i));
	}
	torow = groupTemp->getRow(this);
	stacktop -= (op->p1 + 1);
	++ip;
	next();
ONEGROUP_:
	torow = destrow.data();
	midrow = onegroup.data();
	++ip;
	next();
ROOTMAP_:
	groupItstk[op->p1]   = groupTree->getMap().begin();
	groupItstk[op->p1+1] = groupTree->getMap().end();
	torow = destrow.data();
	++ip;
	next();
NEXTMAP_:
	if (groupItstk[op->p2-2] == groupItstk[op->p2-1]){
		ip = op->p1;
	} else {
		// set top of iterator stack to next map
		groupTemp = &(groupItstk[op->p2-2]++)->second;
		groupItstk[op->p2]   = groupTemp->getMap().begin();
		groupItstk[op->p2+1] = groupTemp->getMap().end();
		++ip;
	}
	next();
NEXTVEC_:
	if (groupItstk[op->p2] == groupItstk[op->p2+1]){
		ip = op->p1;
	} else {
		groupTemp = &(groupItstk[op->p2]++)->second;
		midrow = groupTemp->getVec();
		++ip;
	}
	next();
ADD_GROUPSORT_ROW_:
	torow = (dat*) malloc(sortgroupsize * sizeof(dat));
	initarr(torow, sortgroupsize, (dat{{0},NIL}));
	groupSorter.push_back(torow);
	++ip;
	next();
FREE_MIDROW_:
	freearr(groupTemp->getVec(), groupTemp->meta.rowsize);
	free(groupTemp->getVec());
	groupTemp->meta.freed = true;
	++ip;
	next();

READ_NEXT_GROUP_:
	if (stk0.u.i >= groupSorter.size()){
		ip = op->p1;
	} else {
		torow = groupSorter[stk0.u.i];
		++stk0.u.i;
		++ip;
	}
	next();

ENDRUN_:
	output << outbuf;
	output.flush();
	return;

//error opcodes
CVER_:
CVNO_:
	error("Invalid opcode");
}

void runquery(querySpecs &q){
	vmachine vm(q);
	vm.run();
}
