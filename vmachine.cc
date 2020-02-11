#include "interpretor.h"
#include "vmachine.h"
#include <execution>

//syntactic sugar for stack dereferencing
#define stk0 (*stacktop)
#define stk1 (*(stacktop-1))
#define stkp(N) (*(stacktop-N))
#define push() ++stacktop
#define pop() --stacktop

void vmachine::run(){

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

	for (;;){
		op = ops + ip;
		//cerr << ft("ip {} opcode {} stack {}\n", ip, opMap[op->code], stacktop-stack.data());
		//big switch is flush-left instead of using normal indentation
		switch(op->code){

//put data from stack into torow
//should make a version of this that prints directly rather than using torow
case PUT:
	FREE1(torow[op->p1]);
	torow[op->p1] = stk0;
	DISOWN(stk0);
	pop();
	++ip;
	break;
//put data from filereader directly into torow
case LDPUT:
	csvTemp = files[op->p3]->entries[op->p2];
	FREE1(torow[op->p1]);
	torow[op->p1] = dat{ { .s = csvTemp.val }, T, csvTemp.size };
	++ip;
	break;
case LDPUTALL:
	iTemp1 = op->p1;
	for (auto &f : files)
		for (auto &e : f->entries){
			FREE1(torow[iTemp1]);
			torow[iTemp1++] = dat{ { .s = e.val }, T, e.size };
		}
	++ip;
	break;
case PUTDIST:
	FREE1(torow[op->p1]);
	torow[op->p1] = distinctVal;
	DISOWN(distinctVal);
	++ip;
	break;
//put variable from stack into var vector
case PUTVAR:
	FREE1(vars[op->p1]);
	vars[op->p1] = stk0;
	DISOWN(stk0);
	pop();
	++ip;
	break;

//put variable from var vector into stack
case LDVAR:
	push();
	stk0 = vars[op->p1];
	DISOWN(stk0); //var vector still owns c string
	++ip;
	break;
//load data from filereader to the stack
case LDDUR:
	push();
	iTemp1 = parseDuration(files[op->p1]->entries[op->p2].val, &i64Temp);
	stk0 = dat{ { .i = i64Temp}, DR};
	if (iTemp1) stk0.b |= NIL;
	++ip;
	break;
case LDDATE:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	iTemp1 = dateparse(csvTemp.val, &i64Temp, &iTemp2, csvTemp.size);
	stk0 = dat{ { .i = i64Temp}, DT, iTemp2 };
	if (iTemp1) stk0.b |= NIL;
	++ip;
	break;
case LDTEXT:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0 = dat{ { .s = csvTemp.val }, T, csvTemp.size };
	if (!csvTemp.size) stk0.b |= NIL;
	++ip;
	break;
case LDFLOAT:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.f = strtof(csvTemp.val, &cstrTemp);
	stk0.b = F;
	if (!csvTemp.size || *cstrTemp){ stk0.b |= NIL; }
	++ip;
	break;
case LDINT:
	push();
	csvTemp = files[op->p1]->entries[op->p2];
	stk0.u.i = strtol(csvTemp.val, &cstrTemp, 10);
	stk0.b = I;
	if (!csvTemp.size || *cstrTemp) stk0.b |= NIL;
	++ip;
	break;
case LDNULL:
	push();
	stk0.b = NIL;
	++ip;
	break;
case LDLIT:
	push();
	stk0 = q->dataholder[op->p1];
	++ip;
	break;

//read a new line from a file
case RDLINE:
	ip = files[op->p2]->readline() ? op->p1 : ip+1;
	break;
case RDLINEAT:
	files[op->p2]->readlineat(q->dataholder[op->p1].u.i);
	++ip;
	break;
case SAVEPOSI:
	posVectors[op->p1].push_back(valPos( stk0.u.i, files[op->p2]->pos ));
	break;
case SAVEPOSF:
	posVectors[op->p1].push_back(valPos( stk0.u.f, files[op->p2]->pos ));
	break;
case SAVEPOSS:
	if (ISMAL(stk0)){
		cstrTemp = stk0.u.s;
		DISOWN(stk0);
	} else {
		cstrTemp = (char*) malloc(stk0.z+1);
		strcpy(cstrTemp, stk0.u.s);
	}
	posVectors[op->p1].push_back(valPos( cstrTemp, files[op->p2]->pos ));
	break;

case SORTI:
	sort(execution::par_unseq, posVectors[op->p1].begin(), posVectors[op->p1].end(),
		[](valPos i, valPos j){ return i.val.i < j.val.i; });
	break;

//math operations
case IADD:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i += stk0.u.i;
	pop();
	++ip;
	break;
case FADD:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f += stk0.u.f;
	pop();
	++ip;
	break;
case TADD:
	strplus(stk1, stk0);
	pop();
	++ip;
	break;
case DRADD:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i += stk0.u.i; stk1.b = DR; }
	pop();
	++ip;
	break;
case DTADD:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i += stk0.u.i; stk1.b = DT; }
	pop();
	++ip;
	break;
case ISUB:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i -= stk0.u.i;
	pop();
	++ip;
	break;
case FSUB:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f -= stk0.u.f;
	pop();
	++ip;
	break;
case DTSUB:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i -= stk0.u.i; stk1.b = DT; }
	pop();
	++ip;
	break;
