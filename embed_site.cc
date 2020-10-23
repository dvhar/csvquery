#include "deps/http/server_http.hpp"
#include <memory>
#include <string_view>
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using namespace std;
static bool rejectNonLocals(shared_ptr<HttpServer::Request>& request){
	auto addr = request->remote_endpoint().address();
	if (!addr.is_loopback()){
		cerr << "attempted connection from non-localhost: " << addr << endl;
		return true;
	}
	return false;
}
void embedsite(HttpServer &server){
#include "embed_site.hpp"
}
