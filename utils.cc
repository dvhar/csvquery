#include "interpretor.h"
#include <chrono>
#include <stdlib.h>
#include <thread>
#include "deps/json/escape.h"
#include "deps/html/escape.h"
#include "deps/incbin/incbin.h"
#include <string>
#include <algorithm> // for std::transform
#include <cctype>    // for std::tolower
#include <sys/stat.h>
#include <string>
#include <limits.h>
#include <unistd.h>
#include <cstdio>
#include <dirent.h>

INCBIN(_SINGLERESULT,"../webgui/singleresult.html");
#define max(a,b) (a) > (b) ? (a) : (b)

string version = "1.60";
int runmode;
regex_t leadingZeroString;
regex_t durationPattern;
regex_t intType;
regex_t floatType;
regex cInt("^c\\d+$");
regex posInt("^\\d+$");
regex colNum("^c?\\d+$");
regex csvPat(".*\\.csv$", regex_constants::icase);
regex tsvPat(".*\\.tsv$", regex_constants::icase);
regex hidPat(".*/\\.[^/]+$");
regex filelike(".*[/\\\\\\.].*");
int isDuration(const char* s){ return !regexec(&durationPattern, s, 0, NULL, 0); }
int isInt(const char* s){ return !regexec(&intType, s, 0, NULL, 0); }
int isFloat(const char* s){ return !regexec(&floatType, s, 0, NULL, 0); }
static const array<string_view,7> typenames = { "Null", "Int", "Float", "Date", "Duration", "Text", "Unknown" };

std::vector<std::string> fs_directory_iterator(const std::string& dir) {
    std::vector<std::string> files;
    DIR* dp = opendir(dir.c_str());
    if (!dp) return files;
    struct dirent* ep;
    while ((ep = readdir(dp))) {
        if (std::string(ep->d_name) == "." || std::string(ep->d_name) == "..") continue;
        files.push_back(dir + "/" + ep->d_name);
    }
    closedir(dp);
    return files;
}

std::string fs_parent_path(const std::string& path) {
    if (path.empty()) return "";
    size_t end = path.size();
    while (end > 1 && path[end - 1] == '/')
        --end;
    size_t pos = path.rfind('/', end - 1);
    if (pos == std::string::npos) {
        return "";
    }
    if (pos == 0)
        return "/";
    return path.substr(0, pos);
}

std::string fs_basename(const std::string& path) {
    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return path;
    std::string filename = path.substr(slash + 1);
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) return filename;
    return filename.substr(0, dot);
}

bool fs_create_directories(const std::string& path) {
    std::istringstream iss(path);
    std::string token, built;
    bool success = true;
    while (std::getline(iss, token, '/')) {
        if (token.empty()) continue;
        built += "/" + token;
        if (!fs_exists(built)) {
#if defined(_WIN32)
            if (mkdir(built.c_str()) != 0) {
#else
            if (mkdir(built.c_str(), 0755) != 0) {
#endif
                if (errno != EEXIST) {
                    success = false;
                    break;
                }
            }
        }
    }
    return success;
}

std::string fs_current_path() {
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf))) {
        return std::string(buf);
    } else {
        return ""; // or handle error
    }
}

bool fs_is_regular_file(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) return false;
    return S_ISREG(info.st_mode);
}

bool fs_is_directory(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) return false;
    return S_ISDIR(info.st_mode);
}

bool fs_remove(const std::string& path) {
    return std::remove(path.c_str()) == 0;
}

bool fs_exists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0;
}

std::string fs_canonical(const std::string& path) {
    char resolved[PATH_MAX];
#if defined(_WIN32)
    if (_fullpath(resolved, path.c_str(), PATH_MAX)) {
        return std::string(resolved);
    } else {
        return "";
    }
#else
    if (realpath(path.c_str(), resolved)) {
        return std::string(resolved);
    } else {
        return "";
    }
#endif
}

