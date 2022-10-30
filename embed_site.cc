#include "deps/http/server_http.hpp"
#include "deps/incbin/incbin.h"
#include "interpretor.h"
#include <memory>
#include <fstream>
#include <string_view>
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using namespace std;
static bool rejectNonLocals(shared_ptr<HttpServer::Request>& request){
	static auto lh = boost::asio::ip::address::from_string("::ffff:127.0.0.1");
	auto addr = request->remote_endpoint().address();
	if (!addr.is_loopback() && addr != lh){
		cerr << "attempted connection from non-localhost: " << addr << endl;
		return true;
	}
	return false;
}

#define file1 "../newgui/index.html"
#define file2 "../newgui/main.js"
#define file3 "../newgui/style.css"

#define testing_site
#ifdef testing_site
#define _f(num) st(ifstream(file##num).rdbuf())
#else
#define _ib(num) INCBIN(num,file##num);
_ib(1)
_ib(2)
_ib(3)
#define _f(num) string_view((const char*)g##num##Data, g##num##Size)
#endif

#define f1 _f(1)
#define f2 _f(2)
#define f3 _f(3)

void embedsite(HttpServer &server){
	server.default_resource["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(f1); };
	server.resource["^/main.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(f2); };
	server.resource["^/style.css$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(f3); };
}
