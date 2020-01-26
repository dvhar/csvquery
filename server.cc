#include "deps/http/server_http.hpp"
#include "interpretor.h"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

void runServer(){
	HttpServer server;
	server.config.port = 8060;
	#include "embed_site.hpp"
	cerr << "starting http server\n";
	server.start();
}
