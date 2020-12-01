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
#define IMPORT_BIN(file, sym) asm (\
    ".section .rodata\n"                 \
    ".balign 4\n"                        \
    ".global " #sym "\n"                 \
    #sym ":\n"                           \
    ".incbin \"" file "\"\n"             \
    ".global _sizeof_" #sym "\n"         \
    ".set _sizeof_" #sym ", . - " #sym "\n"\
    ".balign 4\n"                        \
    ".section \".text\"\n")

#include "embed_site.hpp"
