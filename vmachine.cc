#include "interpretor.h"
#include "vmachine.h"
#include <cmath>

//parallel sort only available in gcc's libstdc++
#ifdef _GLIBCXX_EXECUTION
#include <execution>
#endif

//work with stack data
#define stk0 (*stacktop)
#define stk1 (*(stacktop-1))
#define stkt(N) (*(stacktop-N))
#define stkb(N) (*(stackbot+N))
#define push() ++stacktop
#define pop() --stacktop

//jump to next operation
#define debugOpcode  cerr << ft("ip {} opcode {} stack {}\n", ip, opMap[op->code], stacktop-stack.data());
#define debugOpcode
#define next() \
	op = ops + ip; \
	debugOpcode \
	goto *(labels[op->code]);

void vmachine::run(){
	void* labels[] = { &&CVER_, &&CVNO_, &&CVIF_, &&CVIS_, &&CVFI_, &&CVFS_, &&CVDRS_, &&CVDTS_, &&CVSI_, &&CVSF_, &&CVSDR_, &&CVSDT_, &&IADD_, &&FADD_, &&TADD_, &&DTADD_, &&DRADD_, &&ISUB_, &&FSUB_, &&DTSUB_, &&DRSUB_, &&IMULT_, &&FMULT_, &&DRMULT_, &&IDIV_, &&FDIV_, &&DRDIV_, &&INEG_, &&FNEG_, &&PNEG_, &&IMOD_, &&FMOD_, &&IEXP_, &&FEXP_, &&JMP_, &&JMPCNT_, &&JMPTRUE_, &&JMPFALSE_, &&JMPNOTNULL_ELSEPOP_, &&RDLINE_, &&RDLINE_ORDERED_, &&PREP_REREAD_, &&PUT_, &&LDPUT_, &&LDPUTALL_, &&PUTVAR_, &&PUTVAR2_, &&LDINT_, &&LDFLOAT_, &&LDTEXT_, &&LDDATE_, &&LDDUR_, &&LDNULL_, &&LDLIT_, &&LDVAR_, &&IEQ_, &&FEQ_, &&TEQ_, &&LIKE_, &&ILEQ_, &&FLEQ_, &&TLEQ_, &&ILT_, &&FLT_, &&TLT_, &&PRINT_, &&PUSH_, &&POP_, &&POPCPY_, &&ENDRUN_, &&NULFALSE1_, &&NULFALSE2_, &&NDIST_, &&SDIST_, &&PUTDIST_, &&LDDIST_, &&FINC_, &&ENCCHA_, &&DECCHA_, &&SAVEPOSI_JMP_, &&SAVEPOSF_JMP_, &&SAVEPOSS_JMP_, &&SORTI_, &&SORTF_, &&SORTS_, &&GETGROUP_, &&ONEGROUP_, &&SUMI_, &&SUMF_, &&AVGI_, &&AVGF_, &&STDVI_, &&STDVF_, &&COUNT_, &&MINI_, &&MINF_, &&MINS_, &&MAXI_, &&MAXF_, &&MAXS_, &&NEXTMAP_, &&NEXTVEC_, &&ROOTMAP_, &&LDMID_, &&LDPUTMID_, &&LDPUTGRP_, &&LDSTDVI_, &&LDSTDVF_, &&LDAVGI_, &&LDAVGF_ };


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

	//vars for vm operations
	int numPrinted = 0;
	dat* stacktop = stack.data();
	dat* stackbot = stack.data();
	decltype(groupTree.getMap().begin()) itstk[20];
	dat* midrow;
	int ip = 0;
	opcode *op;
	rowgroup *groupTemp;

	//file writer
	ostream output(cout.rdbuf());
	string outbuf;

	next();

//put data from stack into torow
PUT_:
	FREE1(torow[op->p1]);
	torow[op->p1] = stk0;
	DISOWN(stk0);
	pop();
	++ip;
	next();
//put data from filereader directly into torow
LDPUT_:
	csvTemp = files[op->p3]->entries[op->p2];
	FREE1(torow[op->p1]);
	torow[op->p1] = dat{ { s: csvTemp.val }, T_STRING, csvTemp.size };
	++ip;
	next();
LDPUTGRP_:
	csvTemp = files[op->p3]->entries[op->p2];
	if (ISNULL(torow[op->p1]) && csvTemp.size){
		torow[op->p1] = { { s:newStr(csvTemp.val, csvTemp.size) }, T_STRING|MAL, csvTemp.size };
	}
	++ip;
	next();
LDPUTALL_:
	iTemp1 = op->p1;
	for (auto &f : files){
		for (auto &e : f->entries){
			FREE1(torow[iTemp1]);
			torow[iTemp1++] = dat{ { s: e.val }, T_STRING, e.size };
		}
	}
	++ip;
	next();
//put data from midrow to torow
LDPUTMID_:
	FREE1(torow[op->p1]);
	torow[op->p1] = midrow[op->p2];
	DISOWN(midrow[op->p2]);
	++ip;
	next();
LDMID_:
	push();
	FREE2(stk0);
	stk0 = midrow[op->p1];
	DISOWN(midrow[op->p1]);
	++ip;
	next();

PUTDIST_:
	FREE1(torow[op->p1]);
	torow[op->p1] = distinctVal;
	DISOWN(distinctVal);
	++ip;
	next();
//put variable from stack into stackbot
PUTVAR_:
	FREE1(stkb(op->p1));
	stkb(op->p1) = stk0;
	DISOWN(stk0);
	pop();
	++ip;
	next();
//put variable from stack into midrow and stackbot
PUTVAR2_:
	datpTemp = &torow[op->p2];
	if (ISNULL(*datpTemp) && !ISNULL(stk0)){
		FREE1(stkb(op->p1));
		stkb(op->p1) = stk0;
		DISOWN(stkb(op->p1));
		FREE1(*datpTemp);
		*datpTemp = stk0.heap();
		DISOWN(stk0);
	}
	pop();
	++ip;
	next();

//put variable from var vector into stack
LDVAR_:
	push();
	FREE2(stk0);
	stk0 = stkb(op->p1);
	DISOWN(stk0); //var source still owns c string
	++ip;
	next();
//load data from filereader to the stack
LDDUR_:
	push();
	iTemp1 = parseDuration(files[op->p1]->entries[op->p2].val, &i64Temp);
	stk0 = dat{ { i: i64Temp}, T_DURATION};
	if (iTemp1) { SETNULL(stk0); }
	++ip;
	next();
LDDATE_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	iTemp1 = dateparse(csvTemp.val, &i64Temp, &iTemp2, csvTemp.size);
	stk0 = dat{ { i: i64Temp}, T_DATE, (uint) iTemp2 };
	if (iTemp1) { SETNULL(stk0); }
	++ip;
	next();
LDTEXT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0 = dat{ { s: csvTemp.val }, T_STRING, csvTemp.size };
	if (!csvTemp.size) { SETNULL(stk0); }
	++ip;
	next();
LDFLOAT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.f = strtof(csvTemp.val, &cstrTemp);
	stk0.b = T_FLOAT;
	if (!csvTemp.size || *cstrTemp){ SETNULL(stk0); }
	++ip;
	next();
LDINT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.i = strtol(csvTemp.val, &cstrTemp, 10);
	stk0.b = T_INT;
	if (!csvTemp.size || *cstrTemp) { SETNULL(stk0); }
	++ip;
	next();
LDNULL_:
	push();
	{ SETNULL(stk0); }
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
		files[op->p2]->readlineat(posVectors[op->p3][stk0.u.i++].pos);
		++ip;
	} else {
		ip = op->p1;
	}
	next();