case DRSUB:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i -= stk0.u.i; stk1.b = DR; }
	pop();
	if (stk0.u.i < 0) stk0.u.i *= -1;
	++ip;
	break;
case IMULT:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i *= stk0.u.i;
	pop();
	++ip;
	break;
case FMULT:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f *= stk0.u.f;
	pop();
	++ip;
	break;
case DRMULT:
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
	break;
case IDIV:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.i==0) stk1.b |= NIL;
	else stk1.u.i /= stk0.u.i;
	pop();
	++ip;
	break;
case FDIV:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.f==0) stk1.b |= NIL;
	else stk1.u.f /= stk0.u.f;
	pop();
	++ip;
	break;
case DRDIV:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.f==0) stk1.b |= NIL;
	else {
		if (stk0.b == I)
			stk1.u.i /= stk0.u.i;
		else
			stk1.u.i /= stk0.u.f;
	}
	pop();
	++ip;
	break;
case INEG:
	if (ISNULL(stk0)) stk0.b |= NIL;
	else stk0.u.i *= -1;
	++ip;
	break;
case FNEG:
	if (ISNULL(stk0)) stk0.b |= NIL;
	else stk0.u.f *= -1;
	++ip;
	break;
case PNEG:
	stk0.u.p ^= true;
	++ip;
	break;

