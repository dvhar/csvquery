#pragma once

#include <future>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <string_view>
#include <thread>
#include <iomanip>
#include <fstream>
#include <memory>
#include <sys/time.h>
#include <regex>
#include <stdarg.h>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/format.hpp>
#include <forward_list>
#include <random>
#include "deps/dateparse/dateparse.h"
#include "deps/crypto/chacha20.h"
#include "deps/getline/bufreader.h"
#include "deps/json/json.hpp"

#ifdef _WIN32
#include <getopt.h>
#include <tre/regex.h>
#else
#include <regex.h>
#endif

#define EX_STRING handle_err(current_exception())

typedef struct chacha20_context chacha;
typedef long long i64;
typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef i64 dur_t;
template<typename A, typename B>
using flatmap = boost::container::flat_map<A,B>;
using ft = boost::format;
using json = nlohmann::json;
using namespace std;
class node;
using astnode = unique_ptr<node>;

template<typename T>
static void __st(stringstream& ss, T&& v) {
	ss << v;
}
template<typename T, typename... Args>
static void __st(stringstream& ss, T&& v, Args&&... args) {
	ss << v;
	__st(ss, args...);
}
template<typename... Args>
static string st(Args&&... args) {
	stringstream ss;
	__st(ss, args...);
	return ss.str();
}
string gethome();
void perr(string &&s);

class settings_t {
	public:
	bool update = 1;
	bool debug = 0;
	bool autoheader = 0;
	bool autoexit = 1;
	bool termbox = 0;
	bool tablecolor = 1;
	bool tablelinebg = 0;
	bool needcomma = 1;
#ifdef _WIN32
	string configdir = st(getenv("USERPROFILE"),R"(\AppData\csvquery)");
	string configfilepath = configdir + R"(\config.txt)";
#else
	string configdir = st(gethome(),"/.config/csvquery");
	string configfilepath = configdir + "/config";
#endif 
};
#ifdef _WIN32
#define SLASH "\\"
#else
#define SLASH "/"
#endif

extern mt19937 rng;
extern string version;
extern settings_t globalSettings;

template<typename... Args>
static void error(Args&&... A){ throw invalid_argument(st(A...));}

enum nodetypes { N_QUERY, N_PRESELECT, N_WITH, N_VARS, N_SELECT, N_SELECTIONS, N_FROM, N_AFTERFROM, N_JOINCHAIN, N_JOIN, N_WHERE, N_HAVING, N_ORDER, N_EXPRADD, N_EXPRMULT, N_EXPRNEG, N_EXPRCASE, N_CPREDLIST, N_CPRED, N_CWEXPRLIST, N_CWEXPR, N_PREDICATES, N_PREDCOMP, N_VALUE, N_FUNCTION, N_GROUPBY, N_EXPRESSIONS, N_DEXPRESSIONS, N_TYPECONV, N_FILE, N_SETLIST, N_HANDLEALIAS };

enum valTypes { LITERAL, COLUMN, VARIABLE, FUNCTION };
enum varFilters { WHERE_FILTER=1, ORDER_FILTER=4, AGG_FILTER=8, HAVING_FILTER=16, JCOMP_FILTER=32, JSCAN_FILTER=64, SELECT_FILTER=128 };
enum varScopes { V_READ1_SCOPE, V_READ2_SCOPE, V_GROUP_SCOPE, V_SCAN_SCOPE };
enum subquerytypes { SQ_INLIST=1 };