PREP_REREAD_:
	push();
	stk0.u.i = posVectors[op->p1].size();
	push();
	stk0.u.i = 0;
	++ip;
	next();
SAVEPOSI_JMP_:
	posVectors[op->p2].push_back(valPos( stk0.u.i, files[op->p3]->pos ));
	ip = op->p1;
	pop();
	next();
SAVEPOSF_JMP_:
	posVectors[op->p2].push_back(valPos( stk0.u.f, files[op->p3]->pos ));
	ip = op->p1;
	pop();
	next();
SAVEPOSS_JMP_:
	if (ISMAL(stk0)){
		cstrTemp = stk0.u.s;
		DISOWN(stk0);
	} else {
		cstrTemp = (char*) malloc(stk0.z+1);
		strcpy(cstrTemp, stk0.u.s);
	}
	posVectors[op->p2].push_back(valPos( cstrTemp, files[op->p3]->pos ));
	ip = op->p1;
	pop();
	next();

#ifdef _GLIBCXX_EXECUTION
SORTI_:
	sort(execution::par_unseq, posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (a.val.i > b.val.i)^op->p2; });
	++ip;
	next();
SORTF_:
	sort(execution::par_unseq, posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (a.val.f > b.val.f)^op->p2; });
	++ip;
	next();
