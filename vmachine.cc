#include "interpretor.h"
#include "vmachine.h"

#ifndef __APPLE__ //get your shit together, apple
#include <execution>
#endif

//syntactic sugar for stack dereferencing
#define stk0 (*stacktop)
#define stk1 (*(stacktop-1))
#define stkp(N) (*(stacktop-N))
#define push() ++stacktop
#define pop() --stacktop



#define next() \
	op = ops + ip; \
	goto *(labels[op->code]); \
	//cerr << ft("ip {} opcode {} stack {}\n", ip, opMap[op->code], stacktop-stack.data());

void vmachine::run(){

	void* labels[] = { &&CVER_label, &&CVNO_label, &&CVIF_label, &&CVIS_label, &&CVFI_label, &&CVFS_label, &&CVDRS_label, &&CVDRF_label, &&CVDTS_label, &&CVSI_label, &&CVSF_label, &&CVSDR_label, &&CVSDT_label, &&IADD_label, &&FADD_label, &&TADD_label, &&DTADD_label, &&DRADD_label, &&ISUB_label, &&FSUB_label, &&DTSUB_label, &&DRSUB_label, &&IMULT_label, &&FMULT_label, &&DRMULT_label, &&IDIV_label, &&FDIV_label, &&DRDIV_label, &&INEG_label, &&FNEG_label, &&DNEG_label, &&PNEG_label, &&IMOD_label, &&IEXP_label, &&FEXP_label, &&JMP_label, &&JMPCNT_label, &&JMPTRUE_label, &&JMPFALSE_label, &&JMPNOTNULL_ELSEPOP_label, &&RDLINE_label, &&RDLINE_ORDERED_label, &&PREP_REREAD_label, &&PUT_label, &&LDPUT_label, &&LDPUTALL_label, &&PUTVAR_label, &&LDINT_label, &&LDFLOAT_label, &&LDTEXT_label, &&LDDATE_label, &&LDDUR_label, &&LDNULL_label, &&LDLIT_label, &&LDVAR_label, &&IEQ_label, &&FEQ_label, &&TEQ_label, &&NEQ_label, &&LIKE_label, &&ILEQ_label, &&FLEQ_label, &&TLEQ_label, &&ILT_label, &&FLT_label, &&TLT_label, &&PRINT_label, &&POP_label, &&POPCPY_label, &&ENDRUN_label, &&NULFALSE1_label, &&NULFALSE2_label, &&NDIST_label, &&SDIST_label, &&PUTDIST_label, &&FINC_label, &&ENCCHA_label, &&DECCHA_label, &&SAVEPOSI_JMP_label, &&SAVEPOSF_JMP_label, &&SAVEPOSS_JMP_label, &&SORTI_label, &&SORTF_label, &&SORTS_label };

	int64 i64Temp;
	int iTemp1, iTemp2;
	double fTemp;
	char* cstrTemp;
	char bufTemp[40];
	bool boolTemp;
	csvEntry csvTemp;
	pair<char*, int> pairTemp;

	int numPrinted = 0;
	dat* stacktop = stack.data();
	int ip = 0;
	opcode *op;

	next();

//put data from stack into torow
//should make a version of this that prints directly rather than using torow
PUT_label:
	FREE1(torow[op->p1]);
	torow[op->p1] = stk0;
	DISOWN(stk0);
	pop();
	++ip;
	next();
//put data from filereader directly into torow
LDPUT_label:
	csvTemp = files[op->p3]->entries[op->p2];
	FREE1(torow[op->p1]);
	torow[op->p1] = dat{ { .s = csvTemp.val }, T, csvTemp.size };
	++ip;
	next();
LDPUTALL_label:
	iTemp1 = op->p1;
	for (auto &f : files){
		for (auto &e : f->entries){
			FREE1(torow[iTemp1]);
			torow[iTemp1++] = dat{ { .s = e.val }, T, e.size };
		}
	}
	++ip;
	next();
PUTDIST_label:
	FREE1(torow[op->p1]);
	torow[op->p1] = distinctVal;
	DISOWN(distinctVal);
	++ip;
	next();
//put variable from stack into var vector
PUTVAR_label:
	FREE1(vars[op->p1]);
	vars[op->p1] = stk0;
	DISOWN(stk0);
	pop();
	++ip;
	next();

//put variable from var vector into stack
LDVAR_label:
	push();
	stk0 = vars[op->p1];
	DISOWN(stk0); //var vector still owns c string
	++ip;
	next();
//load data from filereader to the stack
LDDUR_label:
	push();
	iTemp1 = parseDuration(files[op->p1]->entries[op->p2].val, &i64Temp);
	stk0 = dat{ { .i = i64Temp}, DR};
	if (iTemp1) stk0.b |= NIL;
	++ip;
	next();
LDDATE_label:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	iTemp1 = dateparse(csvTemp.val, &i64Temp, &iTemp2, csvTemp.size);
	stk0 = dat{ { .i = i64Temp}, DT, iTemp2 };
	if (iTemp1) stk0.b |= NIL;
	++ip;
	next();
LDTEXT_label:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0 = dat{ { .s = csvTemp.val }, T, csvTemp.size };
	if (!csvTemp.size) stk0.b |= NIL;
	++ip;
	next();
LDFLOAT_label:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.f = strtof(csvTemp.val, &cstrTemp);
	stk0.b = F;
	if (!csvTemp.size || *cstrTemp){ stk0.b |= NIL; }
	++ip;
	next();
LDINT_label:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.i = strtol(csvTemp.val, &cstrTemp, 10);
	stk0.b = I;
	if (!csvTemp.size || *cstrTemp) stk0.b |= NIL;
	++ip;
	next();
LDNULL_label:
	push();
	stk0.b = NIL;
	++ip;
	next();
LDLIT_label:
	push();
	stk0 = q->dataholder[op->p1];
	++ip;
	next();

//read a new line from a file
RDLINE_label:
	ip = files[op->p2]->readline() ? op->p1 : ip+1;
	next();
RDLINE_ORDERED_label:
	//stk0 has current read index, stk1 has vector.size()
	if (stk0.u.i < stk1.u.i){
		files[op->p2]->readlineat(posVectors[op->p3][stk0.u.i++].pos);
		++ip;
	} else {
		ip = op->p1;
	}
	next();
PREP_REREAD_label:
	push();
	stk0.u.i = posVectors[op->p1].size();
	push();
	stk0.u.i = 0;
	++ip;
	next();
SAVEPOSI_JMP_label:
	posVectors[op->p2].push_back(valPos( stk0.u.i, files[op->p3]->pos ));
	ip = op->p1;
	next();
SAVEPOSF_JMP_label:
	posVectors[op->p2].push_back(valPos( stk0.u.f, files[op->p3]->pos ));
	ip = op->p1;
	next();
SAVEPOSS_JMP_label:
	if (ISMAL(stk0)){
		cstrTemp = stk0.u.s;
		DISOWN(stk0);
	} else {
		cstrTemp = (char*) malloc(stk0.z+1);
		strcpy(cstrTemp, stk0.u.s);
	}
	posVectors[op->p2].push_back(valPos( cstrTemp, files[op->p3]->pos ));
	ip = op->p1;
	next();

#ifndef __APPLE__ //get your shit together, apple
SORTI_label:
	sort(execution::par_unseq, posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (a.val.i > b.val.i)^op->p2; });
	++ip;
	next();
