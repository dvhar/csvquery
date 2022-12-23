#include "server.h"
#include "deps/websocket/server_ws.hpp"
#include "deps/websocket/client_ws.hpp"
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

static WsServer server;
static map<i64,shared_ptr<WsServer::Connection>> connections;
static void pingbrowsers();
static atomic_int clientCount{0};
static auto lh = boost::asio::ip::address::from_string("::ffff:127.0.0.1");
#define rejectNonlocals() if (auto a=connection->remote_endpoint().address(); !a.is_loopback() && a != lh) return;

shared_ptr<singleQueryResult> runWebQuery(shared_ptr<webquery> wq){
	cout << "webquery " << wq->whichone << ": " << wq->queries[wq->whichone] << endl;
	querySpecs q(wq->queries[wq->whichone]);
	q.sessionId = wq->sessionId;
	if (wq->isSaving()){
		q.setoutputCsv();
		if (wq->queries.size() > 1){
			q.savepath = st(wq->savepath.substr(0, wq->savepath.size()-4), '-', wq->whichone+1, ".csv");
		} else {
			q.savepath = wq->savepath;
		}
	}
	return runHtmlQuery(q);
}

void runqueries(shared_ptr<webquery> wq){
	boost::split(wq->queries, wq->querystring, boost::is_any_of(";"));
	returnData ret;
	ret.originalQuery = wq->querystring;
	try {
		for (auto &q: wq->queries){
			auto&& singleResult = runWebQuery(wq);
			wq->whichone++;
			if (singleResult == nullptr)
				continue;
			ret.entries.push_back(singleResult);
			if (singleResult->clipped)
				ret.maxclipped = max(ret.maxclipped, singleResult->rowlimit);
		}
		perr("Got result of all queries");
		queryReturn = ret.tohtml();
		if (wq->isSaving()){
			string target;
			if (wq->queries.size() > 1){
				target = st(wq->savepath.substr(0, wq->savepath.size()-4),"-{1..",wq->queries.size(),"}.csv");
			} else {
				target = wq->savepath;
			}
			sendMessage(wq->sessionId, "Saved to " + target);
		}
		else if (ret.maxclipped)
			sendMessage(wq->sessionId, st("Only showing first ",ret.maxclipped," results"));
		if (auto& c = connections[wq->sessionId]; c){
			c->send(json{{"type",SK_DONE}}.dump());
		}
	} catch(...) {
		auto e = "Error: "+EX_STRING;
		cerr << e << endl;
		sendMessage(wq->sessionId, e);
	}
}

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
		case SK_QUERY: {
			stopAllQueries(); //TODO: only stop queries for this session
			shared_ptr<webquery> wq = make_shared<webquery>();
			wq->sessionId = fromjson<i64>(j,"sessionId");
			wq->savepath = fromjson<string>(j, "savePath");
			wq->querystring = regex_replace(fromjson<string>(j,"query"), endSemicolon, "");
			auto th = thread([=](){ runqueries(wq); });
			th.detach();
			break;
		}
		}
	};

	wsocket.on_open = [](shared_ptr<WsServer::Connection> connection) {
		rejectNonlocals();
		i64 sesid = (i64) connection.get();
		connection->send(json{{"type",SK_ID},{"id",sesid}}.dump());
		connections[sesid] = connection;
		clientCount++;
		cerr << "opened websocket connection " << sesid << endl;
	};

	wsocket.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string &) {
		rejectNonlocals();
		i64 sesid = (i64) connection.get();
		connections.erase(sesid);
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
	if (auto& c = connections[sesid]; c)
		c->send(json{{"type",SK_MSG},{"text",message}}.dump());
}
void sendPassPrompt(i64 sesid){
	if (auto& c = connections[sesid]; c)
		c->send(json{{"type",SK_PASS}}.dump());
	perr("sent pass prompt\n");
}
