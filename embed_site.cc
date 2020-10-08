#include "deps/http/server_http.hpp"
#include <memory>
#include <string_view>
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using namespace std;
static auto localhost = boost::asio::ip::address::from_string("::1");
static bool rejectNonLocals(shared_ptr<HttpServer::Request>& request){
	if (request->remote_endpoint().address() != localhost){
		cerr << "attempted connection from non-localhost\n";
		return true;
	}
	return false;
}
void embedsite(HttpServer &server){
#include "embed_site.hpp"
}
