#include "interpretor.h"
#include "deps/btree/btree_set.h"
#ifndef VMACH_H
#define VMACH_H

//code prefixes are for Int Float Text Date/Duration
enum codes : unsigned char {
	CVER, CVNO,
	CVIF, CVIS, CVFI, CVFS, CVDRS, CVDRF, CVDTS,
	CVSI, CVSF, CVSDR, CVSDT,
	IADD, FADD, TADD, DTADD, DRADD,
	ISUB, FSUB, DTSUB, DRSUB,
	IMULT, FMULT, DRMULT,
	IDIV, FDIV, DRDIV,
	INEG, FNEG, DNEG, PNEG,
	IMOD,
	IEXP, FEXP,
	JMP, JMPCNT, JMPTRUE, JMPFALSE,
	RDLINE, RDLINEAT,
	PUT, LDPUT, LDPUTALL, PUTVAR,
	LDINT, LDFLOAT, LDTEXT, LDDATE, LDDUR,
	LDNULL, LDLIT, LDVAR,
	IEQ, FEQ, TEQ, NEQ, LIKE,
	ILEQ, FLEQ, TLEQ,
	ILT, FLT, TLT,
	PRINT, POP, POPCPY, ENDRUN, NULFALSE1, NULFALSE2
};
extern map<int, string> opMap;

//2d array for ops indexed by operation and datatype
enum typeOperators {
	OPADD, OPSUB, OPMULT, OPDIV, OPEXP, OPNEG, OPLD, OPEQ, OPLEQ, OPLT
};
static int ops[][6] = {
	{ 0, IADD, FADD, DTADD, DRADD, TADD },
	{ 0, ISUB, FSUB, DTSUB, DRSUB, 0 },
	{ 0, IMULT, FMULT, 0, DRMULT, 0 },
	{ 0, IDIV, FDIV, 0, DRDIV, 0 },
	{ 0, IEXP, FEXP, 0, 0, 0 },
	{ 0, INEG, FNEG, 0, DNEG, 0 },
	{ 0, LDINT, LDFLOAT, LDDATE, LDDUR, LDTEXT },
	{ 0, IEQ, FEQ, IEQ, IEQ, TEQ }, //date and duration comparision uses int operators
	{ 0, ILEQ, FLEQ, ILEQ, ILEQ, TLEQ },
	{ 0, ILT, FLT, ILT, ILT, TLT }
};
static dat* datp;

//8 bit array
const byte I = 1;
const byte F = 2;
const byte DT = 3;
const byte DR = 4;
const byte T = 6;
const byte R = 7;
const byte RMAL = 8; //regex needs regfree() (literals vector only)
const byte MAL = 16; //malloced and responsible for freeing c string
const byte NIL = 32;
#define ISINT(X) ( X.b & I )
#define ISFLOAT(X) ( X.b & F )
#define ISDATE(X) ( X.b & DT )
#define ISDUR(X) ( X.b & DR )
#define ISTEXT(X) ( X.b & T )
#define ISNULL(X) ( X.b & NIL )
#define ISMAL(X) ( X.b & MAL )

//free cstring when arg is array[index] and only index it once
#define FREE1(X) datp = &X;  \
	if ( datp->b & MAL ){ \
		free(datp->u.s); \
		datp->b &=(~MAL);\
	}
//free cstring with fewer steps if arg has no array indexing
#define FREE2(X) \
	if ( X.b & MAL ){ \
		free(X.u.s); \
		X.b &=(~MAL); \
	}

//passed free() responsibility to another dat
#define DISOWN(X) X.b &=(~MAL)

class vmachine {
	querySpecs* q;
	vector<shared_ptr<fileReader>> files;
	opcode* ops;
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
	~vmachine();
};

#endif
