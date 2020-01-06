#include "interpretor.h"
#include "vmachine.h"

map<int, string> opMap = { {CVNO,"CVNO"}, {CVER,"CVER"}, {CVIF,"CVIF"}, {CVIS,"CVIS"}, {CVFI,"CVFI"}, {CVFS,"CVFS"}, {CVDRS,"CVDRS"}, {CVDTS,"CVDTS"}, {CVSI,"CVSI"}, {CVSF,"CVSF"}, {CVSDT,"CVSDT"}, {CVSDR,"CVSDR"}, {IADD,"IADD"}, {FADD,"FADD"}, {TADD,"TADD"}, {DADD,"DADD"}, {ISUB,"ISUB"}, {FSUB,"FSUB"}, {DSUB,"DSUB"}, {IMULT,"IMULT"}, {FMULT,"FMULT"}, {DMULT,"DMULT"}, {IDIV,"IDIV"}, {FDIV,"FDIV"}, {DDIV,"DDIV"}, {INEG,"INEG"}, {FNEG,"FNEG"}, {DNEG,"DNEG"}, {IMOD,"IMOD"}, {IEXP,"IEXP"}, {FEXP,"FEXP"}, {JMP,"JMP"}, {JMPCNT,"JMPCNT"}, {JMPTRUE,"JMPTRUE"}, {JMPFALSE,"JMPFALSE"}, {POP,"POP"}, {RDLINE,"RDLINE"}, {RDLINEAT,"RDLINEAT"}, {PRINT,"PRINT"}, {RAWROW,"RAWROW"}, {PUT,"PUT"}, {LDPUT,"LDPUT"}, {LDPUTALL,"LDPUTALL"}, {PUTVAR,"PUTVAR"}, {LDINT,"LDINT"}, {LDFLOAT,"LDFLOAT"}, {LDTEXT,"LDTEXT"}, {LDDATE,"LDDATE"}, {LDDUR,"LDDUR"}, {LDNULL,"LDNULL"}, {LDLIT,"LDLIT"}, {LDVAR,"LDVAR"}, {IEQ,"IEQ"}, {FEQ,"FEQ"}, {DEQ,"DEQ"}, {TEQ,"TEQ"}, {NEQ,"NEQ"}, {ILEQ,"ILEQ"}, {FLEQ,"FLEQ"}, {DLEQ,"DLEQ"}, {TLEQ,"TLEQ"}, {ILT,"ILT"}, {FLT,"FLT"}, {DLT,"DLT"}, {TLT,"TLT"}, {ENDRUN,"ENDRUN"}
};

void dat::print(){
	if (b & NIL) return;
	switch ( b & 0b00011111 ) {
	case I:  fmt::print("{}",u.i); break;
	case F:  fmt::print("{:.10g}",u.f); break;
	case DT: fmt::print("{}",u.dt); break; //need to format dates
	case DR: fmt::print("{}",u.dr); break; //need to format durations
	case T:  fmt::print("{}",u.s); break;
	}
}

void opcode::print(){
	cerr << ft("code: {: <9}  [{: <2}  {: <2}  {: <2}]\n", opMap[code], p1, p2, p3);
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
	ops = q->bytecode;
	quantityLimit = q->quantityLimit;

	for (auto &d : stack)   memset(&d, 0, sizeof(dat));
	for (auto &d : vars)    memset(&d, 0, sizeof(dat));
	for (auto &d : destrow) memset(&d, 0, sizeof(dat));
	for (auto &d : midrow)  memset(&d, 0, sizeof(dat));
}

//add s2 to s1
//need to make this much safer
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
	FREE(s2);
	s1.b |= MAL;
	s1.z = newlen;
}

