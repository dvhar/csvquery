#include "interpretor.h"
#include "vmachine.h"

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
	{"desc" ,      KW_ORDHOW},
	{"between" ,   KW_BETWEEN},
	{"like" ,      KW_LIKE},
	{"case" ,      KW_CASE},
	{"when" ,      KW_WHEN},
	{"then" ,      KW_THEN},
	{"else" ,      KW_ELSE},
	{"end" ,       KW_END},
	{"in" ,        KW_IN},
	{"group" ,     KW_GROUP},
	{"not" ,       SP_NEGATE},
	{"is" ,        SP_EQ}
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

//map func tokens to opcodes. funcs with variadic type are handled by normal operations array
flatmap<int, int> functionCode = {
	{FN_YEAR, FUNCYEAR},
	{FN_MONTH, FUNCMONTH},
	{FN_MONTHNAME, FUNCMONTHNAME},
	{FN_WEEK, FUNCWEEK},
	{FN_WDAYNAME, FUNCWDAYNAME},
	{FN_YDAY, FUNCYDAY},
	{FN_MDAY, FUNCMDAY},
	{FN_WDAY, FUNCWDAY},
	{FN_HOUR, FUNCHOUR},
	{FN_MINUTE, FUNCMINUTE},
	{FN_SECOND, FUNCSECOND},
	{FN_CEIL, FUNC_CIEL},
	{FN_FLOOR, FUNC_FLOOR},
	{FN_ACOS, FUNC_ACOS},
	{FN_ASIN, FUNC_ASIN},
	{FN_ATAN, FUNC_ATAN},
	{FN_COS, FUNC_COS},
	{FN_SIN, FUNC_SIN},
	{FN_TAN, FUNC_TAN},
	{FN_EXP, FUNC_EXP},
	{FN_LOG, FUNC_LOG},
	{FN_LOG2, FUNC_LOG2},
	{FN_LOG10, FUNC_LOG10},
	{FN_SQRT, FUNC_SQRT},
	{FN_RAND, FUNC_RAND},
	{FN_UPPER, FUNC_UPPER},
	{FN_LOWER, FUNC_LOWER},
	{FN_BASE64_ENCODE, FUNC_BASE64_ENCODE},
	{FN_BASE64_DECODE, FUNC_BASE64_DECODE},
	{FN_HEX_ENCODE, FUNC_HEX_ENCODE},
	{FN_HEX_DECODE, FUNC_HEX_DECODE},
	{FN_LEN, FUNC_LEN},
	{FN_SUBSTR, FUNC_SUBSTR},
	{FN_MD5, FUNC_MD5},
	{FN_SHA1, FUNC_SHA1},
	{FN_SHA256, FUNC_SHA256},
	{FN_SIP, FUNC_SIP},
	{FN_ROUND, FUNC_ROUND},
	{FN_CBRT, FUNC_CBRT},
	{FN_NOW, FUNC_NOW},
	{FN_NOWGM, FUNC_NOWGM},
	{FN_FORMAT, FUNC_FORMAT},
	//make pow use N_MULT
};

