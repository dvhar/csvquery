#include "interpretor.h"

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
	{N_SETLIST,    "N_SETLIST"},
	{N_JOINCHAIN,  "N_JOINCHAIN"},
	{N_JOIN,       "N_JOIN"},
	{N_DEXPRESSIONS,"N_DEXPRESSIONS"},
	{N_WITH,       "N_WITH"},
	{N_VARS,       "N_VARS"},
	{N_TYPECONV,   "N_TYPECONV"},
	{N_FILE,       "N_FILE"},
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
	{"sip",          FN_SIP},
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

// https://www.tutorialgateway.org/sql-date-format/
const flatmap<int, const char*> dateFmtCodes = {
	{ 1  , "%m/%d/%y"}, // mm/dd/yy
	{ 101, "%m/%d/%Y"}, // mm/dd/yyyy
	{ 2  , "%y.%m.%d"}, // yy.mm.dd
	{ 102, "%Y.%m.%d"}, // yyyy.mm.dd
	{ 3  , "%d/%m/%y"}, // dd/mm/yy
	{ 103, "%d/%m/%Y"}, // dd/mm/yyyy
	{ 4  , "%d.%m.%y"}, // dd.mm.yy
	{ 104, "%d.%m.%Y"}, // dd.mm.yyyy
	{ 5  , "%d-%m-%y"}, // dd-mm-yy
	{ 105, "%d-%m-%Y"}, // dd-mm-yyyy
	{ 6  , "%d %b %y"}, // dd mon yy
	{ 106, "%d %b %Y"}, // dd mon yyyy
	{ 7  , "%b %d, %y"}, // Mon dd, yy
	{ 107, "%b %d, %Y"}, // Mon dd, yyyy
	{ 8  , "%I:%M:%S"}, // hh:mi:ss
	{ 108, "%I:%M:%S"}, // hh:mi:ss
	{ 9  , "%b %d %y %I:%M:%S:mmmm%p"}, // mon dd yy hh:mi:ss:mmmmAM (or PM)
	{ 109, "%b %d %Y %I:%M:%S:mmmm%p"}, // mon dd yyyy hh:mi:ss:mmmmAM (or PM)
	{ 10 , "%m-%d-%y"}, // mm-dd-yy
	{ 110, "%m-%d-%Y"}, // mm-dd-yyyy
	{ 11 , "%y/%m/%d"}, // yy/mm/dd
	{ 111, "%Y/%m/%d"}, // yyyy/mm/dd
	{ 12 , "%y%m%d"}, // yymmdd
	{ 112, "%Y%m%d"}, // yyyymmdd
	{ 13 , "%d %b %y %H:%M:%S:mmm"}, // dd mon yy hh:mi:ss:mmm(24h)
	{ 113, "%d %b %Y %H:%M:%S:mmm"}, // dd mon yyyy hh:mi:ss:mmm(24h)
	{ 14 , "%H:%M:%S:mmm"}, //  hh:mi:ss:mmm(24h)
	{ 114, "%H:%M:%S:mmm"}, //  hh:mi:ss:mmm(24h)
	{ 15 , "%y-%m-%d"}, // yy-mm-dd
	{ 115, "%Y-%m-%d"}, // yyyy-mm-dd
	{ 20 , "%y-%m-%d %H:%M:%S"}, //  yy-mm-dd hh:mi:ss(24h)
	{ 120, "%Y-%m-%d %H:%M:%S"}, //  yyyy-mm-dd hh:mi:ss(24h)
	{ 21 , "%y-%m-%d %H:%M:%S.mmm"}, // yy-mm-dd hh:mi:ss.mmm(24h)
	{ 121, "%Y-%m-%d %H:%M:%S.mmm"}, // yyyy-mm-dd hh:mi:ss.mmm(24h)
	{ 126, "%Y-%m-%dT%H:%M:%S.mmm"}, // yyyy-mm-ddThh:mi:ss.mmm (no Spaces)
	{ 127, "%Y-%m-%dT%H:%M:%S.mmmZ"}, // yyyy-mm-ddThh:mi:ss.mmmZ (no Spaces)
	{ 130, "%d %b %Y %I:%M:%S:mmm%p"}, // dd mon yyyy hh:mi:ss:mmmAM
	{ 131, "%d/%b/%Y %I:%M:%S:mmm%p"}, // dd/mm/yyyy hh:mi:ss:mmmAM
	//non standard  additions
	{ 119, "%b %d %Y %I:%M:%S%p"}, // mon dd yyyy hh:mi:ssAM (or PM)
	{ 140, "%d %b %Y %I:%M:%S%p"}, // dd mon yyyy hh:mi:ssAM
	{ 141, "%d/%b/%Y %I:%M:%S%p"}, // dd/mm/yyyy hh:mi:ssAM
	{ 136, "%Y-%m-%dT%H:%M:%S"}, // yyyy-mm-ddThh:mi:ss (no Spaces)
	{ 137, "%Y-%m-%dT%H:%M:%SZ"}, // yyyy-mm-ddThh:mi:ssZ (no Spaces)
};
