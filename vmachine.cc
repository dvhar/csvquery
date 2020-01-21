#include "interpretor.h"
#include "vmachine.h"

//syntactic sugar for stack dereferencing
#define stk0 (*stacktop)
#define stk1 (*(stacktop-1))
#define stkp(N) (*(stacktop-N))

//map for printing opcodes
map<int, string> opMap = { {CVNO,"CVNO"}, {CVER,"CVER"}, {CVIF,"CVIF"}, {CVIS,"CVIS"}, {CVFI,"CVFI"}, {CVFS,"CVFS"}, {CVDRS,"CVDRS"}, {CVDTS,"CVDTS"}, {CVSI,"CVSI"}, {CVSF,"CVSF"}, {CVSDT,"CVSDT"}, {CVSDR,"CVSDR"}, {IADD,"IADD"}, {FADD,"FADD"}, {TADD,"TADD"}, {DTADD,"DTADD"}, {DRADD,"DRADD"}, {ISUB,"ISUB"}, {FSUB,"FSUB"}, {DRSUB,"DRSUB"}, {DTSUB,"DTSUB"}, {IMULT,"IMULT"}, {FMULT,"FMULT"}, {DRMULT,"DRMULT"}, {IDIV,"IDIV"}, {FDIV,"FDIV"}, {DRDIV,"DRDIV"}, {INEG,"INEG"}, {FNEG,"FNEG"}, {DNEG,"DNEG"}, {IMOD,"IMOD"}, {IEXP,"IEXP"}, {FEXP,"FEXP"}, {JMP,"JMP"}, {JMPCNT,"JMPCNT"}, {JMPTRUE,"JMPTRUE"}, {JMPFALSE,"JMPFALSE"}, {POP,"POP"}, {RDLINE,"RDLINE"}, {RDLINEAT,"RDLINEAT"}, {PRINT,"PRINT"}, {PUT,"PUT"}, {LDPUT,"LDPUT"}, {LDPUTALL,"LDPUTALL"}, {PUTVAR,"PUTVAR"}, {LDINT,"LDINT"}, {LDFLOAT,"LDFLOAT"}, {LDTEXT,"LDTEXT"}, {LDDATE,"LDDATE"}, {LDDUR,"LDDUR"}, {LDNULL,"LDNULL"}, {LDLIT,"LDLIT"}, {LDVAR,"LDVAR"}, {IEQ,"IEQ"}, {FEQ,"FEQ"}, {TEQ,"TEQ"}, {NEQ,"NEQ"}, {ILEQ,"ILEQ"}, {FLEQ,"FLEQ"}, {TLEQ,"TLEQ"}, {ILT,"ILT"}, {FLT,"FLT"}, {TLT,"TLT"}, {ENDRUN,"ENDRUN"}, {NULFALSE1,"NULFALSE1"}, {NULFALSE2,"NULFALSE2"}, {POPCPY,"POPCPY"}, {LIKE,"LIKE"}
};
void opcode::print(){
	cerr << ft("code: {: <9}  [{: <2}  {: <2}  {: <2}]\n", opMap[code], p1, p2, p3);
}

string dat::tostring(){
	if (b & NIL) return "";
	switch ( b & 0b00000111 ) {
	case I:  return ft("{}",u.i); break;
	case F:  return ft("{:.10g}",u.f); break;
	case DT: return ft("{}",datestring(u.i)); break;
	case DR: return ft("{}",durstring(u.i, nullptr)); break;
	case T:  return ft("{}",u.s); break;
	case R:  return ft("regex"); break;
	}
	return "";
}
void dat::print(){
	if (b & NIL) return;
	switch ( b & 0b00000111 ) {
	case I:  fmt::print("{}",u.i); break;
	case F:  fmt::print("{:.10g}",u.f); break;
	case DT: fmt::print("{}",datestring(u.i)); break;
	case DR: fmt::print("{}",durstring(u.i, nullptr)); break;
	case T:  fmt::print("{}",u.s); break;
	case R:  fmt::print("regex"); break;
	}
}

