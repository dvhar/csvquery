#include "deps/http/server_http.hpp"
#include <memory>
#include <string_view>
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using namespace std;
static auto localhost = boost::asio::ip::address::from_string("::1");
static auto localhost2 = boost::asio::ip::address::from_string("127.0.0.1");
static auto localhost3 = boost::asio::ip::address::from_string("::ffff:127.0.0.1");
static bool rejectNonLocals(shared_ptr<HttpServer::Request>& request){
	auto addr = request->remote_endpoint().address();
	if (addr != localhost && addr != localhost2 && addr != localhost3){
		cerr << "attempted connection from non-localhost: " << addr << endl;
		return true;
	}
	return false;
}
void embedsite(HttpServer &server){
#include "embed_site.hpp"
}
