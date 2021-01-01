#include "interpretor.h"
#include "vmachine.h"
#include <cmath>
#include <numeric>

#include "deps/parasort/boost/sort/parallel/sort.hpp"
namespace bs_sort = boost::sort::parallel;

//work with stack data
#define stk0 (*stacktop)
#define stk1 (*(stacktop-1))
#define stkt(N) (*(stacktop-(N)))
#define stkb(N) (*(stackbot+(N)))
#define push() (++stacktop)->freedat();
#define pop() --stacktop

#define debugOpcode
#ifndef debugOpcode
#define debugOpcode  perr(st("\nip ",ip," opcode ",opMap[op->code]," stack ",stacktop-stack.data()));
#endif
//jump to next operation
#define next() \
	op = ops + ip; \
	debugOpcode \
	goto *(labels[op->code]);
#define nexti() \
	++ip; \
	next();
#define skipnull() \
	if (stk0.isnull()){ nexti() };
#define ifnotnull if (!stk0.isnull())
#define ifneithernull if (stk0.isnull() || stk1.isnull()) { stk1.setnull(); } else

void vmachine::run(){
	constexpr void* labels[] = { &&CVER_, &&CVNO_, &&CVIF_, &&CVIS_, &&CVFI_, &&CVFS_, &&CVDRS_, &&CVDTS_, &&CVSI_, &&CVSF_, &&CVSDR_, &&CVSDT_, &&IADD_, &&FADD_, &&TADD_, &&DTADD_, &&DRADD_, &&ISUB_, &&FSUB_, &&DTSUB_, &&DRSUB_, &&IMULT_, &&FMULT_, &&DRMULT_, &&IDIV_, &&FDIV_, &&DRDIV_, &&INEG_, &&FNEG_, &&PNEG_, &&IMOD_, &&FMOD_, &&IPOW_, &&FPOW_, &&JMP_, &&JMPCNT_, &&JMPTRUE_, &&JMPFALSE_, &&JMPNOTNULL_ELSEPOP_, &&RDLINE_, &&RDLINE_ORDERED_, &&PREP_REREAD_, &&PUT_, &&LDPUT_, &&LDPUTALL_, &&PUTVAR_, &&PUTVAR2_, &&LDINT_, &&LDFLOAT_, &&LDTEXT_, &&LDDATE_, &&LDDUR_, &&LDLIT_, &&LDVAR_, &&HOLDVAR_, &&IEQ_, &&FEQ_, &&TEQ_, &&LIKE_, &&ILEQ_, &&FLEQ_, &&TLEQ_, &&ILT_, &&FLT_, &&TLT_, &&PRINTCSV_, &&PRINTJSON_, &&PUSH_, &&PUSH_N_, &&POP_, &&POPCPY_, &&ENDRUN_, &&NULFALSE_, &&DIST_, &&PUTDIST_, &&LDDIST_, &&FUNC_INC_, &&FUNC_ENCCHA_, &&FUNC_DECCHA_, &&SAVESORTN_, &&SAVESORTS_, &&SAVEVALPOS_, &&SAVEPOS_, &&SORT_, &&GETGROUP_, &&ONEGROUP_, &&SUMI_, &&SUMF_, &&AVGI_, &&AVGF_, &&STDVI_, &&STDVF_, &&COUNT_, &&MINI_, &&MINF_, &&MINS_, &&MAXI_, &&MAXF_, &&MAXS_, &&NEXTMAP_, &&NEXTVEC_, &&ROOTMAP_, &&LDMID_, &&LDPUTMID_, &&LDPUTGRP_, &&LDSTDVI_, &&LDSTDVF_, &&LDAVGI_, &&LDAVGF_, &&ADD_GROUPSORT_ROW_, &&FREE_MIDROW_, &&GSORT_, &&READ_NEXT_GROUP_, &&NUL_TO_STR_, &&SORTVALPOS_, &&JOINSET_EQ_AND_, &&JOINSET_EQ_, &&JOINSET_LESS_, &&JOINSET_GRT_, &&JOINSET_LESS_AND_, &&JOINSET_GRT_AND_, &&JOINSET_INIT_, &&JOINSET_TRAV_, &&AND_SET_, &&OR_SET_, &&SAVEANDCHAIN_, &&SORT_ANDCHAIN_, &&FUNC_YEAR_, &&FUNC_MONTH_, &&FUNC_WEEK_, &&FUNC_YDAY_, &&FUNC_MDAY_, &&FUNC_WDAY_, &&FUNC_HOUR_, &&FUNC_MINUTE_, &&FUNC_SECOND_, &&FUNC_WDAYNAME_, &&FUNC_MONTHNAME_, &&FUNC_ABSF_, &&FUNC_ABSI_, &&START_MESSAGE_, &&STOP_MESSAGE_, &&FUNC_CIEL_, &&FUNC_FLOOR_, &&FUNC_ACOS_, &&FUNC_ASIN_, &&FUNC_ATAN_, &&FUNC_COS_, &&FUNC_SIN_, &&FUNC_TAN_, &&FUNC_EXP_, &&FUNC_LOG_, &&FUNC_LOG2_, &&FUNC_LOG10_, &&FUNC_SQRT_, &&FUNC_RAND_, &&FUNC_UPPER_, &&FUNC_LOWER_, &&FUNC_BASE64_ENCODE_, &&FUNC_BASE64_DECODE_, &&FUNC_HEX_ENCODE_, &&FUNC_HEX_DECODE_, &&FUNC_LEN_, &&FUNC_SUBSTR_, &&FUNC_MD5_, &&FUNC_SHA1_, &&FUNC_SHA256_, &&FUNC_ROUND_, &&FUNC_CBRT_, &&FUNC_NOW_, &&FUNC_NOWGM_, &&PRINTCSV_HEADER_, &&FUNC_FORMAT_, &&LDCOUNT_, &&BETWEEN_, &&PRINTBOX_, &&PRINTBTREE_, &&INSUBQUERY_, &&FUNC_SIP_, &&XOR_SET_};

	i64 i64Temp;
	int iTemp1, iTemp2;
	double fTemp;
	char* cstrTemp;
	char bufTemp[40];
	bool boolTemp;
	dat *datpTemp;
	csvEntry csvTemp;
	u32 sizeTemp;
	struct tm tmTemp;
	dat nowgmdat = { { .i = nowgm() }, T_DATE };
	dat nowdat = { { .i = nowlocal() }, T_DATE };
	decltype(groupTree->getMap().begin()) groupItstk[20];
	typedef decltype(joinSetStack.front().begin()) jnit ;
	vector<jnit> setItstk((q->numFiles-1)*2);
	startSubqueries();

	next();

//put data from stack into torow
PUT_:
	torow[op->p1].mov(stk0);
	pop();
	nexti();
//put data from filereader directly into torow
LDPUT_:
	csvTemp = files[op->p3]->entries[op->p2];
	torow[op->p1].freedat();
	torow[op->p1] = dat{ { .s = csvTemp.val }, T_STRING, csvTemp.size() };
	nexti();
LDPUTGRP_:
	csvTemp = files[op->p3]->entries[op->p2];
	sizeTemp = csvTemp.size();
	if (sizeTemp && torow[op->p1].isnull())
		torow[op->p1] = { { .s =newStr(csvTemp.val, sizeTemp) }, T_STRING|MAL, sizeTemp };
	nexti();
LDPUTALL_:
	iTemp1 = op->p1;
	for (auto &f : files)
		for (auto e=f->entries, end=f->entries+f->numFields; e<end; ++e){
			torow[iTemp1].freedat();
			torow[iTemp1++] = dat{ { .s = e->val }, T_STRING, e->size() };
		}
	nexti();
//put data from midrow to torow
LDPUTMID_:
	torow[op->p1].mov(midrow[op->p2]);
	nexti();
LDMID_:
	++stacktop;
	stk0.mov(midrow[op->p1]);
	nexti();

PUTDIST_:
	torow[op->p1].mov(distinctVal);
	nexti();
//put variable from stack into stackbot
PUTVAR_:
	stkb(op->p1).mov(stk0);
	pop();
	nexti();
//put variable from stack into midrow and stackbot
PUTVAR2_:
	if (auto& t = torow[op->p2]; t.isnull() && !stk0.isnull()){
		t = stk0.heap();
	}
	stkb(op->p1).mov(stk0);
	pop();
	nexti();

HOLDVAR_:
	if (stkb(op->p1).ismal()){
		groupSortVars.emplace_front(stkb(op->p1).u.s);
		stkb(op->p1).disown();
	} nexti();
LDVAR_:
	push();
	stk0 = stkb(op->p1);
	stk0.disown(); //var source still owns c string
	nexti();
//load data from filereader to the stack
LDDUR_:
	push();
	parseDuration(files[op->p1]->entries[op->p2].val, stk0);
	nexti();
LDDATE_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	iTemp1 = dateparse(csvTemp.val, &i64Temp, &iTemp2, csvTemp.size());
	if (iTemp1) { stk0.setnull(); }
	else stk0 = dat{ { .i = i64Temp}, T_DATE, (u32) iTemp2 };
	nexti();
LDTEXT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	sizeTemp = csvTemp.size();
	if (!sizeTemp) { stk0.setnull(); }
	else stk0 = dat{ { .s = csvTemp.val }, T_STRING, sizeTemp };
	nexti();
LDFLOAT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.f = strtod(csvTemp.val, &cstrTemp);
	stk0.b = T_FLOAT;
	if (!csvTemp.size() || *cstrTemp){ stk0.setnull(); }
	nexti();
LDINT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.i = strtoll(csvTemp.val, &cstrTemp, 10);
	stk0.b = T_INT;
	if (!csvTemp.size() || cstrTemp == csvTemp.val) { stk0.setnull(); }
	nexti();
LDLIT_:
	push();
	stk0 = q->dataholder[op->p1];
	nexti();

//read a new line from a file
RDLINE_:
	ip = files[op->p2]->readline() ? op->p1 : ip+1;
	++linesRead;
	next();
RDLINE_ORDERED_:
	//stk0 has current read index, stk1 has vector.size()
	if (stk0.u.i < stk1.u.i){
		for (auto &f : files)
			f->readlineat(f->positions[sortIdxs[stk0.u.i++]]);
		++ip;
	} else {
		ip = op->p1;
	} next();
PREP_REREAD_:
	push();
	stk0.u.i = sortIdxs.size();
	push();
	stk0.u.i = 0;
	nexti();
NUL_TO_STR_:
	if (stk0.isnull()){
		stk0 = dat{ { .s = (char*) calloc(1,1) }, T_STRING|MAL, 0 };
	} nexti();
SAVESORTN_:
	normalSortVals[op->p1].push_back(stk0.u);
	pop();
	nexti();
SAVESORTS_:
	normalSortVals[op->p1].push_back(stk0.heap().u);
	pop();
	nexti();
SAVEANDCHAIN_:
	{
		auto& file = files[op->p2];
		auto& chain = file->andchains[op->p1];
		int size = chain.values.size();
		chain.positiions.push_back(file->pos);
		for (int i=0; i<size; ++i){
			chain.values[i].push_back(stkt(size-i-1).heap().u);
		}
		stacktop -= size;
	} nexti();
SAVEVALPOS_:
	{
		iTemp1 = op->p2-1;
		auto& f = files[op->p1];
		for (auto& vpv : f->joinValpos){
			vpv.push_back(valpos{stkt(iTemp1).heap().u, f->pos});
			--iTemp1;
		}
		stacktop -= op->p2;
	} nexti();
JOINSET_EQ_AND_:
	{
		auto& chain = files[op->p1]->andchains[op->p2];
		int comp1Functype = chain.functionTypes[0];
		auto& lessfunc = uLessFuncs[comp1Functype];
		int chsize = chain.values.size();
		dat& compval = stkt(chsize-1);
		auto& uvec = chain.values[0];
		int r = chain.indexes.size()-1;
		int l = 0;
		int m;
		while (l < r){
			m = (l+r)/2;
			if (lessfunc(uvec[chain.indexes[m]], compval.u))
				l = m + 1;
			else
				r = m;
		}
		joinSetStack.emplace_front();
		while (l < chain.indexes.size()){
			int valIdx = chain.indexes[l];
			if (!uEqFuncs[comp1Functype](uvec[valIdx], compval.u))
				break;
			for (int i=1; i<chsize; ++i)
				if (!uComparers[chain.relops[i]][chain.functionTypes[i]](chain.values[i][valIdx], stkt(chsize-1-i).u)^chain.negations[i])
					goto skipjoin;
			joinSetStack.front().insert(chain.positiions[valIdx]);
			skipjoin:
			++l;
		}
		stacktop -= chsize;
	} nexti();
JOINSET_GRT_AND_:
	{
		auto& chain = files[op->p1]->andchains[op->p2];
		int comp1Functype = chain.functionTypes[0];
		auto& grtfunc = uGrtFuncs[comp1Functype];
		int chsize = chain.values.size();
		dat& compval = stkt(chsize);
		auto& uvec = chain.values[0];
		int r = chain.indexes.size()-1;
		int l = 0;
		int m;
		while (l < r){
			m = (l+r)/2;
			if (grtfunc(uvec[chain.indexes[m]], compval.u))
				r = m;
			else
				l = m + 1;
		}
		joinSetStack.emplace_front();
		if (stk0.u.i) // GTE
			for (m = r-1, l = chain.indexes[m]; m >= 0; l = chain.indexes[--m]){
				if (!uEqFuncs[comp1Functype](uvec[l], compval.u))
					break;
				for (int i=1; i<chsize; ++i)
					if (!uComparers[chain.relops[i]][chain.functionTypes[i]](chain.values[i][l], stkt(chsize-i).u)^chain.negations[i])
						goto skipjoin4;
				joinSetStack.front().insert(chain.positiions[l]);
				skipjoin4:;
			}
		while (r < chain.indexes.size()){ // GT
			int valIdx = chain.indexes[r];
			for (int i=1; i<chsize; ++i)
				if (!uComparers[chain.relops[i]][chain.functionTypes[i]](chain.values[i][valIdx], stkt(chsize-i).u)^chain.negations[i])
					goto skipjoin5;
			joinSetStack.front().insert(chain.positiions[valIdx]);
			skipjoin5:
			++r;
		}
		stacktop -= (chsize+1);
	} nexti();
JOINSET_LESS_AND_:
	{
		auto& chain = files[op->p1]->andchains[op->p2];
		int comp1Functype = chain.functionTypes[0];
		auto& lessfunc = uLessFuncs[comp1Functype];
		int chsize = chain.values.size();
		dat& compval = stkt(chsize);
		auto& uvec = chain.values[0];
		int r = chain.indexes.size()-1;
		int l = 0;
		int m;
		while (l < r){
			m = (l+r)/2;
			if (lessfunc(uvec[chain.indexes[m]], compval.u))
				l = m + 1;
			else
				r = m;
		}
		joinSetStack.emplace_front();
		if (stk0.u.i) // LTE
			for (m = l, r = chain.indexes[m]; m < chain.indexes.size(); r = chain.indexes[++m]){
				if (!uEqFuncs[comp1Functype](uvec[r], compval.u))
					break;
				for (int i=1; i<chsize; ++i)
					if (!uComparers[chain.relops[i]][chain.functionTypes[i]](chain.values[i][r], stkt(chsize-i).u)^chain.negations[i])
						goto skipjoin3;
				joinSetStack.front().insert(chain.positiions[r]);
				skipjoin3:;
			}
		--l;
		while (l >= 0){ // LT
			int valIdx = chain.indexes[l];
			for (int i=1; i<chsize; ++i)
				if (!uComparers[chain.relops[i]][chain.functionTypes[i]](chain.values[i][valIdx], stkt(chsize-i).u)^chain.negations[i])
					goto skipjoin2;
			joinSetStack.front().insert(chain.positiions[valIdx]);
			skipjoin2:
			--l;
		}
		stacktop -= (chsize+1);
	} nexti();
JOINSET_GRT_:
	{
		auto& grtfunc = uGrtFuncs[op->p3];
		auto& vpvector = files[op->p1]->joinValpos[op->p2];
		int r = vpvector.size()-1;
		int l = 0;
		int m;
		while (l < r){
			m = (l+r)/2;
			if (grtfunc(vpvector[m].val, stk1.u))
				r = m;
			else
				l = m + 1;
		}
		joinSetStack.emplace_front();
		if (stk0.u.i) // GTE
			for (m = r-1; m >= 0 && uEqFuncs[op->p3](vpvector[m].val, stk1.u); --m)
				joinSetStack.front().insert(vpvector[m].pos);
		while (r < vpvector.size()) // GT
			joinSetStack.front().insert(vpvector[r++].pos);
	}
	pop(); //stk0 tells if GTE
	pop(); //comp value in stk1
	nexti();
JOINSET_LESS_:
	{
		auto& lessfunc = uLessFuncs[op->p3];
		auto& vpvector = files[op->p1]->joinValpos[op->p2];
		int r = vpvector.size()-1;
		int l = 0;
		int m;
		while (l < r){
			m = (l+r)/2;
			if (lessfunc(vpvector[m].val, stk1.u))
				l = m + 1;
			else
				r = m;
		}
		joinSetStack.emplace_front();
		if (stk0.u.i) // LTE
			for (m = l; m < vpvector.size() && uEqFuncs[op->p3](vpvector[m].val, stk1.u); ++m)
				joinSetStack.front().insert(vpvector[m].pos);
		--l;
		while (l >= 0)
			joinSetStack.front().insert(vpvector[l--].pos);
	}
	pop(); //stk0 tells if LTE
	pop(); //comp value in stk1
	nexti();
JOINSET_EQ_:
	{
		auto& lessfunc = uLessFuncs[op->p3];
		auto& vpvector = files[op->p1]->joinValpos[op->p2];
		int r = vpvector.size()-1;
		int l = 0;
		int m;
		while (l < r){
			m = (l+r)/2;
			if (lessfunc(vpvector[m].val, stk0.u))
				l = m + 1;
			else
				r = m;
		}
		joinSetStack.emplace_front();
		while (l < vpvector.size() && uEqFuncs[op->p3](vpvector[l].val, stk0.u))
			joinSetStack.front().insert(vpvector[l++].pos);
	}
	pop();
	nexti();
AND_SET_:
	{
		bset<i64> tempset1(move(joinSetStack.front()));
		joinSetStack.pop_front();
		auto& target = joinSetStack.front();
		bset<i64> tempset2(move(target));
		set_intersection(tempset1.begin(), tempset1.end(), tempset2.begin(), tempset2.end(),
				inserter(target, target.begin()));
	} nexti();
XOR_SET_:
	{
		bset<i64> tempset1(move(joinSetStack.front()));
		joinSetStack.pop_front();
		auto& target = joinSetStack.front();
		bset<i64> tempset2(move(target));
		set_symmetric_difference(tempset1.begin(), tempset1.end(), tempset2.begin(), tempset2.end(),
				inserter(target, target.begin()));
	} nexti();
OR_SET_:
	{
		bset<i64> tempset(move(joinSetStack.front()));
		joinSetStack.pop_front();
		auto& target = joinSetStack.front();
		for (auto loc : tempset)
			target.insert(loc);
	} nexti();
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
	} next();
