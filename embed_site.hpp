IMPORT_BIN("../webgui/build/static/js/2.416dc3fd.chunk.js.LICENSE.txt",_2_416dc3fd_chunk_js_LICENSE_txt);
extern const char _2_416dc3fd_chunk_js_LICENSE_txt[], _sizeof__2_416dc3fd_chunk_js_LICENSE_txt[];
const size_t _2_416dc3fd_chunk_js_LICENSE_txt_len = (size_t)_sizeof__2_416dc3fd_chunk_js_LICENSE_txt;
IMPORT_BIN("../webgui/build/static/js/2.416dc3fd.chunk.js",_2_416dc3fd_chunk_js);
extern const char _2_416dc3fd_chunk_js[], _sizeof__2_416dc3fd_chunk_js[];
const size_t _2_416dc3fd_chunk_js_len = (size_t)_sizeof__2_416dc3fd_chunk_js;
IMPORT_BIN("../webgui/build/static/js/runtime-main.8587ab9e.js.map",_runtime_main_8587ab9e_js_map);
extern const char _runtime_main_8587ab9e_js_map[], _sizeof__runtime_main_8587ab9e_js_map[];
const size_t _runtime_main_8587ab9e_js_map_len = (size_t)_sizeof__runtime_main_8587ab9e_js_map;
IMPORT_BIN("../webgui/build/static/js/2.416dc3fd.chunk.js.map",_2_416dc3fd_chunk_js_map);
extern const char _2_416dc3fd_chunk_js_map[], _sizeof__2_416dc3fd_chunk_js_map[];
const size_t _2_416dc3fd_chunk_js_map_len = (size_t)_sizeof__2_416dc3fd_chunk_js_map;
IMPORT_BIN("../webgui/build/static/js/main.8d990a04.chunk.js",_main_8d990a04_chunk_js);
extern const char _main_8d990a04_chunk_js[], _sizeof__main_8d990a04_chunk_js[];
const size_t _main_8d990a04_chunk_js_len = (size_t)_sizeof__main_8d990a04_chunk_js;
IMPORT_BIN("../webgui/build/static/js/main.8d990a04.chunk.js.map",_main_8d990a04_chunk_js_map);
extern const char _main_8d990a04_chunk_js_map[], _sizeof__main_8d990a04_chunk_js_map[];
const size_t _main_8d990a04_chunk_js_map_len = (size_t)_sizeof__main_8d990a04_chunk_js_map;
IMPORT_BIN("../webgui/build/static/js/runtime-main.8587ab9e.js",_runtime_main_8587ab9e_js);
extern const char _runtime_main_8587ab9e_js[], _sizeof__runtime_main_8587ab9e_js[];
const size_t _runtime_main_8587ab9e_js_len = (size_t)_sizeof__runtime_main_8587ab9e_js;
IMPORT_BIN("../webgui/build/static/css/main.1dc6bc63.chunk.css.map",_main_1dc6bc63_chunk_css_map);
extern const char _main_1dc6bc63_chunk_css_map[], _sizeof__main_1dc6bc63_chunk_css_map[];
const size_t _main_1dc6bc63_chunk_css_map_len = (size_t)_sizeof__main_1dc6bc63_chunk_css_map;
IMPORT_BIN("../webgui/build/static/css/main.1dc6bc63.chunk.css",_main_1dc6bc63_chunk_css);
extern const char _main_1dc6bc63_chunk_css[], _sizeof__main_1dc6bc63_chunk_css[];
const size_t _main_1dc6bc63_chunk_css_len = (size_t)_sizeof__main_1dc6bc63_chunk_css;
IMPORT_BIN("../webgui/build/logo512.png",_logo512_png);
extern const char _logo512_png[], _sizeof__logo512_png[];
const size_t _logo512_png_len = (size_t)_sizeof__logo512_png;
IMPORT_BIN("../webgui/build/asset-manifest.json",_asset_manifest_json);
extern const char _asset_manifest_json[], _sizeof__asset_manifest_json[];
const size_t _asset_manifest_json_len = (size_t)_sizeof__asset_manifest_json;
IMPORT_BIN("../webgui/build/index.html",_index_html);
extern const char _index_html[], _sizeof__index_html[];
const size_t _index_html_len = (size_t)_sizeof__index_html;
IMPORT_BIN("../webgui/build/manifest.json",_manifest_json);
extern const char _manifest_json[], _sizeof__manifest_json[];
const size_t _manifest_json_len = (size_t)_sizeof__manifest_json;
IMPORT_BIN("../webgui/build/logo192.png",_logo192_png);
extern const char _logo192_png[], _sizeof__logo192_png[];
const size_t _logo192_png_len = (size_t)_sizeof__logo192_png;
IMPORT_BIN("../webgui/build/robots.txt",_robots_txt);
extern const char _robots_txt[], _sizeof__robots_txt[];
const size_t _robots_txt_len = (size_t)_sizeof__robots_txt;
void embedsite(HttpServer &server){
server.resource["^/static/js/2.416dc3fd.chunk.js.LICENSE.txt$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_2_416dc3fd_chunk_js_LICENSE_txt, _2_416dc3fd_chunk_js_LICENSE_txt_len)); };

server.resource["^/static/js/2.416dc3fd.chunk.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_2_416dc3fd_chunk_js, _2_416dc3fd_chunk_js_len)); };

server.resource["^/static/js/runtime-main.8587ab9e.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_runtime_main_8587ab9e_js_map, _runtime_main_8587ab9e_js_map_len)); };

server.resource["^/static/js/2.416dc3fd.chunk.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_2_416dc3fd_chunk_js_map, _2_416dc3fd_chunk_js_map_len)); };

server.resource["^/static/js/main.8d990a04.chunk.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_main_8d990a04_chunk_js, _main_8d990a04_chunk_js_len)); };

server.resource["^/static/js/main.8d990a04.chunk.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_main_8d990a04_chunk_js_map, _main_8d990a04_chunk_js_map_len)); };

server.resource["^/static/js/runtime-main.8587ab9e.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_runtime_main_8587ab9e_js, _runtime_main_8587ab9e_js_len)); };

server.resource["^/static/css/main.1dc6bc63.chunk.css.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_main_1dc6bc63_chunk_css_map, _main_1dc6bc63_chunk_css_map_len)); };

server.resource["^/static/css/main.1dc6bc63.chunk.css$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_main_1dc6bc63_chunk_css, _main_1dc6bc63_chunk_css_len)); };

server.resource["^/logo512.png$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_logo512_png, _logo512_png_len)); };

server.resource["^/asset-manifest.json$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_asset_manifest_json, _asset_manifest_json_len)); };

server.resource["^/index.html$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_index_html, _index_html_len)); };

server.resource["^/manifest.json$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_manifest_json, _manifest_json_len)); };

server.resource["^/logo192.png$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_logo192_png, _logo192_png_len)); };

server.resource["^/robots.txt$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_robots_txt, _robots_txt_len)); };

server.default_resource["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_index_html, _index_html_len)); };
}
