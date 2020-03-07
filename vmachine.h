#include "interpretor.h"
#include "deps/btree/btree_set.h"
#include "deps/btree/btree_map.h"
#include "deps/b64/b64.h"
#ifndef VMACH_H
#define VMACH_H

#define bmap btree::btree_map
#define bset btree::btree_set

//code prefixes are for Int Float Text Date/Duration
enum codes : unsigned char {
	CVER, CVNO,
	CVIF, CVIS, CVFI, CVFS, CVDRS, CVDTS,
	CVSI, CVSF, CVSDR, CVSDT,
	IADD, FADD, TADD, DTADD, DRADD,
	ISUB, FSUB, DTSUB, DRSUB,
	IMULT, FMULT, DRMULT,
	IDIV, FDIV, DRDIV,
	INEG, FNEG, PNEG,
	IMOD, FMOD,
	IEXP, FEXP,
	JMP, JMPCNT, JMPTRUE, JMPFALSE, JMPNOTNULL_ELSEPOP,
	RDLINE, RDLINE_ORDERED, PREP_REREAD,
	PUT, LDPUT, LDPUTALL, PUTVAR,
	LDINT, LDFLOAT, LDTEXT, LDDATE, LDDUR,
	LDNULL, LDLIT, LDVAR,
	IEQ, FEQ, TEQ, LIKE,
	ILEQ, FLEQ, TLEQ,
	ILT, FLT, TLT,
	PRINT, PUSH, POP, POPCPY, ENDRUN, NULFALSE1, NULFALSE2,
	NDIST, SDIST, PUTDIST,
	FINC, ENCCHA, DECCHA,
	SAVEPOSI_JMP, SAVEPOSF_JMP, SAVEPOSS_JMP, SORTI, SORTF, SORTS,
	GETGROUP
};
extern map<int, string> opMap;

//2d array for ops indexed by operation and datatype
enum typeOperators {
	OPADD, OPSUB, OPMULT, OPMOD, OPDIV, OPEXP, OPNEG, OPLD, OPEQ, OPLEQ, OPLT
};
static int ops[][6] = {
	{ 0, IADD, FADD, DTADD, DRADD, TADD },
	{ 0, ISUB, FSUB, DTSUB, DRSUB, 0 },
	{ 0, IMULT, FMULT, 0, DRMULT, 0 },
	{ 0, IMOD, FMOD, 0, 0, 0 },
	{ 0, IDIV, FDIV, 0, DRDIV, 0 },
	{ 0, IEXP, FEXP, 0, 0, 0 },
	{ 0, INEG, FNEG, 0, INEG, 0 },
	{ 0, LDINT, LDFLOAT, LDDATE, LDDUR, LDTEXT },
	{ 0, IEQ, FEQ, IEQ, IEQ, TEQ }, //date and duration comparision uses int operators
	{ 0, ILEQ, FLEQ, ILEQ, ILEQ, TLEQ },
	{ 0, ILT, FLT, ILT, ILT, TLT }
};
static dat* datp;


const byte I = 1;
const byte F = 2;
const byte DT = 3;
const byte DR = 4;
const byte T = 6;
const byte R = 7;
const byte RMAL = 8; //regex needs regfree() (dataholder vector only)
const byte MAL = 16; //malloced and responsible for freeing c string
const byte NIL = 32;
#define ISINT(X) ( ((X).b & 7) == I )
#define ISFLOAT(X) ( ((X).b & 7) == F )
#define ISDATE(X) ( ((X).b & 7) == DT )
#define ISDUR(X) ( ((X).b & 7) == DR )
#define ISTEXT(X) ( ((X).b & 7) == T )
#define ISNULL(X) ((X).b & NIL )
#define ISMAL(X)  ((X).b & MAL )

//free cstring when arg is array[index] and only index it once
#define FREE1(X) datp = &(X);  \
	if ( (datp)->b & MAL ){ \
		free((datp)->u.s); \
		(datp)->b &=(~MAL);\
	}
//free cstring with fewer steps if arg has no array indexing
#define FREE2(X) \
	if ( (X).b & MAL ){ \
		free((X).u.s); \
		(X).b &=(~MAL); \
	}

//passed free() responsibility to another dat
#define DISOWN(X) (X).b &=(~MAL)

//make sure to free manually like normal malloced c strings
class treeCString {
	public:
		char* s;
		treeCString(dat& d);
		treeCString();
		friend bool operator<(const treeCString& l, const treeCString& r){
			return scomp(l.s, r.s) < 0;
		}
};

class valPos {
	public:
		datunion val;
		int64 pos;
		valPos(int64, int64);
		valPos(double, int64);
		valPos(char*, int64);
		valPos();
};

class rowgroup {
	public:
		void* data;
		int type;
		vector<dat>* getRow(){ return ((vector<dat>*) data); };
		bmap<dat, rowgroup>* getMap(){ return ((bmap<dat, rowgroup>*) data); };
		rowgroup* nextGroup(dat d){
			if (data) {
				auto m = getMap()->find(d);
				if (m != getMap()->end()){
					return &m->second;
				}
			} else {
				data = new bmap<dat, rowgroup>;
				type = 2;
			}
			return &getMap()->insert(pair<dat, rowgroup>(d, rowgroup())).first->second;
		}
		vector<dat>* getVector(int size){
			if (!data) {
				data = new vector<dat>(size);
				type = 1;
			}
			return getRow();
		}
		rowgroup(){ type = 0; data =0; }
		~rowgroup(){
			if (type == 1){
				if (ISTEXT(*getRow()->begin()))
					for (auto &d : *getRow()) FREE2(d);
				delete getRow();
			} else if (type == 2) {
				delete getMap();
			}
		}
};

class vmachine {
	querySpecs* q;
	vector<shared_ptr<fileReader>> files;
	opcode* ops;
	dat* torow;
	dat distinctVal;
	int torowSize;
	int quantityLimit;
	vector<dat> destrow;
	vector<dat> midrow;
	vector<dat> stack;
	vector<vector<valPos>> posVectors;
	rowgroup groupTree;
	//separate btrees for performance
	vector<bset<int64>> bt_nums;
	vector<bset<treeCString>> bt_strings;
	public:
		void run();
		vmachine(querySpecs &q);
		~vmachine();
};

class varScoper {
	public:
		int filter;
		int policy;
		int scope;
		//map[scope][index] = already evaluated
		map<int,map<int,int>> duplicates;
		varScoper* setscope(int, int, int);
		bool checkDuplicates(int);
		bool neededHere(int, int);
};

extern map<int, string> opMap;
void strplus(dat &s1, dat &s2);

#endif