//comparisions - p1 determines how far down the stack to leave the result
// p2 is negator
//may want to combine with jmp
case IEQ:
	iTemp1 = ISNULL(stk0);
	iTemp2 = ISNULL(stk1);
	if (iTemp1 ^ iTemp2) stkp(op->p1).u.p = false;
	else if (iTemp1 & iTemp2) stkp(op->p1).u.p = true;
	else stkp(op->p1).u.p = (stk1.u.i == stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	break;
case FEQ:
	iTemp1 = ISNULL(stk0);
	iTemp2 = ISNULL(stk1);
	if (iTemp1 ^ iTemp2) stkp(op->p1).u.p = false;
	else if (iTemp1 & iTemp2) stkp(op->p1).u.p = true;
	else stkp(op->p1).u.p = (stk1.u.f == stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	break;
case TEQ:
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
	break;
case ILEQ:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.i <= stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	break;
case FLEQ:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.f <= stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	break;
case TLEQ:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else {
		boolTemp = (scomp(stk1.u.s, stk0.u.s) <= 0)^op->p2;
		FREE2(stk0);
		FREE2(stkp(op->p1));
		stkp(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	++ip;
	break;
case ILT:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.i < stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	break;
case FLT:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else stkp(op->p1).u.p = (stk1.u.f < stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	break;
case TLT:
	if (ISNULL(stk0) || ISNULL(stk1)) stkp(op->p1).u.p = false;
	else {
		boolTemp = (scomp(stk1.u.s, stk0.u.s) < 0)^op->p2;
		FREE2(stk0);
		FREE2(stkp(op->p1));
		stkp(op->p1).u.p = boolTemp;
	}
	stacktop -= op->p1;
	++ip;
	break;
case LIKE:
	iTemp1 = !regexec(q->dataholder[op->p1].u.r, stk0.u.s, 0, 0, 0)^op->p2;
	FREE2(stk0);
	stk0.u.p = iTemp1;
	++ip;
	break;

case POP:
	FREE2(stk0);
	pop();
	++ip;
	break;
case POPCPY: //currently only used for bools
	FREE2(stk1);
	stk1 = stk0;
	DISOWN(stk0);
	pop();
	++ip;
	break;
case NULFALSE1:
	if (ISNULL(stk0)){
		FREE2(stk0);
		stk0.u.p = false;
		ip = op->p1;
	} else ++ip;
	break;
case NULFALSE2:
	if (ISNULL(stk0)){
		FREE2(stk0);
		pop();
		FREE2(stk0);
		stk0.u.p = false;
		ip = op->p1;
	} else ++ip;
	break;

//type conversions
case CVIS:
	stk0.z = sprintf(bufTemp, "%lld", stk0.u.i);
	stk0.u.s = (char*) malloc(stk0.z+1);
	memcpy(stk0.u.s, bufTemp, stk0.z+1);
	if (stk0.z >= 0){
		stk0.b = T|MAL;
	} else {
		stk0.b = T|NIL;
	}
	++ip;
	break;
case CVFS:
	stk0.z = sprintf(bufTemp, "%.10g", stk0.u.f);
	stk0.u.s = (char*) malloc(stk0.z+1);
	memcpy(stk0.u.s, bufTemp, stk0.z+1);
	if (stk0.z >= 0){
		stk0.b = T|MAL;
	} else {
		stk0.b = T|NIL;
	}
	++ip;
	break;
case CVFI:
	stk0.u.i = stk0.u.f;
	++ip;
	break;
case CVIF:
	stk0.u.f = stk0.u.i;
	++ip;
	break;
case CVSI:
	iTemp1 = strtol(stk0.u.s, &cstrTemp, 10);
	FREE2(stk0);
	stk0.u.i = iTemp1;
	stk0.b = I;
	if (*cstrTemp){ stk0.b |= NIL; }
	++ip;
	break;
case CVSF:
	fTemp = strtof(stk0.u.s, &cstrTemp);
	FREE2(stk0);
	stk0.u.f = fTemp;
	stk0.b = F;
	if (*cstrTemp){ stk0.b |= NIL; }
	++ip;
	break;
case CVSDT:
	iTemp1 = dateparse(stk0.u.s, &i64Temp, &iTemp2, stk0.z);
	FREE2(stk0);
	stk0.u.i = i64Temp;
	stk0.b = DT;
	stk0.z = iTemp2;
	if (iTemp1) stk0.b |= NIL;
	++ip;
	break;
case CVSDR:
	iTemp1 = parseDuration(stk0.u.s, &i64Temp);
	FREE2(stk0);
	stk0.u.i = i64Temp;
	stk0.b = DR;
	if (iTemp1) stk0.b |= NIL;
	++ip;
	break;
case CVDRS:
	stk0.u.s = (char*) malloc(24);
	durstring(stk0.u.i, stk0.u.s);
	stk0.b = T|MAL;
	stk0.z = strlen(stk0.u.s);
	++ip;
	break;
case CVDTS:
	//make version of datestring that writes directly to arg buf
	cstrTemp = datestring(stk0.u.i);
	stk0.u.s = (char*) malloc(20);
	strncpy(stk0.u.s, cstrTemp, 19);
	stk0.b = T|MAL;
	stk0.z = 19;
	++ip;
	break;

//jump instructions
case JMP:
	ip = op->p1;
	break;
case JMPCNT:
	ip = (numPrinted < quantityLimit) ? op->p1 : ip+1;
	break;
case JMPFALSE:
	ip = !stk0.u.p ? op->p1 : ip+1;
	if (op->p2 == 1){
		FREE2(stk0);
		pop();
	}
	break;
case JMPTRUE:
	ip = stk0.u.p ? op->p1 : ip+1;
	if (op->p2 == 1){
		FREE2(stk0);
		pop();
	}
	break;
case JMPNOTNULL_ELSEPOP:
	if (ISNULL(stk0)){
		FREE2(stk0);
		pop();
		++ip;
	} else {
		ip = op->p1;
	}
	break;

case PRINT:
	for (int i=0; i<torowSize; ++i){
		torow[i].print();
		if (i < torowSize-1) fmt::print(",");
	}
	fmt::print("\n");
	++numPrinted;
	++ip;
	break;

//distinct checkers
case NDIST:
	boolTemp = bt_nums[op->p2].insert(stk0.u.i).second;
	if (boolTemp) {
		distinctVal = stk0;
		++ip;
	} else {
		ip = op->p1;
	}
	pop();
	break;
case SDIST:
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
	break;

//functions
case FINC:
	q->dataholder[op->p1].u.f++;
	push();
	stk0 = q->dataholder[op->p1];
	++ip;
	break;
case ENCCHA:
	pairTemp = q->crypt.chachaEncrypt(op->p1, stk0.z, stk0.u.s);
	if (stk0.b & MAL) free(stk0.u.s);
	stk0.u.s = pairTemp.first;
	stk0.z = pairTemp.second;
	stk0.b = T|MAL;
	++ip;
	break;
case DECCHA:
	pairTemp = q->crypt.chachaDecrypt(op->p1, stk0.z, stk0.u.s);
	if (stk0.b & MAL) free(stk0.u.s);
	stk0.u.s = pairTemp.first;
	stk0.z = pairTemp.second;
	stk0.b = T|MAL;
	++ip;
	break;

case ENDRUN:
	goto endloop;
case 0:
	error("Invalid opcode");

		} //end big switch
	} //end loop

	endloop:
	return;
}


void runquery(querySpecs &q){
	vmachine vm(q);
	vm.run();
}
