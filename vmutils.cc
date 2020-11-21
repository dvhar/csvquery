#include "interpretor.h"
#include "vmachine.h"
#include "server.h"
#include "deps/crypto/sha1.h"
#include "deps/crypto/sha256.h"
#include "deps/crypto/md5.h"
#include "deps/crypto/siphash.h"
//#include "deps/crypto/aes.h"
#include "deps/json/escape.h"
#include <chrono>

//map for printing opcodes
flatmap<int, string_view> opMap = {
	{CVER,"CVER"}, {CVNO,"CVNO"}, {CVIF,"CVIF"}, {CVIS,"CVIS"}, {CVFI,"CVFI"}, {CVFS,"CVFS"}, {CVDRS,"CVDRS"}, {CVDTS,"CVDTS"}, {CVSI,"CVSI"}, {CVSF,"CVSF"}, {CVSDR,"CVSDR"}, {CVSDT,"CVSDT"}, {IADD,"IADD"}, {FADD,"FADD"}, {TADD,"TADD"}, {DTADD,"DTADD"}, {DRADD,"DRADD"}, {ISUB,"ISUB"}, {FSUB,"FSUB"}, {DTSUB,"DTSUB"}, {DRSUB,"DRSUB"}, {IMULT,"IMULT"}, {FMULT,"FMULT"}, {DRMULT,"DRMULT"}, {IDIV,"IDIV"}, {FDIV,"FDIV"}, {DRDIV,"DRDIV"}, {INEG,"INEG"}, {FNEG,"FNEG"}, {PNEG,"PNEG"}, {IMOD,"IMOD"}, {FMOD,"FMOD"}, {IPOW,"IPOW"}, {FPOW,"FPOW"}, {JMP,"JMP"}, {JMPCNT,"JMPCNT"}, {JMPTRUE,"JMPTRUE"}, {JMPFALSE,"JMPFALSE"}, {JMPNOTNULL_ELSEPOP,"JMPNOTNULL_ELSEPOP"}, {RDLINE,"RDLINE"}, {RDLINE_ORDERED,"RDLINE_ORDERED"}, {PREP_REREAD,"PREP_REREAD"}, {PUT,"PUT"}, {LDPUT,"LDPUT"}, {LDPUTALL,"LDPUTALL"}, {PUTVAR,"PUTVAR"}, {PUTVAR2,"PUTVAR2"}, {LDINT,"LDINT"}, {LDFLOAT,"LDFLOAT"}, {LDTEXT,"LDTEXT"}, {LDDATE,"LDDATE"}, {LDDUR,"LDDUR"}, {LDLIT,"LDLIT"}, {LDVAR,"LDVAR"}, {HOLDVAR,"HOLDVAR"}, {IEQ,"IEQ"}, {FEQ,"FEQ"}, {TEQ,"TEQ"}, {LIKE,"LIKE"}, {ILEQ,"ILEQ"}, {FLEQ,"FLEQ"}, {TLEQ,"TLEQ"}, {ILT,"ILT"}, {FLT,"FLT"}, {TLT,"TLT"}, {PRINTJSON,"PRINTJSON"}, {PRINTCSV,"PRINTCSV"}, {PUSH,"PUSH"}, {PUSH_N,"PUSH_N"}, {POP,"POP"}, {POPCPY,"POPCPY"}, {ENDRUN,"ENDRUN"}, {NULFALSE1,"NULFALSE1"}, {NULFALSE2,"NULFALSE2"}, {NDIST,"NDIST"}, {SDIST,"SDIST"}, {PUTDIST,"PUTDIST"}, {LDDIST,"LDDIST"}, {FINC,"FINC"}, {ENCCHA,"ENCCHA"}, {DECCHA,"DECCHA"}, {SAVESORTN,"SAVESORTN"}, {SAVESORTS,"SAVESORTS"}, {SAVEVALPOS,"SAVEVALPOS"}, {SAVEPOS,"SAVEPOS"}, {SORT,"SORT"}, {GETGROUP,"GETGROUP"}, {ONEGROUP,"ONEGROUP"}, {SUMI,"SUMI"}, {SUMF,"SUMF"}, {AVGI,"AVGI"}, {AVGF,"AVGF"}, {STDVI,"STDVI"}, {STDVF,"STDVF"}, {COUNT,"COUNT"}, {MINI,"MINI"}, {MINF,"MINF"}, {MINS,"MINS"}, {MAXI,"MAXI"}, {MAXF,"MAXF"}, {MAXS,"MAXS"}, {NEXTMAP,"NEXTMAP"}, {NEXTVEC,"NEXTVEC"}, {ROOTMAP,"ROOTMAP"}, {LDMID,"LDMID"}, {LDPUTMID,"LDPUTMID"}, {LDPUTGRP,"LDPUTGRP"}, {LDSTDVI,"LDSTDVI"}, {LDSTDVF,"LDSTDVF"}, {LDAVGI,"LDAVGI"}, {LDAVGF,"LDAVGF"}, {ADD_GROUPSORT_ROW,"ADD_GROUPSORT_ROW"}, {FREEMIDROW,"FREEMIDROW"}, {GSORT,"GSORT"}, {READ_NEXT_GROUP,"READ_NEXT_GROUP"}, {NUL_TO_STR,"NUL_TO_STR"}, {SORTVALPOS,"SORTVALPOS"}, {JOINSET_EQ,"JOINSET_EQ"}, {JOINSET_LESS,"JOINSET_LESS"}, {JOINSET_GRT,"JOINSET_GRT"}, {JOINSET_INIT,"JOINSET_INIT"}, {JOINSET_TRAV,"JOINSET_TRAV"}, {AND_SET,"AND_SET"}, {OR_SET,"OR_SET"}, {SAVEANDCHAIN,"SAVEANDCHAIN"}, {JOINSET_EQ_AND,"JOINSET_EQ_AND"}, {SORT_ANDCHAIN,"SORT_ANDCHAIN"},{FUNCYEAR,"FUNCYEAR"}, {FUNCMONTH,"FUNCMONTH"}, {FUNCWEEK,"FUNCWEEK"}, {FUNCYDAY,"FUNCYDAY"}, {FUNCMDAY,"FUNCMDAY"}, {FUNCWDAY,"FUNCWDAY"}, {FUNCHOUR,"FUNCHOUR"}, {FUNCMINUTE,"FUNCMINUTE"}, {FUNCSECOND,"FUNCSECOND"}, {FUNCWDAYNAME,"FUNCWDAYNAME"}, {FUNCMONTHNAME,"FUNCMONTHNAME"}, {JOINSET_LESS_AND,"JOINSET_LESS_AND"}, {JOINSET_GRT_AND,"JOINSET_GRT_AND"}, {FUNCABSF,"FUNCABSF"}, {FUNCABSI,"FUNCABSI"}, {START_MESSAGE,"START_MESSAGE"}, {STOP_MESSAGE,"STOP_MESSAGE"}, {FUNC_CIEL,"FUNC_CIEL"}, {FUNC_FLOOR,"FUNC_FLOOR"}, {FUNC_ACOS,"FUNC_ACOS"}, {FUNC_ASIN,"FUNC_ASIN"}, {FUNC_ATAN,"FUNC_ATAN"}, {FUNC_COS,"FUNC_COS"}, {FUNC_SIN,"FUNC_SIN"}, {FUNC_TAN,"FUNC_TAN"}, {FUNC_EXP,"FUNC_EXP"}, {FUNC_LOG,"FUNC_LOG"}, {FUNC_LOG2,"FUNC_LOG2"}, {FUNC_LOG10,"FUNC_LOG10"}, {FUNC_SQRT,"FUNC_SQRT"}, {FUNC_RAND,"FUNC_RAND"}, {FUNC_UPPER,"FUNC_UPPER"}, {FUNC_LOWER,"FUNC_LOWER"}, {FUNC_BASE64_ENCODE,"FUNC_BASE64_ENCODE"}, {FUNC_BASE64_DECODE,"FUNC_BASE64_DECODE"}, {FUNC_HEX_ENCODE,"FUNC_HEX_ENCODE"}, {FUNC_HEX_DECODE,"FUNC_HEX_DECODE"}, {FUNC_LEN,"FUNC_LEN"}, {FUNC_SUBSTR,"FUNC_SUBSTR"}, {FUNC_MD5,"FUNC_MD5"}, {FUNC_SHA1,"FUNC_SHA1"}, {FUNC_SHA256,"FUNC_SHA256"}, {FUNC_ROUND,"FUNC_ROUND"}, {FUNC_CBRT,"FUNC_CBRT"}, {FUNC_NOW,"FUNC_NOW"}, {FUNC_NOWGM,"FUNC_NOWGM"}, {PRINTCSV_HEADER,"PRINTCSV_HEADER"}, {FUNC_FORMAT,"FUNC_FORMAT"}, {LDCOUNT,"LDCOUNT"}
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
	{FN_ROUND, FUNC_ROUND},
	{FN_CBRT, FUNC_CBRT},
	{FN_NOW, FUNC_NOW},
	{FN_NOWGM, FUNC_NOWGM},
	{FN_FORMAT, FUNC_FORMAT},
	//make pow use N_MULT
};

