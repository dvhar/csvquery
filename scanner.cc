#include <string>
#include <iostream>
#include <stdio.h>
#include <boost/algorithm/string.hpp>
using namespace std;

template<class T>
constexpr size_t len(T &a)
{
    return sizeof(a) / sizeof(typename std::remove_all_extents<T>::type);
}

	//misc
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

map<int, string> enumMap = {
	{EOS ,           "EOS"},
	{ERROR ,         "ERROR"},
	{FINAL ,         "FINAL"},
	{KEYBIT ,        "KEYBIT"},
	{LOGOP ,         "LOGOP"},
	{RELOP ,         "RELOP"},
	{WORD ,          "WORD"},
	{NUMBER ,        "NUMBER"},
	{KEYWORD ,       "KEYWORD"},
	{KW_AND ,        "KW_AND"},
	{KW_OR  ,        "KW_OR"},
	{KW_XOR  ,       "KW_XOR"},
	{KW_SELECT ,     "KW_SELECT"},
	{KW_FROM  ,      "KW_FROM"},
	{KW_HAVING  ,    "KW_HAVING"},
	{KW_AS  ,        "KW_AS"},
	{KW_WHERE ,      "KW_WHERE"},
	{KW_ORDER ,      "KW_ORDER"},
	{KW_BY ,         "KW_BY"},
	{KW_DISTINCT ,   "KW_DISTINCT"},
	{KW_ORDHOW ,     "KW_ORDHOW"},
	{KW_CASE ,       "KW_CASE"},
	{KW_WHEN ,       "KW_WHEN"},
	{KW_THEN ,       "KW_THEN"},
	{KW_ELSE ,       "KW_ELSE"},
	{KW_END ,        "KW_END"},
	{SPECIALBIT ,    "SPECIALBIT"},
	{SPECIAL ,       "SPECIAL"},
	{SP_EQ ,         "SP_EQ"},
	{SP_NEGATE ,     "SP_NEGATE"},
	{SP_NOEQ ,       "SP_NOEQ"},
	{SP_LESS ,       "SP_LESS"},
	{SP_LESSEQ ,     "SP_LESSEQ"},
	{SP_GREAT ,      "SP_GREAT"},
	{SP_GREATEQ ,    "SP_GREATEQ"},
	{SP_SQUOTE ,     "SP_SQUOTE"},
	{SP_DQUOTE ,     "SP_DQUOTE"},
	{SP_COMMA ,      "SP_COMMA"},
	{SP_LPAREN ,     "SP_LPAREN"},
	{SP_RPAREN ,     "SP_RPAREN"},
	{SP_STAR ,       "SP_STAR"},
	{SP_DIV ,        "SP_DIV"},
	{SP_MOD ,        "SP_MOD"},
	{SP_MINUS ,      "SP_MINUS"},
	{SP_PLUS ,       "SP_PLUS"},
	{STATE_INITAL ,  "STATE_INITAL"},
	{STATE_SSPECIAL ,"STATE_SSPECIAL"},
	{STATE_DSPECIAL ,"STATE_DSPECIAL"},
	{STATE_MBSPECIAL,"STATE_MBSPECIAL"},
	{STATE_WORD ,    "STATE_WORD"}
};
//characters of special tokens
int specials[] = { '*','=','!','<','>','\'','"','(',')',',','+','-','%','/','^' };
//non-alphanumeric characters of words
int others[] = { '\\',':','_','.','[',']','~','{','}' };
map<string, int> keywordMap = {
	{"and" ,       KW_AND},
	{"or" ,        KW_OR},
	{"xor" ,       KW_XOR},
	{"select" ,    KW_SELECT},
	{"from" ,      KW_FROM},
	{"having" ,    KW_HAVING},
	{"as" ,        KW_AS},
	{"where" ,     KW_WHERE},
	{"limit" ,     KW_LIMIT},
	{"order" ,     KW_ORDER},
	{"by" ,        KW_BY},
	{"distinct" ,  KW_DISTINCT},
	{"asc" ,       KW_ORDHOW},
	{"between" ,   KW_BETWEEN},
	{"like" ,      KW_LIKE},
	{"case" ,      KW_CASE},
	{"when" ,      KW_WHEN},
	{"then" ,      KW_THEN},
	{"else" ,      KW_ELSE},
	{"end" ,       KW_END},
	{"in" ,        KW_IN},
	{"group" ,     KW_GROUP},
	{"not" ,       SP_NEGATE}
};
//functions are normal words to avoid taking up too many words
//use map when parsing not scanning
map<string, int> functionMap = {
	{"inc" ,      FN_INC},
	{"sum" ,      FN_SUM},
	{"avg" ,      FN_AVG},
	{"min" ,      FN_MIN},
	{"max" ,      FN_MAX},
	{"count" ,    FN_COUNT},
	{"stdev" ,    FN_STDEV},
	{"stdevp" ,   FN_STDEVP},
	{"abs" ,      FN_ABS},
	{"format" ,   FN_FORMAT},
	{"coalesce" , FN_COALESCE},
	{"year" ,     FN_YEAR},
	{"month" ,    FN_MONTH},
	{"monthname", FN_MONTHNAME},
	{"week" ,     FN_WEEK},
	{"day" ,      FN_WDAY},
	{"dayname" ,  FN_WDAYNAME},
	{"dayofyear", FN_YDAY},
	{"dayofmonth",FN_MDAY},
	{"dayofweek", FN_WDAY},
	{"hour" ,     FN_HOUR},
	{"encrypt" ,  FN_ENCRYPT},
	{"decrypt" ,  FN_DECRYPT}
};
map<string, int> joinMap = {
	{"inner" ,  KW_INNER},
	{"outer" ,  KW_OUTER},
	{"left" ,   KW_LEFT},
	{"join" ,   KW_JOIN},
	{"bjoin" ,   KW_JOIN},
	{"sjoin" ,   KW_JOIN}
};
map<string, int> specialMap = {
	{"=" ,  SP_EQ},
	{"!" ,  SP_NEGATE},
	{"<>" , SP_NOEQ},
	{"<" ,  SP_LESS},
	{"<=" , SP_LESSEQ},
	{">" ,  SP_GREAT},
	{">=" , SP_GREATEQ},
	{"'" ,  SP_SQUOTE},
	{"\"" , SP_DQUOTE},
	{"," ,  SP_COMMA},
	{"(" ,  SP_LPAREN},
	{")" ,  SP_RPAREN},
	{"*" ,  SP_STAR},
	{"+" ,  SP_PLUS},
	{"-" ,  SP_MINUS},
	{"%" ,  SP_MOD},
	{"/" ,  SP_DIV},
	{"^" ,  SP_CARROT}
};
int table[NUM_STATES][256];
void initable(){
	static bool tabinit = false;
	if (tabinit) return;
	//initialize table to errr
	int i,j;
	for (i=0; i<NUM_STATES; i++) {
		for (j=0; j<255; j++) { table[i][j] = ERROR; }
	}
	//next state from initial
	for (i=0; i<len(others); i++) { table[0][others[i]] = STATE_WORD; }
	for (i=0; i<len(specials); i++) { table[0][specials[i]] = STATE_SSPECIAL; }
	table[0][255] =  EOS;
	table[0][' '] =  STATE_INITAL;
	table[0]['\n'] = STATE_INITAL;
	table[0]['\t'] = STATE_INITAL;
	table[0][';'] =  STATE_INITAL;
	table[0][0] =	STATE_INITAL;
	table[0]['<'] =  STATE_MBSPECIAL;
	table[0]['>'] =  STATE_MBSPECIAL;
	for (i='a'; i<='z'; i++) { table[0][i] = STATE_WORD; }
	for (i='A'; i<='Z'; i++) { table[0][i] = STATE_WORD; }
	for (i='0'; i<='9'; i++) { table[0][i] = STATE_WORD; }
	//next state from single-char special
	for (i='a'; i<='z'; i++) { table[STATE_SSPECIAL][i] = SPECIAL; }
	for (i='A'; i<='Z'; i++) { table[STATE_SSPECIAL][i] = SPECIAL; }
	for (i='0'; i<='9'; i++) { table[STATE_SSPECIAL][i] = SPECIAL; }
	for (i=0; i<len(others); i++) { table[STATE_SSPECIAL][others[i]] = SPECIAL; }
	for (i=0; i<len(specials); i++) { table[STATE_SSPECIAL][specials[i]] = SPECIAL; }
	table[STATE_SSPECIAL][';'] =  SPECIAL;
	table[STATE_SSPECIAL][' '] =  SPECIAL;
	table[STATE_SSPECIAL]['\n'] = SPECIAL;
	table[STATE_SSPECIAL]['\t'] = SPECIAL;
	table[STATE_SSPECIAL][EOS] =  SPECIAL;
	table[STATE_MBSPECIAL]['='] =  STATE_MBSPECIAL;
	table[STATE_MBSPECIAL]['>'] =  STATE_MBSPECIAL;
	table[STATE_MBSPECIAL][';'] =  STATE_SSPECIAL;
	table[STATE_MBSPECIAL][' '] =  STATE_SSPECIAL;
	table[STATE_MBSPECIAL]['\n'] = STATE_SSPECIAL;
	table[STATE_MBSPECIAL]['\t'] = STATE_SSPECIAL;
	for (i='a'; i<='z'; i++) { table[STATE_MBSPECIAL][i] = SPECIAL; }
	for (i='A'; i<='Z'; i++) { table[STATE_MBSPECIAL][i] = SPECIAL; }
	for (i='0'; i<='9'; i++) { table[STATE_MBSPECIAL][i] = SPECIAL; }
	for (i=0; i<len(others); i++) { table[STATE_MBSPECIAL][others[i]] = SPECIAL; }
	//next state from word
	for (i=0; i<len(specials); i++) { table[STATE_WORD][specials[i]] = WORD; }
	for (i=0; i<len(others); i++) { table[STATE_WORD][others[i]] = STATE_WORD; }
	table[STATE_WORD][' '] =  WORD;
	table[STATE_WORD]['\n'] = WORD;
	table[STATE_WORD]['\t'] = WORD;
	table[STATE_WORD][';'] =  WORD;
	table[STATE_WORD][EOS] =  WORD;
	for (i='a'; i<='z'; i++) { table[STATE_WORD][i] = STATE_WORD; }
	for (i='A'; i<='Z'; i++) { table[STATE_WORD][i] = STATE_WORD; }
	for (i='0'; i<='9'; i++) { table[STATE_WORD][i] = STATE_WORD; }

	/*
	for i=0; i< NUM_STATES; i++{
		for j=0; j< 255; j++{
			Printf("[ %d ][ %c ]=%-34s",i,j,enumMap[table[i][j]])
		}
		Printf("\n")
	}
	*/
	tabinit = true;
}


