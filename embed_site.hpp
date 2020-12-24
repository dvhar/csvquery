IMPORT_BIN("../webgui/build/robots.txt",_robots_txt);
extern const char _robots_txt[], _sizeof__robots_txt[];
const size_t _robots_txt_len = (size_t)_sizeof__robots_txt;
IMPORT_BIN("../webgui/build/asset-manifest.json",_asset_manifest_json);
extern const char _asset_manifest_json[], _sizeof__asset_manifest_json[];
const size_t _asset_manifest_json_len = (size_t)_sizeof__asset_manifest_json;
IMPORT_BIN("../webgui/build/manifest.json",_manifest_json);
extern const char _manifest_json[], _sizeof__manifest_json[];
const size_t _manifest_json_len = (size_t)_sizeof__manifest_json;
IMPORT_BIN("../webgui/build/static/css/main.88b1b6ac.chunk.css",_main_88b1b6ac_chunk_css);
extern const char _main_88b1b6ac_chunk_css[], _sizeof__main_88b1b6ac_chunk_css[];
const size_t _main_88b1b6ac_chunk_css_len = (size_t)_sizeof__main_88b1b6ac_chunk_css;
IMPORT_BIN("../webgui/build/static/css/main.88b1b6ac.chunk.css.map",_main_88b1b6ac_chunk_css_map);
extern const char _main_88b1b6ac_chunk_css_map[], _sizeof__main_88b1b6ac_chunk_css_map[];
const size_t _main_88b1b6ac_chunk_css_map_len = (size_t)_sizeof__main_88b1b6ac_chunk_css_map;
IMPORT_BIN("../webgui/build/static/js/2.d0675a0e.chunk.js",_2_d0675a0e_chunk_js);
extern const char _2_d0675a0e_chunk_js[], _sizeof__2_d0675a0e_chunk_js[];
const size_t _2_d0675a0e_chunk_js_len = (size_t)_sizeof__2_d0675a0e_chunk_js;
IMPORT_BIN("../webgui/build/static/js/2.d0675a0e.chunk.js.LICENSE.txt",_2_d0675a0e_chunk_js_LICENSE_txt);
extern const char _2_d0675a0e_chunk_js_LICENSE_txt[], _sizeof__2_d0675a0e_chunk_js_LICENSE_txt[];
const size_t _2_d0675a0e_chunk_js_LICENSE_txt_len = (size_t)_sizeof__2_d0675a0e_chunk_js_LICENSE_txt;
IMPORT_BIN("../webgui/build/static/js/main.158c5df1.chunk.js.map",_main_158c5df1_chunk_js_map);
extern const char _main_158c5df1_chunk_js_map[], _sizeof__main_158c5df1_chunk_js_map[];
const size_t _main_158c5df1_chunk_js_map_len = (size_t)_sizeof__main_158c5df1_chunk_js_map;
IMPORT_BIN("../webgui/build/static/js/main.158c5df1.chunk.js",_main_158c5df1_chunk_js);
extern const char _main_158c5df1_chunk_js[], _sizeof__main_158c5df1_chunk_js[];
const size_t _main_158c5df1_chunk_js_len = (size_t)_sizeof__main_158c5df1_chunk_js;
IMPORT_BIN("../webgui/build/static/js/runtime-main.3abd2872.js.map",_runtime_main_3abd2872_js_map);
extern const char _runtime_main_3abd2872_js_map[], _sizeof__runtime_main_3abd2872_js_map[];
const size_t _runtime_main_3abd2872_js_map_len = (size_t)_sizeof__runtime_main_3abd2872_js_map;
IMPORT_BIN("../webgui/build/static/js/runtime-main.3abd2872.js",_runtime_main_3abd2872_js);
extern const char _runtime_main_3abd2872_js[], _sizeof__runtime_main_3abd2872_js[];
const size_t _runtime_main_3abd2872_js_len = (size_t)_sizeof__runtime_main_3abd2872_js;
IMPORT_BIN("../webgui/build/static/js/2.d0675a0e.chunk.js.map",_2_d0675a0e_chunk_js_map);
extern const char _2_d0675a0e_chunk_js_map[], _sizeof__2_d0675a0e_chunk_js_map[];
const size_t _2_d0675a0e_chunk_js_map_len = (size_t)_sizeof__2_d0675a0e_chunk_js_map;
IMPORT_BIN("../webgui/build/index.html",_index_html);
extern const char _index_html[], _sizeof__index_html[];
const size_t _index_html_len = (size_t)_sizeof__index_html;
void embedsite(HttpServer &server){
server.resource["^/robots.txt$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_robots_txt, _robots_txt_len)); };

server.resource["^/asset-manifest.json$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_asset_manifest_json, _asset_manifest_json_len)); };

server.resource["^/manifest.json$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_manifest_json, _manifest_json_len)); };

server.resource["^/static/css/main.88b1b6ac.chunk.css$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_main_88b1b6ac_chunk_css, _main_88b1b6ac_chunk_css_len)); };

server.resource["^/static/css/main.88b1b6ac.chunk.css.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_main_88b1b6ac_chunk_css_map, _main_88b1b6ac_chunk_css_map_len)); };

server.resource["^/static/js/2.d0675a0e.chunk.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_2_d0675a0e_chunk_js, _2_d0675a0e_chunk_js_len)); };

server.resource["^/static/js/2.d0675a0e.chunk.js.LICENSE.txt$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_2_d0675a0e_chunk_js_LICENSE_txt, _2_d0675a0e_chunk_js_LICENSE_txt_len)); };

server.resource["^/static/js/main.158c5df1.chunk.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_main_158c5df1_chunk_js_map, _main_158c5df1_chunk_js_map_len)); };

server.resource["^/static/js/main.158c5df1.chunk.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_main_158c5df1_chunk_js, _main_158c5df1_chunk_js_len)); };

server.resource["^/static/js/runtime-main.3abd2872.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_runtime_main_3abd2872_js_map, _runtime_main_3abd2872_js_map_len)); };

server.resource["^/static/js/runtime-main.3abd2872.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_runtime_main_3abd2872_js, _runtime_main_3abd2872_js_len)); };

server.resource["^/static/js/2.d0675a0e.chunk.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_2_d0675a0e_chunk_js_map, _2_d0675a0e_chunk_js_map_len)); };

server.resource["^/index.html$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_index_html, _index_html_len)); };

server.default_resource["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view(_index_html, _index_html_len)); };
}
