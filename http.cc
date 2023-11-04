#include "interpretor.h"
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
static void serve();
static SimpleWeb::CaseInsensitiveMultimap header;
string queryReturn;
void embedsite(HttpServer&);

void runServer(){
	globalSettings.termbox = false;
	serve();
	exit(0);
}

static void serve(){

	auto startdir = filebrowse(boost::filesystem::current_path().string());
	state["startDirlist"] = startdir->tojson();
	state["version"] = version;
	state["configpath"] = globalSettings.configfilepath;
	server.config.port = 8060;
	if (!globalSettings.allowconnections)
		server.config.address = "127.0.0.1";
	header.emplace("Cache-Control","no-store");
	header.emplace("Content-Type", "text/plain");
	embedsite(server);

	server.resource["^/result/$"]["GET"] = [&](Response response, Request request){
			response->write(queryReturn, header);
	};

	server.resource["^/info$"]["GET"] = 
	server.resource["^/info$"]["POST"] = [&](Response response, Request request){

		auto params = request->parse_query_string();
		auto info = params.find("info")->second;

		if (info == "setState"){
			state = json::parse(request->content.string());
			response->write("{}");
		} else if (info == "getState"){
			response->write(state.dump(), header);
		} else if (info == "fileClick"){
			auto j = json::parse(request->content.string());
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
	cout << "Go to http://localhost:8060 to use graphic interface\n"
	     << "Allow connections from other computers: " << (globalSettings.allowconnections ? "true":"false") << endl;
	auto ws = async(servews);
	auto hs = async([](){server.start();});
	if (globalSettings.browser){
		openbrowser();
	}
	ws.get();
	server.stop();
	hs.get();
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