SORTF_label:
	sort(execution::par_unseq, posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (a.val.f > b.val.f)^op->p2; });
	++ip;
	next();
SORTS_label:
	sort(execution::par_unseq, posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (scomp(a.val.s, b.val.s) > 0)^op->p2; });
	++ip;
	next();
#else
SORTI_label:
	sort(posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (a.val.i > b.val.i)^op->p2; });
	++ip;
	next();
SORTF_label:
	sort(posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (a.val.f > b.val.f)^op->p2; });
	++ip;
	next();
SORTS_label:
	sort(posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[op](const valPos &a, const valPos &b){ return (scomp(a.val.s, b.val.s) > 0)^op->p2; });
	++ip;
	next();
#endif

//math operations
IADD_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i += stk0.u.i;
	pop();
	++ip;
	next();
FADD_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f += stk0.u.f;
	pop();
	++ip;
	next();
TADD_label:
	strplus(stk1, stk0);
	pop();
	++ip;
	next();
DRADD_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i += stk0.u.i; stk1.b = DR; }
	pop();
	++ip;
	next();
DTADD_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i += stk0.u.i; stk1.b = DT; }
	pop();
	++ip;
	next();
ISUB_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i -= stk0.u.i;
	pop();
	++ip;
	next();
FSUB_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f -= stk0.u.f;
	pop();
	++ip;
	next();
