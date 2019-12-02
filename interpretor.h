#ifndef TYPES_H
#define TYPES_H
#include <string>
#include <boost/algorithm/string.hpp>
using namespace std;

enum nodetypes { N_QUERY, N_SELECT, N_SELECTIONS, N_FROM, N_JOINCHAIN, N_JOIN, N_WHERE, N_ORDER, N_EXPRADD, N_EXPRMULT, N_EXPRNEG, N_EXPRCASE, N_CPREDLIST, N_CPRED, N_CWEXPRLIST, N_CWEXPR, N_PREDICATES, N_PREDCOMP, N_VALUE, N_FUNCTION, N_GROUPBY, N_EXPRESSIONS, N_DEXPRESSIONS };



class Token {
	public:
	int id;
	string val;
	int line;
	int col;
	bool quoted;
	string Lower();
};
class Node {
	public:
	int label;
	Node *node1;
	Node *node2;
	Node *node3;
	Node *node4;
	Node *node5;
	Node *node6;
	Token tok1;
	Token tok2;
	Token tok3;
	Token tok4;
	Token tok5;
};

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
extern map<string, int> keywordMap;
extern map<string, int> functionMap;
extern map<string, int> joinMap;
extern map<string, int> specialMap;

class QuerySpecs {
	public:
	string QueryString;
	string password;
	vector<Token> tokArray;
	int tokIdx;
	int options;
	int quantityLimit;
	bool joining;
	bool groupby;
	Token Tok();
	Token NextTok();
	Token PeekTok();
	void Reset();
};
/*
class QuerySpecs {
	QueryString string
	tokArray []Token
	aliases bool
	joining bool
	tokIdx int
	quantityLimit int
	quantityRetrieved int
	distinctExpr *Node
	distinctCheck *bt.BTree
	sortExpr *Node
	sortWay int
	save bool
	showLimit int
	stage int
	tree *Node
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

string scanTokens(QuerySpecs &q);
Node* newNode(int l);
Node* newNode(int l, Token t);
void error(const string &err);
bool is_number(const std::string& s);

#endif