SAVEPOS_:
	for (auto &f : files)
		f->positions.push_back(f->pos);
	nexti();
SORT_ANDCHAIN_:
	{
		auto& chain = files[op->p1]->andchains[op->p2];
		chain.indexes.resize(chain.positiions.size());
		iota(begin(chain.indexes), end(chain.indexes), 0);
		function<bool (const int,const int)> uComparers[] = {
			[&](const int a, const int b) { return chain.values[0][a].i < chain.values[0][b].i; },
			[&](const int a, const int b) { return chain.values[0][a].f < chain.values[0][b].f; },
			[&](const int a, const int b) { return strcmp(chain.values[0][a].s, chain.values[0][b].s) < 0; },
		};
		bs_sort::parallel_sort(chain.indexes.begin(), chain.indexes.end(), uComparers[chain.functionTypes[0]]);
	} nexti();
SORTVALPOS_:
	{
		auto& vpvector = files[op->p1]->joinValpos[op->p2];
		function<bool (const valpos&,const valpos&)> valposComparers[] = {
			[&](const valpos &a, const valpos &b) { return a.val.i < b.val.i; },
			[&](const valpos &a, const valpos &b) { return a.val.f < b.val.f; },
			[&](const valpos &a, const valpos &b) {
				return a.val.s && b.val.s && strcmp(a.val.s, b.val.s)<0;
				//TODO: find out how a null is getting in here
			},
		};
		bs_sort::parallel_sort(vpvector.begin(), vpvector.end(), valposComparers[op->p3]);
	} nexti();

