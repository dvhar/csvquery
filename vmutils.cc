#include "interpretor.h"
#include "vmachine.h"
#include "server.h"
#include "deps/crypto/sha1.h"
#include "deps/crypto/sha256.h"
#include "deps/crypto/md5.h"
#include "deps/crypto/siphash.h"
//#include "deps/crypto/aes.h"
#include "deps/json/escape.h"
#include "deps/html/escape.h"
#include <chrono>
#include <ctype.h>

string opcode::print(){
	return (ft("code: %-18s  [%-2d  %-2d  %-2d]")% opMap[code]% p1% p2% p3).str();
}
void dat::appendToHtmlBuffer(string &outbuf){

	static char buf[40];
	char* c = u.s;
	int utf8charlen;
	long utf8codepoint = 0;
	switch (type()) {
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
		outbuf += durstring(*this, nullptr);
		break;
	case T_STRING:
		outbuf += chopAndEscapeHTML(string_view(u.s));
		break;
	}
}
void dat::appendToJsonBuffer(string &outbuf){

	static char buf[40];
	char a = 0;
	outbuf += '"';
	switch (type()) {
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
		outbuf += durstring(*this, nullptr);
		break;
	case T_STRING:
		outbuf += chopAndEscapeJson(string_view(u.s));
		break;
	}
	outbuf += '"';
}
void dat::appendToCsvBuffer(string &outbuf){
	if (b == 0) return;
	static char buf[40];
	char a = 0;
	switch (type()) {
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
		outbuf += durstring(*this, nullptr);
		return;
	case T_STRING:
		for (auto c = (u8*)u.s; *c; c++) a |= abnormal[*c];
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
			} while ((q = strchr(s+1, '"')));
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
	dat dur;
	if (parseDuration((char*)s, dur))
		error("Could not parse ", s, " as duration.");
	return dur;
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
		q->settypes.push_back(0);
		break;
	case T_STRING:
		q->settypes.push_back(1);
		break;
	default:
		error("invalid btree type");
	}
	return q->settypes.size()-1;
}
char treeCString::blank = 0;

