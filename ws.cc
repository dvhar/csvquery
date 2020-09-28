#include "server.h"
#include "deps/websocket/server_ws.hpp"
#include "deps/websocket/client_ws.hpp"
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

static WsServer server;
static map<i64,shared_ptr<WsServer::Connection>> connections;
static mutex seslock;

void servews(){
	server.config.port = 8061;

	auto &wsocket = server.endpoint["^/socket/?$"];

	wsocket.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
		auto out_message = in_message->string();

		cerr << "Server: Message received: \"" << out_message << "\" from " << (i64)connection.get() << endl;
		connection->send(out_message);
	};


	wsocket.on_open = [](shared_ptr<WsServer::Connection> connection) {
		i64 sesid = (i64) connection.get();
		seslock.lock();
		connections[sesid] = connection;
		seslock.unlock();
		connection->send(json{{"Type",SK_ID},{"Id",sesid}}.dump());
	};

	wsocket.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string &) {
		i64 sesid = (i64) connection.get();
		seslock.lock();
		connections.erase(sesid);
		seslock.unlock();
	};

	wsocket.on_handshake = [](shared_ptr<WsServer::Connection>, SimpleWeb::CaseInsensitiveMultimap &) {
		return SimpleWeb::StatusCode::information_switching_protocols;
	};

	wsocket.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
		cerr << "Server: Error in connection " << (i64)connection.get() << ". "
				 << "Error: " << ec << ", error message: " << ec.message() << endl;
	};

	promise<unsigned short> server_port;
	auto a = async([&](){
		server.start([&server_port](unsigned short port) {
			server_port.set_value(port);
		});
});
	cerr << "ws Server listening on port '" << server_port.get_future().get() << "'" << endl;
		 a.get();
}

void sendMessageSock(i64 sesid, char* message){
	seslock.lock();
	auto conn = connections[sesid];
	if (conn){
		conn->send(json{{"Type",SK_MSG},{"Text",message}}.dump());
	}
	seslock.unlock();
}