enum {
	RMAL = 8, //regex needs regfree() (dataholder vector only)
	MAL = 16, //malloced and responsible for freeing c string
	T_NULL = 0,
	T_INT = 1,
	T_FLOAT = 2,
	T_DATE = 3,
	T_DURATION = 4,
	T_STRING = 5,
	NUM_STATES =  5,
	EOS =         255,
	ERROR_STATE = 1<<23,
	AGG_BIT =     1<<26,
	FINAL =       1<<22,
	KEYBIT =      1<<20,
	LOGOP =       1<<24,
	RELOP =       1<<25,
	WORD_TK =     FINAL|1<<27,
	NUMBER =      FINAL|1,
	KEYWORD =     FINAL|KEYBIT,
	KW_AND =      LOGOP|KEYWORD|1,
	KW_OR  =      LOGOP|KEYWORD|2,
	KW_XOR =      LOGOP|KEYWORD|3,
	KW_SELECT =   KEYWORD|4,
	KW_FROM  =    KEYWORD|5,
	KW_HAVING  =  KEYWORD|6,
	KW_AS  =      KEYWORD|7,
	KW_WHERE =    KEYWORD|8,
	KW_LIMIT =    KEYWORD|9,
	KW_GROUP =    KEYWORD|10,
	KW_ORDER =    KEYWORD|11,
	KW_BY =       KEYWORD|12,
	KW_DISTINCT = KEYWORD|13,
	KW_ORDHOW =   KEYWORD|14,
	KW_CASE =     KEYWORD|15,
	KW_WHEN =     KEYWORD|16,
	KW_THEN =     KEYWORD|17,
	KW_ELSE =     KEYWORD|18,
	KW_END =      KEYWORD|19,
	KW_JOIN=      KEYWORD|20,
	KW_INNER=     KEYWORD|21,
	KW_OUTER=     KEYWORD|22,
	KW_LEFT=      KEYWORD|23,
	KW_CROSS=     KEYWORD|24,
	KW_BETWEEN =  RELOP|KEYWORD|25,
	KW_LIKE =     RELOP|KEYWORD|26,
	KW_IN =       RELOP|KEYWORD|27,
	FN_SUM =      KEYWORD|AGG_BIT|28,
	FN_AVG =      KEYWORD|AGG_BIT|29,
	FN_STDEV =    KEYWORD|AGG_BIT|30,
	FN_STDEVP =   KEYWORD|AGG_BIT|31,
	FN_MIN =      KEYWORD|AGG_BIT|32,
	FN_MAX =      KEYWORD|AGG_BIT|33,
	FN_COUNT =    KEYWORD|AGG_BIT|34,
	FN_ABS =      KEYWORD|35,
	FN_FORMAT =   KEYWORD|36,
	FN_COALESCE = KEYWORD|37,
	FN_YEAR =     KEYWORD|38,
	FN_MONTH =    KEYWORD|39,
	FN_MONTHNAME= KEYWORD|40,
	FN_WEEK =     KEYWORD|41,
	FN_WDAY =     KEYWORD|42,
	FN_WDAYNAME = KEYWORD|43,
	FN_YDAY =     KEYWORD|44,
	FN_MDAY =     KEYWORD|45,
	FN_HOUR =     KEYWORD|46,
	FN_MINUTE =   KEYWORD|47,
	FN_SECOND =   KEYWORD|48,
	FN_ENCRYPT =  KEYWORD|49,
	FN_DECRYPT =  KEYWORD|50,
	FN_INC =      KEYWORD|51,
	FN_CEIL =          KEYWORD|68,
	FN_FLOOR =         KEYWORD|69,
	FN_ACOS =          KEYWORD|70,
	FN_ASIN =          KEYWORD|71,
	FN_ATAN =          KEYWORD|72,
	FN_COS =           KEYWORD|73,
	FN_SIN =           KEYWORD|74,
	FN_TAN =           KEYWORD|75,
	FN_EXP =           KEYWORD|76,
	FN_LOG =           KEYWORD|77,
	FN_LOG2 =          KEYWORD|78,
	FN_LOG10 =         KEYWORD|79,
	FN_SQRT =          KEYWORD|80,
	FN_RAND =          KEYWORD|81,
	FN_UPPER =         KEYWORD|82,
	FN_LOWER =         KEYWORD|83,
	FN_BASE64_ENCODE = KEYWORD|84,
	FN_BASE64_DECODE = KEYWORD|85,
	FN_HEX_ENCODE =    KEYWORD|86,
	FN_HEX_DECODE =    KEYWORD|87,
	FN_LEN =           KEYWORD|88,
	FN_SUBSTR =        KEYWORD|89,
	FN_MD5 =           KEYWORD|90,
	FN_SHA1 =          KEYWORD|91,
	FN_SHA256 =        KEYWORD|92,
	FN_STRING =        KEYWORD|93,
	FN_INT =           KEYWORD|94,
	FN_FLOAT =         KEYWORD|95,
	FN_ROUND =         KEYWORD|96,
	FN_POW =           KEYWORD|97,
	FN_CBRT =          KEYWORD|98,
	FN_NOW =           KEYWORD|99,
	FN_NOWGM =         KEYWORD|100,
	FN_DATE =          KEYWORD|101,
	FN_DUR =           KEYWORD|102,
	FN_SIP =           KEYWORD|103,
	SPECIALBIT =  1<<21,
	SPECIAL =      FINAL|SPECIALBIT,
	SP_EQ =        RELOP|SPECIAL|50,
	SP_NOEQ =      RELOP|SPECIAL|51,
	SP_LESS =      RELOP|SPECIAL|52,
	SP_LESSEQ =    RELOP|SPECIAL|53,
	SP_GREAT =     RELOP|SPECIAL|54,
	SP_GREATEQ =   RELOP|SPECIAL|55,
	SP_NEGATE =    SPECIAL|56,
	SP_SQUOTE =    SPECIAL|57,
	SP_DQUOTE =    SPECIAL|58,
	SP_COMMA =     SPECIAL|59,
	SP_LPAREN =    SPECIAL|60,
	SP_RPAREN =    SPECIAL|61,
	SP_STAR =      SPECIAL|62,
	SP_DIV =       SPECIAL|63,
	SP_MOD =       SPECIAL|64,
	SP_CARROT =    SPECIAL|65,
	SP_MINUS =     SPECIAL|66,
	SP_PLUS =      SPECIAL|67,
	STATE_INITAL =    0,
	STATE_SSPECIAL =  1,
	STATE_DSPECIAL =  2,
	STATE_MBSPECIAL = 3,
	STATE_WORD =      4,
	O_C = 1,
	O_NH = 2,
	O_H = 4,
	O_S = 8,
	O_P = 16,
	O_T = 32,
	O_OH = 64,
	O_NOH = 128,
	O_NAN = 256,
	O_AH = 512,
	CMD_ADDALIAS,
	CMD_SHOWTABLES,
	CMD_DROPALIAS,
	COMMENTED_OUT,
};