void vmachine::endQuery() {
	for (auto& op : q->bytecode){
		if (opDoesJump(op.code))
			op.p1 = q->bytecode.size()-1;
	}
}
vmachine::vmachine(querySpecs &qs) :
	ops(qs.bytecode.data()),
	stacktop(stack),
	stackbot(stack),
	distinctVal{0},
	torowSize(qs.colspec.count),
	quantityLimit(qs.quantityLimit),
	files(move(qs.filevec)),
	csvOutput(0),
	sessionId(qs.sessionId),
	id(idCounter++),
	q(&qs)
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
	for (u32 i=0; i<50; ++i) stack[i] = dat{0};
	for(int s: q->settypes)
		vsets.emplace_back(s?
			(virtualSet*) new stringSet() :
			(virtualSet*) new numericSet());

	if (q->outputjson || q->outputhtml){
		result.reset(new singleQueryResult(qs));
	}
	if (globalSettings.termbox && !q->isSubquery){
		termbox.init(q->colspec.types, q->colspec.colnames);
		q->outputcsv = false;
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
	if (runmode == RUN_SINGLE && !q->isSubquery) //skip garbage collection if one-off query
		return;
	distinctVal.freedat();
	for (auto &d : stack)   d.freedat();
	for (auto &d : destrow) d.freedat();
	int i = 0;
	for (auto &vec : normalSortVals)
		if (q->sortInfo[i++].second == T_STRING)
			for (auto u : vec)
				free(u.s); //c strings allways allocated with c style
	if (q->distinctFiltering && !(q->grouping && q->sorting)
			&& *find(q->selectiontypes.begin(), q->selectiontypes.end(), T_STRING) == T_STRING){
		vector<int> strings;
		int i = 0;
		for (auto t : q->selectiontypes){
			if (t == T_STRING)
				strings.push_back(i);
			i++;
		}
		for (auto& arr : normalDistinct){
			for (int idx : strings){
				free(arr.data[idx].s);
			}
			free(arr.data);
		}
	}
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
	int newlen = s1.z+s2.z;
	if (s1.ismal()){
		s1.u.s = (char*) realloc(s1.u.s, newlen+1);
		strcat(s1.u.s+s1.z-1, s2.u.s);
	} else {
		char* ns = (char*) malloc(newlen+1);
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
	auto rawResult = (u8*) alloca(ciphlen+noncesize);
	memcpy(rawResult+noncesize+macsize, d.u.s, d.z+1);
	auto nonce = (u32*)ch->nonce;
	auto rnonce = (u32*)rawResult;
	*rnonce = *nonce = uniqueNonce32();
	memcpy(&ch->ctx.key, ch->key, sizeof(ch->ctx.key));
	chacha20_init_context(&ch->ctx, ch->key, ch->nonce, 1);
	getmac(d.u.s, d.z, (char*)ch->nonce, (char*)ch->key, sizeof(ch->key), rawResult+noncesize);
	chacha20_xor(&ch->ctx, rawResult+noncesize, ciphlen);
	u32 finalSize = encsize(ciphlen+noncesize);
	auto finalResult = (char*) malloc(finalSize+1);
	base64_encode((u8*)rawResult, (u8*)finalResult, ciphlen+noncesize, 0);
	finalResult[finalSize]=0;
	d.freedat();
	d = dat{ {.s= finalResult}, T_STRING|MAL, finalSize };
}
void crypter::chachaDecrypt(dat& d, int i){
	auto len = d.z+1;
	auto ch = ctxs.data()+i;
	auto rawResult = (char*) alloca(len);
	//TODO: get exact unpadded decode size
	u32 decodesize = base64_decode((u8*)d.u.s, (u8*)rawResult, len);
	u32 capsize = decodesize - noncesize - macsize;
	auto nonce = (u32*)ch->nonce;
	auto rnonce = (u32*)rawResult;
	*nonce = *rnonce;
	memcpy(&ch->ctx.key, ch->key, sizeof(ch->ctx.key));
	chacha20_init_context(&ch->ctx, ch->key, ch->nonce, 1);
	chacha20_xor(&ch->ctx, (u8*) rawResult+noncesize, decodesize - noncesize);
	u8 mac[macsize];
	u32 finalsize = strnlen(rawResult+macsize+noncesize, capsize);
	getmac(rawResult+noncesize+macsize, finalsize, rawResult, (char*)ch->key, sizeof(ch->key), mac);
	d.freedat();
	if (*(u32*) mac == *(u32*)(rawResult+noncesize)){
		auto finalResult = (char*) malloc(finalsize+1);
		memcpy(finalResult, rawResult+noncesize+macsize, finalsize);
		finalResult[finalsize]=0;
		d = dat{ {.s= finalResult}, T_STRING|MAL, finalsize };
	}
}
void sha1(dat& d){
	SHA1_CTX ctx;
	sha1_init(&ctx);
	sha1_update(&ctx, (u8*)d.u.s, d.z);
	d.freedat();
	u8 rawhash[20];
	sha1_final(&ctx, rawhash);
	d.z = encsize(20);
	d.u.s = (char*) malloc(d.z+1);
	d.b = T_STRING|MAL;
	base64_encode(rawhash, (u8*)d.u.s, 20, 0);
	d.u.s[d.z] = 0;
}
void sha256(dat& d){
	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, (u8*)d.u.s, d.z);
	d.freedat();
	u8 rawhash[32];
	sha256_final(&ctx, rawhash);
	d.z = encsize(32);
	d.u.s = (char*) malloc(d.z+1);
	d.b = T_STRING|MAL;
	base64_encode(rawhash, (u8*)d.u.s, 32, 0);
	d.u.s[d.z] = 0;
}
void md5(dat& d){
	MD5_CTX ctx;
	md5_init(&ctx);
	md5_update(&ctx, (u8*)d.u.s, d.z);
	d.freedat();
	u8 rawhash[16];
	md5_final(&ctx, rawhash);
	d.z = encsize(16);
	d.u.s = (char*) malloc(d.z+1);
	d.b = T_STRING|MAL;
	base64_encode(rawhash, (u8*)d.u.s, 16, 0);
	d.u.s[d.z] = 0;
}
void sip(dat& d){
	u8 bytes[8];
	static const u8* k = (u8*) "asdfhjkl";
	siphash((const u8*)d.u.s, d.z, k, bytes, 8);
	d.freedat();
	d.u.i = *(i64*)bytes;
	d.b = T_INT;
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
	return 0;
};

dat prepareLike(astnode &n){
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
	case NULFALSE:
	case DIST_AGG:
	case DIST_NORM:
	case JMPNOTNULL_ELSEPOP:
	case NEXTMAP:
	case NEXTVEC:
	case READ_NEXT_GROUP:
	case JOINSET_TRAV:
		return true;
	}
	return false;
}

shared_ptr<singleQueryResult> vmachine::getSingleResult(){
	static auto isCommented = regex(".*--.*(\n.*|$)");
	static auto comment = regex("--.*(\n|$)");
	result->types = q->colspec.types;
	result->colnames = q->colspec.colnames;
	result->query = q->queryString;
	do {
		result->query = regex_replace(result->query, comment, "");
	} while (regex_match(result->query, isCommented));
	return result;
}

