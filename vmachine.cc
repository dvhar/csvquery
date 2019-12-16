#include "interpretor.h"

//add s2 to s1
void strplus(stringy &s1, stringy &s2){
	int z1 = s1.z-1; //strlen, not including null terminator
	s1.z = z1 + s2.z;
	char* ns = (char*) malloc(s1.z);
	strcpy(ns, s1.s);
	strcat(ns+z1, s2.s);
	if (s1.m) free(s1.s);
	if (s2.m) free(s2.s);
	s1.s = ns;
	s1.m = 1;
}

void vmachine::run(){
	
	dat d1; //misc data
	int s1 = 0; //stack top index
	//stack will already be correct size from code generation stage
	byte op;
	ip = MAIN;

	for (;;){
		//big switch is flush-left instead of using normal indentation
		switch(bytes[ip]){
case IADD:
	stack[s1-1].i += stack[s1].i;
	--s1;
	break;

case FADD:
	stack[s1-1].f += stack[s1].f;
	--s1;
	break;

case TADD:
	strplus(stack[s1-1].s, stack[s1].s);
	--s1;
	break;

case DADD:
	break;

case ISUB:
	stack[s1-1].i -= stack[s1].i;
	--s1;
	break;

case FSUB:
	stack[s1-1].f -= stack[s1].f;
	--s1;
	break;

case DSUB:
	break;

case IMULT:
	stack[s1-1].i *= stack[s1].i;
	--s1;
	break;

case FMULT:
	stack[s1-1].f *= stack[s1].f;
	--s1;
	break;

case DMULT:
	break;

case IDIV:
	stack[s1-1].i /= stack[s1].i;
	--s1;
	break;

case FDIV:
	stack[s1-1].f /= stack[s1].f;
	--s1;
	break;

case DDIV:
	stack[s1-1].dr /= stack[s1].f; //make sure duration/num num is float
	--s1;
	break;

case JMP:
	break;

case JMPNEG:
	break;

case JMPPOS:
	break;

case JMPZ:
	break;

case RDLINE:
	break;

case RDLINEAT:
	break;

case PRINT:
	break;

		} //end big switch
	}
}
