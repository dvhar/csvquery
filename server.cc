#include <iostream>
#include <string_view>
#include "deps/http/server_http.hpp"
using namespace std;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

void runServer(){
	HttpServer server;
	server.config.port = 8060;
	//#include "embed_site.hpp"
	cerr << "starting http server\n";
	server.start();
}