vmachine::vmachine(querySpecs &qs){
	q = &qs;

	for (int i=1; i<=q->numFiles; ++i){
		files.push_back(q->files[str2("_f", i)]);
	}

	if (q->grouping){
		//initialize midrow and point torow to it
	} else {
		destrow.resize(q->colspec.count);
		torow = destrow.data();
		torowSize = destrow.size();
		cerr << "torow size: " << torowSize << endl;
	}
	vars.resize(q->vars.size());
	//eventually set stack size based on syntax tree
	stack.resize(50);
	ops = q->bytecode.data();
	quantityLimit = q->quantityLimit;

	for (auto &d : stack)   memset(&d, 0, sizeof(dat));
	for (auto &d : vars)    memset(&d, 0, sizeof(dat));
	for (auto &d : destrow) memset(&d, 0, sizeof(dat));
	for (auto &d : midrow)  memset(&d, 0, sizeof(dat));
}

vmachine::~vmachine(){
	for (auto &d : stack)   FREE2(d);
	for (auto &d : vars)    FREE2(d);
	for (auto &d : destrow) FREE2(d);
	for (auto &d : midrow)  FREE2(d);
}
querySpecs::~querySpecs(){
	for (auto &d : literals){
		FREE2(d);
		if (d.b & RMAL){
			regfree(d.u.r);
			delete d.u.r;
		}
	}
}


//add s2 to s1
void strplus(dat &s1, dat &s2){
	if (ISNULL(s1)) { s1 = s2; DISOWN(s2); return; }
	if (ISNULL(s2)) return;
	int newlen = s1.z+s2.z+1;
	if (ISMAL(s1)){
		s1.u.s = (char*) realloc(s1.u.s, newlen);
		strcat(s1.u.s+s1.z-1, s2.u.s);
	} else {
		char* ns = (char*) malloc(newlen);
		strcpy(ns, s1.u.s);
		strcat(ns+s1.z-1, s2.u.s);
		s1.u.s = ns;
	}
	FREE2(s2);
	s1.b |= MAL;
	s1.z = newlen;
}

