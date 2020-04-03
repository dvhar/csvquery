#include "interpretor.h"
#include "deps/btree/btree_set.h"
#include "deps/b64/b64.h"
#ifndef VMACH_H
#define VMACH_H

#define bset btree::btree_set

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
	NDIST, SDIST, PUTDIST, LDDIST,
	FINC, ENCCHA, DECCHA,
	SAVEPOSI_JMP, SAVEPOSF_JMP, SAVEPOSS_JMP, SORTI, SORTF, SORTS,
	GETGROUP,
	SUMI, SUMF, AVGI, AVGF, STDVI, STDVF, COUNT, MINI, MINF, MINS, MAXI, MAXF, MAXS,
	NEXTMAP, NEXTVEC, ROOTMAP, LDMID, LDPUTMID
};

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


#define ISINT(X) ( ((X).b & 7) == T_INT )
#define ISFLOAT(X) ( ((X).b & 7) == T_FLOAT )
#define ISDATE(X) ( ((X).b & 7) == T_DATE )
#define ISDUR(X) ( ((X).b & 7) == T_DURATION )
#define ISTEXT(X) ( ((X).b & 7) == T_STRING )
#define ISNULL(X) ((X).b & NIL )
#define ISMAL(X)  ((X).b & MAL )
#define SETNULL(X) { (X).b |= NIL; (X).u.i = 0;}

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
		string str(){
			if (type==2) return ft("map-> {}", getMap()->size());
			if (type==1) return "vec";
			return "blank";
		}
		vector<dat>* getRow(){ return ((vector<dat>*) data); };
		map<dat, rowgroup>* getMap(){ return ((map<dat, rowgroup>*) data); };
		rowgroup* nextGroup(dat d){
			if (!data) {
				data = new map<dat, rowgroup>;
				type = 2;
			}
			return &getMap()->insert({d, rowgroup()}).first->second;
		}
		vector<dat>* getVector(int size){
			if (!data) {
				data = new vector<dat>(size, {{0},NIL,0});
				type = 1;
			}
			return getRow();
		}
		rowgroup(){ type = 0; data = 0; }
		~rowgroup(){
			if (type == 1){
				cerr << "destruct row\n";
				if (ISTEXT(*getRow()->begin()))
					for (auto &d : *getRow()) FREE2(d);
				delete getRow();
			} else if (type == 2) {
				cerr << "destruct map\n";
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

extern map<byte, string> opMap;
void strplus(dat &s1, dat &s2);

#endif
