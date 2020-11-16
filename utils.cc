#include "interpretor.h"
#include <chrono>
#include <stdlib.h>
#include <ctype.h>
#include "deps/json/escape.h"
#define max(a,b) (a) > (b) ? (a) : (b)

string version = "1.16";
int runmode;
regex_t leadingZeroString;
regex_t durationPattern;
regex_t intType;
regex_t floatType;
regex cInt("^c\\d+$");
regex posInt("^\\d+$");
regex colNum("^c?\\d+$");
regex extPat(".*\\.csv$", regex_constants::icase);
regex hidPat(".*/\\.[^/]+$");
int isDuration(const char* s){ return !regexec(&durationPattern, s, 0, NULL, 0); }
int isInt(const char* s){ return !regexec(&intType, s, 0, NULL, 0); }
int isFloat(const char* s){ return !regexec(&floatType, s, 0, NULL, 0); }

void initregex(){
	regcomp(&leadingZeroString, "^0[0-9]+$", REG_EXTENDED);
	regcomp(&durationPattern, "^([0-9]+|[0-9]+\\.[0-9]+) ?(seconds|second|minutes|minute|hours|hour|days|day|weeks|week|years|year|s|m|h|d|w|y)$", REG_EXTENDED);
	regcomp(&intType, "^-?[0-9]+$", REG_EXTENDED);
	regcomp(&floatType, "^-?[0-9]*\\.[0-9]+$", REG_EXTENDED);
}
mt19937 rng(chrono::high_resolution_clock::now().time_since_epoch().count());