SORTS_:
	sort(execution::par_unseq, posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (scomp(a.val.s, b.val.s) > 0)^op->p2; });
	++ip;
	next();
#else
SORTI_:
	sort(posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (a.val.i > b.val.i)^op->p2; });
	++ip;
	next();
SORTF_:
	sort(posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (a.val.f > b.val.f)^op->p2; });
	++ip;
	next();
SORTS_:
	sort(posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (scomp(a.val.s, b.val.s) > 0)^op->p2; });
	++ip;
	next();
#endif

//math operations
IADD_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else stk1.u.i += stk0.u.i;
	pop();
	++ip;
	next();
FADD_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
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
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else { stk1.u.i += stk0.u.i; stk1.b = T_DURATION; }
	pop();
	++ip;
	next();
DTADD_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else { stk1.u.i += stk0.u.i; stk1.b = T_DATE; }
	pop();
	++ip;
	next();
ISUB_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else stk1.u.i -= stk0.u.i;
	pop();
	++ip;
	next();
FSUB_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else stk1.u.f -= stk0.u.f;
	pop();
	++ip;
	next();
DTSUB_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else { stk1.u.i -= stk0.u.i; stk1.b = T_DATE; }
	pop();
	++ip;
	next();
DRSUB_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else { stk1.u.i -= stk0.u.i; stk1.b = T_DURATION; }
	pop();
	if (stk0.u.i < 0) stk0.u.i *= -1;
	++ip;
	next();
IMULT_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else stk1.u.i *= stk0.u.i;
	pop();
	++ip;
	next();
FMULT_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else stk1.u.f *= stk0.u.f;
	pop();
	++ip;
	next();
DRMULT_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
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
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.i==0) { SETNULL(stk1); }
	else stk1.u.i /= stk0.u.i;
	pop();
	++ip;
	next();
FDIV_:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.f==0) { SETNULL(stk1); }
	else stk1.u.f /= stk0.u.f;
	pop();
	++ip;
	next();
DRDIV_:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.f==0) { SETNULL(stk1); }
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
	if (ISNULL(stk0)) { SETNULL(stk0); }
	else stk0.u.i *= -1;
	++ip;
	next();
FNEG_:
	if (ISNULL(stk0)) { SETNULL(stk0); }
	else stk0.u.f *= -1;
	++ip;
	next();
PNEG_:
	stk0.u.p ^= true;
	++ip;
	next();
FEXP_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else stk1.u.f = pow(stk1.u.f, stk0.u.f);
	pop();
	++ip;
	next();
IEXP_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else stk1.u.i = pow(stk1.u.i, stk0.u.i);
	pop();
	++ip;
	next();
IMOD_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else stk1.u.i = stk1.u.i % stk0.u.i;
	pop();
	++ip;
	next();
FMOD_:
	if (ISNULL(stk0) || ISNULL(stk1)) { SETNULL(stk1); }
	else stk1.u.f = (int64)stk1.u.f % (int64)stk0.u.f;
	pop();
	++ip;
	next();

