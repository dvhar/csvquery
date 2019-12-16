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
	IMULT, FMULT, DRMULT,
	IDIV, FDIV, DDIV,
	JMP, JMPCOND,
	RDLINE, RDLINEAT,
	PRINT, RAWROW, PUT,
	LDINT, LDFLOAT, LDTEXT, LDDATE, LDDUR
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

//compact data type
union datunion2 {
	int64 i;
	double f;
	time_t dt;
	time_t dr;
	char* s;
};
//data during processing
union datunion1 {
	int64 i;
	double f;
	char* s;
	time_t dt;
	time_t dr;
	bool p;
};
class dat {
	public:
	union datunion1 u;
	byte b; // bits for data about value
	short z; // string size
	void print();
};
class opcode {
	public:
	byte code;
	int p1;
	int p2;
};

class vmachine {
	querySpecs* q;
	vector<shared_ptr<fileReader>> files;
	vector<opcode> ops;
	void run();
	vmachine(querySpecs*);
};

#endif