void vmachine::run(){
	//temporary values
	short sh1;
	date_t ll1;
	int i1, i2;
	double f1;
	char* c1;
	string st1;
	bool bl1;
	csvEntry cv;

	int numPrinted = 0;
	dat* stacktop = stack.data();
	int ip = 0;
	opcode *op;

	for (;;){
		op = ops + ip;
		//cerr << ft("ip {} opcode {} with [{} {} {}]\n", ip, opMap[op->code], op->p1, op->p2, op->p3);
		//big switch is flush-left instead of using normal indentation
		switch(op->code){

//put data from stack into torow
//should make a version of this that prints directly rather than using torow
case PUT:
	FREE1(torow[op->p1]);
	torow[op->p1] = stk0;
	DISOWN(stk0);
	--stacktop;
	++ip;
	break;
//put data from filereader directly into torow
case LDPUT:
	cv = files[op->p3]->entries[op->p2];
	FREE1(torow[op->p1]);
	torow[op->p1] = dat{ { .s = cv.val }, T, (short) cv.size };
	++ip;
	break;
case LDPUTALL:
	i1 = op->p1;
	for (auto &f : files)
		for (auto &e : f->entries){
			FREE1(torow[i1]);
			torow[i1++] = dat{ { .s = e.val }, T, (short) e.size };
		}
	++ip;
	break;

//put variable from stack into var vector
case PUTVAR:
	FREE1(vars[op->p1]);
	vars[op->p1] = stk0;
	DISOWN(stk0);
	--stacktop;
	++ip;
	break;
//put variable from var vector into stack
case LDVAR:
	*(++stacktop) = vars[op->p1];
	DISOWN(stk0); //var vector still owns c string
	++ip;
	break;

//load data from filereader to the stack
case LDDUR:
	++stacktop;
	i1 = parseDuration(files[op->p1]->entries[op->p2].val, &ll1);
	stk0 = dat{ { .i = ll1}, DR};
	if (i1) stk0.b |= NIL;
	++ip;
	break;
case LDDATE:
	++stacktop;
	cv = files[op->p1]->entries[op->p2];
	i1 = dateparse(cv.val, &ll1, &sh1, cv.size);
	stk0 = dat{ { .i = ll1}, DT, sh1 };
	if (i1) stk0.b |= NIL;
	++ip;
	break;
case LDTEXT:
	++stacktop;
	cv = files[op->p1]->entries[op->p2];
	FREE2(stk0);
	stk0 = dat{ { .s = cv.val }, T, (short) cv.size };
	if (!cv.size) stk0.b |= NIL;
	++ip;
	break;
case LDFLOAT:
	++stacktop;
	cv = files[op->p1]->entries[op->p2];
	stk0.u.f = strtof(cv.val, &c1);
	stk0.b = F;
	if (!cv.size || *c1){ stk0.b |= NIL; }
	++ip;
	break;
case LDINT:
	++stacktop;
	cv = files[op->p1]->entries[op->p2];
	stk0.u.i = strtol(cv.val, &c1, 10);
	stk0.b = I;
	if (!cv.size || *c1) stk0.b |= NIL;
	++ip;
	break;
case LDNULL:
	++stacktop;
	FREE2(stk0);
	stk0.b = NIL;
	++ip;
	break;
case LDLIT:
	++stacktop;
	FREE2(stk0);
	stk0 = q->literals[op->p1];
	++ip;
	break;

//read a new line from a file
case RDLINE:
	ip = files[op->p2]->readline() ? op->p1 : ip+1;
	break;
case RDLINEAT:
	++ip;
	//need to add random access feature to filereader
	break;

//math operations
case IADD:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i += stk0.u.i;
	--stacktop;
	++ip;
	break;
case FADD:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f += stk0.u.f;
	--stacktop;
	++ip;
	break;
case TADD:
	strplus(stk1, stk0);
	--stacktop;
	++ip;
	break;
case DRADD:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i += stk0.u.i; stk1.b = DR; }
	--stacktop;
	++ip;
	break;
case DTADD:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i += stk0.u.i; stk1.b = DT; }
	--stacktop;
	++ip;
	break;
case ISUB:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i -= stk0.u.i;
	--stacktop;
	++ip;
	break;
case FSUB:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f -= stk0.u.f;
	--stacktop;
	++ip;
	break;
case DTSUB:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i -= stk0.u.i; stk1.b = DT; }
	--stacktop;
	++ip;
	break;
case DRSUB:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else { stk1.u.i -= stk0.u.i; stk1.b = DR; }
	--stacktop;
	if (stk0.u.i < 0) stk0.u.i *= -1;
	++ip;
	break;
case IMULT:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.i *= stk0.u.i;
	--stacktop;
	++ip;
	break;
case FMULT:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else stk1.u.f *= stk0.u.f;
	--stacktop;
	++ip;
	break;
case DRMULT:
	if (ISNULL(stk0) || ISNULL(stk1)) stk1.b |= NIL;
	else {
		i1 = stk0.b; i2 = stk1.b;
		if (i1 == T_DATE){
			if (i2 == T_INT){ // date * int
				stk1.u.i = stk1.u.i * stk0.u.i;
			} else { // date * float
				stk1.u.i = stk1.u.i * stk0.u.f;
			}
		} else {
			if (i2 == T_INT){ // int * date
				stk1.u.i = stk1.u.i * stk0.u.i;
			} else { // float * date
				stk1.u.i = stk1.u.f * stk0.u.i;
			}
			stk1.b = DR;
		}
	}
	--stacktop;
	++ip;
	break;
case IDIV:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.i==0) stk1.b |= NIL;
	else stk1.u.i /= stk0.u.i;
	--stacktop;
	++ip;
	break;
case FDIV:
	if (ISNULL(stk0) || ISNULL(stk1) || stk0.u.f==0) stk1.b |= NIL;
	else stk1.u.f /= stk0.u.f;
	--stacktop;
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
	--stacktop;
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
	i1 = ISNULL(stk0);
	i2 = ISNULL(stk1);
	if (i1 ^ i2) stkp(op->p1).u.p = false;
	else if (i1 & i2) stkp(op->p1).u.p = true;
	else stkp(op->p1).u.p = (stk1.u.i == stk0.u.i)^op->p2;
	stacktop -= op->p1;
	++ip;
	break;