DTSUB_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i -= stk0.u.i; stk1.b = DT; }
	pop();
	++ip;
	next();
DRSUB_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i -= stk0.u.i; stk1.b = DR; }
	pop();
	if (stk0.u.i < 0) stk0.u.i *= -1;
	++ip;
	next();
IMULT_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i *= stk0.u.i;
	pop();
	++ip;
	next();
FMULT_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f *= stk0.u.f;
	pop();
	++ip;
	next();
DRMULT_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else {
		iTemp1 = stk0.b; iTemp2 = stk1.b;
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
IDIV_label:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.i==0) stk1.b |= NIL;
	else stk1.u.i /= stk0.u.i;
	pop();
	++ip;
	next();
FDIV_label:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.f==0) stk1.b |= NIL;
	else stk1.u.f /= stk0.u.f;
	pop();
	++ip;
	next();
DRDIV_label:
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
INEG_label:
	if (ISNULL(stk0)) stk0.b |= NIL;
	else stk0.u.i *= -1;
	++ip;
	next();
FNEG_label:
	if (ISNULL(stk0)) stk0.b |= NIL;
	else stk0.u.f *= -1;
	++ip;
	next();
PNEG_label:
	stk0.u.p ^= true;
	++ip;
	next();

//comparisions - p1 determines how far down the stack to leave the result
// p2 is negator
//may want to combine with jmp
IEQ_label:
	iTemp1 = ISNULL(stk0);
	iTemp2 = ISNULL(stk1);
	if (iTemp1 ^ iTemp2) stkp(op->p1).u.p = false;
	else if (iTemp1 & iTemp2) stkp(op->p1).u.p = true;
	else stkp(op->p1).u.p = (stk1.u.i == stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FEQ_label:
	iTemp1 = ISNULL(stk0);
	iTemp2 = ISNULL(stk1);
	if (iTemp1 ^ iTemp2) stkp(op->p1).u.p = false;
	else if (iTemp1 & iTemp2) stkp(op->p1).u.p = true;
	else stkp(op->p1).u.p = (stk1.u.f == stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TEQ_label:
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
ILEQ_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.i <= stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FLEQ_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.f <= stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TLEQ_label:
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
ILT_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.i < stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
FLT_label:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.f < stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	next();
TLT_label:
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
LIKE_label:
	iTemp1 = !regexec(q->dataholder[op->p1].u.r, stk0.u.s, 0, 0, 0)^op->p2;
	FREE2(stk0);
	stk0.u.p = iTemp1;
	++ip;
	next();

POP_label:
	FREE2(stk0);
	pop();
	++ip;
	next();
POPCPY_label:
	FREE2(stk1);
	stk1 = stk0;
	DISOWN(stk0);
	pop();
	++ip;
	next();
NULFALSE1_label:
	if (ISNULL(stk0)){
		FREE2(stk0);
		stk0.u.p = false;
		ip = op->p1;
	} else ++ip;
	next();
NULFALSE2_label:
	if (ISNULL(stk0)){
		FREE2(stk0);
		pop();
		FREE2(stk0);
		stk0.u.p = false;
		ip = op->p1;
	} else ++ip;
	next();

//type conversions
CVIS_label:
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
CVFS_label:
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
CVFI_label:
	if (!ISNULL(stk0)){
		stk0.u.i = stk0.u.f;
		stk0.b = I;
	}
	++ip;
	next();
CVIF_label:
	if (!ISNULL(stk0)){
		stk0.u.f = stk0.u.i;
		stk0.b = F;
	}
	++ip;
	next();
CVSI_label:
	if (!ISNULL(stk0)){
		iTemp1 = strtol(stk0.u.s, &cstrTemp, 10);
		FREE2(stk0);
		stk0.u.i = iTemp1;
		stk0.b = I;
		if (*cstrTemp){ stk0.b |= NIL; }
	}
	++ip;
	next();
CVSF_label:
	if (!ISNULL(stk0)){
		fTemp = strtof(stk0.u.s, &cstrTemp);
		FREE2(stk0);
		stk0.u.f = fTemp;
		stk0.b = F;
		if (*cstrTemp){ stk0.b |= NIL; }
	}
	++ip;
	next();
CVSDT_label:
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
CVSDR_label:
	if (!ISNULL(stk0)){
		iTemp1 = parseDuration(stk0.u.s, &i64Temp);
		FREE2(stk0);
		stk0.u.i = i64Temp;
		stk0.b = DR;
		if (iTemp1) stk0.b |= NIL;
	}
	++ip;
	next();
CVDRS_label:
	if (!ISNULL(stk0)){
		stk0.u.s = (char*) malloc(24);
		durstring(stk0.u.i, stk0.u.s);
		stk0.b = T|MAL;
		stk0.z = strlen(stk0.u.s);
	}
	++ip;
	next();
CVDTS_label:
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
JMP_label:
	ip = op->p1;
	next();
JMPCNT_label:
	ip = (numPrinted < quantityLimit) ? op->p1 : ip+1;
	next();
JMPFALSE_label:
	ip = !stk0.u.p ? op->p1 : ip+1;
	if (op->p2 == 1){
		FREE2(stk0);
		pop();
	}
	next();
JMPTRUE_label:
	ip = stk0.u.p ? op->p1 : ip+1;
	if (op->p2 == 1){
		FREE2(stk0);
		pop();
	}
	next();
JMPNOTNULL_ELSEPOP_label:
	if (ISNULL(stk0)){
		FREE2(stk0);
		pop();
		++ip;
	} else {
		ip = op->p1;
	}
	next();

PRINT_label:
	for (int i=0; i<torowSize; ++i){
		torow[i].print();
		if (i < torowSize-1) fmt::print(",");
	}
	fmt::print("\n");
	++numPrinted;
	++ip;
	next();

//distinct checkers
NDIST_label:
	boolTemp = bt_nums[op->p2].insert(stk0.u.i).second;
	if (boolTemp) {
		distinctVal = stk0;
		++ip;
	} else {
		ip = op->p1;
	}
	pop();
	next();
SDIST_label:
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
FINC_label:
	q->dataholder[op->p1].u.f++;
	push();
	stk0 = q->dataholder[op->p1];
	++ip;
	next();
ENCCHA_label:
	pairTemp = q->crypt.chachaEncrypt(op->p1, stk0.z, stk0.u.s);
	if (stk0.b & MAL) free(stk0.u.s);
	stk0.u.s = pairTemp.first;
	stk0.z = pairTemp.second;
	stk0.b = T|MAL;
	++ip;
	next();
DECCHA_label:
	pairTemp = q->crypt.chachaDecrypt(op->p1, stk0.z, stk0.u.s);
	if (stk0.b & MAL) free(stk0.u.s);
	stk0.u.s = pairTemp.first;
	stk0.z = pairTemp.second;
	stk0.b = T|MAL;
	++ip;
	next();

ENDRUN_label:
	goto endloop;

//unimplemeted opcodes
CVER_label:
CVNO_label:
FEXP_label:
IEXP_label:
IMOD_label:
CVDRF_label:
DNEG_label:
NEQ_label:
	error("Invalid opcode");


	endloop:
	return;
}


void runquery(querySpecs &q){
	vmachine vm(q);
	vm.run();
}