class Token {
	public:
	int id;
	string val;
	int line;
	int col;
	bool quoted;
	string Lower();
};
string Token::Lower() {
	string s = val;
	boost::to_lower(s);
	return s;
}

class StringLookahead {
	public:
	string Str;
	int idx;
	int getc();
	int peek();
};
int StringLookahead::getc() {
	if (idx >= Str.length()) { return EOS; }
	idx++;
	return (int) Str[idx-1];
}
int StringLookahead::peek() {
	if (idx >= Str.length()) { return EOS; }
	return (int) Str[idx];
}

Token scanner(StringLookahead &s) {
	initable();
	static int lineNo = 1;
	static int colNo = 1;
	static int waitForQuote;
	int state = STATE_INITAL;
	int nextState, nextchar;
	string S;

	while ( (state & FINAL) == 0 && state < NUM_STATES ) {
		nextState = table[state][s.peek()];
		if ((nextState & ERROR) != 0) {
		//end of string
			if (state == 255) return { 255, "END", lineNo, colNo, false };
			stringstream e;
			e << "line: " << lineNo << ", col: " << colNo << ", char: " << (char) s.peek();
			return { ERROR, e.str(), lineNo, colNo, false };
		}
		if ((nextState & FINAL) != 0) {
			//see if keyword or regular word
			if (nextState == WORD) {
				string s = S;
				boost::to_lower(s);
				if (keywordMap.count(s) && waitForQuote == 0) {
					//return keyword token
					return { keywordMap[s], S, lineNo, colNo, false };
				} else {
					//return word token
					return { nextState, S, lineNo, colNo, false };
				}
			//see if special type or something else
			} else if (nextState == SPECIAL) {
				if (specialMap.count(S)) {
					int sp = specialMap[S];
					//find out if these tokens are in a quote to preserve whitespace
					if ((sp == SP_SQUOTE || sp == SP_DQUOTE) && waitForQuote == 0) {
						waitForQuote = sp;
					} else if ((sp == SP_SQUOTE || sp == SP_DQUOTE) && waitForQuote == sp) {
						waitForQuote = 0;
					}
					//return special token
					return { sp, S, lineNo, colNo, false };
				} else {
					stringstream e;
					e << "line: " << lineNo << ", col: " << colNo << ", char: " << (char) s.peek();
					return { ERROR, e.str(), lineNo, colNo, false };
				}
			} else {
				return { nextState, S, lineNo, colNo, false };
			}

		} else {
			state = nextState;
			nextchar = s.getc();
			colNo++;
			//include whitespace in the token when waiting for a closing quote
			if (waitForQuote != 0) {
				S += nextchar;
			} else if (nextchar != ' ' && nextchar != '\t' && nextchar != '\n' && nextchar != ';'){
				S += nextchar;
			}
			if (nextchar == '\n') { lineNo++; colNo=0; }
			if (nextchar == EOS) {
				return { EOS, "END", lineNo, colNo, false };
			}
		}
	}
	return { EOS, "END", lineNo, colNo, false };

}

