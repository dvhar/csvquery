// http_server.cc - HTTP and WebSocket server in one (raw sockets)
#include "deps/incbin/incbin.h"
#include "interpretor.h"
#include "server.h"
#include <cstring>
#include <string>
#include <string_view>
#include <fstream>
#include <memory>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <atomic>
#include <chrono>
#include <regex>
#include <algorithm>

// For SHA1 (websocket handshake)
#include "deps/crypto/sha1.h"

using namespace std;

// Platform networking headers
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

// ------------ Embedded Static Files ---------------
#define file1 "../webgui/index.html"
#define file2 "../webgui/main.js"
#define file3 "../webgui/style.css"
#define file4 "../webgui/help.html"
#define _ib(num) INCBIN(num,file##num);
_ib(1)
_ib(2)
_ib(3)
_ib(4)
#define _f(num) string_view((const char*)g##num##Data, g##num##Size)

struct Route {
    const char* path;
    string_view (*handler)();
    const char* mime;
};

static string_view get_f1() { return _f(1); }
static string_view get_f2() { return _f(2); }
static string_view get_f3() { return _f(3); }
static string_view get_f4() { return _f(4); }

static Route static_routes[] = {
    {"/", get_f1, "text/html"},
    {"/main.js", get_f2, "application/javascript"},
    {"/style.css", get_f3, "text/css"},
    {"/help.html", get_f4, "text/html"},
    {nullptr, nullptr, nullptr}
};

// ------------- App State and Helpers ---------------
static json state;
static string queryReturn;

void directory::setDir(json& j){
    fpath = fromjson<string>(j,"path");
    parent = fromjson<string>(j,"parent");
    mode = fromjson<int>(j,"mode");
    files = fromjson<vector<string>>(j,"files");
    dirs = fromjson<vector<string>>(j,"dirs");
}

json& directory::tojson(){
    j = {
        {"path",fpath},
        {"parent",parent},
        {"mode",mode},
        {"files",files},
        {"dirs",dirs},
    };
    return j;
}

void openbrowser() {
#if defined _WIN32
    system("cmd /c start http://localhost:8060");
#elif defined __APPLE__
    system("open http://localhost:8060");
#elif defined __linux__
    system("xdg-open http://localhost:8060");
#endif
}

// ------------- HTTP Parsing Helpers ---------------
struct HttpRequest {
    string method, path, version;
    map<string,string> headers;
    string body;
    map<string,string> query_params;
};

static HttpRequest parse_http_request(const char* buffer) {
    istringstream stream(buffer);
    HttpRequest req;
    string line;
    getline(stream, line);
    istringstream l(line);
    l >> req.method >> req.path >> req.version;

    // Parse query params in path
    auto qpos = req.path.find('?');
    if (qpos != string::npos) {
        string params = req.path.substr(qpos+1);
        req.path = req.path.substr(0, qpos);
        istringstream qp(params);
        string kv;
        while (getline(qp, kv, '&')) {
            auto eq = kv.find('=');
            if (eq != string::npos) {
                req.query_params[kv.substr(0,eq)] = kv.substr(eq+1);
            }
        }
    }

    // Headers
    while (getline(stream, line) && line != "\r") {
        if (line.empty() || line == "\n") break;
        auto col = line.find(':');
        if (col != string::npos) {
            string key = line.substr(0,col);
            string value = line.substr(col+1);
            while (!value.empty() && (value[0]==' ' || value[0]=='\t')) value.erase(0,1);
            value.erase(value.find_last_not_of("\r\n")+1);
            req.headers[key] = value;
        }
    }
    // Body
    if (req.headers.count("Content-Length")) {
        int len = stoi(req.headers["Content-Length"]);
        req.body.resize(len);
        stream.read(&req.body[0], len);
    }
    return req;
}

