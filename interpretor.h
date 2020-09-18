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
#define error(A) throw invalid_argument(A)
#define SMALLEST numeric_limits<i64>::min()
using namespace std;

static void perr(string s){
	cerr << s;
}

enum nodetypes { N_QUERY, N_PRESELECT, N_WITH, N_VARS, N_SELECT, N_SELECTIONS, N_FROM, N_AFTERFROM, N_JOINCHAIN, N_JOIN, N_WHERE, N_HAVING, N_ORDER, N_EXPRADD, N_EXPRMULT, N_EXPRNEG, N_EXPRCASE, N_CPREDLIST, N_CPRED, N_CWEXPRLIST, N_CWEXPR, N_PREDICATES, N_PREDCOMP, N_VALUE, N_FUNCTION, N_GROUPBY, N_EXPRESSIONS, N_DEXPRESSIONS, N_TYPECONV };

enum valTypes { LITERAL, COLUMN, VARIABLE, FUNCTION };
enum varFilters { WHERE_FILTER=1, DISTINCT_FILTER=2, ORDER_FILTER=4, GROUP_FILTER=8, HAVING_FILTER=16, JCOMP_FILTER=32, JSCAN_FILTER=64, SELECT_FILTER=128 };
enum varScopes { V_READ1_SCOPE, V_READ2_SCOPE, V_GROUP_SCOPE, V_SCAN_SCOPE };

const int RMAL = 8; //regex needs regfree() (dataholder vector only)
const int MAL = 16; //malloced and responsible for freeing c string