void vmachine::run(){
	//temporary values
	int i1, i2;
	double f1;
	char* c1;
	string st1;
	bool bl1;
	csvEntry cv;
	byte b1;
	dat *d1;

	int numPrinted = 0;
	int s1 = 0; //stack top index
	int ip = 0;
	opcode op;

	for (;;){
		//big switch is flush-left instead of using normal indentation
		op = ops[ip];
		//cerr << ft("ip {} opcode {} with [{} {} {}]\n", ip, opMap[op.code], op.p1, op.p2, op.p3);
		switch(op.code){

//put data from stack into torow
case PUT:
	FREE(torow[op.p1]);
	torow[op.p1] = stack[s1];
	DISOWN(stack[s1]);
	--s1;
	++ip;
	break;
//put data from filereader directly into torow
case LDPUT:
	cv = files[op.p3]->entries[op.p2];
	FREE(torow[op.p1]);
	torow[op.p1] = dat{ { .s = cv.val }, T, (short) cv.size };
	++ip;
	break;
case LDPUTALL:
	i1 = op.p1;
	for (auto &f : files)
		for (auto &e : f->entries){
			FREE(torow[i1]);
			torow[i1++] = dat{ { .s = e.val }, T, (short) e.size };
		}
	++ip;
	break;

//put variable from stack into var vector
case PUTVAR:
	FREE(vars[op.p1]);
	vars[op.p1] = stack[s1];
	DISOWN(stack[s1]);
	--s1;
	++ip;
	break;
//put variable from var vector into stack
case LDVAR:
	stack[++s1] = vars[op.p1];
	DISOWN(stack[s1]); //var vector still owns c string
	++ip;
	break;

//load data from filereader to the stack - need to check for nulls
case LDDUR:
	++s1;
	i1 = parseDuration(files[op.p1]->entries[op.p2].val, (time_t*)&i2);
	stack[s1].u.dr = i2;
	if (i1) b1 |= NIL;
	++ip;
	break;
case LDDATE:
	++ip;
	break;
case LDTEXT:
	++s1;
	cv = files[op.p1]->entries[op.p2];
	FREE(stack[s1]);
	stack[s1] = dat{ { .s = cv.val }, T, (short) cv.size };
	if (!cv.size) stack[s1].b |= NIL;
	++ip;
	break;
case LDFLOAT:
	++s1;
	cv = files[op.p1]->entries[op.p2];
	stack[s1].u.f = strtof(cv.val, &c1);
	b1 = F;
	if (!cv.size || *c1){ b1 |= NIL; }
	stack[s1].b = b1;
	++ip;
	break;
case LDINT:
	++s1;
	cv = files[op.p1]->entries[op.p2];
	stack[s1].u.i = strtol(cv.val, &c1, 10);
	b1 = I;
	if (!cv.size || *c1) b1 |= NIL;
	stack[s1].b = b1;
	++ip;
	break;
case LDNULL:
	++s1;
	FREE(stack[s1]);
	stack[s1].b = NIL;
	++ip;
	break;
case LDLIT:
	++s1;
	FREE(stack[s1]);
	stack[s1] = q->literals[op.p1];
	++ip;
	break;

//read a new line from a file
case RDLINE:
	ip = files[op.p2]->readline() ? op.p1 : ip+1;
	break;
case RDLINEAT:
	++ip;
	//need to add random access feature to filereader
	break;

//math operations
case IADD:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-1].b |= NIL;
	else stack[s1-1].u.i += stack[s1].u.i;
	--s1;
	++ip;
	break;
case FADD:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-1].b |= NIL;
	else stack[s1-1].u.f += stack[s1].u.f;
	--s1;
	++ip;
	break;
case TADD:
	strplus(stack[s1-1], stack[s1]);
	--s1;
	++ip;
	break;
case DADD:
	++ip;
	break;
case ISUB:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-1].b |= NIL;
	else stack[s1-1].u.i -= stack[s1].u.i;
	--s1;
	++ip;
	break;
case FSUB:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-1].b |= NIL;
	else stack[s1-1].u.f -= stack[s1].u.f;
	--s1;
	++ip;
	break;
case DSUB:
	++ip;
	break;
case IMULT:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-1].b |= NIL;
	else stack[s1-1].u.i *= stack[s1].u.i;
	--s1;
	++ip;
	break;
case FMULT:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-1].b |= NIL;
	else stack[s1-1].u.f *= stack[s1].u.f;
	--s1;
	++ip;
	break;
case DMULT:
	break;
case IDIV:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1]) || stack[s1].u.i==0) stack[s1-1].b |= NIL;
	else stack[s1-1].u.i /= stack[s1].u.i;
	--s1;
	++ip;
	break;
case FDIV:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1]) || stack[s1].u.f==0) stack[s1-1].b |= NIL;
	else stack[s1-1].u.f /= stack[s1].u.f;
	--s1;
	++ip;
	break;
case DDIV:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1]) || stack[s1].u.f==0) stack[s1-1].b |= NIL;
	else stack[s1-1].u.dr /= stack[s1].u.f; //make sure duration/num num is float
	--s1;
	++ip;
	break;
case INEG:
	if (ISNULL(stack[s1])) stack[s1].b |= NIL;
	else stack[s1].u.i *= -1;
	++ip;
	break;
case FNEG:
	if (ISNULL(stack[s1])) stack[s1].b |= NIL;
	else stack[s1].u.f *= -1;
	++ip;
	break;

//comparisions - p1 determines how far down the stack to leave the result
// p2 is negator
//may want to combine with jmp
case IEQ:
	i1 = ISNULL(stack[s1]);
	i2 = ISNULL(stack[s1-1]);
	if (i1 ^ i2) stack[s1-op.p1].u.p = false;
	else if (i1 & i2) stack[s1-op.p1].u.p = true;
	else stack[s1-op.p1].u.p = (stack[s1-1].u.i == stack[s1].u.i)^op.p2;
	s1 -= op.p1;
	++ip;
	break;