// ------------- HTTP Response Helper ---------------
static void send_response(SOCKET client, int code, const char* status, 
                         const char* mime, string_view body, 
                         const map<string,string>& extra_headers = {}) {
    string header = "HTTP/1.1 " + to_string(code) + " " + status + "\r\n"
                   "Content-Type: " + string(mime) + "\r\n"
                   "Content-Length: " + to_string(body.size()) + "\r\n"
                   "Connection: close\r\n";
    for (auto& [k,v]: extra_headers)
        header += k + ": " + v + "\r\n";
    header += "\r\n";
    send(client, header.c_str(), header.size(), 0);
    if (!body.empty())
        send(client, body.data(), body.size(), 0);
}

// ------------- WebSocket Support (Raw) -------------
// Simple base64 encoding (for handshake)
static string base64_encode(const unsigned char* data, size_t len) {
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    string ret;
    for (size_t i = 0; i < len; i += 3) {
        unsigned char b[3] = {0, 0, 0};
        size_t n = min(len - i, (size_t)3);
        for (size_t j = 0; j < n; ++j) b[j] = data[i + j];
        ret += table[b[0] >> 2];
        ret += table[((b[0] & 3) << 4) | (b[1] >> 4)];
        ret += (n > 1) ? table[((b[1] & 0xf) << 2) | (b[2] >> 6)] : '=';
        ret += (n > 2) ? table[b[2] & 0x3f] : '=';
    }
    return ret;
}

// WebSocket handshake
static void websocket_handshake(SOCKET client, const HttpRequest& req) {
    auto it = req.headers.find("Sec-WebSocket-Key");
    if (it == req.headers.end()) {
        send_response(client, 400, "Bad Request", "text/plain", "Missing Sec-WebSocket-Key");
        return;
    }
    string key = it->second;
    string guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    string accept_src = key + guid;
    unsigned char sha1[20];
    SHA1_CTX ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (const BYTE*)accept_src.c_str(), accept_src.size());
    sha1_final(&ctx, sha1);
    string accept = base64_encode(sha1, 20);

    string response = "HTTP/1.1 101 Switching Protocols\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
    send(client, response.c_str(), response.size(), 0);
}

// WebSocket frame read/write
static string websocket_read_frame(SOCKET client, int& opcode, bool& closed) {
    unsigned char hdr[2];
    int n = recv(client, (char*)hdr, 2, MSG_WAITALL);
    if (n != 2) { closed = true; return ""; }
    bool fin = hdr[0] & 0x80;
    opcode = hdr[0] & 0x0F;
    bool mask = hdr[1] & 0x80;
    uint64_t len = hdr[1] & 0x7F;

    if (len == 126) {
        unsigned char ext[2];
        recv(client, (char*)ext, 2, MSG_WAITALL);
        len = (ext[0] << 8) | ext[1];
    } else if (len == 127) {
        unsigned char ext[8];
        recv(client, (char*)ext, 8, MSG_WAITALL);
        len = 0;
        for (int i=0; i<8; ++i) len = (len << 8) | ext[i];
    }
    unsigned char maskkey[4] = {0,0,0,0};
    if (mask)
        recv(client, (char*)maskkey, 4, MSG_WAITALL);

    string payload(len, '\0');
    size_t got = 0;
    while (got < len) {
        int nb = recv(client, &payload[got], len-got, 0);
        if (nb <= 0) { closed = true; return ""; }
        got += nb;
    }
    if (mask) {
        for (size_t i=0; i<len; ++i)
            payload[i] ^= maskkey[i%4];
    }
    return payload;
}

static void websocket_send_frame(SOCKET client, int opcode, const string& data) {
    vector<unsigned char> frame;
    frame.push_back(0x80 | (opcode & 0xF));
    if (data.size() < 126) {
        frame.push_back(data.size());
    } else if (data.size() <= 0xFFFF) {
        frame.push_back(126);
        frame.push_back((data.size() >> 8) & 0xFF);
        frame.push_back(data.size() & 0xFF);
    } else {
        frame.push_back(127);
        for (int i=7; i>=0; --i)
            frame.push_back((data.size() >> (8*i)) & 0xFF);
    }
    frame.insert(frame.end(), data.begin(), data.end());
    send(client, (const char*)frame.data(), frame.size(), 0);
}