extern const flatmap<int, string_view> treeMap;
extern const flatmap<string_view, int> keywordMap;
extern const flatmap<string_view, int> functionMap;
extern const flatmap<string_view, int> joinMap;
extern const flatmap<string_view, int> specialMap;
extern const flatmap<int, string_view> typeNames;
extern const flatmap<int, const char*> dateFmtCodes;

static string_view gettypename(int i){
	try { return typeNames.at(i); } catch (...){ return ""; }}
static string_view getnodename(int i){
	try { return treeMap.at(i); } catch (...){ return ""; }}
static int getkeyword(basic_string_view<char> s){
	try { return keywordMap.at(s); } catch (...){ return 0; }}
static int getfunc(basic_string_view<char> s){
	try { return functionMap.at(s); } catch (...){ return 0; }}
static int getjoinkw(basic_string_view<char> s){
	try { return joinMap.at(s); } catch (...){ return 0; }}
static int getspecial(basic_string_view<char> s){
	try { return specialMap.at(s); } catch (...){ return 0; }}

extern regex_t leadingZeroString;
extern regex_t durationPattern;
extern regex_t intType;
extern regex_t floatType;
extern regex cInt;
extern regex posInt;
extern regex colNum;
extern regex extPat;
extern regex hidPat;
extern regex filelike;

