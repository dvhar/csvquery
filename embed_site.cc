#include "deps/http/server_http.hpp"
#include <memory>
#include <string_view>
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using namespace std;
void embedsite(HttpServer &server){
#include "embed_site.hpp"
}
