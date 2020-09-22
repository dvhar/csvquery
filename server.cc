#include "server.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>

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
		ptree pt;
		webquery wq;
		SimpleWeb::CaseInsensitiveMultimap header;

		try {
			read_json(request->content, pt);
			wq.savepath = pt.get("Savepath","");
			wq.qamount = pt.get("Qamount",0);
			wq.fileIO = pt.get("FileIO",0);
			wq.querystring = regex_replace(pt.get("Query",""), endSemicolon, "");

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

			auto ret = make_shared<returnData>();
			ret->status = DAT_ERROR;
			ret->message = move(e);
			ret->originalQuery = move(wq.querystring);
			response->write(ret->tojson().dump());
		}

	}; // end /query/

	server.resource["^/info$"]["POST"] = [&](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){

		ptree pt;
		auto params = request->parse_query_string();
		auto info = params.find("info")->second;
		if (info == "setState"){
			read_json(request->content, pt);
			state.setState(pt);
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

void stateInfo::setState(ptree& pt){
	haveInfo = pt.get("haveInfo",false);
	history.clear();
	for (auto h : pt.get_child("history"))
		history.push_back(h.second.data());
	openDirList.setDir(pt.get_child("openDirList"));
	saveDirList.setDir(pt.get_child("saveDirList"));
}

void directory::setDir(ptree& pt){
	fpath = pt.get("Path","");
	parent = pt.get("Parent","");
	mode = pt.get("Mode","");
	files.clear();
	dirs.clear();
	for (auto h : pt.get_child("Files"))
		files.push_back(h.second.data());
	for (auto h : pt.get_child("Dirs"))
		dirs.push_back(h.second.data());
}

json& stateInfo::tojson(){
	j = {
		{"HaveInfo",haveInfo},
		{"History",history},
		{"OpenDirList",openDirList.tojson()},
		{"SaveDirList",saveDirList.tojson()},
	};
	return j;
}
json& directory::tojson(){
	j = {
		{"Path",fpath},
		{"Parent",parent},
		{"Mode",mode},
		{"files",files},
		{"dirs",dirs},
	};
	return j;
}