//normal sorter
SORT_:
	{
		sortIdxs.resize(files[0]->positions.size());
		iota(sortIdxs.begin(), sortIdxs.end(), 0);
		sortcomp cmp(this);
		bs_sort::parallel_sort(sortIdxs.begin(), sortIdxs.end(), [&](int a, int b){ return cmp(a,b); });
	}
	nexti();
//group sorter - p1 is sort index
GSORT_:
	{
		gsortcomp cmp(this, op->p1);
		sort(groupSorter.begin(), groupSorter.end(), [&](grprow& a, grprow& b){ return cmp(a,b); });
	}
	nexti();

//math operations
IADD_:
	ifneithernull stk1.u.i += stk0.u.i;
	pop();
	nexti();
FADD_:
	ifneithernull stk1.u.f += stk0.u.f;
	pop();
	nexti();
TADD_:
	strplus(stk1, stk0);
	pop();
	nexti();
DRADD_:
	ifneithernull {
		stk1.u.i += stk0.u.i;
		stk1.b = T_DURATION; }
		if (stk1.z && stk0.z)
			stk1.z += stk0.z;
		else
			stk1.z = 0;
	pop();
	nexti();
DTADD_:
	ifneithernull {
		auto p = getfirst(stacktop, T_DATE);
		if (p.second->z == 0){
			stk1.u.i += stk0.u.i;
		} else {
			secs_to_tm(sec(p.first->u.i), &tmTemp);
			auto mcs = mcs(p.first->u.i);
			tmTemp.tm_year += p.second->z;
			stk1.u.i = mktimegm(&tmTemp) + mcs;
		}
		stk1.b = T_DATE;
		stk1.z = 0;
	}
	pop();
	nexti();