struct freeC {
	void operator()(void*x){ free(x); }
};


class token {
	public:
	int id =0;
	string val;
	int line =0;
	int col =0;
	bool quoted =0;
	string lower();
};
//node info keys
enum: int {
	SCANVAL=1,
	VALPOSIDX,
	ANDCHAIN,
	MIDIDX,
	CHAINSIZE,
	CHAINIDX,
	FILENO,
	TOSCAN,
	PARAMTYPE,
	RETTYPE,
	LPMID,
};
class node {
	public:
	int label =0;
	int datatype =0;
	int phase =0;
	bool keep =0; //preserve subtree types
	astnode node1;
	astnode node2;
	astnode node3;
	astnode node4;
	token tok1;
	token tok2;
	token tok3;
	token tok4;
	map<int,int> info;
	~node();
	node(int);
	node();
	node& operator=(const node&);

	//getters for values of different node types
	//TODO: organize and document and finish implementing
	string_view nodelabel(){ return getnodename(label); }
	string& varname(){ return tok1.val; }
	astnode& nnextvar(){ return node2; }
	string& nval(){ return tok1.val; }
	int& valtype(){ return tok2.id; }
	int& trivialvalcol(){ return tok3.id; }
	int& trivialvalalias(){ return tok4.id; }
	astnode& nsubexpr(){ return node1; }
	token& startok(){ return tok1; }
	token& joinDetailsTok(){ return tok3; }
	astnode& nnextselection(){ return node2; }
	astnode& nselections(){ return node2; }
	astnode& nselect(){ return node2; }
	int& quantlimit(){ return tok1.id; }
	int& selectiondestidx(){ return tok4.id; }
	string& selectionalias(){ return tok2.val; }
	int& selectionmididx(){ return tok3.id; }
	int& selectionlpmid(){ return info[LPMID]; }
	int& ordersize(){ return tok2.id; }
	int& aggorderdestidx(){ return tok3.id; }
	int& funcid(){ return tok1.id; }
	token& funcdisttok(){ return tok3; }
	int& funcdistnum(){ return tok4.id; }
	int& selectiondistnum(){ return tok4.id; }
	int& funcparamtype(){ return tok2.id; }
	int& funcreturntype(){ return info[RETTYPE]; }
	int& origparamtype(){ return info[PARAMTYPE]; }
	int& funcmididx(){ return info[MIDIDX]; }
	int& convfromtype(){ return tok1.id; }
	astnode& njoinconds(){ return node2; }
	astnode& nnextjoin(){ return node3; }
	astnode& npredconds(){ return node1; }
	astnode& norderlist(){ return node1; }
	astnode& nnextlist(){ return node2; }
	astnode& norder(){ return node4; }
	astnode& nwhere(){ return node1; }
	astnode& nsetlist(){ return node2; }
	int& orderdirection(){ return tok1.id; }
	int& varmididx(){ return tok3.id; }
	int& negated(){ return tok2.id; }
	astnode& ngrouplist(){ return node1; }
	string& dotsrc(){ return tok3.val; }
	astnode& npredcomp(){ return node1; }
	astnode& nmorepreds(){ return node1; }
	astnode& nnextpreds(){ return node2; }
	int& scannedExpr(){ return info[TOSCAN]; }
	int& andChain(){ return info[ANDCHAIN]; }
	int& chainSize(){ return info[CHAINSIZE]; }
	int& chainIdx(){ return info[CHAINIDX]; }
	int& predFileNum(){ return info[FILENO]; }
	int& predValposIdx(){ return info[VALPOSIDX]; }
	int& relop(){ return tok1.id; }
	int& logop(){ return tok1.id; }
	astnode& npredexp1(){ return node1; }
	astnode& npredexp2(){ return node2; }
	int& hassubquery(){ return tok1.id; }
	int& subqidx(){ return tok2.id; }
	string& password(){ return tok4.val; }
	astnode& nafterfrom(){ return node4; }
	astnode& nfrom(){ return node3; }
	astnode& nhaving(){ return node3; }
	astnode& njoins(){ return node2; }
	astnode& ngroups(){ return node2; }
	astnode& npreselect(){ return node1; }
	astnode& nfile(){ return node1; }
	string& filename(){ return tok1.val; }
	string& filealias(){ return tok4.id? tok4.val : tok1.val; }
	int& mathop(){ return tok1.id; }
	int& casenodetype(){ return tok1.id; }
	int& casetype(){ return tok2.id; }
	astnode& ncaseresultexpr(){ return node2; }
	int& colidx(){ return tok1.id; }
	int& casewhentype(){ return tok3.id; }
	int& optionbits(){ return tok3.id; }
};
class variable {
	public:
	string name;
	int type =0;
	int lit =0;
	int filter =0;
	int phase =0;
	int mrindex =0;
	set<int> filesReferenced;
	int maxfileno =0;
};

