#pragma once

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
#include <sys/types.h>
#include <regex>
#include <stdarg.h>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/format.hpp>
#include "deps/dateparse/dateparse.h"
#include "deps/chacha/chacha20.h"
#include "deps/getline/bufreader.h"
#include <forward_list>
#include <nlohmann/json.hpp>

#ifdef __MINGW32__
#include <getopt.h>
#include <tre/regex.h>
#else
#include <regex.h>
#endif

typedef struct chacha20_context chacha;
typedef long long i64;
typedef unsigned int u32;
typedef i64 dur_t;
#define ft boost::format
#define flatmap boost::container::flat_map
#define SMALLEST numeric_limits<i64>::min()
using namespace std;
using json = nlohmann::json;

static void error(string A){ throw invalid_argument(A);}
static void perr(string s){
	cerr << s;
}

enum nodetypes { N_QUERY, N_PRESELECT, N_WITH, N_VARS, N_SELECT, N_SELECTIONS, N_FROM, N_AFTERFROM, N_JOINCHAIN, N_JOIN, N_WHERE, N_HAVING, N_ORDER, N_EXPRADD, N_EXPRMULT, N_EXPRNEG, N_EXPRCASE, N_CPREDLIST, N_CPRED, N_CWEXPRLIST, N_CWEXPR, N_PREDICATES, N_PREDCOMP, N_VALUE, N_FUNCTION, N_GROUPBY, N_EXPRESSIONS, N_DEXPRESSIONS, N_TYPECONV };

enum valTypes { LITERAL, COLUMN, VARIABLE, FUNCTION };
enum varFilters { WHERE_FILTER=1, DISTINCT_FILTER=2, ORDER_FILTER=4, AGG_FILTER=8, HAVING_FILTER=16, JCOMP_FILTER=32, JSCAN_FILTER=64, SELECT_FILTER=128 };
enum varScopes { V_READ1_SCOPE, V_READ2_SCOPE, V_GROUP_SCOPE, V_SCAN_SCOPE };


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
	KW_RIGHT=     KEYWORD|24,
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
	FN_CIEL =          KEYWORD|68,
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
	O_S = 8
};

extern const flatmap<int, string_view> enumMap;
extern const flatmap<int, string_view> treeMap;
extern const flatmap<string_view, int> keywordMap;
extern const flatmap<string_view, int> functionMap;
extern const flatmap<string_view, int> joinMap;
extern const flatmap<string_view, int> specialMap;
extern const flatmap<int, string_view> nameMap;

extern regex_t leadingZeroString;
extern regex_t durationPattern;
extern regex_t intType;
extern regex_t floatType;
extern regex_t extPattern;
extern regex_t hidPattern;
extern regex cInt;
extern regex posInt;
extern regex colNum;

class token {
	public:
	int id =0;
	string val;
	int line =0;
	int col =0;
	bool quoted =0;
	string lower();
	void print();
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
	RETTYPE
};
class node {
	public:
	int label =0;
	int datatype =0;
	int phase =0;
	bool keep =0; //preserve subtree types
	unique_ptr<node> node1;
	unique_ptr<node> node2;
	unique_ptr<node> node3;
	unique_ptr<node> node4;
	token tok1;
	token tok2;
	token tok3;
	token tok4;
	token tok5;
	map<int,int> info;
	~node();
	node(int);
	node();
	node& operator=(const node&);
	void print();
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
	csvEntry(char* s,char* e){ val = s; terminator = e; };
	csvEntry(){};
};

class valpos;
class andchain;
class fileReader {
	int fieldsFound;
	char* pos1 =0;
	char* pos2 =0;
	char* terminator =0;
	char* buf =0;
	char* escapedQuote =0;
	bufreader fs;
	string filename;
	vector<vector<csvEntry>> gotrows;
	forward_list<unique_ptr<char[]>> gotbuffers;
	vector<csvEntry> entriesVec;
	i64 prevpos =0;
	int equoteCount =0;
	int memidx =0;
	int numrows =0;
	public:
		static char blank;
		bool small =0;
		bool inmemory =0;
		char delim;
		i64 pos =0;
		vector<string> colnames;
		vector<int> types;
		vector<i64> positions;
		vector<vector<valpos>> joinValpos;
		vector<andchain> andchains;
		vector<int> vpTypes;
		csvEntry* entries;
		csvEntry* eend;
		bool noheader =0;
		string id;
		int fileno =0;
		int numFields =0;
	inline void getField();
	inline void compactQuote();
	inline bool checkWidth();
	void inferTypes();
	int getColIdx(string&);
	bool readline();
	bool readlineat(i64);
	fileReader(string&);
	~fileReader();
};

class opcode {
	public:
	int code =0;
	int p1 =0;
	int p2 =0;
	int p3 =0;
	void print();
};