void opcode::print(){
	perr((ft("code: %-18s  [%-2d  %-2d  %-2d]\n")% opMap[code]% p1% p2% p3).str());
}
// (1) csv need quoting: , \n
// (2) csv need escaping: "
// (4) json need escaping: \ " \b \f \n \r \t
char dat::abnormal[] = {0,0,0,0,0,0,0,0,4,4,1|4,0,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2|4,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
void dat::appendToJsonBuffer(string &outbuf){

	static char buf[40];
	char a = 0;
	outbuf += '"';
	switch ( b & 7 ) {
	case T_INT:
		sprintf(buf,"%lld",u.i);
		outbuf += buf;
		break;
	case T_FLOAT:
		sprintf(buf,"%.10g",u.f);
		outbuf += buf;
		break;
	case T_DATE:
		outbuf += datestring(u.i);
		 break;
	case T_DURATION:
		 outbuf += durstring(u.i, nullptr);
		 break;
	case T_STRING:
		for (auto c = (unsigned char*)u.s; *c; c++) a |= abnormal[*c];
		if (a & 4) {
			outbuf += escapeJSON(string_view(u.s));
		} else {
			outbuf += u.s;
		}
		break;
	}
	outbuf += '"';
}
void dat::appendToCsvBuffer(string &outbuf){
	if (b == 0) return;
	static char buf[40];
	char a = 0;
	switch ( b & 7 ) {
	case T_INT:
		sprintf(buf,"%lld",u.i);
		outbuf += buf;
		return;
	case T_FLOAT:
		sprintf(buf,"%.10g",u.f);
		outbuf += buf;
		return;
	case T_DATE:
		outbuf += datestring(u.i);
		return;
	case T_DURATION:
		outbuf += durstring(u.i, nullptr);
		return;
	case T_STRING:
		for (auto c = (unsigned char*)u.s; *c; c++) a |= abnormal[*c];
		if (!(a&3)){
			outbuf += u.s;
		} else if (a & 2) {
			outbuf += '"';
			char* s = u.s;
			auto q = strchr(s, '"');
			do {
				outbuf += string_view(s, q-s);
				outbuf += '"';
				s = q;
			} while (q = strchr(s+1, '"'));
			outbuf += s;
			outbuf += '"';
		} else {
			outbuf += '"';
			outbuf += u.s;
			outbuf += '"';
		}
		return;
	}
}

dat parseIntDat(const char* s){
	char* end = NULL;
	dat idat = { { .i = strtol(s, &end, 10) }, T_INT };
	if (*end != 0)
		error("Could not parse ", s, " as a number.");
	return idat;
}
dat parseFloatDat(const char* s){
	char* end = NULL;
	dat fdat = { { .f = strtod(s, &end) }, T_FLOAT };
	if (*end != 0)
		error("Could not parse ", s, " as a number.");
	return fdat;
}
dat parseDurationDat(const char* s) {
	date_t dur;
	if (parseDuration((char*)s, &dur))
		error("Could not parse ", s, " as duration.");
	if (dur < 0) dur *= -1;
	dat ddat = { { .i = dur }, T_DURATION };
	return ddat;
}
dat parseDateDat(const char* s) {
	date_t date;
	if (dateparse_2(s, &date))
		error("Could not parse ", s, " as date.");
	dat ddat = { { .i = date }, T_DATE };
	return ddat;
}
dat parseStringDat(const char* s) {
	//may want to malloc
	dat ddat = { { .s = (char*)s }, T_STRING, static_cast<u32>(strlen(s)) };
	return ddat;
}
int addBtree(int type, querySpecs *q){
	//returns index of btree
	switch (type){
	case T_INT:
	case T_DATE:
	case T_DURATION:
	case T_FLOAT:
		return q->btn++;
	case T_STRING:
		return q->bts++;
	default:
		error("invalid btree type");
	}
	return 0;
}

void vmachine::endQuery() {
	for (auto& op : q->bytecode){
		if (opDoesJump(op.code))
			op.p1 = q->bytecode.size()-1;
	}
}
vmachine::vmachine(querySpecs &qs) :
	csvOutput(0),
	q(&qs),
	id(idCounter++),
	sessionId(qs.sessionId),
	files(move(qs.filevec)),
	torowSize(qs.colspec.count),
	ops(qs.bytecode.data()),
	quantityLimit(qs.quantityLimit),
	stacktop(stack.data()),
	stackbot(stack.data()),
	distinctVal{0}
{
	updates.sessionId = sessionId;
	destrow.resize(q->colspec.count, {0});
	torow = destrow.data();
	if (q->grouping == 1){
		onegroup = vector<dat>(q->midcount, {0});
		torow = onegroup.data();
	}
	if (q->grouping)
		groupTree.reset(new rowgroup());
	if (q->sorting){
		if (q->grouping)
			sortgroupsize = q->colspec.count + q->sortcount;
		else
			normalSortVals.resize(q->sortcount);
	}
	stack.fill(dat{0});
	bt_nums.resize(q->btn);
	bt_strings.resize(q->bts);
	if (q->outputjson){
		jsonresult.reset(new singleQueryResult(qs));
	}
	if (q->outputcsv){
		if (!q->savepath.empty()){
			outfile.open(qs.savepath, ofstream::out | ofstream::app);
			csvOutput.rdbuf(outfile.rdbuf());
		} else {
			csvOutput.rdbuf(cout.rdbuf());
		}
	}
}

vmachine::~vmachine(){
	if (runmode == RUN_SINGLE) //skip garbage collection if one-off query
		return;
	distinctVal.freedat();
	for (auto &d : stack)   d.freedat();
	for (auto &d : destrow) d.freedat();
	for (auto &btree : bt_strings)
		for (auto &tcs : btree)
			free(tcs.s); //treeCString always allocated with c style
	int i = 0;
	for (auto &vec : normalSortVals)
		if (q->sortInfo[i++].second == T_STRING)
			for (auto u : vec)
				free(u.s); //c strings allways allocated with c style
}
querySpecs::~querySpecs(){
	for (auto &d : dataholder){
		d.freedat();
		if (d.b & RMAL){
			regfree(d.u.r);
			delete d.u.r; //regex always allocated with 'new'
		}
	}
}

varScoper* varScoper::setscope(int f, int s, int f2){
	scopefilter = f;
	scope = s;
	fileno = f2;
	return this;
}
varScoper* varScoper::setscope(int f, int s){
	scopefilter = f;
	scope = s;
	fileno = 0;
	return this;
}
bool varScoper::neededHere(int index, int varfilter, int havefile){
	int match = varfilter & scopefilter;
	if (scopefilter & (JSCAN_FILTER|JCOMP_FILTER)){
		if (fileno > havefile)
			match = 0;
	}
	return match && checkDuplicates(index);
}
bool varScoper::checkDuplicates(int index){
	if (duplicates.count(scope) && duplicates[scope].count(index)){
		return false;
	}
	duplicates[scope][index] = 1;
	return true;
}

//add s2 to s1
void strplus(dat &s1, dat &s2){
	if (s1.isnull()) { s1 = s2; s2.disown(); return; }
	if (s2.isnull()) return;
	int newlen = s1.z+s2.z+1;
	if (s1.ismal()){
		s1.u.s = (char*) realloc(s1.u.s, newlen);
		strcat(s1.u.s+s1.z-1, s2.u.s);
	} else {
		char* ns = (char*) malloc(newlen);
		strcpy(ns, s1.u.s);
		strcat(ns+s1.z-1, s2.u.s);
		s1.u.s = ns;
	}
	s2.freedat();
	s1.b |= MAL;
	s1.z = newlen;
}
int crypter::newChacha(string pass){
	ctxs.emplace_back();
	chactx& ch = ctxs.back();
	for (u32 i=0; i<sizeof(ch.key); i++)
		ch.key[i] = pass[i%pass.length()];
	memset(&ch.nonce, 0, sizeof(ch.nonce));
	return ctxs.size()-1;
}
u32 uniqueNonce32(){
	static bset<u32> nonces;
	u32 n = rng(), i = 0;
	while (!nonces.insert(n).second){
		if (++i > 1000000)
			error("Not enough unique nonces. Use a different security solution.");
		++n;
	}
	return n;
}
static const u32 noncesize = sizeof(u32);
static const u32 macsize = sizeof(u32);
void crypter::chachaEncrypt(dat& d, int i){
	auto ciphlen = d.z+1+macsize;
	auto ch = ctxs.data()+i;
	auto rawResult = (uint8_t*) alloca(ciphlen+noncesize);
	memcpy(rawResult+noncesize+macsize, d.u.s, d.z+1);
	auto nonce = (u32*)ch->nonce;
	auto rnonce = (u32*)rawResult;
	*rnonce = *nonce = uniqueNonce32();
	memcpy(&ch->ctx.key, ch->key, sizeof(ch->ctx.key));
	chacha20_init_context(&ch->ctx, ch->key, ch->nonce, 1); //find out what counter param does
	getmac(d.u.s, d.z, (char*)ch->nonce, (char*)ch->key, sizeof(ch->key), rawResult+noncesize);
	chacha20_xor(&ch->ctx, rawResult+noncesize, ciphlen);
	u32 finalSize = encsize(ciphlen+noncesize);
	auto finalResult = (char*) malloc(finalSize+1);
	base64_encode((BYTE*)rawResult, (BYTE*)finalResult, ciphlen+noncesize, 0);
	finalResult[finalSize]=0;
	d.freedat();
	d = dat{ {s: finalResult}, T_STRING|MAL, finalSize };
}
void crypter::chachaDecrypt(dat& d, int i){
	auto len = d.z+1;
	auto ch = ctxs.data()+i;
	auto rawResult = (char*) alloca(len);
	//TODO: get exact unpadded decode size
	u32 decodesize = base64_decode((BYTE*)d.u.s, (BYTE*)rawResult, len);
	u32 capsize = decodesize - noncesize - macsize;
	auto nonce = (u32*)ch->nonce;
	auto rnonce = (u32*)rawResult;
	*nonce = *rnonce;
	memcpy(&ch->ctx.key, ch->key, sizeof(ch->ctx.key));
	chacha20_init_context(&ch->ctx, ch->key, ch->nonce, 1); //find out what counter param does
	chacha20_xor(&ch->ctx, (uint8_t*) rawResult+noncesize, decodesize - noncesize);
	uint8_t mac[macsize];
	u32 finalsize = strnlen(rawResult+macsize+noncesize, capsize);
	getmac(rawResult+noncesize+macsize, finalsize, rawResult, (char*)ch->key, sizeof(ch->key), mac);
	d.freedat();
	if (*(u32*) mac == *(u32*)(rawResult+noncesize)){
		auto finalResult = (char*) malloc(finalsize+1);
		memcpy(finalResult, rawResult+noncesize+macsize, finalsize);
		finalResult[finalsize]=0;
		d = dat{ {s: finalResult}, T_STRING|MAL, finalsize };
	}
}
void sha1(dat& d){
	SHA1_CTX ctx;
	sha1_init(&ctx);
	sha1_update(&ctx, (BYTE*)d.u.s, d.z);
	d.freedat();
	BYTE rawhash[20];
	sha1_final(&ctx, rawhash);
	d.z = encsize(20);
	d.u.s = (char*) malloc(d.z+1);
	d.b = T_STRING|MAL;
	base64_encode(rawhash, (BYTE*)d.u.s, 20, 0);
	d.u.s[d.z] = 0;
}
void sha256(dat& d){
	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, (BYTE*)d.u.s, d.z);
	d.freedat();
	BYTE rawhash[32];
	sha256_final(&ctx, rawhash);
	d.z = encsize(32);
	d.u.s = (char*) malloc(d.z+1);
	d.b = T_STRING|MAL;
	base64_encode(rawhash, (BYTE*)d.u.s, 32, 0);
	d.u.s[d.z] = 0;
}
void md5(dat& d){
	MD5_CTX ctx;
	md5_init(&ctx);
	md5_update(&ctx, (BYTE*)d.u.s, d.z);
	d.freedat();
	BYTE rawhash[16];
	md5_final(&ctx, rawhash);
	d.z = encsize(16);
	d.u.s = (char*) malloc(d.z+1);
	d.b = T_STRING|MAL;
	base64_encode(rawhash, (BYTE*)d.u.s, 16, 0);
	d.u.s[d.z] = 0;
}
static double dec_places[] = { .000000001,.00000001,.0000001,.000001,.00001,.0001,.001,.01,.1,1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000,10000000000 };
double round(double input, int decimals){
	double m = dec_places[decimals+9];
	return round(input*m)/m;
}
double floor(double input, int decimals){
	double m = dec_places[decimals+9];
	return floor(input*m)/m;
}
double ceil(double input, int decimals){
	double m = dec_places[decimals+9];
	return ceil(input*m)/m;
}


