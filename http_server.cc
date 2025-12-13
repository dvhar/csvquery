// combined_server.cc
#include "deps/incbin/incbin.h"
#include "interpretor.h"
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

using namespace std;

// Embedded files
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

// --- App State and Helpers (from http.cc) ---
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
void sendPassPrompt(i64 sesid){}
void sendMessage(i64 sesid, string message){}
void sendMessage(i64 sesid, const char* message){}
// Dummy/open functions for context
// Replace with actual implementations in your codebase
/*struct directory {
    string fpath, parent;
    int mode;
    vector<string> files, dirs;
    json j;
    json& tojson() {
        j = {{"path",fpath},{"parent",parent},{"mode",mode},{"files",files},{"dirs",dirs}};
        return j;
    }
    void setDir(json&) {}
};
shared_ptr<directory> filebrowse(string) { return make_shared<directory>(); }
string version = "1.0";
struct { bool termbox = false; string configfilepath = "/tmp/cfg"; bool allowconnections = false; bool browser = true;} globalSettings;
*/
void openbrowser() {
#if defined _WIN32
    system("cmd /c start http://localhost:8060");
#elif defined __APPLE__
    system("open http://localhost:8060");
#elif defined __linux__
    system("xdg-open http://localhost:8060");
#endif
}

// -- End helpers --

// --- Routing ---
static Route static_routes[] = {
    {"/", get_f1, "text/html"},
    {"/main.js", get_f2, "application/javascript"},
    {"/style.css", get_f3, "text/css"},
    {"/help.html", get_f4, "text/html"},
    {nullptr, nullptr, nullptr}
};

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

// Helper to parse HTTP headers and request line
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
            // trim
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

// -- /info logic (from http.cc) --
static mutex global_mutex; // To guard state

static void handle_info(SOCKET client, const HttpRequest& req) {
    lock_guard<mutex> lock(global_mutex);
    map<string,string> headers = { {"Cache-Control","no-store"}, {"Content-Type","application/json"} };
    string resp = "{}";
    string info;
    if (req.method == "GET" || req.method == "POST") {
        // Info from query string
        auto it = req.query_params.find("info");
        if (it != req.query_params.end()) {
            info = it->second;
        } else {
            // Try from POST body as JSON
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

// --- Main request handler ---
static void handle_request(SOCKET client) {
    char buffer[8192];
    int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return;
    buffer[bytes] = '\0';

    HttpRequest req = parse_http_request(buffer);

    if (req.method != "GET" && req.method != "POST") {
        send_response(client, 405, "Method Not Allowed", "text/plain", "Method Not Allowed");
        return;
    }

    // Static files
    for (int i = 0; static_routes[i].path; i++) {
        if (req.path == static_routes[i].path) {
            send_response(client, 200, "OK", static_routes[i].mime, static_routes[i].handler());
            return;
        }
    }
    // /result/
    if (req.path == "/result/") {
        handle_result(client, req);
        return;
    }
    // /info
    if (req.path == "/info") {
        handle_info(client, req);
        return;
    }

    send_response(client, 404, "Not Found", "text/plain", "Not Found");
}

// Entry point: starts the server
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

        handle_request(client);
        closesocket(client);
    }

    closesocket(server);
#ifdef _WIN32
    WSACleanup();
#endif
}

void runServer(){
	globalSettings.termbox = false;
	run_raw_http_server();
	exit(0);
}

// Example main
/*
int main() {
    globalSettings.termbox = false; // If needed
    // initialize state, etc.
    run_raw_http_server();
    return 0;
}
*/

