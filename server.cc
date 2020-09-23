#include "server.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#if 0
#elif defined _WIN32
#define openbrowser() system("cmd /c start http://localhost:8060")
#elif defined __APPLE__
#define openbrowser() system("open http://localhost:8060")
#elif defined __linux__
#define openbrowser() system("xdg-open http://localhost:8060")
#else
	#error "not win mac or linux"	
#endif

void runServer(){
	webserver ws;
	ws.serve();
	json j;
}

void webserver::serve(){
	auto endSemicolon = regex(";\\s*$");
	server.config.port = 8060;
	embed(server);

	server.resource["^/query/$"]["POST"] = [&](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){

		// compatible with old version
		webquery wq;
		SimpleWeb::CaseInsensitiveMultimap header;

		try {
			json req = json::parse(request->content.string());
			wq.savepath = fromjson<string>(req, "Savepath");
			wq.qamount = fromjson<int>(req, "Qamount");
			wq.fileIO = fromjson<int>(req, "FileIO");
			wq.querystring = regex_replace(fromjson<string>(req,"Query"), endSemicolon, "");

			auto ret = runqueries(wq);
			ret->status = DAT_GOOD;
			ret->originalQuery = move(wq.querystring);
			if (wq.isSaving())
				ret->message = "Saved to " + wq.savepath;
			header.emplace("Cache-Control","no-store");
			header.emplace("Content-Type", "text/plain");
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

	}; // end /query/

	server.resource["^/info$"]["GET"] = [&](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		auto params = request->parse_query_string();
		auto info = params.find("info")->second;
		if (info == "getState"){
			response->write(state);
		}
	};

	server.resource["^/info$"]["POST"] = [&](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){

		auto params = request->parse_query_string();
		auto info = params.find("info")->second;
		if (info == "setState"){
			state = request->content.string();
			response->write("{}");
			return;
		} else if (info == "fileClick"){
		}
		response->write("{}");

	};

	//openbrowser();
	perr("starting http server\n");
	server.start();
}

shared_ptr<returnData> webserver::runqueries(webquery &wq){
	boost::split(wq.queries, wq.querystring, boost::is_any_of(";"));
	auto ret = make_shared<returnData>();
	for (auto &q: wq.queries){
		ret->entries.push_back(runWebQuery(wq));
		wq.whichone++;
	}
	return ret;
}

shared_ptr<singleQueryResult> webserver::runWebQuery(webquery &wq){
	cout << "webquery " << wq.whichone << ": " << wq.queries[wq.whichone] << endl;
	querySpecs q(wq.queries[wq.whichone]);
	if (wq.isSaving())
		q.setoutputCsv();
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