int getSortComparer(querySpecs *q, int i){
	auto info = q->sortInfo[i];
	switch (info.second + info.first * 10){
	case T_INT:
	case T_DATE:
	case T_DURATION:
		return 0;
	case T_INT + 10:
	case T_DATE + 10:
	case T_DURATION + 10:
		return 3;
	case T_FLOAT:
		return 1;
	case T_FLOAT + 10:
		return 4;
	case T_STRING:
		return 2;
	case T_STRING + 10:
		return 5;
	}
	error("invalid sort function");
};

dat prepareLike(unique_ptr<node> &n){
	dat reg;
	reg.u.r = new regex_t;
	reg.b = RMAL;
	string s = n->tok3.val;
	boost::replace_all(s, "_", ".");
	boost::replace_all(s, "%", ".*");
	if (regcomp(reg.u.r, ("^"+s+"$").c_str(), REG_EXTENDED|REG_ICASE))
		error("Could not parse 'like' pattern");
	return reg;
}

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

void messager::send(){
	fprintf(stderr, "\r\33[2K%s", buf);
	if (runmode == RUN_SERVER)
		sendMessage(sessionId, buf);
}
void messager::start(char* msg, int* n1, int* n2){
	stop();
	stopmaster = promise<void>();
	stopslave = stopmaster.get_future();
	runner = thread([&](char* msg,int* num1,int* num2){
		if (!delay){
			snprintf(buf, 200, msg, *num1, *num2);
			send();
		}
		while(stopslave.wait_for(chrono::milliseconds(500)) == future_status::timeout){
			delay = false;
			snprintf(buf, 200, msg, *num1, *num2);
			send();
		}
	}, msg, n1?:&blank, n2?:&blank);
}
void messager::say(char* msg, int* n1, int* n2){
	stop();
	if (delay)
		return;
	snprintf(buf, 200, msg, *(n1?:&blank), *(n2?:&blank));
	send();
}
void messager::stop(){
	if (runner.joinable()){
		stopmaster.set_value();
		runner.join();
	}
	if (!delay)
		cerr << "r\33[2K\r";
	if (runmode == RUN_SERVER)
		sendMessage(sessionId, "");
}
messager::~messager(){
	if (runner.joinable()){
		stopmaster.set_value();
		runner.join();
	}
}

