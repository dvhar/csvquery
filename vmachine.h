#pragma once

#include "interpretor.h"
#include "deps/btree/btree_set.h"
#include "deps/crypto/base64.h"
#include <math.h>
#include <mutex>
#include <future>

template<typename T>
using bset = btree::btree_set<T>;
using grprow = const unique_ptr<dat[], freeC>;

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
	IPOW, FPOW,
	JMP, JMPCNT, JMPTRUE, JMPFALSE, JMPNOTNULL_ELSEPOP,
	RDLINE, RDLINE_ORDERED, PREP_REREAD,
	PUT, LDPUT, LDPUTALL, PUTVAR, PUTVAR2,
	LDINT, LDFLOAT, LDTEXT, LDDATE, LDDUR,
	LDLIT, LDVAR, HOLDVAR,
	IEQ, FEQ, TEQ, LIKE,
	ILEQ, FLEQ, TLEQ,
	ILT, FLT, TLT,
	PRINTCSV, PRINTJSON, PUSH, PUSH_N, POP, POPCPY, ENDRUN, NULFALSE,
	NDIST, SDIST, PUTDIST, LDDIST,
	FINC, ENCCHA, DECCHA,
	SAVESORTN, SAVESORTS, SAVEVALPOS, SAVEPOS, SORT,
	GETGROUP, ONEGROUP,
	SUMI, SUMF, AVGI, AVGF, STDVI, STDVF, COUNT, MINI, MINF, MINS, MAXI, MAXF, MAXS,
	NEXTMAP, NEXTVEC, ROOTMAP, LDMID, LDPUTMID, LDPUTGRP,
	LDSTDVI, LDSTDVF, LDAVGI, LDAVGF,
	ADD_GROUPSORT_ROW, FREEMIDROW, GSORT, READ_NEXT_GROUP, NUL_TO_STR, SORTVALPOS,
	JOINSET_EQ_AND, JOINSET_EQ, JOINSET_LESS, JOINSET_GRT, JOINSET_LESS_AND, JOINSET_GRT_AND,
	JOINSET_INIT, JOINSET_TRAV, AND_SET, OR_SET,
	SAVEANDCHAIN, SORT_ANDCHAIN,
	FUNCYEAR, FUNCMONTH, FUNCWEEK, FUNCYDAY, FUNCMDAY, FUNCWDAY, FUNCHOUR, FUNCMINUTE, FUNCSECOND, FUNCWDAYNAME, FUNCMONTHNAME,
	FUNCABSF, FUNCABSI,
	START_MESSAGE, STOP_MESSAGE,
	FUNC_CIEL, FUNC_FLOOR, FUNC_ACOS, FUNC_ASIN, FUNC_ATAN, FUNC_COS, FUNC_SIN, FUNC_TAN, FUNC_EXP,
	FUNC_LOG, FUNC_LOG2, FUNC_LOG10, FUNC_SQRT, FUNC_RAND, FUNC_UPPER, FUNC_LOWER, FUNC_BASE64_ENCODE,
	FUNC_BASE64_DECODE, FUNC_HEX_ENCODE, FUNC_HEX_DECODE, FUNC_LEN, FUNC_SUBSTR, FUNC_MD5, FUNC_SHA1,
	FUNC_SHA256, FUNC_ROUND, FUNC_CBRT, FUNC_NOW, FUNC_NOWGM, PRINTCSV_HEADER, FUNC_FORMAT,
	LDCOUNT, BETWEEN, PRINTBOX

};

//2d array for ops indexed by operation and datatype
enum typeOperators {
	OPADD, OPSUB, OPMULT, OPMOD, OPDIV, OPPOW, OPNEG, OPLD, OPEQ, OPLEQ, OPLT,
	OPMAX, OPMIN, OPSUM, OPAVG, OPSTV, OPLSTV, OPLAVG, OPSVSRT, OPDIST, OPABS
};
static int operations[][6] = {
	{ 0, IADD, FADD, DTADD, DRADD, TADD },
	{ 0, ISUB, FSUB, DTSUB, DRSUB, 0 },
	{ 0, IMULT, FMULT, 0, DRMULT, 0 },
	{ 0, IMOD, FMOD, 0, 0, 0 },
	{ 0, IDIV, FDIV, 0, DRDIV, 0 },
	{ 0, IPOW, FPOW, 0, 0, 0 },
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
	{ 0, FUNCABSI, FUNCABSF, 0, FUNCABSI, 0 },
};

