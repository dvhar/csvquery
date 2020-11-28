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
IMPORT_BIN("webgui/build/static/js/main.6886388d.chunk.js",webgui_build_static_js_main_6886388d_chunk_js);
extern const char webgui_build_static_js_main_6886388d_chunk_js[], _sizeof_webgui_build_static_js_main_6886388d_chunk_js[];
const size_t webgui_build_static_js_main_6886388d_chunk_js_len = (size_t)_sizeof_webgui_build_static_js_main_6886388d_chunk_js;
IMPORT_BIN("webgui/build/static/js/2.416dc3fd.chunk.js.LICENSE.txt",webgui_build_static_js_2_416dc3fd_chunk_js_LICENSE_txt);
extern const char webgui_build_static_js_2_416dc3fd_chunk_js_LICENSE_txt[], _sizeof_webgui_build_static_js_2_416dc3fd_chunk_js_LICENSE_txt[];
const size_t webgui_build_static_js_2_416dc3fd_chunk_js_LICENSE_txt_len = (size_t)_sizeof_webgui_build_static_js_2_416dc3fd_chunk_js_LICENSE_txt;
IMPORT_BIN("webgui/build/static/js/2.416dc3fd.chunk.js",webgui_build_static_js_2_416dc3fd_chunk_js);
extern const char webgui_build_static_js_2_416dc3fd_chunk_js[], _sizeof_webgui_build_static_js_2_416dc3fd_chunk_js[];
const size_t webgui_build_static_js_2_416dc3fd_chunk_js_len = (size_t)_sizeof_webgui_build_static_js_2_416dc3fd_chunk_js;
IMPORT_BIN("webgui/build/static/js/runtime-main.8587ab9e.js.map",webgui_build_static_js_runtime_main_8587ab9e_js_map);
extern const char webgui_build_static_js_runtime_main_8587ab9e_js_map[], _sizeof_webgui_build_static_js_runtime_main_8587ab9e_js_map[];
const size_t webgui_build_static_js_runtime_main_8587ab9e_js_map_len = (size_t)_sizeof_webgui_build_static_js_runtime_main_8587ab9e_js_map;
IMPORT_BIN("webgui/build/static/js/2.416dc3fd.chunk.js.map",webgui_build_static_js_2_416dc3fd_chunk_js_map);
extern const char webgui_build_static_js_2_416dc3fd_chunk_js_map[], _sizeof_webgui_build_static_js_2_416dc3fd_chunk_js_map[];
const size_t webgui_build_static_js_2_416dc3fd_chunk_js_map_len = (size_t)_sizeof_webgui_build_static_js_2_416dc3fd_chunk_js_map;
IMPORT_BIN("webgui/build/static/js/main.6886388d.chunk.js.map",webgui_build_static_js_main_6886388d_chunk_js_map);
extern const char webgui_build_static_js_main_6886388d_chunk_js_map[], _sizeof_webgui_build_static_js_main_6886388d_chunk_js_map[];
const size_t webgui_build_static_js_main_6886388d_chunk_js_map_len = (size_t)_sizeof_webgui_build_static_js_main_6886388d_chunk_js_map;
IMPORT_BIN("webgui/build/static/js/runtime-main.8587ab9e.js",webgui_build_static_js_runtime_main_8587ab9e_js);
extern const char webgui_build_static_js_runtime_main_8587ab9e_js[], _sizeof_webgui_build_static_js_runtime_main_8587ab9e_js[];
const size_t webgui_build_static_js_runtime_main_8587ab9e_js_len = (size_t)_sizeof_webgui_build_static_js_runtime_main_8587ab9e_js;
IMPORT_BIN("webgui/build/static/css/main.1dc6bc63.chunk.css.map",webgui_build_static_css_main_1dc6bc63_chunk_css_map);
extern const char webgui_build_static_css_main_1dc6bc63_chunk_css_map[], _sizeof_webgui_build_static_css_main_1dc6bc63_chunk_css_map[];
const size_t webgui_build_static_css_main_1dc6bc63_chunk_css_map_len = (size_t)_sizeof_webgui_build_static_css_main_1dc6bc63_chunk_css_map;
IMPORT_BIN("webgui/build/static/css/main.1dc6bc63.chunk.css",webgui_build_static_css_main_1dc6bc63_chunk_css);
extern const char webgui_build_static_css_main_1dc6bc63_chunk_css[], _sizeof_webgui_build_static_css_main_1dc6bc63_chunk_css[];
const size_t webgui_build_static_css_main_1dc6bc63_chunk_css_len = (size_t)_sizeof_webgui_build_static_css_main_1dc6bc63_chunk_css;
IMPORT_BIN("webgui/build/logo512.png",webgui_build_logo512_png);
extern const char webgui_build_logo512_png[], _sizeof_webgui_build_logo512_png[];
const size_t webgui_build_logo512_png_len = (size_t)_sizeof_webgui_build_logo512_png;
IMPORT_BIN("webgui/build/asset-manifest.json",webgui_build_asset_manifest_json);
extern const char webgui_build_asset_manifest_json[], _sizeof_webgui_build_asset_manifest_json[];
const size_t webgui_build_asset_manifest_json_len = (size_t)_sizeof_webgui_build_asset_manifest_json;
IMPORT_BIN("webgui/build/index.html",webgui_build_index_html);
extern const char webgui_build_index_html[], _sizeof_webgui_build_index_html[];
const size_t webgui_build_index_html_len = (size_t)_sizeof_webgui_build_index_html;
IMPORT_BIN("webgui/build/manifest.json",webgui_build_manifest_json);
extern const char webgui_build_manifest_json[], _sizeof_webgui_build_manifest_json[];
const size_t webgui_build_manifest_json_len = (size_t)_sizeof_webgui_build_manifest_json;
IMPORT_BIN("webgui/build/logo192.png",webgui_build_logo192_png);
extern const char webgui_build_logo192_png[], _sizeof_webgui_build_logo192_png[];
const size_t webgui_build_logo192_png_len = (size_t)_sizeof_webgui_build_logo192_png;
IMPORT_BIN("webgui/build/robots.txt",webgui_build_robots_txt);
extern const char webgui_build_robots_txt[], _sizeof_webgui_build_robots_txt[];
const size_t webgui_build_robots_txt_len = (size_t)_sizeof_webgui_build_robots_txt;
void embedsite(HttpServer &server){
server.resource["^/webgui/build/static/js/main.6886388d.chunk.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/static/js/2.416dc3fd.chunk.js.LICENSE.txt$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/static/js/2.416dc3fd.chunk.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/static/js/runtime-main.8587ab9e.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/static/js/2.416dc3fd.chunk.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/static/js/main.6886388d.chunk.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/static/js/runtime-main.8587ab9e.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/static/css/main.1dc6bc63.chunk.css.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/static/css/main.1dc6bc63.chunk.css$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/logo512.png$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/asset-manifest.json$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/index.html$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/manifest.json$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/logo192.png$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

server.resource["^/webgui/build/robots.txt$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(webgui_build_robots_txt, webgui_build_robots_txt_len)); };

}