ISUB_:
	ifneithernull stk1.u.i -= stk0.u.i;
	pop();
	nexti();
FSUB_:
	ifneithernull stk1.u.f -= stk0.u.f;
	pop();
	nexti();
DTSUB_:
	ifneithernull {
		if (stk0.b == T_DURATION){
			if (stk0.z){
				auto mcs = mcs(stk1.u.i);
				secs_to_tm(sec(stk1.u.i), &tmTemp);
				tmTemp.tm_year -= stk0.z;
				stk1.u.i = mktimegm(&tmTemp) + mcs;
			} else {
				stk1.u.i -= stk0.u.i;
			}
			stk1.b = T_DATE;
		} else {
			stk1 = dat{ { .i = stk1.u.i - stk0.u.i }, T_DURATION };
		}
	}
	pop();
	nexti();
DRSUB_:
	ifneithernull {
		stk1.u.i -= stk0.u.i;
		stk1.b = T_DURATION;
		if (stk0.z && stk1.z)
			stk1.z -= stk0.z;
		else
			stk1.z = 0;
	}
	pop();
	nexti();
IMULT_:
	ifneithernull stk1.u.i *= stk0.u.i;
	pop();
	nexti();
FMULT_:
	ifneithernull stk1.u.f *= stk0.u.f;
	pop();
	nexti();
DRMULT_:
	ifneithernull {
		auto p = getfirst(stacktop, T_DURATION);
		if (p.second->b == T_INT){
			i64Temp = p.first->u.i * p.second->u.i;
			iTemp1 = p.first->z * p.second->u.i;
			stk1.u.i = i64Temp;
			stk1.z = iTemp1;
		} else {
			stk1.u.i = p.first->u.i * p.second->u.f;
			stk1.z = 0;
		}
		stk1.b = T_DURATION;
	}
	pop();
	nexti();
IDIV_:
	if (stk0.isnull() || stk1.isnull() || stk0.u.i==0) { stk1.setnull(); }
	else stk1.u.i /= stk0.u.i;
	pop();
	nexti();
FDIV_:
	if (stk0.isnull() || stk1.isnull() || stk0.u.f==0) { stk1.setnull(); }
	else stk1.u.f /= stk0.u.f;
	pop();
	nexti();
DRDIV_:
	if (stk0.isnull() || stk1.isnull() || stk0.u.i==0) { stk1.setnull(); }
	else {
		if (stk0.b == T_INT)
			stk1.u.i /= stk0.u.i;
		else
			stk1.u.i /= stk0.u.f;
	}
	stk1.z = 0;
	pop();
	nexti();
INEG_:
	ifnotnull
	if (stk0.u.i != 0){ stk0.u.i *= -1; stk0.z *= -1; };
	nexti();