//type conversion opcodes - [from][to]
static int typeConv[6][6] = {
	{0, 0,  0,  0,  0,  0 },
	{0, CVNO, CVIF, CVER, CVER, CVIS },
	{0, CVFI, CVNO, CVER, CVER, CVFS },
	{0, CVER, CVER, CVNO, CVER, CVDTS},
	{0, CVER, CVER, CVER, CVNO, CVDRS},
	{0, CVSI, CVSF, CVSDT,CVSDR,CVNO},
};

dat parseIntDat(const char* s);
dat parseFloatDat(const char* s);
dat parseDurationDat(const char* s) ;
dat parseDateDat(const char* s);
dat parseStringDat(const char* s);
int addBtree(int type, querySpecs *q);

bool opDoesJump(int opcode);
//placeholder for jmp positions that can't be determined until later
class jumpPositions {
	map<int, int> jumps;
	int uniqueKey =0;
	public:
	int newPlaceholder() { return --uniqueKey; };
	void setPlace(int k, int v) { jumps[k] = v; };
	void updateBytecode(vector<opcode> &vec);
	jumpPositions() { uniqueKey = -1; };
};

//make sure to free manually like normal malloced c strings
class treeCString {
	public:
		char* s;
		treeCString(dat& d){
			if (d.b & MAL){
				s = d.u.s;
				d.disown();
			} else {
				if (d.isnull()){
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

class stddev {
	public:
	forward_list<double> numbers;
	stddev(double num){
		numbers.push_front(num);
	}
	dat eval(int sample){
		double count = 0.0, avg = 0.0, sum = 0.0;
		for (auto n: numbers){
			avg += n;
			++count;
		}
		if (count <= 1)
			return {0};
		avg /= count;
		for (auto n: numbers)
			sum += (n-avg)*(n-avg);
		numbers.clear();
		return {{.f =pow(sum/(count-sample),0.5)},T_FLOAT};
	}
};

class messager {
	char buf[200];
	future<void> stopslave;
	promise<void> stopmaster;
	thread runner;
	int blank = 0;
	atomic_bool delay;
	public:
	i64 sessionId;
	static const int scanning = 0;
	static const int retrieving = 1;
	static const int sorting = 2;
	static const int reading  = 3;
	static const int readingfiltered = 4;
	static const int scanningjoin = 5;
	static const int indexing = 6;
	static const int readingfirst = 7;
	void say(char* msg, int* n1, int* n2);
	void start(char* msg, int* n1, int* n2);
	void stop();
	void send();
	messager(): delay(true) {}
	~messager();
};

class boxprinter {
	int rowlimit = 100;
	int numcol = 0;
	int numrow = 0;
	vector<int> types;
	vector<size_t> widths;
	vector<string> names;
	vector<vector<string>> datarows;
	public:
	void init(vector<int>& types_, vector<string>& names_){
		types = types_;
		names = names_;
		numcol = names.size();
		widths.resize(numcol);
		for (int i=0; i<numcol; ++i)
			widths[i] = names[i].size();
	};
	void addrow(dat* sourcerow);
	void print();
	~boxprinter(){ if (globalSettings.termbox) print(); };
};

class rowgroup;
class singleQueryResult;
class vmachine {
	static atomic_int idCounter;
	rowgroup *groupTemp = nullptr;
	opcode* ops = nullptr;
	opcode *op = nullptr;
	dat* torow = nullptr;
	dat* midrow = nullptr;
	dat* stacktop = nullptr;
	dat* stackbot = nullptr;
	dat distinctVal = {0};
	int torowSize =0;
	int sortgroupsize =0;
	int quantityLimit =0;
	int totalPrinted =0;
	int numJsonPrinted =0;
	int linesRead = 0;
	int ip = 0;
	vector<stddev> stdvs;
	vector<dat> destrow;
	vector<dat> onegroup;
	array<dat,50> stack;
	vector<shared_ptr<fileReader>> files;
	vector<unique_ptr<dat[], freeC>> groupSorter;
	vector<int> sortIdxs;
	forward_list<bset<i64>> joinSetStack;
	forward_list<unique_ptr<char[], freeC>> groupSortVars;
	unique_ptr<rowgroup> groupTree;
	shared_ptr<singleQueryResult> jsonresult;
	string outbuf;
	ostream csvOutput;
	ofstream outfile;
	messager updates;
	boxprinter termbox;
	//datunion comparers
	static const function<bool (const datunion, const datunion&)> uLessFuncs[3];
	static const function<bool (const datunion, const datunion&)> uGrtFuncs[3];
	static const function<bool (const datunion, const datunion&)> uLessEqFuncs[3];
	static const function<bool (const datunion, const datunion&)> uGrtEqFuncs[3];
	static const function<bool (const datunion, const datunion&)> uEqFuncs[3];
	static const function<bool (const datunion, const datunion&)> uNeqFuncs[3];
	static const function<bool (const datunion, const datunion&)> uRxpFuncs[3];
	static const function<bool (const datunion, const datunion&)>* uComparers[7];
	public:
	static flatmap<int,int> relopIdx;
	i64 sessionId =0;
	int id =0;
	vector<vector<datunion>> normalSortVals;
	vector<bset<i64>> bt_nums;
	vector<bset<treeCString>> bt_strings;
	querySpecs* q;
	void endQuery();
	void run();
	shared_ptr<singleQueryResult> getJsonResult();
	vmachine(querySpecs &q);
	~vmachine();
};

class rowgroup {
	public:
		struct {
			u32 rowOrGroup : 2;
			bool mallocedKey : 1;
			bool freed : 1;
			u32 distinctNSetIdx : 26;
			u32 distinctSSetIdx : 26;
			u32 rowsize : 8;
		} meta;
		union { dat* vecp; map<dat, rowgroup>* mapp; } data;
		dat* getVec(){ return data.vecp; };
		map<dat, rowgroup>& getMap(){ return *data.mapp; };
		rowgroup& nextGroup(dat& d){
			if (!data.mapp) {
				data.mapp = new map<dat, rowgroup>;
				meta.rowOrGroup = 2;
				if (d.istext())
					meta.mallocedKey = true;
			}
			auto key = d.heap();
			auto&& inserted = getMap().insert({key, rowgroup()});
			if (!inserted.second)
				key.freedat();
			return inserted.first->second;
		}
		dat* getRow(vmachine *v){
			if (!data.vecp) {
				meta.rowsize = v->q->midcount;
				meta.rowOrGroup = 1;
				data.vecp = (dat*) calloc(meta.rowsize, sizeof(dat));
				if (v->q->distinctNFuncs){
					meta.distinctNSetIdx = v->bt_nums.size();
					for(int i=0; i<v->q->distinctNFuncs; ++i)
						v->bt_nums.emplace_back();
				}
				if (v->q->distinctSFuncs){
					meta.distinctSSetIdx = v->bt_strings.size();
					for(int i=0; i<v->q->distinctSFuncs; ++i)
						v->bt_strings.emplace_back();
				}
			}
			return getVec();
		}
		void freeRow(){
			for(u32 i=meta.rowsize; i--;) data.vecp[i].freedat();
			free(data.vecp);
			meta.freed = true;
		}
		rowgroup(){ meta = {0}; data = {0}; }
		~rowgroup(){
			if (meta.rowOrGroup == 1){
				if (!meta.freed)
					freeRow();
			} else if (meta.rowOrGroup == 2) {
				if (meta.mallocedKey)
					for (auto &m : getMap())
						free(m.first.u.s);
				delete data.mapp;
			}
		}
};

class varScoper {
	public:
		int scopefilter =0;
		int policy =0;
		int scope =0;
		int fileno =0;
		//map[scope][index] = already evaluated
		map<int,map<int,int>> duplicates;
		varScoper* setscope(int, int);
		varScoper* setscope(int, int, int);
		bool checkDuplicates(int);
		bool neededHere(int, int, int);
};
extern flatmap<int, string_view> opMap;
extern flatmap<int, int> functionCode;

#define has(A,B) ((A)==(B) || ((A) & (B)))
#define incSelectCount()  if has(n->phase, agg_phase) select_count++;
#define addop0(A)       if has(n->phase, agg_phase) addop(A)
#define addop1(A,B)     if has(n->phase, agg_phase) addop(A, B)
#define addop2(A,B,C)   if has(n->phase, agg_phase) addop(A, B, C)
#define addop3(A,B,C,D) if has(n->phase, agg_phase) addop(A, B, C, D)
//#define debugAddop
#ifndef debugAddop
#define debugAddop perr(st("addop: " , opMap[code], "  ip:", v.size(), '\n'));
#endif

void strplus(dat &s1, dat &s2);
void trav(rowgroup &r);
int getSortComparer(querySpecs *q, int i);
dat prepareLike(astnode &n);
void sha1(dat&);
void sha256(dat&);
void md5(dat&);
double round(double input, int decimals);
double ceil(double input, int decimals);
double floor(double input, int decimals);

class qinstance {
	public:
	unique_ptr<vmachine> vm;
	querySpecs* q;
	int id;
	qinstance(querySpecs& qs) {
		q = &qs;
	}
	~qinstance() { perr("Destructing query instance\n"); }
	int run(){
		prepareQuery(*q);
		perr("Constructing vm\n");
		vm.reset(new vmachine(*q));
		id = vm->id;
		perr("Starting query\n");
		vm->run();
		perr("Finished query\n");
		return id;
	}
};
class queryQueue {
	mutex mtx;
	list<qinstance> queries;
	public:
	future<void> runquery(querySpecs&);
	future<shared_ptr<singleQueryResult>> runqueryJson(querySpecs&);
	void endall();
	void setPassword(i64 sesid, string& pass);
};

extern function<i64 (const datunion,const datunion)> datunionDiffs[6];
class sortcomp {
	vector<datunion>* vals;
	int sortcount;
	vector<function<i64 (const datunion,const datunion)>> comps;
	sortcomp();
	sortcomp(sortcomp&);
	public:
		sortcomp(vmachine* vm){
			vals = vm->normalSortVals.data();
			sortcount = vm->q->sortcount;
			for (int i=0; i< vm->q->sortcount; i++)
				comps.push_back(datunionDiffs[getSortComparer(vm->q, i)]);
		}
		bool operator()(const int a, const int b){
			i64 dif;
			int sortval = 0;
			do dif = comps[sortval](vals[sortval][a], vals[sortval][b]);
			while (dif == 0 && ++sortval < sortcount);
			return dif>0;
		};
};
class gsortcomp {
	int sortcount;
	int sortidx;
	vector<function<i64 (const datunion, const datunion)>> comps;
	gsortcomp();
	gsortcomp(gsortcomp&);
	public:
		gsortcomp(vmachine* vm, int idx){
			sortcount = vm->q->sortcount;
			sortidx = idx;
			for (int i=0; i< vm->q->sortcount; i++)
				comps.push_back(datunionDiffs[getSortComparer(vm->q, i)]);
		}
		bool operator()(grprow& a, grprow& b){
			i64 dif;
			int sortval = sortidx;
			do dif = comps[sortval-sortidx](a[sortval].u, b[sortval].u);
			while (dif == 0 && ++sortval < sortcount+sortidx);
			return dif>0;
		};
};
static pair<dat*,dat*> getfirst(dat* stacktop, int firsttype){
	if (firsttype == (stacktop->b & 7))
		return {stacktop,stacktop-1};
	return {stacktop-1,stacktop};
}
