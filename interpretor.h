#ifndef TYPES_H
#define TYPES_H
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <memory>
#include <sys/time.h>
#include <regex>
#include <regex.h>
#include <stdarg.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#define BUFSIZE  1024*1024
#define int64 long long
#define byte unsigned char
using namespace std;

//virtual machine stuff
//labels of subroutine instruction number
int MAIN, VARS_NORM, VARS_AGG, SEL_NORM, SEL_AGG, WHERE, HAVING, ORDER;

//code prefixes are for Int Float Text Date/Duration
enum codes : unsigned char {
	IADD, FADD, TADD, DADD,
	ISUB, FSUB, DSUB,
	IMULT, FMULT, DMULT,
	IDIV, FDIV, DDIV,
	JMP, JMPNEG, JMPPOS, JMPZ,
	RDLINE, RDLINEAT,
	PRINT
	};

//string type for vm
class stringy {
	public:
	char* s;
	byte m; //1 for malloced, 0 for buffered
	int z; //size of buffered
};
union dat {
	int64 i;
	double f;
	stringy s;
	time_t dt;
	time_t dr;
};

class vmachine {
	int ip; //instruction number
	vector<byte> bytes;
	vector<dat> vars;
	vector<dat> stack;
	void run();
};

//scanning and parsing stuff
enum nodetypes { N_QUERY, N_PRESELECT, N_WITH, N_VARS, N_SELECT, N_SELECTIONS, N_FROM, N_AFTERFROM, N_JOINCHAIN, N_JOIN, N_WHERE, N_HAVING, N_ORDER, N_EXPRADD, N_EXPRMULT, N_EXPRNEG, N_EXPRCASE, N_CPREDLIST, N_CPRED, N_CWEXPRLIST, N_CWEXPR, N_PREDICATES, N_PREDCOMP, N_VALUE, N_FUNCTION, N_GROUPBY, N_EXPRESSIONS, N_DEXPRESSIONS };

enum valTypes { LITERAL, COLUMN, VARIABLE, FUNCTION };

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
	void print();
};
class variable {
	public:
	string name;
	int type;
	int lit;
	set<int> types;
};

//file stuff
class fileReader {
	public:
	vector<string> colnames;
	vector<int> types;
	vector<int> sizes;
	vector<char*> line;
	int widestField;
	int fieldsFound;
	char* pos1;
	char* pos2;
	char* terminator;
	char buf[BUFSIZE];
	int getField();
	int getWidth();
	int numFields;
	bool noheader;
	streampos pos;
	ifstream fs;
	string id;
	string err;
	void inferTypes();
	void print();
	int getColIdx(string);
	int readline();
	fileReader(string);
};

//general query stuff
class querySpecs {
	public:
	string queryString;
	string password;
	vector<token> tokArray;
	vector<variable> vars;
	map<string, shared_ptr<fileReader>> files;
	unique_ptr<node> tree;
	int tokIdx;
	int options;
	int quantityLimit;
	bool joining;
	bool grouping;
	bool sorting;
	token tok();
	token nextTok();
	token peekTok();
	bool numIsCol();
	void reset();
	void init(string);
	void addVar(string);
};

//data declarations and const definitions
const int T_NULL = 0;
const int T_INT = 1;
const int T_FLOAT = 2;
const int T_DATE = 3;
const int T_DURATION = 4;
const int T_STRING = 5;
const int NUM_STATES =  5;
const int EOS =         255;
const int ERROR =       1<<23;
const int AGG_BIT =     1<<26;
const int FINAL =       1<<22;
const int KEYBIT =      1<<20;
const int LOGOP =       1<<24;
const int RELOP =       1<<25;
const int WORD =        FINAL|1<<27;
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

extern int typeChart[12][12];

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

//function headers
void scanTokens(querySpecs &q);
void parseQuery(querySpecs &q);
void error(const string &err);
bool is_number(const std::string& s);
void printTree(unique_ptr<node> &n, int ident);
string lower(string);
int scomp(const char*, const char*);
int slcomp(const char*, const char*);
int isInt(const char*);
int isFloat(const char*);
int dateParse(const char*, struct timeval*);
int parseDuration(char*, time_t*);
string nstring(string, int);
void openfiles(querySpecs &q, unique_ptr<node> &n);
void applyTypes(querySpecs &q);
int getNarrowestType(char* value, int startType);
int isInList(int n, int count, ...);

#endif
