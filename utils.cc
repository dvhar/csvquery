#include "interpretor.h"
#include <chrono>
#include <stdlib.h>
#include <ctype.h>
#include <thread>
#include "deps/json/escape.h"
#define max(a,b) (a) > (b) ? (a) : (b)

string version = "1.50";
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
regex filelike(".*[/\\\\\\.].*");
int isDuration(const char* s){ return !regexec(&durationPattern, s, 0, NULL, 0); }
int isInt(const char* s){ return !regexec(&intType, s, 0, NULL, 0); }
int isFloat(const char* s){ return !regexec(&floatType, s, 0, NULL, 0); }

void initregex(){
	regcomp(&leadingZeroString, "^0[0-9]+$", REG_EXTENDED);
	regcomp(&durationPattern, "^([0-9]+|[0-9]+\\.[0-9]+) ?(seconds|second|minutes|minute|hours|hour|days|day|weeks|week|years|year|ms|s|m|h|d|w|y)$", REG_EXTENDED);
	regcomp(&intType, "^-?[0-9]+$", REG_EXTENDED);
	regcomp(&floatType, "^-?[0-9]*\\.[0-9]+$", REG_EXTENDED);
}
mt19937 rng(chrono::high_resolution_clock::now().time_since_epoch().count());


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
	return vars.front();
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
int querySpecs::addSubquery(astnode& subtree, int sqtype){
	try {
		perr("adding subquery");
		subqueries.emplace_back(new querySpecs(subtree, sqtype));
		auto& sq = subqueries.back();
		sq.prep = thread([&]{prepareQuery(*sq.query);});
	} catch (...){
		error("SUBQUERY: ",EX_STRING);
	}
	perr("added subquery");
	return subqueries.size()-1;
}

void subquery::terminate(exception_ptr e){
	ex = e;
	if(topfinaltypes.valid()){
		topfinaltypesp.set_exception(ex);
	}
}
void subquery::terminateOutter(exception_ptr e){
	ex = e;
	if (topinnertypes.valid() &&
			topinnertypes.wait_for(chrono::seconds(0)) == future_status::timeout) {
		topinnertypesp.set_exception(ex);
	}
}


void printTree(astnode &n, int ident){
	if (n == nullptr) return;
	ident++;
	string s = "";
	for (int i=0;i<ident;i++) s += "  ";
	perr(st( s , n->nodelabel() , '\n', s,
		ft("[%1% %2% %3% %4%] t:%5% p:%6% k:%7%")
		% n->tok1.val
		% n->tok2.val
		% n->tok3.val
		% n->tok4.val
		% n->datatype
		% n->phase
		% n->keep));
	printTree(n->node1,ident);
	printTree(n->node2,ident);
	printTree(n->node3,ident);
	printTree(n->node4,ident);
}

int parseDuration(char* str, dat& d) {
	d.setnull();
	if (!isDuration(str)) {return -1;}
	char* part2;
	double quantity = strtod(str, &part2);
	const double day = 86400;
	while (*part2 == ' ') ++part2;
	switch (part2[0]){
	case 'y':{
			if (fmod(quantity,1)==0){
				d.z = quantity;
			}
			i64 q = i64(quantity);
			double leapdays = q/4 - q/100 + q/400;
			quantity *= 31536000;
			quantity += leapdays*day;
		 }
		break;
	case 'w':
		quantity *= 604800;
		break;
	case 'd':
		quantity *= day;
		break;
	case 'h':
		quantity *= 3600;
		break;
	case 'm':
		if (part2[1] == 's'){
			quantity /= 1000;
		} else {
			quantity *= 60;
		}
		break;
	case 's':
		break;
	default:
		return -1;
	}
	d.u.i = quantity * 1E6;
	d.b = T_DURATION;
	return 0;
}

int getNarrowestType(char* value, int startType) {
	static date_t t;
	static dat d;
	if (value[0] == '\0' || !strcasecmp(value,"null") || !strcmp(value,"NA")) {
	  startType = max(T_NULL, startType);
	} else if (!regexec(&leadingZeroString, value, 0, NULL, 0)){ startType = T_STRING;
	} else if (isInt(value))                       { startType = max(T_INT, startType);
	} else if (isFloat(value))                     { startType = max(T_FLOAT, startType);
	} else if (!dateparse_2(value, &t))            { startType = max(T_DATE, startType);
	  //in case duration gets mistaken for a date
	   if (!parseDuration(value, d))               { startType = max(T_DURATION, startType); }
	} else if (!parseDuration(value, d))           { startType = max(T_DURATION, startType);
	} else                                         { startType = T_STRING; }
	return startType;
}

//use static buf if null arg, otherwise make sure size 24
char* durstring(dat& d, char* str){
	static char durbuf[24];
	auto dur = d.u.i;
	if (dur < 0)
		dur *= -1;
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
	dur_t mics = dur % 1000000;
	dur_t secs = (dur % 60000000) / 1000000;
	dur_t mins = (dur % 3600000000) / 60000000;
	dur_t hours = (dur % 86400000000) / 3600000000;
	dur_t days = dur / 86400000000;
	if (d.z && dur % 31536000000000 == 0){
		sprintf(s, "%dy", d.z);
		return dest;
	}
	if (d.u.i < 0){
		s[0] = '-';
	}
	if (days){
		while(*s) ++s;
		sprintf(s, "%lldd", days);
	}
	if (hours){
		while(*s) ++s;
		sprintf(s, "%lldh", hours);
	}
	if (mins){
		while(*s) ++s;
		sprintf(s, "%lldm", mins);
	}
	if (secs){
		while(*s) ++s;
		sprintf(s, "%llds", secs);
	}
	if (mics) {
		while(*s) ++s;
		if (mics % 1000){
			sprintf(s, "%lldmcs", mics);
		} else {
			sprintf(s, "%lldms", mics/1000);
		}
	}
	return dest;
}

