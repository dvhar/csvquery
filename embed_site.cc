#include "deps/http/server_http.hpp"
#include <memory>
#include <string_view>
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using namespace std;
static bool rejectNonLocals(shared_ptr<HttpServer::Request>& request){
	static auto lh = boost::asio::ip::address::from_string("::ffff:127.0.0.1");
	auto addr = request->remote_endpoint().address();
	if (!addr.is_loopback() && addr != lh){
		cerr << "attempted connection from non-localhost: " << addr << endl;
		return true;
	}
	return false;
}
#include "embed_site.hpp"
