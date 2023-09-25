#include "deps/http/server_http.hpp"
#include "deps/incbin/incbin.h"
#include "interpretor.h"
#include <memory>
#include <fstream>
#include <string_view>
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
typedef shared_ptr<HttpServer::Response> Response;
typedef shared_ptr<HttpServer::Request> Request;
using namespace std;

//uncomment to load files from disk when page loads for testing, else they are embedded during compilation
//#define testing_site

#define file1 "../webgui/index.html"
#define file2 "../webgui/main.js"
#define file3 "../webgui/style.css"
#define file4 "../webgui/help.html"

#ifdef testing_site
#define _f(num) st(ifstream(file##num).rdbuf())
#else
#define _ib(num) INCBIN(num,file##num);
_ib(1)
_ib(2)
_ib(3)
_ib(4)
#define _f(num) string_view((const char*)g##num##Data, g##num##Size)
#endif

#define f1 _f(1)
#define f2 _f(2)
#define f3 _f(3)
#define f4 _f(4)

void embedsite(HttpServer &server){
	server.default_resource["GET"] = [](Response response, Request request){ response->write(f1); };
	server.resource["^/main.js$"]["GET"] = [](Response response, Request request){ response->write(f2); };
	server.resource["^/style.css$"]["GET"] = [](Response response, Request request){ response->write(f3); };
	server.resource["^/help.html$"]["GET"] = [](Response response, Request request){ response->write(f4); };
}