//map for printing opcodes
flatmap<int, string_view> opMap = {
	{CVER,"CVER"},
	{CVNO,"CVNO"},
	{CVIF,"CVIF"},
	{CVIS,"CVIS"},
	{CVFI,"CVFI"},
	{CVFS,"CVFS"},
	{CVDRS,"CVDRS"},
	{CVDTS,"CVDTS"},
	{CVSI,"CVSI"},
	{CVSF,"CVSF"},
	{CVSDR,"CVSDR"},
	{CVSDT,"CVSDT"},
	{IADD,"IADD"},
	{FADD,"FADD"},
	{TADD,"TADD"},
	{DTADD,"DTADD"},
	{DRADD,"DRADD"},
	{ISUB,"ISUB"},
	{FSUB,"FSUB"},
	{DTSUB,"DTSUB"},
	{DRSUB,"DRSUB"},
	{IMULT,"IMULT"},
	{FMULT,"FMULT"},
	{DRMULT,"DRMULT"},
	{IDIV,"IDIV"},
	{FDIV,"FDIV"},
	{DRDIV,"DRDIV"},
	{INEG,"INEG"},
	{FNEG,"FNEG"},
	{PNEG,"PNEG"},
	{IMOD,"IMOD"},
	{FMOD,"FMOD"},
	{IPOW,"IPOW"},
	{FPOW,"FPOW"},
	{JMP,"JMP"},
	{JMPCNT,"JMPCNT"},
	{JMPTRUE,"JMPTRUE"},
	{JMPFALSE,"JMPFALSE"},
	{JMPNOTNULL_ELSEPOP,"JMPNOTNULL_ELSEPOP"},
	{RDLINE,"RDLINE"},
	{RDLINE_ORDERED,"RDLINE_ORDERED"},
	{PREP_REREAD,"PREP_REREAD"},
	{PUT,"PUT"},
	{LDPUT,"LDPUT"},
	{LDPUTALL,"LDPUTALL"},
	{PUTVAR,"PUTVAR"},
	{PUTVAR2,"PUTVAR2"},
	{LDINT,"LDINT"},
	{LDFLOAT,"LDFLOAT"},
	{LDTEXT,"LDTEXT"},
	{LDDATE,"LDDATE"},
	{LDDUR,"LDDUR"},
	{LDLIT,"LDLIT"},
	{LDVAR,"LDVAR"},
	{HOLDVAR,"HOLDVAR"},
	{IEQ,"IEQ"},
	{FEQ,"FEQ"},
	{TEQ,"TEQ"},
	{LIKE,"LIKE"},
	{ILEQ,"ILEQ"},
	{FLEQ,"FLEQ"},
	{TLEQ,"TLEQ"},
	{ILT,"ILT"},
	{FLT,"FLT"},
	{TLT,"TLT"},
	{PRINTJSON,"PRINTJSON"},
	{PRINTCSV,"PRINTCSV"},
	{PUSH,"PUSH"},
	{PUSH_N,"PUSH_N"},
	{POP,"POP"},
	{POPCPY,"POPCPY"},
	{ENDRUN,"ENDRUN"},
	{NULFALSE,"NULFALSE"},
	{DIST,"DIST"},
	{PUTDIST,"PUTDIST"},
	{LDDIST,"LDDIST"},
	{FINC,"FINC"},
	{ENCCHA,"ENCCHA"},
	{DECCHA,"DECCHA"},
	{SAVESORTN,"SAVESORTN"},
	{SAVESORTS,"SAVESORTS"},
	{SAVEVALPOS,"SAVEVALPOS"},
	{SAVEPOS,"SAVEPOS"},
	{SORT,"SORT"},
	{GETGROUP,"GETGROUP"},
	{ONEGROUP,"ONEGROUP"},
	{SUMI,"SUMI"},
	{SUMF,"SUMF"},
	{AVGI,"AVGI"},
	{AVGF,"AVGF"},
	{STDVI,"STDVI"},
	{STDVF,"STDVF"},
	{COUNT,"COUNT"},
	{MINI,"MINI"},
	{MINF,"MINF"},
	{MINS,"MINS"},
	{MAXI,"MAXI"},
	{MAXF,"MAXF"},
	{MAXS,"MAXS"},
	{NEXTMAP,"NEXTMAP"},
	{NEXTVEC,"NEXTVEC"},
	{ROOTMAP,"ROOTMAP"},
	{LDMID,"LDMID"},
	{LDPUTMID,"LDPUTMID"},
	{LDPUTGRP,"LDPUTGRP"},
	{LDSTDVI,"LDSTDVI"},
	{LDSTDVF,"LDSTDVF"},
	{LDAVGI,"LDAVGI"},
	{LDAVGF,"LDAVGF"},
	{ADD_GROUPSORT_ROW,"ADD_GROUPSORT_ROW"},
	{FREEMIDROW,"FREEMIDROW"},
	{GSORT,"GSORT"},
	{READ_NEXT_GROUP,"READ_NEXT_GROUP"},
	{NUL_TO_STR,"NUL_TO_STR"},
	{SORTVALPOS,"SORTVALPOS"},
	{JOINSET_EQ,"JOINSET_EQ"},
	{JOINSET_LESS,"JOINSET_LESS"},
	{JOINSET_GRT,"JOINSET_GRT"},
	{JOINSET_INIT,"JOINSET_INIT"},
	{JOINSET_TRAV,"JOINSET_TRAV"},
	{AND_SET,"AND_SET"},
	{OR_SET,"OR_SET"},
	{SAVEANDCHAIN,"SAVEANDCHAIN"},
	{JOINSET_EQ_AND,"JOINSET_EQ_AND"},
	{SORT_ANDCHAIN,"SORT_ANDCHAIN"},
	{FUNCYEAR,"FUNCYEAR"},
	{FUNCMONTH,"FUNCMONTH"},
	{FUNCWEEK,"FUNCWEEK"},
	{FUNCYDAY,"FUNCYDAY"},
	{FUNCMDAY,"FUNCMDAY"},
	{FUNCWDAY,"FUNCWDAY"},
	{FUNCHOUR,"FUNCHOUR"},
	{FUNCMINUTE,"FUNCMINUTE"},
	{FUNCSECOND,"FUNCSECOND"},
	{FUNCWDAYNAME,"FUNCWDAYNAME"},
	{FUNCMONTHNAME,"FUNCMONTHNAME"},
	{JOINSET_LESS_AND,"JOINSET_LESS_AND"},
	{JOINSET_GRT_AND,"JOINSET_GRT_AND"},
	{FUNCABSF,"FUNCABSF"},
	{FUNCABSI,"FUNCABSI"},
	{START_MESSAGE,"START_MESSAGE"},
	{STOP_MESSAGE,"STOP_MESSAGE"},
	{FUNC_CIEL,"FUNC_CIEL"},
	{FUNC_FLOOR,"FUNC_FLOOR"},
	{FUNC_ACOS,"FUNC_ACOS"},
	{FUNC_ASIN,"FUNC_ASIN"},
	{FUNC_ATAN,"FUNC_ATAN"},
	{FUNC_COS,"FUNC_COS"},
	{FUNC_SIN,"FUNC_SIN"},
	{FUNC_TAN,"FUNC_TAN"},
	{FUNC_EXP,"FUNC_EXP"},
	{FUNC_LOG,"FUNC_LOG"},
	{FUNC_LOG2,"FUNC_LOG2"},
	{FUNC_LOG10,"FUNC_LOG10"},
	{FUNC_SQRT,"FUNC_SQRT"},
	{FUNC_RAND,"FUNC_RAND"},
	{FUNC_UPPER,"FUNC_UPPER"},
	{FUNC_LOWER,"FUNC_LOWER"},
	{FUNC_BASE64_ENCODE,"FUNC_BASE64_ENCODE"},
	{FUNC_BASE64_DECODE,"FUNC_BASE64_DECODE"},
	{FUNC_HEX_ENCODE,"FUNC_HEX_ENCODE"},
	{FUNC_HEX_DECODE,"FUNC_HEX_DECODE"},
	{FUNC_LEN,"FUNC_LEN"},
	{FUNC_SUBSTR,"FUNC_SUBSTR"},
	{FUNC_MD5,"FUNC_MD5"},
	{FUNC_SHA1,"FUNC_SHA1"},
	{FUNC_SHA256,"FUNC_SHA256"},
	{FUNC_ROUND,"FUNC_ROUND"},
	{FUNC_CBRT,"FUNC_CBRT"},
	{FUNC_NOW,"FUNC_NOW"},
	{FUNC_NOWGM,"FUNC_NOWGM"},
	{PRINTCSV_HEADER,"PRINTCSV_HEADER"},
	{FUNC_FORMAT,"FUNC_FORMAT"},
	{LDCOUNT,"LDCOUNT"},
	{BETWEEN,"BETWEEN"},
	{PRINTBOX,"PRINTBOX"},
	{PRINTBTREE,"PRINTBTREE"},
	{INSUBQUERY,"INSUBQUERY"},
	{FUNC_SIP,"FUNC_SIP"}
};