shared_ptr<singleQueryResult> showTables(querySpecs &q){

	boost::filesystem::path thisdir(globalSettings.configdir);
	auto tables = vector<tuple<string,string>>();
	regex re(".*alias-.*");
	for (auto& f : boost::filesystem::directory_iterator(thisdir)){
		auto&& aliasfile = f.path().string();
		if (regex_match(aliasfile,re)){
			string alias = boost::filesystem::basename(aliasfile);
			alias = alias.substr(6, alias.size()-4);
			ifstream afile(aliasfile);
			string apath;
			afile >> apath;
			tables.emplace_back(alias, apath);
		}
	}
	vector<string> colnames = {"Name","Details"};
	vector<int> types = {5,5};
	if (q.outputhtml){
		auto ret = make_shared<singleQueryResult>();
		ret->numcols = 2;
		ret->rowlimit = 10000;
		ret->query = "show tables";
		ret->colnames = move(colnames);
		ret->types = move(types);
		for (auto& t : tables){
			ret->numrows++;
			ret->Vals.push_back(st("<tr><td>", escapeHTML(get<0>(t)), "</td><td>", escapeHTML(get<1>(t)), "</td></tr>"));
		}
		return ret;
	} else {
		boxprinter bp;
		bp.init(types,colnames);
		dat row[2];
		for (auto& t : tables){
			row[0] = dat{{.s=(char*)get<0>(t).data()},T_STRING};
			row[1] = dat{{.s=(char*)get<1>(t).data()},T_STRING};
			bp.addrow(row);
		}
		return nullptr;
	}

}

void queryQueue::runquery(querySpecs& q){
	auto qi = qinstance(q);
	qi.runq();
}
shared_ptr<singleQueryResult> queryQueue::runqueryJson(querySpecs& q){
	q.setoutputJson();
	mtx.lock();
	queries.emplace_back(q);
	mtx.unlock();
	auto& thisq = queries.back();
	auto id = thisq.runq();
	auto ret = thisq.getResult();
	perr("Got json result\n");
	mtx.lock();
	queries.remove_if([&](qinstance& qi){ return qi.id == id; });
	mtx.unlock();
	perr("Remove query from queue\n");
	return ret;
}
// TODO: make this more dry when done
shared_ptr<singleQueryResult>queryQueue::runqueryHtml(querySpecs& q){
	q.setoutputHtml();
	mtx.lock();
	queries.emplace_back(q);
	auto& thisq = queries.back();
	mtx.unlock();
	auto id = thisq.runq();
	auto ret = thisq.getResult();
	perr("Got html result\n");
	mtx.lock();
	queries.remove_if([&](qinstance& qi){ return qi.id == id; });
	mtx.unlock();
	perr("Remove query from queue\n");
	return ret;
}
void queryQueue::endall(){
	mtx.lock();
	for (auto& qi : queries) qi.stop();
	mtx.unlock();
}
void queryQueue::setPassword(i64 sesid, string& pass){
	mtx.lock();
	for (auto& qi : queries)
		if (qi.sesid == sesid){ //TODO: use id not sessionId
			qi.setPass(pass);
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

atomic_int vmachine::idCounter(0);
void runPlainQuery(querySpecs &q){
	qrunner.runquery(q);
}
shared_ptr<singleQueryResult> runJsonQuery(querySpecs &q){
	return qrunner.runqueryJson(q);
}
shared_ptr<singleQueryResult> runHtmlQuery(querySpecs &q){
	return qrunner.runqueryHtml(q);
}
void boxprinter::addrow(dat* sourcerow){
	if (++numrow > rowlimit)
		print();
	datarows.emplace_back();
	auto& newrow = datarows.back();
	for (int i=0; i<numcol; ++i){
		auto&& field = sourcerow[i].str();
		widths[i] = max(widths[i], field.size());
		newrow.push_back(field);
	}
}
static const char* colors[] = {
	"\033[38;5;142m",
	"\033[38;5;87m",
	"\033[38;5;82m",
	"\033[38;5;205m",
	"\033[38;5;196m",
	"\033[38;5;208m",
	"\033[0m",    //clearcolor
	"\033[1;90m", //grey
	"\033[1;93m", //yellow
};
static const char* nocolors[] = { "","","","","","","","","" };
static const char** colortable;
#define tc(i) colortable[types[i]]
#define clearcolor colortable[6]
#define grey  colortable[7]
#define yellow colortable[8]
#define bg1 "\033[48;5;232m"
#define bg2 "\033[48;5;235m"
void boxprinter::print(){
	colortable = globalSettings.tablecolor ? colors : nocolors;
	size_t totalwidth = accumulate(widths.begin(), widths.end(), numcol-1);
	stringstream rowseps;
	for (auto w : widths)
		rowseps << '+' << string(w,'-');
	rowseps << "+";
	string rowsep = rowseps.str();
	int i,j=0;
	bool bg = globalSettings.tablelinebg;
	if (bg) cout << bg1;
	cout << '+' << string(totalwidth,'-') << '+' << clearcolor << '\n';
	for (auto& cell : names){
		if (bg) cout << bg1;
		cout << '|' << yellow << right << setw(widths[i++]) << cell << clearcolor;
	}
	if (bg) cout << bg1 << '|' << clearcolor << '\n' << bg1 << rowsep << clearcolor << '\n' << grey;
	else cout << "|\n" << rowsep << '\n' << grey;
	for (auto& row : datarows){
		i = 0;
		if (bg) cout << ((j^=1) ?  bg2 : bg1);
		cout << grey;
		for (auto& cell : row){
			cout << '|' << tc(i) << right << setw(widths[i]) << cell << grey;
			++i;
		}
		cout << '|' << clearcolor << '\n';
		row.clear();
	}
	cout << clearcolor;
	datarows.clear();
	numrow = 0;
}