// --------- WebSocket Session Management -----------
static mutex ws_mutex;
static map<i64, SOCKET> ws_clients;
static atomic<i64> next_ws_id{1};
static atomic<int> ws_client_count{0};

// --------- WebSocket Message Protocol -------------
#define SK_STOP 1
#define SK_PASS 2
#define SK_QUERY 3
#define SK_ID 4
#define SK_MSG 5
#define SK_DONE 6
#define SK_PING 7

// Forward declarations for functions used from your codebase
// (implementations must be linked elsewhere!)
extern void stopAllQueries();
extern void returnPassword(i64 sessionId, string pass);
extern void runqueries(shared_ptr<webquery> wq); // takes sessionId, savepath, querystring
extern string version;
extern shared_ptr<directory> filebrowse(string);
extern string fs_current_path();
extern regex endSemicolon;

// --------- sendMessage/sendPassPrompt for WS ------
void sendMessage(i64 wsid, string&& message) {
    lock_guard<mutex> lock(ws_mutex);
    auto it = ws_clients.find(wsid);
    if (it != ws_clients.end()) {
        string msg = json{{"type",SK_MSG},{"text",message}}.dump();
        websocket_send_frame(it->second, 0x1, msg);
    }
}
void sendPassPrompt(i64 wsid) {
    lock_guard<mutex> lock(ws_mutex);
    auto it = ws_clients.find(wsid);
    if (it != ws_clients.end()) {
        string msg = json{{"type",SK_PASS}}.dump();
        websocket_send_frame(it->second, 0x1, msg);
    }
}

// ------------- WebSocket Main Loop -----------------
static void websocket_main_loop(SOCKET client, i64 wsid) {
    ws_client_count++;
    try {
        // Send SK_ID
        string msg = json{{"type",SK_ID},{"id",wsid}}.dump();
        websocket_send_frame(client, 0x1, msg);
        bool closed = false;
        while (!closed) {
            int opcode = 0;
            string payload = websocket_read_frame(client, opcode, closed);
            if (closed) break;
            if (opcode == 0x8) { // CLOSE
                break;
            } else if (opcode == 0x9) { // PING
                websocket_send_frame(client, 0xA, payload); // PONG
            } else if (opcode == 0x1) { // TEXT
                // Parse as JSON, handle as in ws.cpp
                try {
                    auto j = json::parse(payload);
                    int type = fromjson<int>(j,"type");
                    if (type == SK_STOP) {
                        stopAllQueries();
                    } else if (type == SK_PASS) {
                        returnPassword(wsid, fromjson<string>(j,"text"));
                    } else if (type == SK_QUERY) {
                        stopAllQueries();
                        shared_ptr<webquery> wq = make_shared<webquery>();
                        wq->sessionId = wsid;
                        wq->savepath = fromjson<string>(j, "savePath");
                        wq->querystring = regex_replace(fromjson<string>(j,"query"), endSemicolon, "");
                        auto th = thread([=](){ runqueries(wq); });
                        th.detach();
                    }
                } catch (...) {
                    // Ignore parse errors / bad messages
                }
            }
            // else ignore
        }
    } catch (...) {
        // Handle unexpected exceptions per client
    }
    {
        lock_guard<mutex> lock(ws_mutex);
        ws_clients.erase(wsid);
    }
    ws_client_count--;
    if (ws_client_count < 0) ws_client_count = 0;
}

// ------------- HTTP /info and /result logic --------------
static mutex global_mutex; // To guard state