bool opDoesJump(int opcode){
	switch (opcode){
	case JMP:
	case JMPCNT:
	case JMPTRUE:
	case JMPFALSE:
	case RDLINE:
	case RDLINE_ORDERED:
	case NULFALSE1:
	case NULFALSE2:
	case NDIST:
	case SDIST:
	case JMPNOTNULL_ELSEPOP:
	case NEXTMAP:
	case NEXTVEC:
	case READ_NEXT_GROUP:
	case JOINSET_TRAV:
		return true;
	}
	return false;
}

shared_ptr<singleQueryResult> vmachine::getJsonResult(){
	static auto cm = regex(".*--.*(\n.*|$)");
	static auto commented = regex("--.*(\n|$)");
	jsonresult->types = q->colspec.types;
	jsonresult->colnames = q->colspec.colnames;
	jsonresult->pos.resize(q->colspec.count);
	iota(jsonresult->pos.begin(), jsonresult->pos.end(),1); //TODO: update gui to not need this
	jsonresult->query = q->queryString;
	do {
		jsonresult->query = regex_replace(jsonresult->query, commented, "");
	} while (regex_match(jsonresult->query, cm));
	return jsonresult;
}


future<void> queryQueue::runquery(querySpecs& q){
	return async([&](){
		mtx.lock();
		queries.emplace_back(q);
		auto& thisq = queries.back();
		mtx.unlock();
		thisq.run();
		mtx.lock();
		queries.remove_if([&](qinstance& qi){ return qi.vm->id == thisq.vm->id; });
		mtx.unlock();
	});
}
future<shared_ptr<singleQueryResult>> queryQueue::runqueryJson(querySpecs& q){
	return async([&](){
		q.setoutputJson();
		mtx.lock();
		queries.emplace_back(q);
		auto& thisq = queries.back();
		mtx.unlock();
		thisq.run();
		auto ret = thisq.vm->getJsonResult();
		perr("Got json result\n");
		mtx.lock();
		queries.remove_if([&](qinstance& qi){ return qi.vm->id == thisq.vm->id; });
		mtx.unlock();
		perr("Remove query from queue\n");
		return ret;
	});
}
void queryQueue::endall(){
	mtx.lock();
	for (auto& qi : queries)
		qi.vm->endQuery();
	mtx.unlock();
}
void queryQueue::setPassword(i64 sesid, string& pass){
	mtx.lock();
	for (auto& qi : queries)
		if (qi.q->sessionId == sesid){ //TODO: use id not sessionId
			qi.q->setPassword(pass);
			break;
		}
	mtx.unlock();
}
static queryQueue qrunner;
void stopAllQueries(){
	qrunner.endall();
}
void returnPassword(i64 sesid, string pass){
	qrunner.setPassword(sesid, pass);
}

atomic_int vmachine::idCounter = 0;
void runPlainQuery(querySpecs &q){
	auto r = qrunner.runquery(q);
	r.get();
}
shared_ptr<singleQueryResult> runJsonQuery(querySpecs &q){
	auto r = qrunner.runqueryJson(q);
	return r.get();
}
