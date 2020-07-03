#include "interpretor.h"
#include "deps/btree/btree_set.h"
#include "deps/b64/b64.h"
#include <forward_list>
#ifndef VMACH_H
#define VMACH_H

#define bset btree::btree_set

enum codes : int {
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
	LDNULL, LDLIT, LDVAR, HOLDVAR,
	IEQ, FEQ, TEQ, LIKE,
	ILEQ, FLEQ, TLEQ,
	ILT, FLT, TLT,
	PRINT, PUSH, PUSH_0, POP, POPCPY, ENDRUN, NULFALSE1, NULFALSE2,
	NDIST, SDIST, PUTDIST, LDDIST,
	FINC, ENCCHA, DECCHA,
	SAVESORTN, SAVESORTS, SAVEPOS, SORT,
	GETGROUP, ONEGROUP,
	SUMI, SUMF, AVGI, AVGF, STDVI, STDVF, COUNT, MINI, MINF, MINS, MAXI, MAXF, MAXS,
	NEXTMAP, NEXTVEC, ROOTMAP, LDMID, LDPUTMID, LDPUTGRP,
	LDSTDVI, LDSTDVF, LDAVGI, LDAVGF,
	ADD_GROUPSORT_ROW, FREEMIDROW, GSORT, READ_NEXT_GROUP, NUL_TO_STR
};

//2d array for ops indexed by operation and datatype
enum typeOperators {
	OPADD, OPSUB, OPMULT, OPMOD, OPDIV, OPEXP, OPNEG, OPLD, OPEQ, OPLEQ, OPLT,
	OPMAX, OPMIN, OPSUM, OPAVG, OPSTV, OPLSTV, OPLAVG, OPSVSRT, OPDIST
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
	{ 0, IEQ, FEQ, IEQ, IEQ, TEQ },
	{ 0, ILEQ, FLEQ, ILEQ, ILEQ, TLEQ },
	{ 0, ILT, FLT, ILT, ILT, TLT },
	{ 0, MAXI, MAXF, MAXI, MAXI, MAXS },
	{ 0, MINI, MINF, MINI, MINI, MINS },
	{ 0, SUMI, SUMF, 0, SUMI, 0 },
	{ 0, AVGI, AVGF, AVGI, AVGI, 0 },
	{ 0, STDVI, STDVF, 0, STDVI, 0 },
	{ 0, LDSTDVI, LDSTDVF, 0, LDSTDVI, 0 },
	{ 0, LDAVGI, LDAVGF, LDAVGI, LDAVGI, 0 },
	{ 0, SAVESORTN, SAVESORTN, SAVESORTN, SAVESORTN, SAVESORTS },
	{ 0, NDIST, NDIST, NDIST, NDIST, SDIST },
};

#define ISINT(X) ( ((X).b & 7) == T_INT )
#define ISFLOAT(X) ( ((X).b & 7) == T_FLOAT )
#define ISDATE(X) ( ((X).b & 7) == T_DATE )
#define ISDUR(X) ( ((X).b & 7) == T_DURATION )
#define ISTEXT(X) ( ((X).b & 7) == T_STRING )
#define ISNULL(X) ((X).b & NIL )
#define ISMAL(X)  ((X).b & MAL )
#define SETNULL(X) { (X).b |= NIL; (X).u.i = 0;}

#define valSize(C) uint(C.terminator - C.val)

#define FREE2(X) \
	if ( (X).b & MAL ){ \
		free((X).u.s); \
		(X).b = NIL; \
	}

//passed free() responsibility to another dat
#define DISOWN(X) (X).b &=(~MAL)

static inline void freearr(dat* arr, int n) { for (auto i=0; i<n; ++i) FREE2(arr[i]); }
static inline void initarr(dat* arr, int n, dat&& d) { for (auto i=0; i<n; ++i) arr[i] = d; }

//make sure to free manually like normal malloced c strings
class treeCString {
	public:
		char* s;
		treeCString(dat& d){
			if (d.b & MAL){
				s = d.u.s;
				DISOWN(d);
			} else {
				if (d.b & NIL){
					s = (char*) calloc(1,1);
				} else {
					s = (char*) malloc(d.z+1);
					memcpy(s, d.u.s, d.z+1);
				}
			}
		}
		treeCString(){
			s = nullptr;
		}
		friend bool operator<(const treeCString& l, const treeCString& r){
			return strcmp(l.s, r.s) < 0;
		}
};

class rowgroup;
class vmachine {
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
	vector<int> sortIdxs;
	vector<vector<datunion>> normalSortVals;
	forward_list<char*> groupSortVars;
	unique_ptr<rowgroup> groupTree;
	public:
		vector<bset<int64>> bt_nums;
		vector<bset<treeCString>> bt_strings;
		querySpecs* q;
		void run();
		vmachine(querySpecs &q);
		~vmachine();
};

class rowgroup {
	public:
		struct {
			uint rowOrGroup : 2;
			uint mallocedKey : 1;
			bool freed : 1;
			int distinctNSetIdx : 23;
			int distinctSSetIdx : 23;
			int rowsize : 14;
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
		dat* getRow(vmachine *v){
			if (!data.vecp) {
				meta.rowsize = v->q->midcount;
				meta.rowOrGroup = 1;
				data.vecp = (dat*) malloc(meta.rowsize * sizeof(dat));
				initarr(data.vecp, meta.rowsize, (dat{{0},NIL}));
				if (v->q->distinctNFuncs){
					meta.distinctNSetIdx = v->bt_nums.size();
					for(int i=0; i<v->q->distinctNFuncs; ++i)
						v->bt_nums.emplace_back(bset<int64>());
				}
				if (v->q->distinctSFuncs){
					meta.distinctSSetIdx = v->bt_strings.size();
					for(int i=0; i<v->q->distinctSFuncs; ++i)
						v->bt_strings.emplace_back(bset<treeCString>());
				}
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
void trav(rowgroup &r);
int getSortComparer(querySpecs *q, int i);

#endif
