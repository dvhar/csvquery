#include "interpretor.h"
#include "vmachine.h"
#include <cmath>

#ifndef _APPLE_ //get your shit together, apple
#include <execution>
#endif

//work with stack data
#define stk0 (*stacktop)
#define stk1 (*(stacktop-1))
#define stkp(N) (*(stacktop-N))
#define push() ++stacktop
#define pop() --stacktop

//jump to next operation
#define debugOpcode /* cerr << ft("ip {} opcode {} stack {}\n", ip, opMap[op->code], stacktop-stack.data()); */
#define next() \
	op = ops + ip; \
	debugOpcode \
	goto *(labels[op->code]);

void vmachine::run(){

	void* labels[] = { &&CVER_, &&CVNO_, &&CVIF_, &&CVIS_, &&CVFI_, &&CVFS_, &&CVDRS_, &&CVDTS_, &&CVSI_, &&CVSF_, &&CVSDR_, &&CVSDT_, &&IADD_, &&FADD_, &&TADD_, &&DTADD_, &&DRADD_, &&ISUB_, &&FSUB_, &&DTSUB_, &&DRSUB_, &&IMULT_, &&FMULT_, &&DRMULT_, &&IDIV_, &&FDIV_, &&DRDIV_, &&INEG_, &&FNEG_, &&PNEG_, &&IMOD_, &&FMOD_, &&IEXP_, &&FEXP_, &&JMP_, &&JMPCNT_, &&JMPTRUE_, &&JMPFALSE_, &&JMPNOTNULL_ELSEPOP_, &&RDLINE_, &&RDLINE_ORDERED_, &&PREP_REREAD_, &&PUT_, &&LDPUT_, &&LDPUTALL_, &&PUTVAR_, &&LDINT_, &&LDFLOAT_, &&LDTEXT_, &&LDDATE_, &&LDDUR_, &&LDNULL_, &&LDLIT_, &&LDVAR_, &&IEQ_, &&FEQ_, &&TEQ_, &&LIKE_, &&ILEQ_, &&FLEQ_, &&TLEQ_, &&ILT_, &&FLT_, &&TLT_, &&PRINT_, &&POP_, &&POPCPY_, &&ENDRUN_, &&NULFALSE1_, &&NULFALSE2_, &&NDIST_, &&SDIST_, &&PUTDIST_, &&FINC_, &&ENCCHA_, &&DECCHA_, &&SAVEPOSI_JMP_, &&SAVEPOSF_JMP_, &&SAVEPOSS_JMP_, &&SORTI_, &&SORTF_, &&SORTS_ };

	//vars for data
	int64 i64Temp;
	int iTemp1, iTemp2;
	double fTemp;
	char* cstrTemp;
	char bufTemp[40];
	bool boolTemp;
	csvEntry csvTemp;
	pair<char*, int> pairTemp;

	//vars for vm operations
	int numPrinted = 0;
	dat* stacktop = stack.data();
	int ip = 0;
	opcode *op;

	next();

//put data from stack into torow
//should make a version of this that prints directly rather than using torow
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
	torow[op->p1] = dat{ { .s = csvTemp.val }, T, csvTemp.size };
	++ip;
	next();
LDPUTALL_:
	iTemp1 = op->p1;
	for (auto &f : files){
		for (auto &e : f->entries){
			FREE1(torow[iTemp1]);
			torow[iTemp1++] = dat{ { .s = e.val }, T, e.size };
		}
	}
	++ip;
	next();
PUTDIST_:
	FREE1(torow[op->p1]);
	torow[op->p1] = distinctVal;
	DISOWN(distinctVal);
	++ip;
	next();
//put variable from stack into var vector
PUTVAR_:
	FREE1(vars[op->p1]);
	vars[op->p1] = stk0;
	DISOWN(stk0);
	pop();
	++ip;
	next();

//put variable from var vector into stack
LDVAR_:
	push();
	stk0 = vars[op->p1];
	DISOWN(stk0); //var vector still owns c string
	++ip;
	next();
//load data from filereader to the stack
LDDUR_:
	push();
	iTemp1 = parseDuration(files[op->p1]->entries[op->p2].val, &i64Temp);
	stk0 = dat{ { .i = i64Temp}, DR};
	if (iTemp1) stk0.b |= NIL;
	++ip;
	next();
LDDATE_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	iTemp1 = dateparse(csvTemp.val, &i64Temp, &iTemp2, csvTemp.size);
	stk0 = dat{ { .i = i64Temp}, DT, iTemp2 };
	if (iTemp1) stk0.b |= NIL;
	++ip;
	next();
LDTEXT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0 = dat{ { .s = csvTemp.val }, T, csvTemp.size };
	if (!csvTemp.size) stk0.b |= NIL;
	++ip;
	next();
LDFLOAT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.f = strtof(csvTemp.val, &cstrTemp);
	stk0.b = F;
	if (!csvTemp.size || *cstrTemp){ stk0.b |= NIL; }
	++ip;
	next();
LDINT_:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.i = strtol(csvTemp.val, &cstrTemp, 10);
	stk0.b = I;
	if (!csvTemp.size || *cstrTemp) stk0.b |= NIL;
	++ip;
	next();
LDNULL_:
	push();
	stk0.b = NIL;
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

#ifndef _APPLE_ //get your shit together, apple
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
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i += stk0.u.i;
	pop();
	++ip;
	next();
FADD_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
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
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i += stk0.u.i; stk1.b = DR; }
	pop();
	++ip;
	next();