case FEQ:
	i1 = ISNULL(stk0);
	i2 = ISNULL(stk1);
	if (i1 ^ i2) stkp(op->p1).u.p = false;
	else if (i1 & i2) stkp(op->p1).u.p = true;
	else stkp(op->p1).u.p = (stk1.u.f == stk0.u.f)^op->p2;
	stacktop -= op->p1;
	++ip;
	break;
case TEQ:
	i1 = ISNULL(stk0);
	i2 = ISNULL(stk1);
	if (!(i1|i2)){ //none null
		bl1 = (scomp(stk1.u.s, stk0.u.s) == 0)^op->p2;
		FREE2(stk0);
		FREE2(stkp(op->p1));
		stkp(op->p1).u.p = bl1;
	} else if (i1 & i2) { //both null
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
		bl1 = (scomp(stk1.u.s, stk0.u.s) <= 0)^op->p2;
		FREE2(stk0);
		FREE2(stkp(op->p1));
		stkp(op->p1).u.p = bl1;
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
		bl1 = (scomp(stk1.u.s, stk0.u.s) < 0)^op->p2;
		FREE2(stk0);
		FREE2(stkp(op->p1));
		stkp(op->p1).u.p = bl1;
	}
	stacktop -= op->p1;
	++ip;
	break;
case LIKE:
	i1 = !regexec(q->literals[op->p1].u.r, stk0.u.s, 0, 0, 0)^op->p2;
	FREE2(stk0);
	stk0.u.p = i1;
	++ip;
	break;

case POP:
	FREE2(stk0);
	--stacktop;
	++ip;
	break;
case POPCPY: //currently only used for bools
	FREE2(stk1);
	stk1 = stk0;
	--stacktop;
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
		--stacktop;
		FREE2(stk0);
		stk0.u.p = false;
		ip = op->p1;
	} else ++ip;
	break;

//type conversions
//should use realloc instead of fmt for tostring conversions
case CVIS:
	st1 = str1(stk0.u.i);
	i1 = st1.size()+1;
	stk0.u.s = (char*) malloc(i1);
	memcpy((void*) st1.c_str(), stk0.u.s, i1);
	stk0.b = T|MAL;
	++ip;
	break;
case CVFS:
	st1 = ft("{:.10g}",stk0.u.f);
	i1 = st1.size()+1;
	stk0.u.s = (char*) malloc(i1);
	memcpy(stk0.u.s, (void*) st1.c_str(), i1);
	stk0.b = T|MAL;
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
	i1 = strtol(stk0.u.s, &c1, 10);
	FREE2(stk0);
	stk0.u.i = i1;
	stk0.b = F;
	if (*c1){ stk0.b |= NIL; }
	++ip;
	break;
case CVSF:
	f1 = strtof(stk0.u.s, &c1);
	FREE2(stk0);
	stk0.u.f = f1;
	stk0.b = F;
	if (*c1){ stk0.b |= NIL; }
	++ip;
	break;
case CVSDT:
	i1 = dateparse(stk0.u.s, &ll1, &sh1, stk0.z);
	FREE2(stk0);
	stk0.u.i = ll1;
	stk0.b = DT;
	stk0.z = sh1;
	if (i1) stk0.b |= NIL;
	++ip;
	break;
case CVSDR:
	i1 = parseDuration(stk0.u.s, &ll1);
	FREE2(stk0);
	stk0.u.i = ll1;
	stk0.b = DR;
	if (i1) stk0.b |= NIL;
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
	c1 = datestring(stk0.u.i);
	stk0.u.s = (char*) malloc(20);
	strncpy(stk0.u.s, c1, 19);
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
		--stacktop;
	}
	break;
case JMPTRUE:
	ip = stk0.u.p ? op->p1 : ip+1;
	if (op->p2 == 1){
		FREE2(stk0);
		--stacktop;
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
