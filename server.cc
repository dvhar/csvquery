#include "deps/http/server_http.hpp"
#include "interpretor.h"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

void runServer(){
	HttpServer server;
	server.config.port = 8060;
	server.resource["^/test$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		response->write("testing http server");
	};
#include "embed_site.h"
	cerr << "starting http server\n";
	server.start();
}