const int T_NULL = 0;
const int T_INT = 1;
const int T_FLOAT = 2;
const int T_DATE = 3;
const int T_DURATION = 4;
const int T_STRING = 5;
const int NUM_STATES =  5;
const int EOS =         255;
const int ERROR_STATE = 1<<23;
const int AGG_BIT =     1<<26;
const int FINAL =       1<<22;
const int KEYBIT =      1<<20;
const int LOGOP =       1<<24;
const int RELOP =       1<<25;
const int WORD_TK =     FINAL|1<<27;
const int NUMBER =      FINAL|1;
const int KEYWORD =     FINAL|KEYBIT;
const int KW_AND =      LOGOP|KEYWORD|1;
const int KW_OR  =      LOGOP|KEYWORD|2;
const int KW_XOR =      LOGOP|KEYWORD|3;
const int KW_SELECT =   KEYWORD|4;
const int KW_FROM  =    KEYWORD|5;
const int KW_HAVING  =  KEYWORD|6;
const int KW_AS  =      KEYWORD|7;
const int KW_WHERE =    KEYWORD|8;
const int KW_LIMIT =    KEYWORD|9;
const int KW_GROUP =    KEYWORD|10;
const int KW_ORDER =    KEYWORD|11;
const int KW_BY =       KEYWORD|12;
const int KW_DISTINCT = KEYWORD|13;
const int KW_ORDHOW =   KEYWORD|14;
const int KW_CASE =     KEYWORD|15;
const int KW_WHEN =     KEYWORD|16;
const int KW_THEN =     KEYWORD|17;
const int KW_ELSE =     KEYWORD|18;
const int KW_END =      KEYWORD|19;
const int KW_JOIN=      KEYWORD|20;
const int KW_INNER=     KEYWORD|21;
const int KW_OUTER=     KEYWORD|22;
const int KW_LEFT=      KEYWORD|23;
const int KW_RIGHT=     KEYWORD|24;
const int KW_BETWEEN =  RELOP|KEYWORD|25;
const int KW_LIKE =     RELOP|KEYWORD|26;
const int KW_IN =       RELOP|KEYWORD|27;
const int FN_SUM =      KEYWORD|AGG_BIT|28;
const int FN_AVG =      KEYWORD|AGG_BIT|29;
const int FN_STDEV =    KEYWORD|AGG_BIT|30;
const int FN_STDEVP =   KEYWORD|AGG_BIT|31;
const int FN_MIN =      KEYWORD|AGG_BIT|32;
const int FN_MAX =      KEYWORD|AGG_BIT|33;
const int FN_COUNT =    KEYWORD|AGG_BIT|34;
const int FN_ABS =      KEYWORD|35;
const int FN_FORMAT =   KEYWORD|36;
const int FN_COALESCE = KEYWORD|37;
const int FN_YEAR =     KEYWORD|38;
const int FN_MONTH =    KEYWORD|39;
const int FN_MONTHNAME= KEYWORD|40;
const int FN_WEEK =     KEYWORD|41;
const int FN_WDAY =     KEYWORD|42;
const int FN_WDAYNAME = KEYWORD|43;
const int FN_YDAY =     KEYWORD|44;
const int FN_MDAY =     KEYWORD|45;
const int FN_HOUR =     KEYWORD|46;
const int FN_MINUTE =   KEYWORD|47;
const int FN_SECOND =   KEYWORD|48;
const int FN_ENCRYPT =  KEYWORD|49;
const int FN_DECRYPT =  KEYWORD|50;
const int FN_INC =      KEYWORD|51;
const int SPECIALBIT =  1<<21;
const int SPECIAL =      FINAL|SPECIALBIT;
const int SP_EQ =        RELOP|SPECIAL|50;
const int SP_NOEQ =      RELOP|SPECIAL|51;
const int SP_LESS =      RELOP|SPECIAL|52;
const int SP_LESSEQ =    RELOP|SPECIAL|53;
const int SP_GREAT =     RELOP|SPECIAL|54;
const int SP_GREATEQ =   RELOP|SPECIAL|55;
const int SP_NEGATE =    SPECIAL|56;
const int SP_SQUOTE =    SPECIAL|57;
const int SP_DQUOTE =    SPECIAL|58;
const int SP_COMMA =     SPECIAL|59;
const int SP_LPAREN =    SPECIAL|60;
const int SP_RPAREN =    SPECIAL|61;
const int SP_STAR =      SPECIAL|62;
const int SP_DIV =       SPECIAL|63;
const int SP_MOD =       SPECIAL|64;
const int SP_CARROT =    SPECIAL|65;
const int SP_MINUS =     SPECIAL|66;
const int SP_PLUS =      SPECIAL|67;
const int STATE_INITAL =    0;
const int STATE_SSPECIAL =  1;
const int STATE_DSPECIAL =  2;
const int STATE_MBSPECIAL = 3;
const int STATE_WORD =      4;
const int O_C = 1;
const int O_NH = 2;
const int O_H = 4;
const int O_S = 8;

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
	TOSCAN
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
		bool small;
		bool inmemory;
		char delim;
		i64 pos;
		vector<string> colnames;
		vector<int> types;
		vector<i64> positions;
		vector<vector<valpos>> joinValpos;
		vector<andchain> andchains;
		vector<int> vpTypes;
		csvEntry* entries;
		csvEntry* eend;
		bool noheader;
		string id;
		int fileno;
		int numFields;
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
	int code;
	int p1;
	int p2;
	int p3;
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
	void appendToJsonBuffer(string&);
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
	int count;
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
	pair<char*,int> chachaEncrypt(int, int, char*);
	pair<char*,int> chachaDecrypt(int, int, char*);
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

class singleQueryResult {
	public:
	int numrows =0;
	int showLimit =0;
	int numcols =0;
	int status =0;
	vector<int> types;
	vector<string> colnames;
	vector<int> pos;
	list<string> Vals; //each string is whole row
	string query;
	string tojson();
	singleQueryResult(querySpecs &q){
		numcols = q.colspec.count;
		showLimit = 20000 / numcols;
	}
};
class returnData {
	public:
	forward_list<shared_ptr<singleQueryResult>> entries;
	int status;
	string originalQuery;
	bool clipped;
	string message;
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
