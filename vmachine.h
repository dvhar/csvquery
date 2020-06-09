#include "interpretor.h"
#include "deps/btree/btree_set.h"
#include "deps/b64/b64.h"
#ifndef VMACH_H
#define VMACH_H

#define bset btree::btree_set
#define initarr(A,N,D) for (auto i=0; i<N; ++i) A[i] = D;
#define freearr(A,N) for (auto i=0; i<N; ++i) FREE1(A[i]);

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
	PUT, LDPUT, LDPUTALL, PUTVAR, PUTVAR2,
	LDINT, LDFLOAT, LDTEXT, LDDATE, LDDUR,
	LDNULL, LDLIT, LDVAR,
	IEQ, FEQ, TEQ, LIKE,
	ILEQ, FLEQ, TLEQ,
	ILT, FLT, TLT,
	PRINT, PUSH, POP, POPCPY, ENDRUN, NULFALSE1, NULFALSE2,
	NDIST, SDIST, PUTDIST, LDDIST,
	FINC, ENCCHA, DECCHA,
	SAVEPOSI_JMP, SAVEPOSF_JMP, SAVEPOSS_JMP, SORTI, SORTF, SORTS,
	GETGROUP, ONEGROUP,
	SUMI, SUMF, AVGI, AVGF, STDVI, STDVF, COUNT, MINI, MINF, MINS, MAXI, MAXF, MAXS,
	NEXTMAP, NEXTVEC, ROOTMAP, LDMID, LDPUTMID, LDPUTGRP,
	LDSTDVI, LDSTDVF, LDAVGI, LDAVGF,
	GROUPSORTROW, FREEMIDROW
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

#define valSize(C) uint(C.terminator - C.val)

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
			return strcmp(l.s, r.s) < 0;
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
		struct {
			int rowOrGroup : 3;
			int mallocedKey : 1;
			int freed : 1;
			int rowsize : 27;
		} meta;
		union { dat* vecp; map<dat, rowgroup>* mapp; } data;
		dat* getVec(){ return data.vecp; };
		map<dat, rowgroup>& getMap(){ return *data.mapp; };
		rowgroup& nextGroup(dat& d){
			if (!data.mapp) {
				data.mapp = new map<dat, rowgroup>;
				meta.rowOrGroup = 2;
				if (ISTEXT(d))
					meta.mallocedKey = 1;
			}
			auto key = d.heap();
			auto&& inserted = getMap().insert({key, rowgroup()});
			if (!inserted.second)
				FREE2(key);
			return inserted.first->second;
		}
		dat* getRow(int size){
			if (!data.vecp) {
				data.vecp = (dat*) malloc(size * sizeof(dat));
				initarr(data.vecp, size, (dat{{0},NIL}));
				meta.rowOrGroup = 1;
				meta.rowsize = size;
			}
			return getVec();
		}
		rowgroup(){ meta = {0}; data = {0}; }
		~rowgroup(){
			if (meta.rowOrGroup == 1){
				if (!meta.freed){
					freearr(data.vecp, meta.rowsize);
					delete getVec();
				}
			} else if (meta.rowOrGroup == 2) {
				if (meta.mallocedKey)
					for (auto &m : getMap())
						free(m.first.u.s);
				delete &getMap();
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
	int sortgroupsize;
	int quantityLimit;
	vector<dat> destrow;
	vector<dat> onegroup;
	vector<dat> stack;
	vector<dat*> groupSorter;
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
void trav(rowgroup &r);

#endif
