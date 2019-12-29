#include "interpretor.h"
#ifndef VMACH_H
#define VMACH_H

//virtual machine stuff
//labels of subroutine instruction number
extern int MAIN, VARS_NORM, VARS_AGG, SEL_NORM, SEL_AGG, WHERE, HAVING, ORDER;

//code prefixes are for Int Float Text Date/Duration
enum codes : unsigned char {
	IADD, FADD, TADD, DADD,
	ISUB, FSUB, DSUB,
	IMULT, FMULT, DMULT,
	IDIV, FDIV, DDIV,
	INEG, FNEG, DNEG,
	IMOD,
	IEXP, FEXP,
	JMP, JMPTRUE, JMPFALSE, POP,
	RDLINE, RDLINEAT,
	PRINT, RAWROW, PUT, LDPUT, PUTVAR, LDVAR,
	LDINT, LDFLOAT, LDTEXT, LDDATE, LDDUR,
	IEQ, FEQ, DEQ, TEQ, NEQ,
	ILEQ, FLEQ, DLEQ, TLEQ,
	ILT, FLT, DLT, TLT
};
extern map<int, string> opMap;

//2d array for ops indexed by operation and datatype
enum typeOperators {
	OPADD, OPSUB, OPMULT, OPDIV, OPEXP, OPNEG, OPLD, OPEQ
};
static int ops[][6] = {
	{ 0, IADD, FADD, DADD, DADD, TADD },
	{ 0, ISUB, FSUB, DSUB, DSUB, 0 },
	{ 0, IMULT, FMULT, 0, DMULT, 0 },
	{ 0, IDIV, FDIV, 0, DDIV, 0 },
	{ 0, IEXP, FEXP, 0, 0, 0 },
	{ 0, INEG, FNEG, 0, DNEG, 0 },
	{ 0, LDINT, LDFLOAT, LDDATE, LDDUR, LDTEXT },
	{ 0, IEQ, FEQ, DEQ, DEQ, TEQ },
	{ 0, ILEQ, FLEQ, DLEQ, DLEQ, TLEQ },
	{ 0, ILT, FLT, DLT, DLT, TLT }
};

//8 bit array
const byte I = 1;
const byte F = 2;
const byte DT = 4;
const byte DR = 8;
const byte T = 16;
const byte NIL = 32;
const byte MAL = 64; //malloced
#define ISINT(X) ( X.b & I )
#define ISFLOAT(X) ( X.b & F )
#define ISDATE(X) ( X.b & DT )
#define ISDUR(X) ( X.b & DR )
#define ISTEXT(X) ( X.b & T )
#define ISNULL(X) ( X.b & NIL )
#define ISMAL(X) ( X.b & MAL )

//data during processing
union datunion {
	int64 i;
	double f;
	char* s;
	time_t dt;
	time_t dr;
	bool p;
};
class dat {
	public:
	union datunion u;
	byte b; // bits for data about value
	short z; // string size
	void print();
};
class vmachine {
	querySpecs* q;
	vector<shared_ptr<fileReader>> files;
	vector<opcode> ops;
	vector<dat> torow;
	vector<dat> stack;
	vector<dat> vars;
	void run();
	vmachine(querySpecs*);
};

#endif