DTADD_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i += stk0.u.i; stk1.b = DT; }
	pop();
	++ip;
	next();
ISUB_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i -= stk0.u.i;
	pop();
	++ip;
	next();
FSUB_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f -= stk0.u.f;
	pop();
	++ip;
	next();
DTSUB_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i -= stk0.u.i; stk1.b = DT; }
	pop();
	++ip;
	next();
DRSUB_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i -= stk0.u.i; stk1.b = DR; }
	pop();
	if (stk0.u.i < 0) stk0.u.i *= -1;
	++ip;
	next();
IMULT_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i *= stk0.u.i;
	pop();
	++ip;
	next();
FMULT_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f *= stk0.u.f;
	pop();
	++ip;
	next();
DRMULT_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
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
			stk1.b = DR;
		}
	}
	pop();
	++ip;
	next();
IDIV_:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.i==0) stk1.b |= NIL;
	else stk1.u.i /= stk0.u.i;
	pop();
	++ip;
	next();
FDIV_:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.f==0) stk1.b |= NIL;
	else stk1.u.f /= stk0.u.f;
	pop();
	++ip;
	next();
DRDIV_:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.f==0) stk1.b |= NIL;
	else {
		if (stk0.b == I)
			stk1.u.i /= stk0.u.i;
		else
			stk1.u.i /= stk0.u.f;
	}
	pop();
	++ip;
	next();
INEG_:
	if (ISNULL(stk0)) stk0.b |= NIL;
	else stk0.u.i *= -1;
	++ip;
	next();
FNEG_:
	if (ISNULL(stk0)) stk0.b |= NIL;
	else stk0.u.f *= -1;
	++ip;
	next();
PNEG_:
	stk0.u.p ^= true;
	++ip;
	next();
FEXP_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f = pow(stk1.u.f, stk0.u.f);
	pop();
	++ip;
	next();
IEXP_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i = pow(stk1.u.i, stk0.u.i);
	pop();
	++ip;
	next();
IMOD_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i = stk1.u.i % stk0.u.i;
	pop();
	++ip;
	next();
FMOD_:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f = (int64)stk1.u.f % (int64)stk0.u.f;
	pop();
	++ip;
	next();