union datunion {
	i64 i; //also used for date and duration
	double f;
	char* s;
	bool p;
	regex_t* r;
	chacha* ch;
};
inline static char* newStr(char* src, int size){
	char* s = (char*) malloc(size+1);
	strcpy(s, src);
	return s;
}
class dat {
	public:
	datunion u;
	u32 b; // metadata bit array
	u32 z; // string size and avg count
	static char abnormal[];
	void appendToCsvBuffer(string&);
	void appendToJsonBuffer(vector<string>&);
	string str(){ string st; appendToCsvBuffer(st); return st; }
	bool istext() const { return (b & 7) == T_STRING; }
	bool isnull() const { return b == 0; }
	bool ismal() const { return b & MAL; }
	void setnull(){ b = 0; u.i = 0; }
	void disown(){ b &=(~MAL); }
	void freedat(){
		if (ismal())
			free(u.s);
		setnull();
	}
	friend bool operator<(const dat& l, const dat& r){
		if (r.isnull())
			return false;
		if (l.istext())
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
		freedat();
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
	uint8_t key[32];
	uint8_t nonce[12];
};
class crypter {
	public:
	vector<chactx> ctxs;
	int newChacha(string);
	pair<char*,int> chachaEncrypt(int, size_t, char*);
	pair<char*,int> chachaDecrypt(int, size_t, char*);
};

class querySpecs {
	public:
	string savepath;
	string queryString;
	string password;
	vector<token> tokArray;
	vector<pair<int,int>> sortInfo; // asc, datatype
	vector<variable> vars;
	vector<dat> dataholder;
	vector<opcode> bytecode;
	unique_ptr<node> tree;
	map<string, shared_ptr<fileReader>> files;
	resultSpecs colspec = {0};
	crypter crypt = {};
	i64 sessionId = 0;
	int distinctSFuncs =0;
	int distinctNFuncs =0;
	int midcount =0;
	int numFiles =0;
	u32 tokIdx =0;
	int options =0;
	int btn =0;
	int bts =0;
	int quantityLimit =0;
	int posVecs =0;
	int sorting =0;
	int sortcount =0;
	int grouping =0; //1 = one group, 2 = groups
	bool outputjson =0;
	bool outputcsv =0;
	bool joining =0;
	bool whereFiltering =0;
	bool havingFiltering =0;
	bool distinctFiltering =0;
	token tok();
	token nextTok();
	token peekTok();
	bool numIsCol();
	void setoutputCsv(){ outputcsv = true; };
	void setoutputJson(){ outputjson = true; };
	void init(string);
	void addVar(string);
	shared_ptr<fileReader>& getFileReader(int);
	variable& var(string);
	~querySpecs();
	querySpecs(string &s){ queryString = s; };
};

//might be replacable with just a json object?
class singleQueryResult {
	json j;
	public:
	bool clipped = false;
	int numrows =0;
	int showLimit =0;
	int numcols =0;
	int status =0;
	vector<int> types;
	vector<string> colnames;
	vector<int> pos;
	list<vector<string>> Vals;
	string query;
	json& tojson();
	singleQueryResult(querySpecs &q){
		numcols = q.colspec.count;
		showLimit = 20000 / numcols;
	}
};
class returnData {
	json j;
	public:
	list<shared_ptr<singleQueryResult>> entries;
	int status =0;
	int maxclipped =0;
	string originalQuery;
	string message;
	json& tojson();
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
int varIsAgg(string lkup, querySpecs &q);
void parseQuery(querySpecs &q);
bool is_number(const std::string& s);
void printTree(unique_ptr<node> &n, int ident);
int slcomp(const char*, const char*);
int isInt(const char*);
int isFloat(const char*);
int dateParse(const char*, struct timeval*);
int parseDuration(char*, date_t*);
int getNarrowestType(char* value, int startType);
int isInList(int n, int count, ...);
void openfiles(querySpecs &q, unique_ptr<node> &n);
void applyTypes(querySpecs &q);
void analyzeTree(querySpecs &q);
void codeGen(querySpecs &q);
void runquery(querySpecs &q);
shared_ptr<singleQueryResult> runqueryJson(querySpecs &q);
int getVarLocation(string lkup, querySpecs *q);
int getVarIdx(string lkup, querySpecs *q);
int getVarType(string lkup, querySpecs *q);
int getFileNo(string s, querySpecs *q);
char* durstring(dur_t dur, char* str);
void runServer();
string handle_err(exception_ptr eptr);
unique_ptr<node>& findFirstNode(unique_ptr<node> &n, int label);
void prepareQuery(querySpecs &q);
void stopAllQueries();
shared_ptr<directory> filebrowse(string);
string promptPassword();

struct freeC {
	void operator()(void*x){ free(x); }
};

template<typename T>
static void __st(stringstream& ss, T v) {
	ss << v;
}
template<typename T, typename... Args>
static void __st(stringstream& ss, T first, Args... args) {
	ss << first;
	__st(ss, args...);
}
template<typename T, typename... Args>
static string st(T first, Args... args) {
	stringstream ss;
	__st(ss, first, args...);
	return ss.str();
}
extern int runmode;
enum runmodes { RUN_CMD, RUN_SINGLE, RUN_SERVER };

template<typename T>
static T fromjson(json& j, string&& key){
	try {
		return j[key].get<T>();
	} catch (exception e) {
		return T{};
	}
}