case FEQ:
	i1 = ISNULL(stack[s1]);
	i2 = ISNULL(stack[s1-1]);
	if (i1 ^ i2) stack[s1-op.p1].u.p = false;
	else if (i1 & i2) stack[s1-op.p1].u.p = true;
	else stack[s1-op.p1].u.p = (stack[s1-1].u.f == stack[s1].u.f)^op.p2;
	s1 -= op.p1;
	++ip;
	break;
case DEQ:
	++ip;
	break;
case TEQ:
	i1 = ISNULL(stack[s1]);
	i2 = ISNULL(stack[s1-1]);
	if (!(i1|i2)){ //none null
		bl1 = (scomp(stack[s1-1].u.s, stack[s1].u.s) == 0)^op.p2;
		FREE(stack[s1]);
		FREE(stack[s1-op.p1]);
		stack[s1-op.p1].u.p = bl1;
	} else if (i1 & i2) { //both null
		stack[s1-op.p1].u.p = true;
	} else { //one null
		FREE(stack[s1]);
		FREE(stack[s1-op.p1]);
		stack[s1-op.p1].u.p = false;
	}
	s1 -= op.p1;
	++ip;
	break;
case ILEQ:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = (stack[s1-1].u.i <= stack[s1].u.i)^op.p2;
	s1 -= op.p1;
	++ip;
	break;
case FLEQ:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = (stack[s1-1].u.f <= stack[s1].u.f)^op.p2;
	s1 -= op.p1;
	++ip;
	break;
case DLEQ:
	++ip;
	break;
case TLEQ:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = (scomp(stack[s1-1].u.s, stack[s1].u.s) <= 0)^op.p2;
	s1 -= op.p1;
	++ip;
	break;
case ILT:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = (stack[s1-1].u.i < stack[s1].u.i)^op.p2;
	s1 -= op.p1;
	++ip;
	break;
case FLT:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = (stack[s1-1].u.f < stack[s1].u.f)^op.p2;
	s1 -= op.p1;
	++ip;
	break;
case DLT:
	++ip;
	break;
case TLT:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = (scomp(stack[s1-1].u.s, stack[s1].u.s) < 0)^op.p2;
	s1 -= op.p1;
	++ip;
	break;

case POP:
	FREE(stack[s1]); //add more if ever pop more than one
	s1 -= op.p1;
	++ip;
	break;

//type conversions
case CVIS:
	d1 = &stack[s1];
	st1 = str1(d1->u.i);
	i1 = st1.size()+1;
	d1->u.s = (char*) malloc(i1);
	memcpy((void*) st1.c_str(), d1->u.s, i1);
	d1->b = T|MAL;
	++ip;
	break;
case CVFS:
	d1 = &stack[s1];
	st1 = ft("{:.10g}",d1->u.f);
	i1 = st1.size()+1;
	d1->u.s = (char*) malloc(i1);
	memcpy(d1->u.s, (void*) st1.c_str(), i1);
	d1->b = T|MAL;
	++ip;
	break;
case CVFI:
	stack[s1].u.i = stack[s1].u.f;
	++ip;
	break;
case CVIF:
	stack[s1].u.f = stack[s1].u.i;
	++ip;
	break;
case CVSI:
	d1 = &stack[s1];
	i1 = strtol(d1->u.s, &c1, 10);
	FREE((*d1));
	b1 = F;
	if (*c1){ b1 |= NIL; }
	d1->b = b1;
	d1->u.i = i1;
	++ip;
	break;
case CVSF:
	d1 = &stack[s1];
	f1 = strtof(d1->u.s, &c1);
	FREE((*d1));
	b1 = F;
	if (*c1){ b1 |= NIL; }
	d1->b = b1;
	d1->u.f = f1;
	++ip;
	break;
case CVSDT:
	++ip;
	break;
case CVSDR:
	++ip;
	break;
case CVDRS:
	++ip;
	break;
case CVDTS:
	++ip;
	break;

//jump instructions
case JMP:
	ip = op.p1;
	break;
case JMPCNT:
	ip = (numPrinted < quantityLimit) ? ip = op.p1 : ip+1;
	break;
case JMPFALSE:
	ip = !stack[s1].u.p ? op.p1 : ip+1;
	if (op.p2 == 1){
		FREE(stack[s1]);
		--s1;
	}
	break;
case JMPTRUE:
	ip = stack[s1].u.p ? op.p1 : ip+1;
	if (op.p2 == 1){
		FREE(stack[s1]);
		--s1;
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

		} //end big switch
	} //end loop
	endloop:
	return;
}


void runquery(querySpecs &q){
	vmachine vm(q);
	vm.run();
}