//comparisions - p1 determines how far down the stack to leave the result
// p2 is negator
//may want to combine with jmp
IEQ_:
	iTemp1 = ISNULL(stk0);
	iTemp2 = ISNULL(stk1);
	if (iTemp1 ^ iTemp2) stkp(op->p1).u.p = false;
	else if (iTemp1 & iTemp2) stkp(op->p1).u.p = true;
	else stkp(op->p1).u.p = (stk1.u.i == stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FEQ_:
	iTemp1 = ISNULL(stk0);
	iTemp2 = ISNULL(stk1);
	if (iTemp1 ^ iTemp2) stkp(op->p1).u.p = false;
	else if (iTemp1 & iTemp2) stkp(op->p1).u.p = true;
	else stkp(op->p1).u.p = (stk1.u.f == stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TEQ_:
	iTemp1 = ISNULL(stk0);
	iTemp2 = ISNULL(stk1);
	if (!(iTemp1|iTemp2)){ //none null
		boolTemp = (scomp(stk1.u.s, stk0.u.s) == 0)^op->p2;
		FREE2(stk0);
		FREE2(stkp(op->p1));
		stkp(op->p1).u.p = boolTemp;
	} else if (iTemp1 & iTemp2) { //both null
		stkp(op->p1).u.p = true;
	} else { //one null
		FREE2(stk0);
		FREE2(stkp(op->p1));
		stkp(op->p1).u.p = false;
	}
	stacktop -= op->p1;
	++ip;
	next();
ILEQ_:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.i <= stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FLEQ_:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.f <= stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TLEQ_:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else {
		boolTemp = (scomp(stk1.u.s, stk0.u.s) <= 0)^op->p2;
		FREE2(stk0);
		FREE2(stkp(op->p1));
		stkp(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	++ip;
	next();
ILT_:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.i < stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FLT_:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.f < stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TLT_:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else {
		boolTemp = (scomp(stk1.u.s, stk0.u.s) < 0)^op->p2;
		FREE2(stk0);
		FREE2(stkp(op->p1));
		stkp(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	++ip;
	next();
LIKE_:
	iTemp1 = !regexec(q->dataholder[op->p1].u.r, stk0.u.s, 0, 0, 0)^op->p2;
	FREE2(stk0);
	stk0.u.p = iTemp1;
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
		if (stk0.z >= 0){
			stk0.b = T|MAL;
		} else {
			stk0.b = T|NIL;
		}
	}
	++ip;
	next();
CVFS_:
	if (!ISNULL(stk0)){
		stk0.z = sprintf(bufTemp, "%.10g", stk0.u.f);
		stk0.u.s = (char*) malloc(stk0.z+1);
		memcpy(stk0.u.s, bufTemp, stk0.z+1);
		if (stk0.z >= 0){
			stk0.b = T|MAL;
		} else {
			stk0.b = T|NIL;
		}
	}
	++ip;
	next();
CVFI_:
	if (!ISNULL(stk0)){
		stk0.u.i = stk0.u.f;
		stk0.b = I;
	}
	++ip;
	next();
CVIF_:
	if (!ISNULL(stk0)){
		stk0.u.f = stk0.u.i;
		stk0.b = F;
	}
	++ip;
	next();
CVSI_:
	if (!ISNULL(stk0)){
		iTemp1 = strtol(stk0.u.s, &cstrTemp, 10);
		FREE2(stk0);
		stk0.u.i = iTemp1;
		stk0.b = I;
		if (*cstrTemp){ stk0.b |= NIL; }
	}
	++ip;
	next();
CVSF_:
	if (!ISNULL(stk0)){
		fTemp = strtof(stk0.u.s, &cstrTemp);
		FREE2(stk0);
		stk0.u.f = fTemp;
		stk0.b = F;
		if (*cstrTemp){ stk0.b |= NIL; }
	}
	++ip;
	next();
CVSDT_:
	if (!ISNULL(stk0)){
		iTemp1 = dateparse(stk0.u.s, &i64Temp, &iTemp2, stk0.z);
		FREE2(stk0);
		stk0.u.i = i64Temp;
		stk0.b = DT;
		stk0.z = iTemp2;
		if (iTemp1) stk0.b |= NIL;
	}
	++ip;
	next();
CVSDR_:
	if (!ISNULL(stk0)){
		iTemp1 = parseDuration(stk0.u.s, &i64Temp);
		FREE2(stk0);
		stk0.u.i = i64Temp;
		stk0.b = DR;
		if (iTemp1) stk0.b |= NIL;
	}
	++ip;
	next();
CVDRS_:
	if (!ISNULL(stk0)){
		stk0.u.s = (char*) malloc(24);
		durstring(stk0.u.i, stk0.u.s);
		stk0.b = T|MAL;
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
		stk0.b = T|MAL;
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
	for (int i=0; i<torowSize; ++i){
		torow[i].print();
		if (i < torowSize-1) fmt::print(",");
	}
	fmt::print("\n");
	++numPrinted;
	++ip;
	next();

//distinct checkers
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
	pairTemp = q->crypt.chachaEncrypt(op->p1, stk0.z, stk0.u.s);
	if (stk0.b & MAL) free(stk0.u.s);
	stk0.u.s = pairTemp.first;
	stk0.z = pairTemp.second;
	stk0.b = T|MAL;
	++ip;
	next();
DECCHA_:
	pairTemp = q->crypt.chachaDecrypt(op->p1, stk0.z, stk0.u.s);
	if (stk0.b & MAL) free(stk0.u.s);
	stk0.u.s = pairTemp.first;
	stk0.z = pairTemp.second;
	stk0.b = T|MAL;
	++ip;
	next();

ENDRUN_:
	goto halt;

//unimplemeted opcodes
CVER_:
CVNO_:
	error("Invalid opcode");


	halt:
	return;
}


void runquery(querySpecs &q){
	vmachine vm(q);
	vm.run();
}
