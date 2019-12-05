#include "interpretor.h"


string token::lower() {
	string s = val;
	boost::to_lower(s);
	return s;
}
void token::print() {
	cout << "    id: " << id << " -> " << enumMap[id] << endl
		<< "    val: " << val << endl;
}


unique_ptr<node> newNode(int l){
	unique_ptr<node> n(new node);
	n->tok1 = n->tok2 = n->tok3 = n->tok4 = n->tok5 = token{};
	n->label = l;
	return n;
}
unique_ptr<node> newNode(int l, token t){
	unique_ptr<node> n(new node);
	n->tok2 = n->tok3 = n->tok4 = n->tok5 = token{};
	n->label = l;
	n->tok1 = t;
	return n;
}

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

bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

void error(const string &err){
	throw invalid_argument(err);
}

token querySpecs::nextTok() {
	if (tokIdx < tokArray.size()-1) tokIdx++;
	return tokArray[tokIdx];
}
token querySpecs::peekTok() {
	if (tokIdx < tokArray.size()-1) return tokArray[tokIdx+1];
	return tokArray[tokIdx];
}
token querySpecs::tok() { return tokArray[tokIdx]; }
void querySpecs::Reset() { tokIdx = 0; }

void querySpecs::init(string s){
	queryString = s;
	tokIdx = options = quantityLimit = 0;
	joining = groupby = false;
}

void printTree(unique_ptr<node> &n, int ident){
	if (n == nullptr) return;
	ident++;
	string s = "";
	for (int i=0;i<ident;i++) s += "  ";
	cout << s << enumMap[n->label] << endl
		<< s << n->tok1.val << "  " << n->tok2.val << "  " << n->tok3.val << endl;
	printTree(n->node1,ident);
	printTree(n->node2,ident);
	printTree(n->node3,ident);
	printTree(n->node4,ident);
}
