#ifndef TYPES_H
#define TYPES_H
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <thread>
#include <iomanip>
#include <fstream>
#include <memory>
#include <sys/time.h>
#include <sys/types.h>
#include <regex>
#include <stdarg.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "deps/dateparse/dateparse.h"
#include "deps/chacha/chacha20.h"

#ifdef __MINGW32__
#include <getopt.h>
#include <tre/regex.h>
#else
#include <regex.h>
#endif


#define BUFSIZE  1024*1024
#define chacha struct chacha20_context
#define int64 long long
#define dur_t long long
#define byte unsigned char
#define ft boost::format
#define error(A) throw invalid_argument(A)
#define SMALLEST ((int64) 1 << 63)
using namespace std;



enum nodetypes { N_QUERY, N_PRESELECT, N_WITH, N_VARS, N_SELECT, N_SELECTIONS, N_FROM, N_AFTERFROM, N_JOINCHAIN, N_JOIN, N_WHERE, N_HAVING, N_ORDER, N_EXPRADD, N_EXPRMULT, N_EXPRNEG, N_EXPRCASE, N_CPREDLIST, N_CPRED, N_CWEXPRLIST, N_CWEXPR, N_PREDICATES, N_PREDCOMP, N_VALUE, N_FUNCTION, N_GROUPBY, N_EXPRESSIONS, N_DEXPRESSIONS, N_TYPECONV };

enum valTypes { LITERAL, COLUMN, VARIABLE, FUNCTION };
enum varFilters { WHERE_FILTER=1, DISTINCT_FILTER=2, ORDER_FILTER=4, GROUP_FILTER=8, HAVING_FILTER=16, JCOMP_FILTER=32, JSCAN_FILTER=64, SELECT_FILTER=128 };
enum varScopes { V_SCOPE1, V_SCOPE2, V_SCANSCOPE };


const byte RMAL = 8; //regex needs regfree() (dataholder vector only)
const byte MAL = 16; //malloced and responsible for freeing c string
const byte NIL = 32;

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
const int FN_ENCRYPT =  KEYWORD|47;
const int FN_DECRYPT =  KEYWORD|48;
const int FN_INC =      KEYWORD|49;
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

extern map<int, string> enumMap;
extern map<int, string> treeMap;
extern map<string, int> keywordMap;
extern map<string, int> functionMap;
extern map<string, int> joinMap;
extern map<string, int> specialMap;

extern regex_t leadingZeroString;
extern regex_t durationPattern;
extern regex_t intType;
extern regex_t floatType;
extern regex cInt;
extern regex posInt;
extern regex colNum;

class token {
	public:
	int id;
	string val;
	int line;
	int col;
	bool quoted;
	string lower();
	void print();
};
class node {
	public:
	int label;
	int datatype;
	int phase;
	bool keep; //preserve subtree types
	unique_ptr<node> node1;
	unique_ptr<node> node2;
	unique_ptr<node> node3;
	unique_ptr<node> node4;
	token tok1;
	token tok2;
	token tok3;
	token tok4;
	token tok5;
	token tok6;
	void print();
};
class variable {
	public:
	string name;
	int type;
	int lit;
	int filter;
	int phase;
	int mrindex;
	set<int> filesReferenced;
	int maxfileno;
};

class csvEntry {
	public:
	char* val;
	char* terminator;
};

class valpos;
class fileReader {
	int fieldsFound;
	char* pos1;
	char* pos2;
	char* terminator;
	char buf[BUFSIZE];
	ifstream fs;
	string filename;
	streampos prevpos;
	public:
		int linecount;
		streampos pos;
		vector<string> colnames;
		vector<int> types;
		vector<int64> positions;
		vector<vector<valpos>> joinValpos;
		vector<int> vpTypes;
		vector<csvEntry> entries;
		bool noheader;
		string id;
		int fileno;
		int numFields;
	inline void getField();
	inline bool checkWidth();
	void inferTypes();
	void print();
	int getColIdx(string);
	bool readline();
	void numlines();
	bool readlineat(int64);
	fileReader(string);
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
	int64 i; //also used for date and duration
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
	union datunion u;
	unsigned int b; // metadata bit array
	unsigned int z; // string size
	void appendToBuffer(string&);
	string str(){ string st; appendToBuffer(st); return st; }
	friend bool operator<(const dat& l, const dat& r){
		if ((l.b & 7) == T_STRING) {
			if ((l.b | r.b) & NIL){
				return (r.b & NIL) < (l.b & NIL);
			}
			return strcmp(l.u.s, r.u.s) < 0;
		} else
			return l.u.i < r.u.i;
	}
	dat heap(){
		dat d;
		d.z = z;
		if ((b & 7) == T_STRING && (b & MAL) == 0){
			d.u.s = (char*) malloc(z+1);
			strcpy(d.u.s, u.s);
			d.b = b | MAL;
		} else {
			d.b = b;
			d.u.s = u.s;
		}
		b &= ~MAL;
		return d;
	}
};

class valpos {
	public:
	datunion val;
	int64 pos;
};

class resultSpecs {
	public:
	int count;
	vector<int> types;
	vector<string> colnames;
};

class singleQueryResult {
	public:
	int numrows;
	int numcols;
	vector<int> types;
	vector<string> colnames;
	vector<vector<char*>> values;
	string query;
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
	string queryString;
	string password;
	vector<token> tokArray;
	vector<pair<int,int>> sortInfo; // asc, datatype
	vector<variable> vars;
	vector<dat> dataholder;
	vector<opcode> bytecode;
	map<string, shared_ptr<fileReader>> files;
	unique_ptr<node> tree;
	resultSpecs colspec;
	crypter crypt;
	int distinctSFuncs;
	int distinctNFuncs;
	int midcount;
	int numFiles;
	int tokIdx;
	int options;
	int btn;
	int bts;
	int quantityLimit;
	int posVecs;
	int sorting;
	int sortcount;
	bool joining;
	bool strictJoin; //require 1 file on each side of '='
	int grouping; //1 = one group, 2 = groups
	bool whereFiltering;
	bool havingFiltering;
	token tok();
	token nextTok();
	token peekTok();
	bool numIsCol();
	void init(string);
	void addVar(string);
	shared_ptr<fileReader>& getFileReader(int);
	variable& var(string);
	~querySpecs();
	querySpecs(string &s);
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
int getVarLocation(string lkup, querySpecs *q);
int getVarIdx(string lkup, querySpecs *q);
int getVarType(string lkup, querySpecs *q);
int getFileNo(string s, querySpecs *q);
char* durstring(dur_t dur, char* str);
void runServer();
unique_ptr<node>& findFirstNode(unique_ptr<node> &n, int label);

template<typename T>
static string st(T v) {
	stringstream ss;
	ss << v;
	return ss.str();
}
template<typename T, typename... Args>
static string st(T first, Args... args) {
	stringstream ss;
	ss << first << st(args...);
	return ss.str();
}
extern int runmode;
enum runmodes { RUN_CMD, RUN_SINGLE, RUN_SERVER };

#endif
