#include "server.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include "deps/http/server_http.hpp"
#include <boost/asio/ip/address.hpp>
#include <boost/filesystem.hpp>
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
typedef shared_ptr<HttpServer::Response> Response;
typedef shared_ptr<HttpServer::Request> Request;

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
shared_ptr<returnData> runqueries(webquery &wq);
static shared_ptr<singleQueryResult> runWebQuery(webquery &wq);
static void serve();
static SimpleWeb::CaseInsensitiveMultimap header;
string queryReturn;
void embedsite(HttpServer&);

void runServer(){
	globalSettings.termbox = false;
	serve();
	exit(0);
}

static bool rejectNonLocals(Request& request){
	static auto lh = boost::asio::ip::address::from_string("::ffff:127.0.0.1");
	auto addr = request->remote_endpoint().address();
	if (!addr.is_loopback() && addr != lh){
		cerr << "attempted connection from non-localhost: " << addr << endl;
		return true;
	}
	return false;
}

static void serve(){

	auto startdir = filebrowse(boost::filesystem::current_path().string());
	state["openDirList"] = startdir->tojson();
	state["saveDirList"] = startdir->tojson();
	state["version"] = version;
	state["notifyUpdate"] = globalSettings.update;
	state["configpath"] = globalSettings.configfilepath;
	server.config.port = 8060;
	header.emplace("Cache-Control","no-store");
	header.emplace("Content-Type", "text/plain");
	embedsite(server);

	server.resource["^/query/$"]["POST"] = [&](Response response, Request request){

		if (rejectNonLocals(request)) return;
		webquery wq;
		try {
			auto&& reqstr = request->content.string();
			auto j = json::parse(reqstr);
			wq.savepath = fromjson<string>(j, "savePath");
			wq.qamount = fromjson<int>(j, "qamount");
			wq.fileIO = fromjson<int>(j, "fileIO");
			wq.querystring = regex_replace(fromjson<string>(j,"query"), endSemicolon, "");
			wq.sessionId = fromjson<i64>(j,"sessionId");

			auto ret = runqueries(wq);
			ret->status = DAT_GOOD;
			ret->originalQuery = wq.querystring;
			if (wq.isSaving())
				sendMessage(wq.sessionId, "Saved to " + wq.savepath);
			else if (ret->maxclipped)
				sendMessage(wq.sessionId, st("Only showing first ",ret->maxclipped," results"));
			perr("Writing http respons");
			response->write(ret->tohtml(), header);

		} catch (...){
			auto e = EX_STRING;
			cerr << "Error: " << e << endl;
			returnData ret;
			ret.status = DAT_ERROR;
			ret.message = move(e);
			ret.originalQuery = wq.querystring;
			response->write(ret.tojson().str());
		}
	};

	server.resource["^/info$"]["GET"] = 
	server.resource["^/info$"]["POST"] = [&](Response response, Request request){

		if (rejectNonLocals(request)) return;
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
					state["saveDirList"] = j;
				}
				response->write(j.dump(), header);
			} catch (...) {
				auto e = EX_STRING;
				cerr << e << endl;
				response->write(json{{"error",e}}.dump());
			}
		} else {
			response->write("{}");
		}
	};

	perr("starting http server");
	cout << "Go to http://localhost:8060 to use graphic interface\n";
	auto ws = async(servews);
	auto hs = async([](){server.start();});
	openbrowser();
	ws.get();
	server.stop();
	hs.get();
}

shared_ptr<returnData> runqueries(webquery &wq){
	boost::split(wq.queries, wq.querystring, boost::is_any_of(";"));
	auto ret = make_shared<returnData>();
	for (auto &q: wq.queries){
		auto&& singleResult = runWebQuery(wq);
		wq.whichone++;
		if (singleResult == nullptr)
			continue;
		perr("Got result of single query");
		ret->entries.push_back(singleResult);
		if (singleResult->clipped)
			ret->maxclipped = max(ret->maxclipped, singleResult->rowlimit);
	}
	perr("Got result of all queries");
	return ret;
}

static shared_ptr<singleQueryResult> runWebQuery(webquery &wq){
	cout << "webquery " << wq.whichone << ": " << wq.queries[wq.whichone] << endl;
	querySpecs q(wq.queries[wq.whichone]);
	q.sessionId = wq.sessionId;
	if (wq.isSaving()){
		q.setoutputCsv();
		bool hascsv = regex_match(wq.savepath, csvPat);
		bool multi = wq.queries.size() > 1;
		if (multi && !hascsv){
			q.savepath = st(wq.savepath, '-', wq.whichone+1, ".csv");
			wq.savepath += ".csv";
		} else if (multi && hascsv){
			q.savepath = st(wq.savepath.substr(0, wq.savepath.size()-4), '-', wq.whichone+1, ".csv");
		} else if (!hascsv){
			q.savepath = wq.savepath += ".csv";
		} else {
			q.savepath = wq.savepath;
		}
	}
	//return runJsonQuery(q);
	return runHtmlQuery(q);
}

void directory::setDir(json& j){
	fpath = fromjson<string>(j,"path");
	parent = fromjson<string>(j,"parent");
	mode = fromjson<int>(j,"mode");
	files = fromjson<vector<string>>(j,"files");
	dirs = fromjson<vector<string>>(j,"dirs");
}

json& directory::tojson(){
	j = {
		{"path",fpath},
		{"parent",parent},
		{"mode",mode},
		{"files",files},
		{"dirs",dirs},
	};
	return j;
}
