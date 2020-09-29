#include "server.h"
#include "deps/websocket/server_ws.hpp"
#include "deps/websocket/client_ws.hpp"
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

static WsServer server;
static map<i64,shared_ptr<WsServer::Connection>> connections;
static mutex seslock;
void pingbrowsers();

void servews(){
	server.config.port = 8061;

	auto &wsocket = server.endpoint["^/socket/?$"];

	wsocket.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
		auto j = json::parse(in_message->string());
		switch (fromjson<int>(j,"Type")){
		case SK_STOP:
			stopAllQueries();
			cerr << "stopped queries\n";
		}
	};

	wsocket.on_open = [](shared_ptr<WsServer::Connection> connection) {
		i64 sesid = (i64) connection.get();
		connection->send(json{{"Type",SK_ID},{"Id",sesid}}.dump());
		seslock.lock();
		connections[sesid] = connection;
		seslock.unlock();
		cerr << "opened websocket connection " << connection.get() << endl;
	};

	wsocket.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string &) {
		i64 sesid = (i64) connection.get();
		seslock.lock();
		connections.erase(sesid);
		seslock.unlock();
		cerr << "closed websocket connection " << connection.get() << endl;
	};

	wsocket.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
		cerr << "Error in connection " << (i64)connection.get() << ": "
			<< ec << ", error message: " << ec.message() << endl;
	};

	auto a = async([&](){server.start();});
	auto b = async(pingbrowsers);
	a.get();
	b.get();
}

void pingbrowsers(){
	string ping(json{{"Type",SK_PING}}.dump());
	while(1){
		sleep(1);
		for (auto& c : server.get_connections())
			c->send(ping);
	}
}

void sendMessageSock(i64 sesid, char* message){
	seslock.lock();
	auto conn = connections[sesid];
	if (conn){
		conn->send(json{{"Type",SK_MSG},{"Text",message}}.dump());
	}
	seslock.unlock();
}