// (1) csv need quoting: , \n
// (2) csv need escaping: "
// (4) json need escaping: \ " \b \f \n \r \t
char dat::abnormal[] = {0,0,0,0,0,0,0,0,4,4,1|4,0,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2|4,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//datunion comparers - dunno why but fastest with first param value and second reference
const function<bool (const datunion, const datunion&)> vmachine::uLessFuncs[3] = {
	[](const datunion u, const datunion &d){ return u.i < d.i; },
	[](const datunion u, const datunion &d){ return u.f < d.f; },
	[](const datunion u, const datunion &d){ return strcmp(u.s, d.s)<0; },
};
const function<bool (const datunion, const datunion&)> vmachine::uGrtFuncs[3] = {
	[](const datunion u, const datunion &d){ return u.i > d.i; },
	[](const datunion u, const datunion &d){ return u.f > d.f; },
	[](const datunion u, const datunion &d){ return strcmp(u.s, d.s)>0; },
};
const function<bool (const datunion, const datunion&)> vmachine::uLessEqFuncs[3] = {
	[](const datunion u, const datunion &d){ return u.i <= d.i; },
	[](const datunion u, const datunion &d){ return u.f <= d.f; },
	[](const datunion u, const datunion &d){ return strcmp(u.s, d.s)<=0; },
};
const function<bool (const datunion, const datunion&)> vmachine::uGrtEqFuncs[3] = {
	[](const datunion u, const datunion &d){ return u.i >= d.i; },
	[](const datunion u, const datunion &d){ return u.f >= d.f; },
	[](const datunion u, const datunion &d){ return strcmp(u.s, d.s)>=0; },
};
const function<bool (const datunion, const datunion&)> vmachine::uEqFuncs[3] = {
	[](const datunion u, const datunion &d){ return u.i == d.i; },
	[](const datunion u, const datunion &d){ return u.f == d.f; },
	[](const datunion u, const datunion &d){ return !strcmp(u.s, d.s); },
};
const function<bool (const datunion, const datunion&)> vmachine::uNeqFuncs[3] = {
	[](const datunion u, const datunion &d){ return u.i != d.i; },
	[](const datunion u, const datunion &d){ return u.f != d.f; },
	[](const datunion u, const datunion &d){ return strcmp(u.s, d.s); },
};
const function<bool (const datunion, const datunion&)> vmachine::uRxpFuncs[3] = {
	[](const datunion u, const datunion &d){ return false; }, //unused
	[](const datunion u, const datunion &d){ return false; }, //unused
	[](const datunion u, const datunion &d){ return !regexec(d.r, u.s, 0,0,0); },
};
const function<bool (const datunion, const datunion&)>* vmachine::uComparers[7] = {
	uLessFuncs, uGrtFuncs, uLessEqFuncs, uGrtEqFuncs, uEqFuncs, uNeqFuncs, uRxpFuncs
};
flatmap<int,int> vmachine::relopIdx = {
	{SP_LESS,0},{SP_GREAT,1},{SP_LESSEQ,2},{SP_GREATEQ,3},{SP_EQ,4},{SP_NOEQ,5},{KW_LIKE,6}
};
//return difference for sort comparers
function<i64 (const datunion,const datunion)> datunionDiffs[6] = {
	[](const auto a, const auto b) { return a.i - b.i; },
	[](const auto a, const auto b) { return a.f - b.f; },
	[](const auto a, const auto b) { return strcmp(a.s, b.s); },
	[](const auto a, const auto b) { return b.i - a.i; },
	[](const auto a, const auto b) { return b.f - a.f; },
	[](const auto a, const auto b) { return strcmp(b.s, a.s); },
};

//2d array for ops indexed by operation and datatype. used with operations[][]
int operations[20][6] = {
	{ 0, IADD, FADD, DTADD, DRADD, TADD },
	{ 0, ISUB, FSUB, DTSUB, DRSUB, 0 },
	{ 0, IMULT, FMULT, 0, DRMULT, 0 },
	{ 0, IMOD, FMOD, 0, 0, 0 },
	{ 0, IDIV, FDIV, 0, DRDIV, 0 },
	{ 0, IPOW, FPOW, 0, 0, 0 },
	{ 0, INEG, FNEG, 0, INEG, 0 },
	{ 0, LDINT, LDFLOAT, LDDATE, LDDUR, LDTEXT },
	{ 0, IEQ, FEQ, IEQ, IEQ, TEQ },
	{ 0, ILEQ, FLEQ, ILEQ, ILEQ, TLEQ },
	{ 0, ILT, FLT, ILT, ILT, TLT },
	{ 0, MAXI, MAXF, MAXI, MAXI, MAXS },
	{ 0, MINI, MINF, MINI, MINI, MINS },
	{ 0, SUMI, SUMF, 0, SUMI, 0 },
	{ 0, AVGI, AVGF, AVGI, AVGI, 0 },
	{ 0, STDVI, STDVF, 0, STDVI, 0 },
	{ 0, LDSTDVI, LDSTDVF, 0, LDSTDVI, 0 },
	{ 0, LDAVGI, LDAVGF, LDAVGI, LDAVGI, 0 },
	{ 0, SAVESORTN, SAVESORTN, SAVESORTN, SAVESORTN, SAVESORTS },
	{ 0, FUNCABSI, FUNCABSF, 0, FUNCABSI, 0 },
};

//type conversion opcodes - [from][to]
int typeConv[6][6] = {
	{0, 0,  0,  0,  0,  0 },
	{0, CVNO, CVIF, CVER, CVER, CVIS },
	{0, CVFI, CVNO, CVER, CVER, CVFS },
	{0, CVER, CVER, CVNO, CVER, CVDTS},
	{0, CVER, CVER, CVER, CVNO, CVDRS},
	{0, CVSI, CVSF, CVSDT,CVSDR,CVNO},
};