FNEG_:
	ifnotnull
	if (stk0.u.i != 0) stk0.u.f *= -1;
	nexti();
PNEG_:
	stk0.u.p ^= true;
	nexti();
FPOW_:
	ifneithernull stk1.u.f = pow(stk1.u.f, stk0.u.f);
	pop();
	nexti();
IPOW_:
	ifneithernull stk1.u.i = pow(stk1.u.i, stk0.u.i);
	pop();
	nexti();
IMOD_:
	ifneithernull stk1.u.i = stk1.u.i % stk0.u.i;
	pop();
	nexti();
FMOD_:
	ifneithernull stk1.u.f = fmod(stk1.u.f, stk0.u.f);
	pop();
	nexti();

//comparisions - p1 determines how far down the stack to leave the result
// p2 is negator
IEQ_:
	iTemp1 = stk0.isnull();
	iTemp2 = stk1.isnull();
	if (iTemp1 ^ iTemp2) stkt(op->p1).u.p = false^op->p2;
	else if (iTemp1 & iTemp2) stkt(op->p1).u.p = true^op->p2;
	else stkt(op->p1).u.p = (stk1.u.i == stk0.u.i)^op->p2;
	stacktop -= op->p1;
	nexti();
FEQ_:
	iTemp1 = stk0.isnull();
	iTemp2 = stk1.isnull();
	if (iTemp1 ^ iTemp2) stkt(op->p1).u.p = false^op->p2;
	else if (iTemp1 & iTemp2) stkt(op->p1).u.p = true^op->p2;
	else stkt(op->p1).u.p = (stk1.u.f == stk0.u.f)^op->p2;
	stacktop -= op->p1;
	nexti();
