#include "interpretor.h"
#include <stdlib.h>
#include <ctype.h>
#define max(a,b) (a) > (b) ? (a) : (b)
#define isInt(A) !regexec(&intType, A, 0, NULL, 0) //make faster version
#define isFloat(A) !regexec(&floatType, A, 0, NULL, 0)
#define isDuration(A) !regexec(&durationPattern, A, 0, NULL, 0)

regex_t leadingZeroString;
regex_t durationPattern;
regex_t intType;
regex_t floatType;
regex cInt("^c\\d+$");
regex posInt("^\\d+$");
regex colNum("^c?\\d+$");

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
map<int, string> treeMap = {
	{N_QUERY,      "N_QUERY"},
	{N_SELECT,     "N_SELECT"},
	{N_PRESELECT,  "N_PRESELECT"},
	{N_SELECTIONS, "N_SELECTIONS"},
	{N_FROM,       "N_FROM"},
	{N_AFTERFROM,  "N_AFTERFROM"},
	{N_WHERE,      "N_WHERE"},
	{N_HAVING,     "N_HAVING"},
	{N_ORDER,      "N_ORDER"},
	{N_EXPRADD,    "N_EXPRADD"},
	{N_EXPRMULT,   "N_EXPRMULT"},
	{N_EXPRNEG,    "N_EXPRNEG"},
	{N_CPREDLIST,  "N_CPREDLIST"},
	{N_CPRED,      "N_CPRED"},
	{N_PREDICATES, "N_PREDICATES"},
	{N_PREDCOMP,   "N_PREDCOMP"},
	{N_CWEXPRLIST, "N_CWEXPRLIST"},
	{N_CWEXPR,     "N_CWEXPR"},
	{N_EXPRCASE,   "N_EXPRCASE"},
	{N_VALUE,      "N_VALUE"},
	{N_FUNCTION,   "N_FUNCTION"},
	{N_GROUPBY,    "N_GROUPBY"},
	{N_EXPRESSIONS,"N_EXPRESSIONS"},
	{N_JOINCHAIN,  "N_JOINCHAIN"},
	{N_JOIN,       "N_JOIN"},
	{N_DEXPRESSIONS,"N_DEXPRESSIONS"},
	{N_WITH,       "N_WITH"},
	{N_VARS,       "N_VARS"}
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

token querySpecs::nextTok() {
	if (tokIdx < tokArray.size()-1) tokIdx++;
	return tokArray[tokIdx];
}
token querySpecs::peekTok() {
	if (tokIdx < tokArray.size()-1) return tokArray[tokIdx+1];
	return tokArray[tokIdx];
}
token querySpecs::tok() { return tokArray[tokIdx]; }
void querySpecs::addVar(string name) {
	variable var;
	var.name = name;
	var.type = 0;
	var.lit = 0;
	var.filter = 0;
	vars.push_back(var);
}
void querySpecs::init(string s){
	queryString = s;
	tokIdx = options = quantityLimit = numFiles = colspec.count = 0;
	whereFiltering = havingFiltering = joining = grouping = sorting = false;
}
bool querySpecs::numIsCol() { return (options & O_C) != 0; }

void printTree(unique_ptr<node> &n, int ident){
	if (n == nullptr) return;
	ident++;
	string s = "";
	for (int i=0;i<ident;i++) s += "  ";
	cout << s << treeMap[n->label] << endl
		<< s 
		<< ft("[{} {} {} {} {}]"
		,n->tok1.val
		,n->tok2.val
		,n->tok3.val
		,n->tok4.val
		,n->tok5.val)
		<< n->datatype << endl;
	printTree(n->node1,ident);
	printTree(n->node2,ident);
	printTree(n->node3,ident);
	printTree(n->node4,ident);
}

//fast c string compare
int scomp(const char* s1, const char*s2){
	while (*s1 && *s1==*s2) { ++s1; ++s2; }
	return *s1 - *s2;
}
//same but lowercase
int slcomp(const char* s1, const char*s2){
	while (*s1 && tolower(*s1)==*s2) { ++s1; ++s2; }
	return *s1 - *s2;
}
int isInList(int n, int count, ...)
{
    int i, temp;
    va_list args;
    va_start(args, count);
    for(i=0; i<count; i++)
    {
        temp = va_arg(args, int);
		if (n == temp) return 1;
    }
    va_end(args);
    return 0;
}

int parseDuration(char* str, date_t* t) {
	if (!isDuration(str)) {return -1;}
	char* part2;
	double quantity = strtod(str, &part2);
	while (*part2 == ' ') ++part2;
	//make this faster
	if (slcomp(part2,"y") || slcomp(part2,"year") || slcomp(part2,"years"))
		quantity *= 31536000;
	else if (slcomp(part2,"w") || slcomp(part2,"week") || slcomp(part2,"weeks"))
		quantity *= 604800;
	else if (slcomp(part2,"d") || slcomp(part2,"day") || slcomp(part2,"days"))
		quantity *= 86400;
	else if (slcomp(part2,"h") || slcomp(part2,"hour") || slcomp(part2,"hours"))
		quantity *= 3600;
	else if (slcomp(part2,"m") || slcomp(part2,"minute") || slcomp(part2,"minutes"))
		quantity *= 60;
	else if (slcomp(part2,"s") || slcomp(part2,"second") || slcomp(part2,"seconds"))
		{}
	else return -1;
	*t = quantity * 1E6;
	return 0;
}

int getNarrowestType(char* value, int startType) {
	date_t t;
	struct timeval tv;
	if (!slcomp(value,"null") || !scomp(value,"NA") || value[0] == '\0') {
	  startType = max(T_NULL, startType);
	} else if (!regexec(&leadingZeroString, value, 0, NULL, 0)){ startType = T_STRING;
	} else if (isInt(value))                       { startType = max(T_INT, startType);
	} else if (isFloat(value))                     { startType = max(T_FLOAT, startType);
	} else if (!dateparse_2(value, &tv))           { startType = max(T_DATE, startType);
	  //in case duration gets mistaken for a date
	   if (!parseDuration(value, &t))              { startType = max(T_DURATION, startType); }
	} else if (!parseDuration(value, &t))          { startType = max(T_DURATION, startType);
	} else                                         { startType = T_STRING; }
	return startType;
}

int getVarIdx(string lkup, querySpecs &q){
	for (int i=0; i<q.vars.size(); i++)
		if (lkup == q.vars[i].name)
			return i;
	error("variable not found");
	return 0;
}
int getVarType(string lkup, querySpecs &q){
	for (auto &v : q.vars)
		if (lkup == v.name)
			return v.type;
	error("variable not found");
	return 0;
}

int getFileNo(string s, querySpecs &q){
	for (int i=1; i<=q.numFiles; ++i)
		if (q.files[str2("_f", i)]->id == q.files[s]->id)
			return i-1;
	error("file number not founde");
	return -1;
}

//use static buf if null arg, otherwise make sure size 24
char* durstring(dur_t dur, char* str){
	static char durbuf[24];
	static char* formats[2][2] = {
		{ (char*)"%lld:%lld:%lld", (char*)"%lld:%lld:%lld.%lld" },
		{ (char*)"_%lld:%lld:%lld", (char*)"_%lld:%lld:%lld.%lld"}
	};
	if (dur < 0)
		dur *= -1;
	dur_t mics = dur % 1000000;
	dur_t secs = (dur % 60000000) / 1000000;
	dur_t mins = (dur % 3600000000) / 60000000;
	dur_t hours = (dur % 86400000000) / 3600000000;
	dur_t days = dur / 86400000000;
	int f1=0, f2=0;
	char *dest;
	char *s;
	if (str == nullptr)
		dest = durbuf;
	else
		dest = str;
	if (dur == 0){
		strcpy(dest, "0d");
		return dest;
	}
	s = dest;
	memset(s, 0, 24);
	if (days){
		sprintf(s, "%lldd", days);
		f1 = 1;
	}
	if (mics){
		f2 = 1;
	}
	if (hours || mins || secs){
		while(*s) ++s;
		sprintf(s, formats[f1][f2], hours, mins, secs, mics);
	} else {
		while(*s) ++s;
		if (mics < 1000){
			sprintf(s, "%lldms", mics/1000);
		} else {
			sprintf(s, "%lldmcs", mics);
		}
	}
	return dest;
}
