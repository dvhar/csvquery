#include "server.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include "deps/http/server_http.hpp"
#include <filesystem>
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

#if defined _WIN32
#define openbrowser() system("cmd /c start http://localhost:8060")
#elif defined __APPLE__
#define openbrowser() system("open http://localhost:8060")
#elif defined __linux__
#define openbrowser() system("xdg-open http://localhost:8060")
#else
#error "not win mac or linux"	
#endif

static json state;
static HttpServer server;
static shared_ptr<returnData> runqueries(webquery &wq);
static shared_ptr<singleQueryResult> runWebQuery(webquery &wq);
static void serve();
static SimpleWeb::CaseInsensitiveMultimap header;
void embedsite(HttpServer&);

void runServer(){
	serve();
}

static void serve(){

	auto startdir = filebrowse(filesystem::current_path());
	state["openDirList"] = startdir->tojson();
	state["saveDirList"] = startdir->tojson();
	auto endSemicolon = regex(";\\s*$");
	server.config.port = 8060;
	header.emplace("Cache-Control","no-store");
	header.emplace("Content-Type", "text/plain");
	embedsite(server);

	server.resource["^/query/$"]["POST"] = [&](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){

		webquery wq;
		try {
			auto&& reqstr = request->content.string();
			auto j = json::parse(reqstr);
			wq.savepath = fromjson<string>(j, "SavePath");
			wq.qamount = fromjson<int>(j, "Qamount");
			wq.fileIO = fromjson<int>(j, "FileIO");
			wq.querystring = regex_replace(fromjson<string>(j,"Query"), endSemicolon, "");
			wq.sessionId = fromjson<i64>(j,"SessionId");

			auto ret = runqueries(wq);
			ret->status = DAT_GOOD;
			ret->originalQuery = move(wq.querystring);
			if (wq.isSaving()){
				auto msg = "Saved to " + wq.savepath;
				sendMessage(wq.sessionId, msg.c_str());
			}
			response->write(ret->tojson().dump(), header);

		} catch (...){
			auto e = handle_err(current_exception());
			cerr << "Error: " << e << endl;
			returnData ret;
			ret.status = DAT_ERROR;
			ret.message = move(e);
			ret.originalQuery = move(wq.querystring);
			response->write(ret.tojson().dump());
		}
	};

	server.resource["^/info$"]["GET"] = 
	server.resource["^/info$"]["POST"] = [&](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){

		auto params = request->parse_query_string();
		auto info = params.find("info")->second;

		if (info == "setState"){
			state = json::parse(request->content.string());
			response->write("{}");
		} else if (info == "getState"){
			response->write(state.dump(), header);
		} else if (info == "fileClick"){
			auto j = json::parse(request->content.string());
			cerr << j.dump() << endl;
			string mode = fromjson<string>(j,"mode");
			try {
				auto newlist = filebrowse(fromjson<string>(j,"path"));
				newlist->mode = fromjson<string>(j,"mode");
				j = newlist->tojson();
				if (mode == "open"){
					state["openDirList"] = j;
				} else if (mode == "save"){
					state["openDirList"] = j;
				}
				response->write(j.dump(), header);
			} catch (...) {
				auto e = handle_err(current_exception());
				cerr << e << endl;
				sendMessage(fromjson<i64>(j,"SessionId"), e.c_str());
				response->write("{}");
			}
		} else {
			response->write("{}");
		}
	};

	openbrowser();
	perr("starting http server\n");
	auto ws = async(servews);
	server.start();
	ws.get();
}

static shared_ptr<returnData> runqueries(webquery &wq){
	boost::split(wq.queries, wq.querystring, boost::is_any_of(";"));
	auto ret = make_shared<returnData>();
	for (auto &q: wq.queries){
		ret->entries.push_back(runWebQuery(wq));
		wq.whichone++;
	}
	return ret;
}

static shared_ptr<singleQueryResult> runWebQuery(webquery &wq){
	cout << "webquery " << wq.whichone << ": " << wq.queries[wq.whichone] << endl;
	querySpecs q(wq.queries[wq.whichone]);
	q.sessionId = wq.sessionId;
	if (wq.isSaving()){
		q.setoutputCsv();
		bool hascsv = !regexec(&extPattern, wq.savepath.c_str(), 0,0,0);
		bool multi = wq.queries.size() > 1;
		if (multi && !hascsv){
			q.savepath = st(wq.savepath, '-', wq.whichone+1, ".csv");
			wq.savepath += ".csv";
		} else if (multi && hascsv){
			q.savepath = st(wq.savepath.substr(0, wq.savepath.size()-4), '-', wq.whichone+1, ".csv");
		} else if (!hascsv){
			q.savepath = wq.savepath += ".csv";
		}
	}
	return runqueryJson(q);
}

void directory::setDir(json& j){
	fpath = fromjson<string>(j,"Path");
	parent = fromjson<string>(j,"Parent");
	mode = fromjson<int>(j,"Mode");
	files = fromjson<vector<string>>(j,"Files");
	dirs = fromjson<vector<string>>(j,"Dirs");
}

json& directory::tojson(){
	j = {
		{"Path",fpath},
		{"Parent",parent},
		{"Mode",mode},
		{"Files",files},
		{"Dirs",dirs},
	};
	return j;
}
