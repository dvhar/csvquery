#ifndef TYPES_H
#define TYPES_H
#include <string>
#include <iostream>
#include <memory>
#include <boost/algorithm/string.hpp>
using namespace std;

enum nodetypes { N_QUERY, N_PRESELECT, N_WITH, N_VARS, N_SELECT, N_SELECTIONS, N_FROM, N_AFTERFROM, N_JOINCHAIN, N_JOIN, N_WHERE, N_ORDER, N_EXPRADD, N_EXPRMULT, N_EXPRNEG, N_EXPRCASE, N_CPREDLIST, N_CPRED, N_CWEXPRLIST, N_CWEXPR, N_PREDICATES, N_PREDCOMP, N_VALUE, N_FUNCTION, N_GROUPBY, N_EXPRESSIONS, N_DEXPRESSIONS };



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
	unique_ptr<node> node1;
	unique_ptr<node> node2;
	unique_ptr<node> node3;
	unique_ptr<node> node4;
	token tok1;
	token tok2;
	token tok3;
	token tok4;
	token tok5;
};
class querySpecs {
	public:
	string queryString;
	string password;
	vector<token> tokArray;
	int tokIdx;
	int options;
	int quantityLimit;
	bool joining;
	bool groupby;
	token tok();
	token nextTok();
	token peekTok();
	void Reset();
	void init(string);
};
/*
class querySpecs {
	queryString string
	tokArray []token
	aliases bool
	joining bool
	tokIdx int
	quantityLimit int
	quantityRetrieved int
	distinctExpr *node
	distinctCheck *bt.BTree
	sortExpr *node
	sortWay int
	save bool
	showLimit int
	stage int
	tree *node
	files map[string]*FileData
	numfiles int
	fromRow []string
	toRow []Value
	midRow []Value
	midExess int
	intColumn bool
	groupby bool
	noheader bool
	bigjoin bool
	joinSortVals []J2ValPos
	gettingSortVals bool
	password string
};
*/


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

extern map<int, string> enumMap;
extern map<int, string> treeMap;
extern map<string, int> keywordMap;
extern map<string, int> functionMap;
extern map<string, int> joinMap;
extern map<string, int> specialMap;

void scanTokens(querySpecs &q);
unique_ptr<node> parseQuery(querySpecs &q);
unique_ptr<node> newNode(int l);
unique_ptr<node> newNode(int l, token t);
void error(const string &err);
bool is_number(const std::string& s);
void printTree(unique_ptr<node> &n, int ident);

#endif
