#include "server.h"

void runServer(){
	HttpServer server;
	server.config.port = 8060;
	embed(server);
	cerr << "starting http server\n"sv;
	server.start();
}