static void handle_info(SOCKET client, const HttpRequest& req) {
    lock_guard<mutex> lock(global_mutex);
    map<string,string> headers = { {"Cache-Control","no-store"}, {"Content-Type","application/json"} };
    string resp = "{}";
    string info;
    if (req.method == "GET" || req.method == "POST") {
        auto it = req.query_params.find("info");
        if (it != req.query_params.end()) {
            info = it->second;
        } else {
            try {
                auto j = json::parse(req.body);
                if (j.contains("info"))
                    info = j["info"].get<string>();
            } catch(...) {}
        }
        if (info == "setState") {
            try {
                state = json::parse(req.body);
            } catch(...) {}
            resp = "{}";
        } else if (info == "getState") {
            resp = state.dump();
        } else if (info == "fileClick") {
            try {
                auto j = json::parse(req.body);
                string mode = fromjson<string>(j,"mode");
                auto newlist = filebrowse(fromjson<string>(j,"path"));
                newlist->mode = fromjson<string>(j,"mode");
                j = newlist->tojson();
                if (mode == "open") {
                    state["openDirList"] = j;
                } else if (mode == "save") {
                    state["saveDirList"] = j;
                }
                resp = j.dump();
            } catch (...) {
                auto e = EX_STRING;
                cerr << e << endl;
                resp = json{{"error",e}}.dump();
            }
        }
    }
    send_response(client, 200, "OK", "application/json", resp, headers);
}

static void handle_result(SOCKET client, const HttpRequest& req) {
    map<string,string> headers = { {"Cache-Control","no-store"}, {"Content-Type", "text/plain"} };
    send_response(client, 200, "OK", "text/plain", queryReturn, headers);
}

// ------------- Main request handler ---------------------
static void handle_request(SOCKET client) {
    char buffer[8192];
    int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return;
    buffer[bytes] = '\0';

    HttpRequest req = parse_http_request(buffer);

    // --------- WebSocket Upgrade ---------
    if ((req.path == "/socket" || req.path == "/socket/") && 
        req.headers.count("Upgrade") && 
        req.headers.at("Upgrade") == "websocket")
    {
        websocket_handshake(client, req);
        i64 wsid = next_ws_id++;
        {
            lock_guard<mutex> lock(ws_mutex);
            ws_clients[wsid] = client;
        }
        websocket_main_loop(client, wsid);
        return;
    }

    if (req.method != "GET" && req.method != "POST") {
        send_response(client, 405, "Method Not Allowed", "text/plain", "Method Not Allowed");
        return;
    }

    // --------- Static files ---------
    for (int i = 0; static_routes[i].path; i++) {
        if (req.path == static_routes[i].path) {
            send_response(client, 200, "OK", static_routes[i].mime, static_routes[i].handler());
            return;
        }
    }
    // --------- /result/ ---------
    if (req.path == "/result/") {
        handle_result(client, req);
        return;
    }
    // --------- /info ---------
    if (req.path == "/info") {
        handle_info(client, req);
        return;
    }

    send_response(client, 404, "Not Found", "text/plain", "Not Found");
}

// --------------- Server entry point -------------------
void run_raw_http_server(int port = 8060) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    auto startdir = filebrowse(fs_current_path());
    state["startDirlist"] = startdir->tojson();
    state["version"] = version;
    state["configpath"] = globalSettings.configfilepath;

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        cerr << "Could not create socket\n";
        return;
    }
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "Could not bind\n";
        closesocket(server);
        return;
    }

    if (listen(server, 10) == SOCKET_ERROR) {
        cerr << "Could not listen\n";
        closesocket(server);
        return;
    }
    cout << "Go to http://localhost:" << port << " to use graphic interface\n";
    cout << "Allow connections from other computers: " << (globalSettings.allowconnections ? "true":"false") << endl;
    if (globalSettings.browser)
        openbrowser();

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        SOCKET client = accept(server, (sockaddr*)&client_addr, &client_len);

        if (client == INVALID_SOCKET) continue;

        // Handle each client in a new thread
        thread([client]() {
            handle_request(client);
            closesocket(client);
        }).detach();
    }

    closesocket(server);
#ifdef _WIN32
    WSACleanup();
#endif
}

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
        {
          lock_guard<mutex> lock(ws_mutex);
          auto it = ws_clients.find(wq->sessionId);
          if (it != ws_clients.end()) {
            string msg = json{{"type",SK_DONE}}.dump();
            websocket_send_frame(it->second, 0x1, msg); // 0x1 = text frame
          }
        }
	} catch(...) {
		auto e = "Error: "+EX_STRING;
		cerr << e << endl;
		sendMessage(wq->sessionId, string(e));
	}
}


void runServer(){
    globalSettings.termbox = false;
    run_raw_http_server();
    exit(0);
}