std::string replace_all(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

// Replace the first occurrence of 'from' with 'to' in 'str'
std::string replace_first(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    std::string result = str;
    size_t pos = result.find(from);
    if (pos != std::string::npos) {
        result.replace(pos, from.length(), to);
    }
    return result;
}

// Return a lower-case copy of the input string
std::string to_lower_copy(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

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

variable& querySpecs::var(string name) {
	for (auto &v : vars)
		if (name == v.name)
			return v;
	error("Variable not found");
	return vars.front();
}
void querySpecs::addVar(string name) {
	if (find(vars.begin(), vars.end(), variable{.name = name}) != vars.end())
		error("Alias '",name,"' defined more than once");
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
	perr(st(s , n->nodelabel() , '\n', s,
		"[t1:", n->tok1.val,
        " t2:", n->tok2.val,
        " t3:", n->tok3.val,
        " t4:", n->tok4.val,
        "] t:", n->datatype,
        " p:", n->phase,
        " k:", n->keep));
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
			return n->val();
		return "";
	}
	return nodeName(n->node1, q);
}

string singleQueryResult::tohtml(){
	string tplate((const char*)g_SINGLERESULTData, g_SINGLERESULTSize);
	tplate = replace_first(tplate,"{{ querytext }}", chopAndEscapeHTML(query));
	tplate = replace_all(tplate,"{{ colnum }}", to_string(numcols));
	tplate = replace_all(tplate,"{{ rownum }}", to_string(numrows));
	{
		stringstream filterlist;
		stringstream togglelist;
		stringstream namelist;
		stringstream typelist;
		for (int i=0; i<colnames.size(); i++){
			auto name = chopAndEscapeHTML(colnames[i]);
			filterlist << "<option onclick=\"populatefilter(this," << i << ")\">" << name << "</option>";
			togglelist << "<option onclick=\"toggleColumn(this," << i << ")\">" << name << "</option>";
			namelist << "<th>" << name << "</th>";
			typelist << "<td>" << i+1 << "<span>-" << typenames[types[i]] << "</span></td>";
		}
        tplate = replace_first(tplate,"{{ populatefilter-names }}", filterlist.str());
        tplate = replace_first(tplate,"{{ toggleColumn-names }}", togglelist.str());
        tplate = replace_first(tplate,"{{ th-names }}", namelist.str());
        tplate = replace_first(tplate,"{{ td-types }}", typelist.str());
	}
	stringstream rows;
	for (auto& row:vals)
		rows << row << endl;
	tplate = replace_first(tplate,"{{ tr-vals }}", rows.str());
	return tplate;
}
//nlohmann library can't handle invalid utf-8
stringstream& singleQueryResult::tojson(){
	static string_view com = ",";
	static string_view nocom = "";
	marshaller << "{\"numrows\":" << numrows
		<< ",\"rowlimit\":" << rowlimit
		<< ",\"numcols\":" << numcols;
	auto delim = &nocom;
	marshaller << ",\"types\":[";
	for (auto v : types){
		marshaller << *delim << v;
		delim = &com;
	}
	marshaller << "],\"colnames\":[";
	delim = &nocom;
	for (auto &v : colnames){
		marshaller << *delim << '"' << chopAndEscapeJson(string_view(v)) << '"';
		delim = &com;
	}
	marshaller << "],\"vals\":[";
	delim = &nocom;
	for (auto &v : vals){
		marshaller << *delim << v;
		delim = &com;
	}
	marshaller << "],\"status\":" << status
		<< ",\"query\":\"" << escapeJSON(query) << "\"}";
	return marshaller;
}
stringstream& returnData::tojson(){
	static string_view com = ",";
	static string_view nocom = "";
	ss	<< "{\"entries\":[";
	auto delim = &nocom;
	for (auto &v : entries){
		ss << *delim << v->tojson().rdbuf();
		delim = &com;
	}
	ss << "],\"status\":" << status
		<< ",\"originalQuery\":\"" << escapeJSON(originalQuery)
		<< "\",\"clipped\":" << (clipped ? "true":"false")
		<< ",\"message\":\"" << escapeJSON(message) << "\"}";
	return ss;
}
string returnData::tohtml(){
	for (auto &v : entries){
		ss << v->tohtml();
	}
	return ss.str();
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
		if (q.canskip && EX_STRING == "Empty query"){
			perr("Query commented out\n");
			return COMMENTED_OUT;
		}
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
