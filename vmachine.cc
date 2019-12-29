#include "interpretor.h"
#include "vmachine.h"

//subroutine labels
int MAIN, VARS_NORM, VARS_AGG, SEL_NORM, SEL_AGG, WHERE, HAVING, ORDER;

map<int, string> opMap = {
	{IADD,   "IADD"},
	{FADD,   "FADD"},
	{TADD,   "TADD"},
	{DADD,   "DADD"},
	{ISUB,   "ISUB"},
	{FSUB,   "FSUB"},
	{DSUB,   "DSUB"},
	{IMULT,   "IMULT"},
	{FMULT,   "FMULT"},
	{DMULT,   "DMULT"},
	{IDIV,   "IDIV"},
	{FDIV,   "FDIV"},
	{DDIV,   "DDIV"},
	{INEG,   "INEG"},
	{FNEG,   "FNEG"},
	{DNEG,   "DNEG"},
	{IMOD,   "IMOD"},
	{IEXP,   "IEXP"},
	{FEXP,   "FEXP"},
	{JMP,   "JMP"},
	{JMPCOND,   "JMPCOND"},
	{RDLINE,   "RDLINE"},
	{RDLINEAT,   "RDLINEAT"},
	{PRINT,   "PRINT"},
	{RAWROW,   "RAWROW"},
	{PUT,   "PUT"},
	{LDPUT,   "LDPUT"},
	{PUTVAR,   "PUTVAR"},
	{LDVAR,   "LDVAR"},
	{LDINT,   "LDINT"},
	{LDFLOAT,   "LDFLOAT"},
	{LDTEXT,   "LDTEXT"},
	{LDDATE,   "LDDATE"},
	{LDDUR,   "LDDUR"}
};
void dat::print(){
	if (b & NIL) return;
	switch ( b & 0b00011111 ) {
	case I: cout << u.i << ","; break;
	case F: cout << u.f << ","; break;
	case DT: cout << u.dt << ","; break; //need to format dates
	case DR: cout << u.dr << ","; break; //need to format durations
	case T: cout << u.s << ","; break;
	}
}

void opcode::print(){
	cerr << fmt::format("code: {}  p1: {}  p2:  {}  p3: {}\n", opMap[code], p1, p2, p3);
}

vmachine::vmachine(querySpecs* qs){
	q = qs;
	for (int i=0; i<q->numFiles; ++i)
		files.push_back(q->files[str2("_f", i)]);

}

//add s2 to s1
void strplus(dat &s1, dat &s2){
	if (ISNULL(s1)) { s1 = s2; return; }
	if (ISNULL(s2)) return;
	int newlen = s1.z+s2.z-1;
	if (ISMAL(s1)){
		s1.u.s = (char*) realloc(s1.u.s, newlen);
		strcat(s1.u.s+s1.z-1, s2.u.s);
	} else {
		char* ns = (char*) malloc(newlen);
		strcpy(ns, s1.u.s);
		strcat(ns+s1.z-1, s2.u.s);
		s1.u.s = ns;
	}
	if (ISMAL(s2)) free(s2.u.s);
	s1.b |= MAL;
	s1.z = newlen;
}

void vmachine::run(){
	
	int s1 = 0; //stack top index
	int t1, t2; //temporary values
	int ip = MAIN;
	opcode op;

	for (;;){
		//big switch is flush-left instead of using normal indentation
		op = ops[ip];
		switch(op.code){

//put data from stack into torow
case PUT:
	torow[op.p1] = stack[s1];
	--s1;
	++ip;
	break;
//put data from filereader directly into torow
case LDPUT:
	torow[op.p1].u.s = files[op.p3]->line[op.p2];
	torow[op.p1].z = files[op.p3]->sizes[op.p2];
	torow[op.p1].b = T;
	++ip;
	break;

//put variable from stack into var vector
case PUTVAR:
	vars[op.p1] = stack[s1--];
	++ip;
	break;
//put variable from var vector into stack
case LDVAR:
	stack[++s1] = vars[op.p1];
	++ip;
	break;

//load data from filereader to the stack - need to check for nulls
case LDDUR:
	++ip;
	break;
case LDDATE:
	++ip;
	break;
case LDTEXT:
	++s1;
	stack[s1].u.s = files[op.p1]->line[op.p2];
	stack[s1].z = files[op.p1]->sizes[op.p2];
	stack[s1].b = T;
	++ip;
	break;
case LDFLOAT:
	++s1;
	stack[s1].u.f = atof(files[op.p1]->line[op.p2]);
	stack[s1].b = F;
	++ip;
	break;
case LDINT:
	++s1;
	stack[s1].u.i = atoi(files[op.p1]->line[op.p2]);
	stack[s1].b = I;
	++ip;
	break;

//read a new line from a file
case RDLINE:
	files[op.p1]->readline();
	++ip;
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
case IEQ:
	t1 = ISNULL(stack[s1]);
	t2 = ISNULL(stack[s1-1]);
	if (t1 ^ t2) stack[s1-op.p1].u.p = false;
	else if (t1 && t2) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = stack[s1-1].u.i == stack[s1].u.i;
	break;
case FEQ:
	t1 = ISNULL(stack[s1]);
	t2 = ISNULL(stack[s1-1]);
	if (t1 ^ t2) stack[s1-op.p1].u.p = false;
	else if (t1 && t2) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = stack[s1-1].u.f == stack[s1].u.f;
	break;
case DEQ:
	break;
case TEQ:
	t1 = ISNULL(stack[s1]);
	t2 = ISNULL(stack[s1-1]);
	if (t1 ^ t2) stack[s1-op.p1].u.p = false;
	else if (t1 && t2) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = scomp(stack[s1-1].u.s, stack[s1].u.s) == 0;
	break;
case ILEQ:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = stack[s1-1].u.i <= stack[s1].u.i;
	break;
case FLEQ:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = stack[s1-1].u.f <= stack[s1].u.f;
	break;
case DLEQ:
	break;
case TLEQ:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = scomp(stack[s1-1].u.s, stack[s1].u.s) <= 0;
	break;
case ILT:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = stack[s1-1].u.i < stack[s1].u.i;
	break;
case FLT:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = stack[s1-1].u.f < stack[s1].u.f;
	break;
case DLT:
	break;
case TLT:
	if (ISNULL(stack[s1]) || ISNULL(stack[s1-1])) stack[s1-op.p1].u.p = false;
	else stack[s1-op.p1].u.p = scomp(stack[s1-1].u.s, stack[s1].u.s) < 0;
	break;

//compare to null
case NEQ:
	stack[s1].u.p = ISNULL(stack[s1]);
	break;

//jump instructions
case JMP:
	ip = op.p1;
	break;

case JMPCOND:
	if (stack[s1].u.p)
		ip = op.p1;
	--s1;
	break;

case PRINT:
	for (auto &d : torow)
		d.print();
	cout << endl;
	++ip;
	break;

		} //end big switch
	}
}