//comparisions - p1 determines how far down the stack to leave the result
// p2 is negator
IEQ_:
	iTemp1 = ISNULL(stk0);
	iTemp2 = ISNULL(stk1);
	if (iTemp1 ^ iTemp2) stkt(op->p1).u.p = false;
	else if (iTemp1 & iTemp2) stkt(op->p1).u.p = true;
	else stkt(op->p1).u.p = (stk1.u.i == stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FEQ_:
	iTemp1 = ISNULL(stk0);
	iTemp2 = ISNULL(stk1);
	if (iTemp1 ^ iTemp2) stkt(op->p1).u.p = false;
	else if (iTemp1 & iTemp2) stkt(op->p1).u.p = true;
	else stkt(op->p1).u.p = (stk1.u.f == stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TEQ_:
	iTemp1 = ISNULL(stk0);
	iTemp2 = ISNULL(stk1);
	if (!(iTemp1|iTemp2)){ //none null
		boolTemp = (scomp(stk1.u.s, stk0.u.s) == 0)^op->p2;
		FREE2(stk0);
		FREE2(stkt(op->p1));
		stkt(op->p1).u.p = boolTemp;
	} else if (iTemp1 & iTemp2) { //both null
		stkt(op->p1).u.p = true^op->p2;
	} else { //one null
		FREE2(stk0);
		FREE2(stkt(op->p1));
		stkt(op->p1).u.p = false^op->p2;
	}
	stacktop -= op->p1;
	++ip;
	next();
ILEQ_:
	if (ISNULL(stk0) || ISNULL(stk1)) stkt(op->p1).u.p = false;
	else stkt(op->p1).u.p = (stk1.u.i <= stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FLEQ_:
	if (ISNULL(stk0) || ISNULL(stk1)) stkt(op->p1).u.p = false;
	else stkt(op->p1).u.p = (stk1.u.f <= stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TLEQ_:
	if (ISNULL(stk0) || ISNULL(stk1)) {
		FREE2(stkt(op->p1));
		stkt(op->p1).u.p = false;
	} else {
		boolTemp = (scomp(stk1.u.s, stk0.u.s) <= 0)^op->p2;
		FREE2(stk0);
		FREE2(stkt(op->p1));
		stkt(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	++ip;
	next();
ILT_:
	if (ISNULL(stk0) || ISNULL(stk1)) stkt(op->p1).u.p = false;
	else stkt(op->p1).u.p = (stk1.u.i < stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FLT_:
	if (ISNULL(stk0) || ISNULL(stk1)) stkt(op->p1).u.p = false;
	else stkt(op->p1).u.p = (stk1.u.f < stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TLT_:
	if (ISNULL(stk0) || ISNULL(stk1)){
		FREE2(stkt(op->p1));
		stkt(op->p1).u.p = false;
	} else {
		boolTemp = (scomp(stk1.u.s, stk0.u.s) < 0)^op->p2;
		FREE2(stk0);
		FREE2(stkt(op->p1));
		stkt(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	++ip;
	next();
LIKE_:
	if (ISNULL(stk0)){
		stk0.u.p = op->p1;
	} else {
		iTemp1 = !regexec(q->dataholder[op->p1].u.r, stk0.u.s, 0, 0, 0)^op->p2;
		FREE2(stk0);
		stk0.u.p = iTemp1;
	}
	++ip;
	next();

PUSH_:
	push();	
	++ip;
	next();
POP_:
	FREE2(stk0);
	pop();
	++ip;
	next();
POPCPY_:
	FREE2(stk1);
	stk1 = stk0;
	DISOWN(stk0);
	pop();
	++ip;
	next();
NULFALSE1_:
	if (ISNULL(stk0)){
		FREE2(stk0);
		stk0.u.p = false;
		ip = op->p1;
	} else ++ip;
	next();
NULFALSE2_:
	if (ISNULL(stk0)){
		FREE2(stk0);
		pop();
		FREE2(stk0);
		stk0.u.p = false;
		ip = op->p1;
	} else ++ip;
	next();

//type conversions
CVIS_:
	if (!ISNULL(stk0)){
		stk0.z = sprintf(bufTemp, "%lld", stk0.u.i);
		stk0.u.s = (char*) malloc(stk0.z+1);
		memcpy(stk0.u.s, bufTemp, stk0.z+1);
		stk0.b = T_STRING|MAL;
	}
	++ip;
	next();
CVFS_:
	if (!ISNULL(stk0)){
		stk0.z = sprintf(bufTemp, "%.10g", stk0.u.f);
		stk0.u.s = (char*) malloc(stk0.z+1);
		memcpy(stk0.u.s, bufTemp, stk0.z+1);
		stk0.b = T_STRING|MAL;
	}
	++ip;
	next();
CVFI_:
	if (!ISNULL(stk0)){
		stk0.u.i = stk0.u.f;
		stk0.b = T_INT;
	}
	++ip;
	next();
CVIF_:
	if (!ISNULL(stk0)){
		stk0.u.f = stk0.u.i;
		stk0.b = T_FLOAT;
	}
	++ip;
	next();
CVSI_:
	if (!ISNULL(stk0)){
		iTemp1 = strtol(stk0.u.s, &cstrTemp, 10);
		FREE2(stk0);
		stk0.u.i = iTemp1;
		stk0.b = T_INT;
		if (*cstrTemp){ SETNULL(stk0); }
	}
	++ip;
	next();
CVSF_:
	if (!ISNULL(stk0)){
		fTemp = strtof(stk0.u.s, &cstrTemp);
		FREE2(stk0);
		stk0.u.f = fTemp;
		stk0.b = T_FLOAT;
		if (*cstrTemp){ SETNULL(stk0); }
	}
	++ip;
	next();
CVSDT_:
	if (!ISNULL(stk0)){
		iTemp1 = dateparse(stk0.u.s, &i64Temp, &iTemp2, stk0.z);
		FREE2(stk0);
		stk0.u.i = i64Temp;
		stk0.b = T_DATE;
		stk0.z = iTemp2;
		if (iTemp1) { SETNULL(stk0); }
	}
	++ip;
	next();
CVSDR_:
	if (!ISNULL(stk0)){
		iTemp1 = parseDuration(stk0.u.s, &i64Temp);
		FREE2(stk0);
		stk0.u.i = i64Temp;
		stk0.b = T_DURATION;
		if (iTemp1) { SETNULL(stk0); }
	}
	++ip;
	next();
CVDRS_:
	if (!ISNULL(stk0)){
		stk0.u.s = (char*) malloc(24);
		durstring(stk0.u.i, stk0.u.s);
		stk0.b = T_STRING|MAL;
		stk0.z = strlen(stk0.u.s);
	}
	++ip;
	next();
CVDTS_:
	if (!ISNULL(stk0)){
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
		FREE2(stk0);
		pop();
	}
	next();
JMPTRUE_:
	ip = stk0.u.p ? op->p1 : ip+1;
	if (op->p2 == 1){
		FREE2(stk0);
		pop();
	}
	next();
JMPNOTNULL_ELSEPOP_:
	if (ISNULL(stk0)){
		FREE2(stk0);
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
	if (outbuf.size() > 900 || 1){ //skip buffering for debug
		output << outbuf;
		outbuf.clear();
	}
	if (++iTemp1 < torowSize){
		outbuf += ',';
		goto printfield;
	}
	//outbuf += '\n';
	output << "\n";
	++numPrinted;
	++ip;
	next();

//distinct checkers
LDDIST_:
	push();
	stk0 = distinctVal;
	DISOWN(distinctVal);
	++ip;
	next();
NDIST_:
	boolTemp = bt_nums[op->p2].insert(stk0.u.i).second;
	if (boolTemp) {
		distinctVal = stk0;
		++ip;
	} else {
		ip = op->p1;
	}
	pop();
	next();
SDIST_:
	boolTemp = bt_strings[op->p2].insert(treeCString(stk0)).second;
	if (boolTemp) {
		if (op->p3){ //not hidden
			FREE2(distinctVal);
			distinctVal = stk0;
			DISOWN(stk0);
		} else {
			FREE2(stk0);
		}
		++ip;
	} else {
		FREE2(stk0);
		ip = op->p1;
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
	auto&& pt1 = q->crypt.chachaEncrypt(op->p1, stk0.z, stk0.u.s);
	if (stk0.b & MAL) free(stk0.u.s);
	stk0.u.s = pt1.first;
	stk0.z = pt1.second;
	stk0.b = T_STRING|MAL;
	++ip;
	next();
DECCHA_:
	auto&& pt2 = q->crypt.chachaDecrypt(op->p1, stk0.z, stk0.u.s);
	if (stk0.b & MAL) free(stk0.u.s);
	stk0.u.s = pt2.first;
	stk0.z = pt2.second;
	stk0.b = T_STRING|MAL;
	++ip;
	next();
//aggregates
LDSTDVI_:
	++ip;
	next()
LDSTDVF_:
	++ip;
	next()
LDAVGI_:
	push();
	FREE2(stk0);
	stk0.u.i = (double)midrow[op->p1].u.i / (double)midrow[op->p1].z;
	stk0.b = T_INT;
	++ip;
	next()
LDAVGF_:
	push();
	FREE2(stk0);
	stk0.u.f = midrow[op->p1].u.f / (double)midrow[op->p1].z;
	stk0.b = T_FLOAT;
	++ip;
	next()
AVGI_:
	if (!ISNULL(stk0))
		torow[op->p1].z++;
SUMI_:
	if (!ISNULL(stk0)){
		torow[op->p1].u.i += stk0.u.i;
		torow[op->p1].b = stk0.b;
	}
	pop();
	++ip;
	next();
AVGF_:
	if (!ISNULL(stk0))
		torow[op->p1].z++;
SUMF_:
	if (!ISNULL(stk0)){
		torow[op->p1].u.f += stk0.u.f;
		torow[op->p1].b = stk0.b;
	}
	pop();
	++ip;
	next();
STDVI_:
STDVF_:
	//TODO
	pop();
	++ip;
	next();
COUNT_:
	if (op->p2){ //count(*)
		torow[op->p1].b = T_FLOAT;
		torow[op->p1].u.f++;
	} else {
		if (!ISNULL(stk0)) {
			torow[op->p1].b = T_FLOAT;
			torow[op->p1].u.f++;
		}
		pop();
	}
	++ip;
	next();
MINI_:
	if (ISNULL(torow[op->p1]) || (!ISNULL(stk0) && torow[op->p1].u.i > stk0.u.i)){
		torow[op->p1] = stk0;
	}
	pop();
	++ip;
	next();
MINF_:
	if (ISNULL(torow[op->p1]) || (!ISNULL(stk0) && torow[op->p1].u.f > stk0.u.f))
		torow[op->p1] = stk0;
	pop();
	++ip;
	next();
MINS_:
	if (ISNULL(torow[op->p1]) || (!ISNULL(stk0) && scomp(torow[op->p1].u.s, stk0.u.s) > 0)){
		FREE1(torow[op->p1]);
		torow[op->p1] = stk0;
		DISOWN(stk0);
	}
	pop();
	++ip;
	next();
MAXI_:
	if (ISNULL(torow[op->p1]) || (!ISNULL(stk0) && torow[op->p1].u.i < stk0.u.i))
		torow[op->p1] = stk0;
	pop();
	++ip;
	next();
MAXF_:
	if (ISNULL(torow[op->p1]) || (!ISNULL(stk0) && torow[op->p1].u.f < stk0.u.f))
		torow[op->p1] = stk0;
	pop();
	++ip;
	next();
MAXS_:
	if (ISNULL(torow[op->p1]) || (!ISNULL(stk0) && scomp(torow[op->p1].u.s, stk0.u.s) < 0)){
		FREE1(torow[op->p1]);
		torow[op->p1] = stk0;
		DISOWN(stk0);
	}
	pop();
	++ip;
	next();
GETGROUP_:
	groupTemp = &groupTree;
	for (int i=op->p1; i >= 0; --i){
		groupTemp = &groupTemp->nextGroup(stkt(i));
	}
	torow = groupTemp->getVector(q->midcount).data();
	stacktop -= (op->p1 + 1);
	++ip;
	next();
ONEGROUP_:
	torow = destrow.data();
	midrow = onegroup.data();
	++ip;
	next();
ROOTMAP_:
	//trav(groupTree);
	itstk[op->p1]   = groupTree.getMap().begin();
	itstk[op->p1+1] = groupTree.getMap().end();
	torow = destrow.data();
	++ip;
	next();
NEXTMAP_:
	if (itstk[op->p2-2] == itstk[op->p2-1]){
		ip = op->p1;
	} else {
		// set top of iterator stack to next map
		groupTemp = &(itstk[op->p2-2]++)->second;
		itstk[op->p2]   = groupTemp->getMap().begin();
		itstk[op->p2+1] = groupTemp->getMap().end();
		++ip;
	}
	next();
NEXTVEC_:
	if (itstk[op->p2] == itstk[op->p2+1]){
		ip = op->p1;
	} else {
		auto&& vecTemp = (itstk[op->p2]++)->second.getRow();
		midrow = vecTemp.data();
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