class csvEntry {
	public:
	char* val;
	char* terminator;
	csvEntry(char* s,char* e) : val(s), terminator(e) {}
	csvEntry(){}
	u32 size(){ return (u32)(terminator-val); }
};

class fastvector {
	u32 _size = 0;
	u32 _cap = 0;
	unique_ptr<csvEntry[], freeC> row;
	public:
	csvEntry* release(){
		auto ret = row.release();
		row.reset((csvEntry*) malloc(sizeof(csvEntry)*_size));
		_cap = _size;
		_size = 0;
		return ret;
	};
	csvEntry* data(){ return row.get(); };
	csvEntry* begin(){ return row.get(); };
	csvEntry* end(){ return row.get()+_size; };
	u32 size(){ return _size; };
	void clear(){ _size = 0; };
	void emplace_back(char* s, char* t){
		if (_size >= _cap){
			_cap = 1+_cap*2;
			auto ptr = row.release();
			row.reset((csvEntry*) realloc(ptr, _cap * sizeof(csvEntry)));
		}
		row[_size++] = csvEntry{s,t};
	};
	csvEntry& operator[](int i){ return row[i]; };
};

class valpos;
class andchain;
class querySpecs;
class fileReader {
	int fieldsFound;
	char* pos1 = 0;
	char* pos2 = 0;
	char* terminator = 0;
	char* buf = 0;
	char* escapedQuote = 0;
	bufreader br;
	string filename;
	vector<unique_ptr<csvEntry[], freeC>> gotrows;
	fastvector entriesVec;
	i64 prevpos = 0;
	int equoteCount = 0;
	int memidx = 0;
	int numrows = 0;
	querySpecs *q;
	public:
		static char blank;
		bool small =0;
		bool inmemory =0;
		bool noheader = false;
		bool autoheader = false;
		char delim;
		i64 pos =0;
		vector<string> colnames;
		vector<int> types;
		vector<i64> positions;
		vector<vector<valpos>> joinValpos;
		vector<andchain> andchains;
		vector<int> vpTypes;
		csvEntry* entries;
		string id;
		int fileno =0;
		int numFields =0;
	inline char* nextIsDelim();
	inline void getField();
	inline void getQuotedField();
	inline void compactQuote();
	inline bool checkWidth();
	int size(){ return br.fsize; };
	void inferTypes();
	int getColIdx(string&);
	bool readline();
	bool readlineat(i64);
	fileReader(string&, querySpecs &q);
	~fileReader();
};

class opcode {
	public:
	int code =0;
	int p1 =0;
	int p2 =0;
	int p3 =0;
	string print();
};
inline static char* newStr(char* src, int size){
	char* s = (char*) malloc(size+1);
	strcpy(s, src);
	return s;
}

