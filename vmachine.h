#include "interpretor.h"
#ifndef VMACH_H
#define VMACH_H

//code prefixes are for Int Float Text Date/Duration
enum codes : unsigned char {
	CVNO, CVER,
	CVIF, CVIS, CVFI, CVFS, CVDRS, CVDRF, CVDTS,
	CVSI, CVSF, CVSDR, CVSDT,
	IADD, FADD, TADD, DADD,
	ISUB, FSUB, DSUB,
	IMULT, FMULT, DMULT,
	IDIV, FDIV, DDIV,
	INEG, FNEG, DNEG,
	IMOD,
	IEXP, FEXP,
	JMP, JMPCNT, JMPTRUE, JMPFALSE, POP,
	RDLINE, RDLINEAT,
	PRINT, RAWROW, PUT, LDPUT, LDPUTALL, PUTVAR,
	LDINT, LDFLOAT, LDTEXT, LDDATE, LDDUR,
	LDNULL, LDLIT, LDVAR,
	IEQ, FEQ, DEQ, TEQ, NEQ,
	ILEQ, FLEQ, DLEQ, TLEQ,
	ILT, FLT, DLT, TLT,
	ENDRUN
};
extern map<int, string> opMap;

//2d array for ops indexed by operation and datatype
enum typeOperators {
	OPADD, OPSUB, OPMULT, OPDIV, OPEXP, OPNEG, OPLD, OPEQ, OPLEQ, OPLT
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
static dat* datp;

//8 bit array
const byte I = 1;
const byte F = 2;
const byte DT = 4;
const byte DR = 8;
const byte T = 16;
const byte NIL = 32;
const byte MAL = 64; //malloced and responsible for freeing c string
#define ISINT(X) ( X.b & I )
#define ISFLOAT(X) ( X.b & F )
#define ISDATE(X) ( X.b & DT )
#define ISDUR(X) ( X.b & DR )
#define ISTEXT(X) ( X.b & T )
#define ISNULL(X) ( X.b & NIL )
#define ISMAL(X) ( X.b & MAL )

//free cstring in array with one indexing operation
#define FREE1(X) datp = &X;  \
	if ( datp->b & MAL ) \
		free(datp->u.s); \
	datp->b &=(~MAL); \
//free cstring with fewer steps if arg has no array indexing
#define FREE2(X) \
	if ( X.b & MAL ) \
		free(X.u.s); \
	X.b &=(~MAL); \

//passed free() responsibility to another dat
#define DISOWN(X) X.b &=(~MAL)

class vmachine {
	querySpecs* q;
	vector<shared_ptr<fileReader>> files;
	vector<opcode> ops;
	dat* torow;
	int torowSize;
	int quantityLimit;
	vector<dat> destrow;
	vector<dat> midrow;
	vector<dat> stack;
	vector<dat> vars;
	public:
	void run();
	vmachine(querySpecs &q);
};

#endif