TEQ_:
	iTemp1 = stk0.isnull();
	iTemp2 = stk1.isnull();
	if (iTemp1^iTemp2){ //one null
		stkt(op->p1).freedat();
		stkt(op->p1).u.p = false^op->p2;
	} else if (iTemp1 & iTemp2) { //both null
		stkt(op->p1).u.p = true^op->p2;
	} else { //none null
		boolTemp = (strcmp(stk1.u.s, stk0.u.s) == 0)^op->p2;
		stkt(op->p1).freedat();
		stkt(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	nexti();
ILEQ_:
	if (stk0.isnull() || stk1.isnull()) stkt(op->p1).u.p = false^op->p2;
	else stkt(op->p1).u.p = (stk1.u.i <= stk0.u.i)^op->p2;
	stacktop -= op->p1;
	nexti();
FLEQ_:
	if (stk0.isnull() || stk1.isnull()) stkt(op->p1).u.p = false^op->p2;
	else stkt(op->p1).u.p = (stk1.u.f <= stk0.u.f)^op->p2;
	stacktop -= op->p1;
	nexti();
TLEQ_:
	if (stk0.isnull() || stk1.isnull()) {
		stkt(op->p1).freedat();
		stkt(op->p1).u.p = false^op->p2;
	} else {
		boolTemp = (strcmp(stk1.u.s, stk0.u.s) <= 0)^op->p2;
		stkt(op->p1).freedat();
		stkt(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	nexti();
ILT_:
	if (stk0.isnull() || stk1.isnull()) stkt(op->p1).u.p = false^op->p2;
	else stkt(op->p1).u.p = (stk1.u.i < stk0.u.i)^op->p2;
	stacktop -= op->p1;
	nexti();
FLT_:
	if (stk0.isnull() || stk1.isnull()) stkt(op->p1).u.p = false^op->p2;
	else stkt(op->p1).u.p = (stk1.u.f < stk0.u.f)^op->p2;
	stacktop -= op->p1;
	nexti();
TLT_:
	if (stk0.isnull() || stk1.isnull()){
		stkt(op->p1).freedat();
		stkt(op->p1).u.p = false^op->p2;
	} else {
		boolTemp = (strcmp(stk1.u.s, stk0.u.s) < 0)^op->p2;
		stkt(op->p1).freedat();
		stkt(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	nexti();
//nulls already checked for betweens
BETWEEN_:
	i64Temp = datunionDiffs[op->p1](stkt(2).u, stk1.u);
	if (i64Temp >= 0){
		boolTemp = i64Temp == 0 || datunionDiffs[op->p1](stkt(2).u, stk0.u) <= 0;
	} else {
		boolTemp = datunionDiffs[op->p1](stkt(2).u, stk0.u) >= 0;
	}
	stacktop -= 2;
	stk0.freedat();
	stk0.u.p = boolTemp^op->p2;
	nexti();
LIKE_:
	if (stk0.isnull()){
		stk0.u.p = op->p2;
	} else {
		iTemp1 = !regexec(q->dataholder[op->p1].u.r, stk0.u.s, 0, 0, 0)^op->p2;
		stk0.freedat();
		stk0.u.p = iTemp1;
	} nexti();
INSUBQUERY_:
	stk0.u.p = q->subqueries[op->p1].resultSet->contains(stk0);
	nexti();

PUSH_N_:
	push();
	stk0.u.i = op->p1;
	nexti();
PUSH_:
	push();	
	nexti();
POP_:
	pop();
	nexti();
POPCPY_:
	stk1.mov(stk0);
	pop();
	nexti();
NULFALSE_:
	if (stk0.isnull()){
		stacktop -= op->p2;
		stk0.freedat();
		stk0.u.p = false;
		ip = op->p1;
	} else ++ip;
	next();

//type conversions
CVIS_:
	ifnotnull{
		stk0.z = sprintf(bufTemp, "%lld", stk0.u.i);
		stk0.u.s = (char*) malloc(stk0.z+1);
		memcpy(stk0.u.s, bufTemp, stk0.z+1);
		stk0.b = T_STRING|MAL;
	} nexti();
CVFS_:
	ifnotnull{
		stk0.z = sprintf(bufTemp, "%.10g", stk0.u.f);
		stk0.u.s = (char*) malloc(stk0.z+1);
		memcpy(stk0.u.s, bufTemp, stk0.z+1);
		stk0.b = T_STRING|MAL;
	} nexti();
CVFI_:
	ifnotnull{
		stk0.u.i = stk0.u.f;
		stk0.b = T_INT;
	} nexti();
CVIF_:
	ifnotnull{
		stk0.u.f = stk0.u.i;
		stk0.b = T_FLOAT;
	} nexti();
CVSI_:
	ifnotnull{
		auto s = stk0.u.s;
		i64Temp = strtoll(s, &cstrTemp, 10);
		stk0.freedat();
		stk0.u.i = i64Temp;
		stk0.b = T_INT;
		if (cstrTemp == s){ stk0.setnull(); }
	} nexti();
CVSF_:
	ifnotnull{
		fTemp = strtod(stk0.u.s, &cstrTemp);
		stk0.freedat();
		stk0.u.f = fTemp;
		stk0.b = T_FLOAT;
		if (*cstrTemp){ stk0.setnull(); }
	} nexti();
CVSDT_:
	ifnotnull{
		iTemp1 = dateparse(stk0.u.s, &i64Temp, &iTemp2, stk0.z);
		stk0.freedat();
		stk0.u.i = i64Temp;
		stk0.b = T_DATE;
		stk0.z = iTemp2;
		if (iTemp1) { stk0.setnull(); }
	} nexti();
CVSDR_:
	ifnotnull{
		parseDuration(stk0.u.s, stk0);
	} nexti();
CVDRS_:
	ifnotnull{
		cstrTemp = (char*) malloc(24);
		durstring(stk0, cstrTemp);
		stk0.u.s = cstrTemp;
		stk0.b = T_STRING|MAL;
		stk0.z = strlen(stk0.u.s);
	} nexti();
CVDTS_:
	ifnotnull{
		//TODO: make version of datestring that writes directly to arg buf
		stk0.u.s = strdup(datestring(stk0.u.i));
		stk0.b = T_STRING|MAL;
		stk0.z = strlen(stk0.u.s);
	} nexti();

JMP_:
	ip = op->p1;
	next();
JMPCNT_:
	ip = (totalPrinted < quantityLimit) ? op->p1 : ip+1;
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
	if (stk0.isnull()){
		pop();
		++ip;
	} else {
		ip = op->p1;
	} next();

PRINTJSON_:
	if (numJsonPrinted < jsonresult->rowlimit){
		iTemp1 = 0;	
		string jbuf = "[";
		printjsonfield:
		torow[iTemp1].appendToJsonBuffer(jbuf);
		if (++iTemp1 < torowSize){
			jbuf += ',';
			goto printjsonfield;
		}
		jbuf += ']';
		++numJsonPrinted;
		jsonresult->Vals.push_back(move(jbuf));
	} else {
		jsonresult->clipped = jsonresult->rowlimit;
	}
	jsonresult->numrows++;
	totalPrinted += op->p1; //in case not csv printing
	nexti();
PRINTCSV_HEADER_:
	iTemp1 = 0;	
	for (auto &name : q->colspec.colnames){
		if (name.find(',') != string::npos){
			outbuf += '"';
			outbuf += name;
			outbuf += '"';
		} else {
			outbuf += name;
		}
		if (++iTemp1 < torowSize)
			outbuf += ',';
	}
	outbuf += '\n';
	nexti();
PRINTCSV_:
	iTemp1 = 0;	
	printcsvfield:
	torow[iTemp1].appendToCsvBuffer(outbuf);
	if (outbuf.size() > 900){
		//TODO: clear and stop messager stderr output
		csvOutput << outbuf;
		outbuf.clear();
	}
	if (++iTemp1 < torowSize){
		outbuf += ',';
		goto printcsvfield;
	}
	outbuf += '\n';
	++totalPrinted;
	nexti();
PRINTBOX_:
	termbox.addrow(torow);
	++totalPrinted;
	nexti();
PRINTBTREE_:
	for (int i=0; i<torowSize; ++i)
		vsets[op->p1]->insert(torow[i]);
	nexti();

//distinct checkers
LDDIST_:
	push();
	stk0 = distinctVal; //distinctVal never owns heap data
	nexti();
DIST_:
	iTemp1 = op->p3 ? groupTemp->meta.distinctSetIdx : 0;
	if (vsets[op->p2+iTemp1]->insert(stk0)) {
		distinctVal = stk0;
		++ip;
	} else {
		ip = op->p1;
	}
	pop();
	next();

//functions
FUNC_ABSI_:
	ifnotnull stk0.u.i = abs(stk0.u.i);
	nexti();
FUNC_ABSF_:
	ifnotnull stk0.u.f = abs(stk0.u.f);
	nexti();
FUNC_INC_:
	q->dataholder[op->p1].u.f++;
	push();
	stk0 = q->dataholder[op->p1];
	nexti();
FUNC_ENCCHA_:
	ifnotnull q->crypt.chachaEncrypt(stk0, op->p1);
	nexti();
FUNC_DECCHA_:
	ifnotnull q->crypt.chachaDecrypt(stk0, op->p1);
	nexti();

FUNC_NOW_:
	push();
	stk0 = nowdat;
	nexti();
FUNC_NOWGM_:
	push();
	stk0 = nowgmdat;
	nexti();

#define get_tm() \
	skipnull(); \
	secs_to_tm(sec(stk0.u.i), &tmTemp);
FUNC_YEAR_:
	get_tm();
	stk0 = { { .i = tmTemp.tm_year + 1900 }, T_INT };
	nexti();
FUNC_MONTH_:
	get_tm();
	stk0 = { { .i = tmTemp.tm_mon+1 }, T_INT };
	nexti();
FUNC_WEEK_:
	get_tm();
	stk0 = { { .i = ((tmTemp.tm_yday+1) / 7) + 1 }, T_INT };
	nexti();
FUNC_YDAY_:
	get_tm();
	stk0 = { { .i = tmTemp.tm_yday+1 }, T_INT };
	nexti();
FUNC_MDAY_:
	get_tm();
	stk0 = { { .i = tmTemp.tm_mday }, T_INT };
	nexti();
FUNC_WDAY_:
	get_tm();
	stk0 = { { .i = tmTemp.tm_wday+1 }, T_INT };
	nexti();
FUNC_HOUR_:
	get_tm();
	stk0 = { { .i = tmTemp.tm_hour }, T_INT };
	nexti();
FUNC_MINUTE_:
	get_tm();
	stk0 = { { .i = tmTemp.tm_min }, T_INT };
	nexti();
FUNC_SECOND_:
	get_tm();
	stk0 = { { .i = tmTemp.tm_sec }, T_INT };
	nexti();
FUNC_WDAYNAME_:
	get_tm();
	stk0 = { { .s = daynames[tmTemp.tm_wday] }, T_STRING, daylens[tmTemp.tm_wday] };
	nexti();
FUNC_MONTHNAME_:
	get_tm();
	stk0 = { { .s = monthnames[tmTemp.tm_mon] }, T_STRING, monthlens[tmTemp.tm_mon] };
	nexti();
FUNC_ROUND_:
	ifnotnull stk0.u.f = round(stk0.u.f, op->p1);
	nexti();
FUNC_CIEL_:
	ifnotnull stk0.u.f = ceil(stk0.u.f, op->p1);
	nexti();
FUNC_FLOOR_:
	ifnotnull stk0.u.f = floor(stk0.u.f, op->p1);
	nexti();
FUNC_ACOS_:
	ifnotnull {
		stk0.u.f = acos(stk0.u.f);
		if (op->p2 && isnan(stk0.u.f)) stk0.setnull();
	}
	nexti();
FUNC_ASIN_:
	ifnotnull {
		stk0.u.f = asin(stk0.u.f);
		if (op->p2 && isnan(stk0.u.f)) stk0.setnull();
	}
	nexti();
FUNC_ATAN_:
	ifnotnull {
		stk0.u.f = atan(stk0.u.f);
		if (op->p2 && isnan(stk0.u.f)) stk0.setnull();
	}
	nexti();
FUNC_COS_:
	ifnotnull stk0.u.f = cos(stk0.u.f);
	nexti();
FUNC_SIN_:
	ifnotnull stk0.u.f = sin(stk0.u.f);
	nexti();
FUNC_TAN_:
	ifnotnull stk0.u.f = tan(stk0.u.f);
	nexti();
FUNC_EXP_:
	ifnotnull stk0.u.f = exp(stk0.u.f);
	nexti();
FUNC_LOG_:
	ifnotnull {
		stk0.u.f = log(stk0.u.f);
		if (op->p2 && isinf(stk0.u.f)) stk0.setnull();
	} nexti();
FUNC_LOG2_:
	ifnotnull {
		stk0.u.f = log2(stk0.u.f);
		if (op->p2 && isinf(stk0.u.f)) stk0.setnull();
	} nexti();
FUNC_LOG10_:
	ifnotnull {
		stk0.u.f = log10(stk0.u.f);
		if (op->p2 && isinf(stk0.u.f)) stk0.setnull();
	} nexti();
FUNC_SQRT_:
	ifnotnull {
		stk0.u.f = sqrt(stk0.u.f);
		if (op->p2 && isnan(stk0.u.f)) stk0.setnull();
	} nexti();
FUNC_CBRT_:
	ifnotnull stk0.u.f = cbrt(stk0.u.f);
	nexti();
FUNC_RAND_:
	push();
	stk0.u.f = rng();
	stk0.b = T_FLOAT;
	nexti();
FUNC_UPPER_:
	ifnotnull {
		stk0 = stk0.heap();
		for(auto c = stk0.u.s; *c; ++c) *c = toupper(*c);
	} nexti();
FUNC_LOWER_:
	ifnotnull {
		stk0 = stk0.heap();
		for(auto c = stk0.u.s; *c; ++c) *c = tolower(*c);
	} nexti();
FUNC_BASE64_ENCODE_:
	ifnotnull {
		u32 newsize = encsize(stk0.z);
		auto newstring = (char*)malloc(newsize+1);
		base64_encode((BYTE*)stk0.u.s, (BYTE*) newstring, (int)stk0.z, 0);
		newstring[newsize] = 0;
		stk0.freedat();
		stk0 = dat{ {.s = newstring}, T_STRING|MAL, newsize};
	} nexti();
FUNC_BASE64_DECODE_:
	ifnotnull {
		auto newstring = (char*)malloc(stk0.z+1);
		stk0.z = base64_decode((BYTE*)stk0.u.s, (BYTE*)newstring, (int)stk0.z);
		newstring = (char*)realloc(newstring, stk0.z+1);
		newstring[stk0.z] = 0;
		stk0.freedat();
		stk0 = dat{ {.s = newstring}, T_STRING|MAL, stk0.z};
	} nexti();
FUNC_HEX_ENCODE_:
FUNC_HEX_DECODE_:
FUNC_LEN_:
	ifnotnull {
		auto sz = stk0.z;
		stk0.freedat();
		stk0 = dat{ {.f = (double)sz}, T_FLOAT };
	} else {
		stk0 = dat{ {.f = 0}, T_FLOAT };
	} nexti();
FUNC_SUBSTR_:
	ifnotnull {
		if (op->p3){
			auto d = stk0;
			regmatch_t pm;
			if (!regexec(q->dataholder[op->p1].u.r, d.u.s, 1, &pm, 0)){
				stk0.z = pm.rm_eo - pm.rm_so;
				stk0.u.s = (char*)malloc(stk0.z+1);
				stk0.b |= MAL;
				strncpy(stk0.u.s, d.u.s+pm.rm_so, stk0.z);
				stk0.u.s[stk0.z] = 0;
			} else
				stk0.setnull();
			d.freedat();
		} else {
			if (op->p1+op->p2 <= stk0.z) {
				auto d = stk0;
				stk0.u.s = (char*)malloc(op->p2+1);
				stk0.b |= MAL;
				stk0.z = op->p2;
				strncpy(stk0.u.s, d.u.s+op->p1, op->p2);
				stk0.u.s[op->p2] = 0;
				d.freedat();
			} else if (op->p1 < stk0.z) {
				auto d = stk0;
				stk0.u.s = strdup(d.u.s+op->p1);
				stk0.z = strlen(stk0.u.s);
				stk0.b |= MAL;
				d.freedat();
			} else
				stk0.freedat();
		}
	} nexti();
FUNC_SIP_:
	ifnotnull sip(stk0);
	nexti();
FUNC_MD5_:
	ifnotnull md5(stk0);
	nexti();
FUNC_SHA1_:
	ifnotnull sha1(stk0);
	nexti();
FUNC_SHA256_:
	ifnotnull sha256(stk0);
	nexti();
FUNC_FORMAT_:
	ifnotnull {
		auto fmt = q->dataholder[op->p1].u.s;
		auto dstr = datestringfmt(stk0.u.i, fmt);
		iTemp1 = strlen(dstr);
		if (iTemp1)
			stk0 = dat{ {.s = strdup(dstr)}, T_STRING|MAL, (u32)iTemp1 };
	}
	nexti();

//aggregates
LDCOUNT_:
	++stacktop;
	stk0.mov(midrow[op->p1]);
	if (stk0.isnull())
		stk0 = dat{ { .f = 0 }, T_FLOAT };
	nexti();
LDSTDVI_:
	nexti(); //TODO
LDSTDVF_:
	push();
	if (!midrow[op->p1].isnull())
		stk0 = stdvs[midrow[op->p1].u.i].eval(op->p2);
	nexti();
LDAVGI_:
	push();
	if (auto& d = midrow[op->p1]; !d.isnull() && d.z){
		stk0.u.i = d.u.i / d.z;
		stk0.b = T_INT;
	} nexti();
LDAVGF_:
	push();
	if (auto& d = midrow[op->p1]; !d.isnull() && d.z){
		stk0.u.f = d.u.f / d.z;
		stk0.b = T_FLOAT;
	} nexti();
AVGI_:
	ifnotnull torow[op->p1].z++;
SUMI_:
	ifnotnull{
		torow[op->p1].u.i += stk0.u.i;
		torow[op->p1].b = stk0.b;
	}
	pop();
	nexti();
AVGF_:
	ifnotnull
		torow[op->p1].z++;
SUMF_:
	ifnotnull{
		torow[op->p1].u.f += stk0.u.f;
		torow[op->p1].b = stk0.b;
	}
	pop();
	nexti();
STDVI_:
	//TODO - duration
	pop();
	nexti();
STDVF_:
	{
		ifnotnull{
			if (torow[op->p1].isnull()){
				torow[op->p1] = {{.i =static_cast<i64>(stdvs.size())},T_INT};
				stdvs.emplace_back(stk0.u.f);
			} else
				stdvs[torow[op->p1].u.i].numbers.push_front(stk0.u.f);
		}
	}
	pop();
	nexti();
COUNT_:
	if (op->p2){ //count(*)
		torow[op->p1].b = T_FLOAT;
		torow[op->p1].u.f++;
	} else {
		ifnotnull {
			torow[op->p1].b = T_FLOAT;
			torow[op->p1].u.f++;
		}
		pop();
	} nexti();
MINI_:
	if (dat& t = torow[op->p1]; t.isnull() || (!stk0.isnull() && t.u.i > stk0.u.i))
		t = stk0;
	pop();
	nexti();
MINF_:
	if (dat& t = torow[op->p1]; t.isnull() || (!stk0.isnull() && t.u.f > stk0.u.f))
		t = stk0;
	pop();
	nexti();
MINS_:
	if (dat& t = torow[op->p1]; t.isnull() || (!stk0.isnull() && strcmp(t.u.s, stk0.u.s) > 0)){
		t.freedat();
		t = stk0.heap();
	}
	pop();
	nexti();
MAXI_:
	if (dat& t = torow[op->p1]; t.isnull() || (!stk0.isnull() && t.u.i < stk0.u.i))
		t = stk0;
	pop();
	nexti();
MAXF_:
	if (dat& t = torow[op->p1]; t.isnull() || (!stk0.isnull() && t.u.f < stk0.u.f))
		t = stk0;
	pop();
	nexti();
MAXS_:
	if (dat& t = torow[op->p1]; t.isnull() || (!stk0.isnull() && strcmp(t.u.s, stk0.u.s) < 0)){
		t.freedat();
		t = stk0.heap();
	}
	pop();
	nexti();
GETGROUP_:
	groupTemp = groupTree.get();
	for (int i=op->p1; i >= 0; --i)
		groupTemp = &groupTemp->nextGroup(stkt(i));
	torow = groupTemp->getRow(this);
	stacktop -= (op->p1 + 1);
	nexti();
ONEGROUP_:
	torow = destrow.data();
	midrow = onegroup.data();
	nexti();
ROOTMAP_:
	groupItstk[op->p1]   = groupTree->getMap().begin();
	groupItstk[op->p1+1] = groupTree->getMap().end();
	torow = destrow.data();
	nexti();
NEXTMAP_:
	if (groupItstk[op->p2-2] == groupItstk[op->p2-1]){
		ip = op->p1;
	} else {
		// set top of iterator stack to next map
		groupTemp = &(groupItstk[op->p2-2]++)->second;
		groupItstk[op->p2]   = groupTemp->getMap().begin();
		groupItstk[op->p2+1] = groupTemp->getMap().end();
		++ip;
	} next();
NEXTVEC_:
	if (groupItstk[op->p2] == groupItstk[op->p2+1]){
		ip = op->p1;
	} else {
		groupTemp = &(groupItstk[op->p2]++)->second;
		midrow = groupTemp->getVec();
		++ip;
	} next();
ADD_GROUPSORT_ROW_:
	torow = (dat*) calloc(sortgroupsize, sizeof(dat));
	groupSorter.emplace_back(torow);
	nexti();
FREE_MIDROW_:
	groupTemp->freeRow();
	nexti();

READ_NEXT_GROUP_:
	if (stk0.u.i >= (i64)groupSorter.size()){
		ip = op->p1;
	} else {
		torow = groupSorter[stk0.u.i++].get();
		++ip;
	} next();

#define R (char*)
START_MESSAGE_:
	linesRead = 0;
	switch(op->p1){
	case 0: updates.start(R "Scanned %d lines", &linesRead, 0); break;
	case 1: updates.start(R "Found %d results", &totalPrinted, 0); break;
	case 2: updates.say  (R "Sorting", 0, 0); break;
	case 3: updates.start(R "Read %d lines", &linesRead,0); break;
	case 4: updates.start(R "Read %d lines, Found %d results", &linesRead, &totalPrinted); break;
	case 5: updates.start(R "Scanned %d lines from join file", &linesRead, 0); break;
	case 6: updates.say  (R "Processing indexes", 0, 0); break;
	case 7: updates.start(R "Read %d lines from file1, found %d join results", &linesRead, &totalPrinted); break;
	} nexti();
STOP_MESSAGE_:
	updates.stop();
	nexti();

ENDRUN_:
	updates.stop();
	csvOutput << outbuf;
	csvOutput.flush();
	return;

//error opcodes
CVER_:
CVNO_:
	error("Invalid opcode");
}

void vmachine::startSubqueries(){
	for (auto& sq : q->subqueries){
		switch(sq.query->isSubquery){
		case SQ_INLIST: {
			vmachine vm(*sq.query);
			vm.run();
			sq.resultSet.reset(vm.vsets[sq.btreeIdx].release());
			}
		}
	}
}