union datunion {
	i64 i; //also used for date and duration
	double f;
	char* s;
	bool p;
	regex_t* r;
};
class dat {
	public:
	datunion u;
	u32 b; // metadata bit array
	u32 z; // string size and avg count
	static char abnormal[];
	void appendToCsvBuffer(string&);
	void appendToJsonBuffer(string&);
	string str(){ string st; appendToCsvBuffer(st); return st; }
	int type() const { return (b & 7); }
	bool istext() const { return type() == T_STRING; }
	bool isnull() const { return b == 0; }
	bool ismal() const { return b & MAL; }
	void setnull(){ *this = {0}; }
	void disown(){ b &=(~MAL); }
	void freedat(){
		if (ismal())
			free(u.s);
		setnull();
	}
	friend bool operator<(const dat& l, const dat& r){
		if (r.isnull())
			return false;
		if (l.istext()) //false if null
			return strcmp(l.u.s, r.u.s) < 0;
		if (l.isnull())
			return true;
		return l.u.i < r.u.i;
	}
	dat heap(){
		dat d;
		if (istext() && !ismal()){
			d.u.s = (char*) malloc(z+1);
			strcpy(d.u.s, u.s);
			d.b = b | MAL;
			d.z = z;
		} else {
			d = *this;
		}
		disown();
		return d;
	}
	void mov(dat& d){
		if (ismal())
			free(u.s);
		*this = d;
		d.disown();
	}
};

class andchain {
	public:
	vector<int> indexes;
	vector<int> functionTypes;
	vector<int> relops;
	vector<bool> negations;
	vector<i64> positiions;
	vector<vector<datunion>> values;
	andchain(int size){
		values.resize(size);
	}
};

class valpos {
	public:
	datunion val;
	i64 pos;
};

class resultSpecs {
	public:
	int count =0;
	vector<int> types;
	vector<string> colnames;
};

class chactx {
	public:
	chacha ctx;
	u8 key[32];
	u8 nonce[12];
};
class crypter {
	public:
	vector<chactx> ctxs;
	int newChacha(string);
	void chachaEncrypt(dat&, int);
	void chachaDecrypt(dat&, int);
};
class subquery;
class querySpecs {
	public:
	string savepath;
	string queryString;
	string password;
	vector<token> tokArray;
	vector<pair<int,int>> sortInfo; // desc, datatype
	vector<variable> vars;
	vector<dat> dataholder;
	vector<opcode> bytecode;
	vector<int> settypes;
	astnode tree;
	map<string, shared_ptr<fileReader>> filemap;
	vector<shared_ptr<fileReader>> filevec;
	promise<string> passReturn;
	subquery* thisSq;
	vector<subquery> subqueries;
	resultSpecs colspec = {0};
	crypter crypt = {};
	i64 sessionId = 0;
	int midcount =0;
	int numFiles =0;
	u32 tokIdx =0;
	int options =0;
	int quantityLimit =0;
	int posVecs =0;
	int sorting =0;
	int sortcount =0;
	int grouping =0; //1 = one group, 2 = groups
	int isSubquery =0; //1 = in list predicate
	bool distinctFuncs =0;
	bool outputjson =0;
	bool outputcsv =0;
	bool outputcsvheader =0;
	bool outheaderOpt =0;
	bool joining =0;
	bool whereFiltering =0;
	bool havingFiltering =0;
	bool distinctFiltering =0;
	bool needPass =0;
	bool canskip =0;
	function<bool(const datunion*, const datunion*)> unionArrayLess;
	function<bool(const dat*, const dat*)> datArrayLess;
	token tok();
	token nextTok();
	token peekTok();
	token lastTok();
	bool numIsCol();
	void setoutputCsv(){ outputcsv = true; };
	void setoutputJson(){ outputjson = true; };
	void addVar(string);
	int getVarIdx(string);
	int getVarType(string);
	int getFileNo(string s);
	int trivialColumnType(astnode&);
	void setPassword(string s);
	shared_ptr<fileReader>& getFileReader(int);
	void promptPassword();
	int addSubquery(astnode&, int);
	variable& var(string);
	~querySpecs();
	querySpecs(string &q) : queryString(q) {};
	querySpecs(string &q, string &f) : queryString(q), savepath(f) {};
	querySpecs(astnode &n, int sqtype) {
		tree.reset(n.release());
		isSubquery = sqtype;
	};
};
class virtualSet {
	public:
		virtual bool contains(dat&)=0;
		virtual bool insert(dat&)=0;
		virtual ~virtualSet(){};
};
class subquery {
	public:
	int singleDatatype = 0;
	int btreeIdx = 0;
	thread prep;
	//pair is type, canbestring
	shared_future<vector<pair<int,bool>>> topinnertypes;
	promise<vector<pair<int,bool>>> topinnertypesp;
	future<vector<int>> topfinaltypes;
	promise<vector<int>> topfinaltypesp;
	unique_ptr<querySpecs> query;
	unique_ptr<virtualSet> resultSet;
	exception_ptr ex = nullptr;
	subquery(querySpecs* q) : query(q) {
		q->thisSq = this;
		topinnertypes = topinnertypesp.get_future().share();
		topfinaltypes = topfinaltypesp.get_future();
	};
	void terminate(exception_ptr);
	void terminateOutter(exception_ptr);
};

