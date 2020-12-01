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
static auto lh = boost::asio::ip::address::from_string("::ffff:127.0.0.1");
#define rejectNonlocals() if (auto a=connection->remote_endpoint().address(); !a.is_loopback() && a != lh) return;

void servews(){
	server.config.port = 8061;
	auto &wsocket = server.endpoint["^/socket/?$"];

	wsocket.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
		rejectNonlocals();
		auto j = json::parse(in_message->string());
		switch (fromjson<int>(j,"type")){
		case SK_STOP:
			stopAllQueries();
			cerr << "stopped queries\n";
			break;
		case SK_PASS:
			returnPassword((i64)connection.get(), fromjson<string>(j,"text"));
			break;
		}
	};

	wsocket.on_open = [](shared_ptr<WsServer::Connection> connection) {
		rejectNonlocals();
		i64 sesid = (i64) connection.get();
		connection->send(json{{"type",SK_ID},{"id",sesid}}.dump());
		seslock.lock();
		connections[sesid] = connection;
		seslock.unlock();
		clientCount++;
		cerr << "opened websocket connection " << connection.get() << endl;
	};

	wsocket.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string &) {
		rejectNonlocals();
		i64 sesid = (i64) connection.get();
		seslock.lock();
		connections.erase(sesid);
		seslock.unlock();
		clientCount--;
		if (clientCount < 0)
			clientCount = 0;
		cerr << "closed websocket connection " << connection.get() << endl;
	};

	wsocket.on_error = [&](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
		rejectNonlocals();
		cerr << "Error in connection " << (i64)connection.get() << ": "
			<< ec << ", error message: " << ec.message() << endl;
		wsocket.on_close(connection, 0, {});
	};

	auto a = async(pingbrowsers);
	server.start();
	a.get();
}

static void pingbrowsers(){
	string ping(json{{"type",SK_PING}}.dump());
	int deathcount = 0;
	while(1){
		this_thread::sleep_for(chrono::seconds(1));
		for (auto& c : server.get_connections())
			c->send(ping);
		if (clientCount > 0)
			deathcount = 0;
		else if (++deathcount >= 180 && globalSettings.autoexit){
			server.stop();
			stopAllQueries();
			return;
		}
	}
}

void sendMessage(i64 sesid, string message){ sendMessage(sesid, message.c_str()); }
void sendMessage(i64 sesid, const char* message){
	seslock.lock();
	if (auto& c = connections[sesid]; c)
		c->send(json{{"type",SK_MSG},{"text",message}}.dump());
	seslock.unlock();
}
void sendPassPrompt(i64 sesid){
	seslock.lock();
	if (auto& c = connections[sesid]; c)
		c->send(json{{"type",SK_PASS}}.dump());
	seslock.unlock();
	perr("sent pass prompt\n");
}
