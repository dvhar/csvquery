#include "deps/http/server_http.hpp"
#include "deps/incbin/incbin.h"
#include <memory>
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


INCBIN(1,"../newgui/index.html");
INCBIN(2,"../newgui/main.js");
INCBIN(3,"../newgui/style.css");

void embedsite(HttpServer &server){
server.default_resource["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g1Data, g1Size)); };
server.resource["^/main.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g2Data, g2Size)); };
server.resource["^/style.css$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g3Data, g3Size)); };
}