class singleQueryResult {
	stringstream j;
	public:
	int numrows = 0;
	int rowlimit = 0;
	int numcols = 0;
	int status = 0;
	int clipped = 0;
	bool skip = 0;
	vector<int> types;
	vector<string> colnames;
	list<string> Vals; //each string is whole row of encoded json
	string query;
	stringstream& tojson();
	singleQueryResult(querySpecs &q){
		numcols = q.colspec.count;
		rowlimit = 20000 / numcols;
	}
	singleQueryResult(){}
};
class returnData {
	stringstream j;
	public:
	list<shared_ptr<singleQueryResult>> entries;
	int status = 0;
	int maxclipped = 0;
	bool clipped = 0;
	string originalQuery;
	string message;
	stringstream& tojson();
};

class directory {
	public:
	json j;
	string fpath;
	string parent;
	string mode;
	vector<string> files;
	vector<string> dirs;
	void setDir(json&);
	json& tojson();
};

void scanTokens(querySpecs &q);
void parseQuery(querySpecs &q);
bool is_number(const std::string& s);
void printTree(astnode &n, int ident);
int isInt(const char*);
int isFloat(const char*);
int isDuration(const char*);
int dateParse(const char*, struct timeval*);
int parseDuration(char*, dat&);
int getNarrowestType(char* value, int startType);
void openfiles(querySpecs &q);
void applyTypes(querySpecs &q);
int earlyAnalyze(querySpecs &q);
void midAnalyze(querySpecs &q);
void lateAnalyze(querySpecs &q);
void codeGen(querySpecs &q);
void runPlainQuery(querySpecs &q);
shared_ptr<singleQueryResult> runJsonQuery(querySpecs &q);
char* durstring(dat& dur, char* str);
void runServer();
string handle_err(exception_ptr eptr);
astnode& findFirstNode(astnode &n, int label);
int prepareQuery(querySpecs &q);
void stopAllQueries();
shared_ptr<directory> filebrowse(string);
bool isTrivialColumn(astnode &n);
bool isTrivialAlias(astnode &n);
string nodeName(astnode &n, querySpecs* q);
void sendMessage(i64,const char*);
void sendMessage(i64 sesid, string);
void sendPassPrompt(i64 sesid);
void hideInput();
void initregex();
void findExtension(string& fname);
const char* dateFormatCode(string& s);
i64 totalram();

extern int runmode;
enum runmodes { RUN_SINGLE, RUN_SERVER };

template<typename T>
static T fromjson(json& j, string&& key){
	try {
		return j[key].get<T>();
	} catch (exception &e) {
		return T{};
	}
}