//turn query text into tokens and check if ints are columns or numbers
class QuerySpecs {
	public:
	string QueryString;
	vector<Token> tokArray;
};

string scanTokens(QuerySpecs &q) {
	int lineNo = 1;
	int colNo = 1;
	StringLookahead input = {q.QueryString, 0};
	while(1) {
		Token t = scanner(input);
		//turn tokens inside quotes into single token
		if (t.id == SP_SQUOTE || t.id == SP_DQUOTE) {
			int quote = t.id;
			string S = "";
			for (Token t = scanner(input); t.id != quote && t.id != EOS ; t = scanner(input)) {
				if (t.id == ERROR) return "scanner error: "+t.val;
				S += t.val;
			}
			if (t.id != quote)  return "Quote was not terminated";
			t = {WORD,S,t.line,t.col,true};
		}
		q.tokArray.push_back(t);
		if (t.id == ERROR) return "scanner error: "+t.val;
		if (t.id == EOS) break;
	}
	return "";
}

int main(){
	int s;
	QuerySpecs q;
	while (!feof(stdin)){
		scanf("%c",&s);
		q.QueryString.push_back((char)s);
	}
	scanTokens(q);
	for (vector<Token>::iterator it = q.tokArray.begin(); it != q.tokArray.end(); ++it){
		cout << it->val << endl;;
	}

}