node& node::operator=(const node& other){
	tok1 = other.tok1;
	tok2 = other.tok2;
	tok3 = other.tok3;
	tok4 = other.tok4;
	label = other.label;
	phase = other.phase;
	datatype = other.datatype;
	keep = other.keep;
	return *this;
}
node::node(){
	tok1 = tok2 = tok3 = tok4 = token{};
	label = phase = datatype = 0;
	keep = false;
}
node::node(int l){
	tok1 = tok2 = tok3 = tok4 = token{};
	label = l;
	phase = datatype = 0;
	keep = false;
}
node::~node(){
}

astnode& findFirstNode(astnode &n, int label){
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

int querySpecs::trivialColumnType(astnode &n){
	if (n == nullptr) return 0;
	if (n->label == N_VALUE && n->trivialvalcol()){
		return filemap[n->dotsrc()]->types[n->colidx()];
	}
	return trivialColumnType(n->node1);
}
bool isTrivialColumn(astnode &n){
	if (n == nullptr) return false;
	if (n->label == N_VALUE && n->trivialvalcol())
		return true;
	return isTrivialColumn(n->node1);
}
bool isTrivialAlias(astnode &n){
	if (n == nullptr) return false;
	if (n->label == N_VALUE && n->trivialvalalias())
		return true;
	return isTrivialAlias(n->node1);
}
string nodeName(astnode &n, querySpecs* q){
	if (n == nullptr) return "";
	if (n->label == N_VALUE){
		if (n->valtype() == COLUMN && n->trivialvalcol()){
			if (!q->filemap[n->dotsrc()]->noheader)
			return q->filemap[n->dotsrc()]->colnames[n->colidx()];
		}
		if (n->valtype() == VARIABLE)
			return n->nval();
		return "";
	}
	return nodeName(n->node1, q);
}

//nlohmann library can't handle invalid utf-8
stringstream& singleQueryResult::tojson(){
	static string_view com = ",";
	static string_view nocom = "";
	j << "{\"numrows\":" << numrows
		<< ",\"rowlimit\":" << rowlimit
		<< ",\"numcols\":" << numcols;
	auto delim = &nocom;
	j << ",\"types\":[";
	for (auto v : types){
		j << *delim << v;
		delim = &com;
	}
	j << "],\"colnames\":[";
	delim = &nocom;
	for (auto &v : colnames){
		j << *delim << '"' << escapeJSON(v) << '"';
		delim = &com;
	}
	j << "],\"vals\":[";
	delim = &nocom;
	for (auto &v : Vals){
		j << *delim << v;
		delim = &com;
	}
	j << "],\"status\":" << status
		<< ",\"query\":\"" << escapeJSON(query) << "\"}";
	return j;
}
stringstream& returnData::tojson(){
	static string_view com = ",";
	static string_view nocom = "";
 	j	<< "{\"entries\":[";
	auto delim = &nocom;
	for (auto &v : entries){
		j << *delim << v->tojson().rdbuf();
		delim = &com;
	}
	j << "],\"status\":" << status
		<< ",\"originalQuery\":\"" << escapeJSON(originalQuery)
		<< "\",\"clipped\":" << (clipped ? "true":"false")
		<< ",\"message\":\"" << escapeJSON(message) << "\"}";
	return j;
}

// https://www.tutorialgateway.org/sql-date-format/
const char* dateFormatCode(string& s){
	if (!isInt(s.c_str())){
		return s.c_str();
	}
	try {
		return dateFmtCodes.at(atoi(s.c_str()));
	} catch (...) {
		error(s," does not match any date formats");
	}
	return nullptr;
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

settings_t globalSettings;

#define R (const char*)
void perr(string&& message){
	static const char* dbcolors[] = {
		R "\033[38;5;82m",
		R "\033[38;5;205m",
		R "\033[38;5;87m",
		R "\033[38;5;196m",
		R "\033[38;5;208m",
		R "\033[38;5;82m",
	};
	static atomic_int i(0);
	static map<thread::id,const char*> threadcolors;
	static mutex dbmtx;
	if (globalSettings.debug){
		dbmtx.lock();
		auto threadid = this_thread::get_id();
		if (!threadcolors.count(threadid))
			threadcolors[threadid] = dbcolors[(i++)%6];
		cerr << threadcolors[threadid] << message << "\033[0m" << endl;
		dbmtx.unlock();
	}
}

int prepareQuery(querySpecs &q){
	exception_ptr ex = nullptr;
	try {
		scanTokens(q);
		if (q.canskip && q.tokArray.size() == 1 && q.tokArray[0].id == EOS){
			perr("Query commented out\n");
			return COMMENTED_OUT;
		}
		parseQuery(q);
		//return code for operations other than queries
		if (int nonquery = earlyAnalyze(q); nonquery){
			return nonquery;
		}
		openfiles(q);
		midAnalyze(q);
		q.promptPassword();
		applyTypes(q);
		lateAnalyze(q);
		printTree(q.tree, 0);
		codeGen(q);
	} catch (...){
		ex = current_exception();
	}

	for (auto& sq : q.subqueries){
		if (ex) sq.terminate(ex);
		sq.prep.join();
		if (sq.ex) ex = sq.ex;
	}

	if (ex){
		if (q.isSubquery){
			q.thisSq->terminateOutter(ex);
		}else{
			rethrow_exception(ex);
		}
	}
	return 0;
};
