#include "server.h"
#include "deps/websocket/server_ws.hpp"
#include "deps/websocket/client_ws.hpp"
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

static WsServer server;
static map<i64,shared_ptr<WsServer::Connection>> connections;
static mutex seslock;
static void pingbrowsers();
static atomic_int clientCount{0};

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
		clientCount++;
		cerr << "opened websocket connection " << connection.get() << endl;
	};

	wsocket.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string &) {
		i64 sesid = (i64) connection.get();
		seslock.lock();
		connections.erase(sesid);
		seslock.unlock();
		clientCount--;
		cerr << "closed websocket connection " << connection.get() << endl;
	};

	wsocket.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
		cerr << "Error in connection " << (i64)connection.get() << ": "
			<< ec << ", error message: " << ec.message() << endl;
	};

	auto a = async(pingbrowsers);
	server.start();
	a.get();
}

static void pingbrowsers(){
	string ping(json{{"Type",SK_PING}}.dump());
	int deathcount = 0;
	while(1){
		this_thread::sleep_for(chrono::seconds(1));
		for (auto& c : server.get_connections())
			c->send(ping);
		if (clientCount > 0)
			deathcount = 0;
		else if (++deathcount >= 180)
			exit(0);
	}
}

void sendMessage(i64 sesid, string message){ sendMessage(sesid, message.c_str()); }
void sendMessage(i64 sesid, const char* message){
	seslock.lock();
	if (auto& c = connections[sesid]; c)
		c->send(json{{"Type",SK_MSG},{"Text",message}}.dump());
	seslock.unlock();
}