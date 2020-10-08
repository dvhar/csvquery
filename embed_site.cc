#include "deps/http/server_http.hpp"
#include <memory>
#include <string_view>
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using namespace std;
static auto localhost = boost::asio::ip::address::from_string("::1");
static bool rejectNonLocals(shared_ptr<HttpServer::Request>& request){
	auto addr = request->remote_endpoint().address();
	if (addr != localhost){
		cerr << "attempted connection from non-localhost: " << addr << endl;
		return true;
	}
	return false;
}
void embedsite(HttpServer &server){
#include "embed_site.hpp"
}