const flatmap<int, string_view> typeNames = {
	{T_STRING,   "text"},
	{T_INT,      "int"},
	{T_FLOAT,    "float"},
	{T_DATE,     "date"},
	{T_DURATION, "duration"},
	{T_NULL,     "null"},
};
const flatmap<int, string_view> treeMap = {
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
	{N_VARS,       "N_VARS"},
	{N_TYPECONV,   "N_TYPECONV"}
};
const flatmap<string_view, int> keywordMap = {
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
const flatmap<string_view, int> functionMap = {
	{"inc" ,      FN_INC},
	{"sum" ,      FN_SUM},
	{"avg" ,      FN_AVG},
	{"min" ,      FN_MIN},
	{"max" ,      FN_MAX},
	{"count" ,    FN_COUNT},
	{"stdev" ,    FN_STDEV},
	{"stddev" ,   FN_STDEV},
	{"stdevp" ,   FN_STDEVP},
	{"stddevp" ,  FN_STDEVP},
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
	{"minute" ,   FN_MINUTE},
	{"second" ,   FN_SECOND},
	{"encrypt" ,  FN_ENCRYPT},
	{"decrypt" ,  FN_DECRYPT},
	{"ceil",         FN_CEIL},
	{"floor",        FN_FLOOR},
	{"acos",         FN_ACOS},
	{"asin",         FN_ASIN},
	{"atan",         FN_ATAN},
	{"cos",          FN_COS},
	{"sin",          FN_SIN},
	{"tan",          FN_TAN},
	{"exp",          FN_EXP},
	{"log",          FN_LOG},
	{"log2",         FN_LOG2},
	{"log10",        FN_LOG10},
	{"sqrt",         FN_SQRT},
	{"rand",         FN_RAND},
	{"upper",        FN_UPPER},
	{"lower",        FN_LOWER},
	{"base64_encode",FN_BASE64_ENCODE},
	{"base64_decode",FN_BASE64_DECODE},
	//{"hex_encode",   FN_HEX_ENCODE},
	//{"hex_decode",   FN_HEX_DECODE},
	{"len",          FN_LEN},
	{"substr",       FN_SUBSTR},
	{"md5",          FN_MD5},
	{"sha1",         FN_SHA1},
	{"sha256",       FN_SHA256},
	{"string",       FN_STRING},
	{"text",         FN_STRING},
	{"int",          FN_INT},
	{"float",        FN_FLOAT},
	{"date",         FN_DATE},
	{"duration",     FN_DUR},
	{"round",        FN_ROUND},
	{"pow",          FN_POW},
	{"cbrt",         FN_CBRT},
	{"now",          FN_NOW},
	{"nowlocal",     FN_NOW},
	{"nowgm",        FN_NOWGM},

};
//use WORD for these?
const flatmap<string_view, int> joinMap = {
	{"inner" ,  KW_INNER},
	{"left" ,   KW_LEFT},
	{"join" ,   KW_JOIN},
	//{"outer" ,  KW_OUTER},
	//{"cross" ,  KW_CROSS},
};
const flatmap<string_view, int> specialMap = {
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

token querySpecs::lastTok() {
	return tokIdx > 0 ? tokArray[tokIdx-1] : token{};
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
variable& querySpecs::var(string name) {
	for (auto &v : vars)
		if (name == v.name)
			return v;
	error("Variable not found");
}
void querySpecs::addVar(string name) {
	vars.push_back({name,0,0,0,0,{}});
}
bool querySpecs::numIsCol() { return (options & O_C) != 0; }
shared_ptr<fileReader>& querySpecs::getFileReader(int i) {
	return filevec.at(i);
}
int querySpecs::getVarIdx(string lkup){
	for (u32 i=0; i<vars.size(); i++)
		if (lkup == vars[i].name)
			return i;
	error("variable not found");
	return 0;
}
int querySpecs::getVarType(string lkup){
	for (auto &v : vars)
		if (lkup == v.name)
			return v.type;
	error("variable not found");
	return 0;
}

int querySpecs::getFileNo(string s){
	auto& f = filemap[s];
	if (f == nullptr)
		error("file number not founde");
	return f->fileno;
}
void querySpecs::promptPassword(){
	if (needPass && password.empty()){
		if (runmode == RUN_SERVER){
			sendPassPrompt(sessionId);
			auto passFuture = passReturn.get_future();
			if (passFuture.wait_for(chrono::minutes(1)) == future_status::timeout)
				error("Timed out while waiting for encryption password");
			password = passFuture.get();
		} else {
			cerr << "Enter encryption password:\n";
			hideInput();
			cin >> password;
		}
		if (password.empty())
			error("Encryption password is empty");
	}
}
void querySpecs::setPassword(string s){
	passReturn.set_value(s);
};


void printTree(unique_ptr<node> &n, int ident){
	if (n == nullptr) return;
	ident++;
	string s = "";
	for (int i=0;i<ident;i++) s += "  ";
	perr(st( s , getnodename(n->label) , '\n',
		s ,
		ft("[%1% %2% %3% %4% %5%] t:%6% ph:%7%")
		% n->tok1.val
		% n->tok2.val
		% n->tok3.val
		% n->tok4.val
		% n->tok5.val
		% n->datatype
		% n->phase , '\n'));
	printTree(n->node1,ident);
	printTree(n->node2,ident);
	printTree(n->node3,ident);
	printTree(n->node4,ident);
}

int parseDuration(char* str, date_t* t) {
	if (!isDuration(str)) {return -1;}
	char* part2;
	double quantity = strtod(str, &part2);
	while (*part2 == ' ') ++part2;
	switch (part2[0]){
	case 'y':
		quantity *= 31536000;
		break;
	case 'w':
		quantity *= 604800;
		break;
	case 'd':
		quantity *= 86400;
		break;
	case 'h':
		quantity *= 3600;
		break;
	case 'm':
		quantity *= 60;
		break;
	case 's':
		break;
	default:
		return -1;
	}
	*t = quantity * 1E6;
	return 0;
}

int getNarrowestType(char* value, int startType) {
	date_t t;
	if (value[0] == '\0' || !strcasecmp(value,"null") || !strcmp(value,"NA")) {
	  startType = max(T_NULL, startType);
	} else if (!regexec(&leadingZeroString, value, 0, NULL, 0)){ startType = T_STRING;
	} else if (isInt(value))                       { startType = max(T_INT, startType);
	} else if (isFloat(value))                     { startType = max(T_FLOAT, startType);
	} else if (!dateparse_2(value, &t))            { startType = max(T_DATE, startType);
	  //in case duration gets mistaken for a date
	   if (!parseDuration(value, &t))              { startType = max(T_DURATION, startType); }
	} else if (!parseDuration(value, &t))          { startType = max(T_DURATION, startType);
	} else                                         { startType = T_STRING; }
	return startType;
}

//use static buf if null arg, otherwise make sure size 24
char* durstring(dur_t dur, char* str){
	static char durbuf[24];
	static char* formats[2][2] = {
		{ (char*)"%02lld:%02lld:%02lld", (char*)"%02lld:%02lld:%02lld.%02lld" },
		{ (char*)"_%02lld:%02lld:%02lld", (char*)"_%02lld:%02lld:%02lld.%02lld"}
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
	} else if (mics) {
		while(*s) ++s;
		if (mics < 1000){
			sprintf(s, "%lldms", mics/1000);
		} else {
			sprintf(s, "%lldmcs", mics);
		}
	}
	return dest;
}

node& node::operator=(const node& other){
	tok1 = other.tok1;
	tok2 = other.tok2;
	tok3 = other.tok3;
	tok4 = other.tok4;
	tok5 = other.tok5;
	label = other.label;
	phase = other.phase;
	datatype = other.datatype;
	keep = other.keep;
	return *this;
}
node::node(){
	tok1 = tok2 = tok3 = tok4 = tok5 = token{};
	label = phase = datatype = 0;
	keep = false;
}
node::node(int l){
	tok1 = tok2 = tok3 = tok4 = tok5 = token{};
	label = l;
	phase = datatype = 0;
	keep = false;
}
node::~node(){
}

unique_ptr<node>& findFirstNode(unique_ptr<node> &n, int label){
	if (n == nullptr || n->label == label)
		return n;
	return findFirstNode(n->node1, label)
		?: findFirstNode(n->node2, label)
		?: findFirstNode(n->node3, label)
		?: findFirstNode(n->node4, label);
}

string handle_err(exception_ptr eptr) {
    try {
		rethrow_exception(eptr);
    } catch(const std::exception& e) {
        return  e.what();
    }
}

bool isTrivialColumn(unique_ptr<node> &n){
	if (n == nullptr) return false;
	if (n->label == N_VALUE && n->tok3.id)
		return true;
	return isTrivialColumn(n->node1);
}
bool isTrivialAlias(unique_ptr<node> &n){
	if (n == nullptr) return false;
	if (n->label == N_VALUE && n->tok4.id)
		return true;
	return isTrivialAlias(n->node1);
}
string nodeName(unique_ptr<node> &n, querySpecs* q){
	if (n == nullptr) return "";
	if (n->label == N_VALUE){
		if (n->tok2.id == COLUMN && n->tok3.id){
			if (!q->filemap[n->tok3.val]->noheader)
			return q->filemap[n->tok3.val]->colnames[n->tok1.id];
		}
		if (n->tok2.id == VARIABLE)
			return n->tok1.val;
		return "";
	}
	return nodeName(n->node1, q);
}

//nlohmann library can't handle invalid utf-8
stringstream& singleQueryResult::tojson(){
	static string_view com = ",";
	static string_view nocom = "";
	j << "{\"Numrows\":" << numrows
		<< ",\"ShowLimit\":" << showLimit
		<< ",\"Numcols\":" << numcols;
	auto delim = &nocom;
	j << ",\"Types\":[";
	for (auto v : types){
		j << *delim << v;
		delim = &com;
	}
	j << "],\"Colnames\":[";
	delim = &nocom;
	for (auto &v : colnames){
		j << *delim << '"' << escapeJSON(v) << '"';
		delim = &com;
	}
	j << "],\"Pos\":[";
	delim = &nocom;
	for (auto v : pos){
		j << *delim << v;
		delim = &com;
	}
	j << "],\"Vals\":[";
	delim = &nocom;
	for (auto &v : Vals){
		j << *delim << v;
		delim = &com;
	}
	j << "],\"Status\":" << status
		<< ",\"Query\":\"" << escapeJSON(query) << "\"}";
	return j;
}
stringstream& returnData::tojson(){
	static string_view com = ",";
	static string_view nocom = "";
 	j	<< "{\"Entries\":[";
	auto delim = &nocom;
	for (auto &v : entries){
		j << *delim << v->tojson().rdbuf();
		delim = &com;
	}
	j << "],\"Status\":" << status
		<< ",\"OriginalQuery\":\"" << escapeJSON(originalQuery)
		<< "\",\"Clipped\":" << (clipped ? "true":"false")
		<< ",\"Message\":\"" << escapeJSON(message) << "\"}";
	return j;
}

string gethome(){
	auto h = getenv("HOME");
	if (h) return string(h);
	h = getenv("USERPROFILE");
	if (h) return string(h);
	h = getenv("HOMEDRIVE");
	auto p = getenv("HOMEPATH");
	if (h && p) return st(h,p);
	return "";
}

void prepareQuery(querySpecs &q){
	scanTokens(q);
	parseQuery(q);
	openfiles(q, q.tree);
	q.promptPassword();
	applyTypes(q);
	analyzeTree(q);
	printTree(q.tree, 0);
	codeGen(q);
};
