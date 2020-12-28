INCBIN(_robots_txt,"../webgui/build/robots.txt");
INCBIN(_asset_manifest_json,"../webgui/build/asset-manifest.json");
INCBIN(_manifest_json,"../webgui/build/manifest.json");
INCBIN(_main_88b1b6ac_chunk_css,"../webgui/build/static/css/main.88b1b6ac.chunk.css");
INCBIN(_main_88b1b6ac_chunk_css_map,"../webgui/build/static/css/main.88b1b6ac.chunk.css.map");
INCBIN(_main_158c5df1_chunk_js,"../webgui/build/static/js/main.158c5df1.chunk.js");
INCBIN(_2_d0675a0e_chunk_js,"../webgui/build/static/js/2.d0675a0e.chunk.js");
INCBIN(_runtime_main_3abd2872_js,"../webgui/build/static/js/runtime-main.3abd2872.js");
INCBIN(_main_158c5df1_chunk_js_map,"../webgui/build/static/js/main.158c5df1.chunk.js.map");
INCBIN(_runtime_main_3abd2872_js_map,"../webgui/build/static/js/runtime-main.3abd2872.js.map");
INCBIN(_2_d0675a0e_chunk_js_map,"../webgui/build/static/js/2.d0675a0e.chunk.js.map");
INCBIN(_2_d0675a0e_chunk_js_LICENSE_txt,"../webgui/build/static/js/2.d0675a0e.chunk.js.LICENSE.txt");
INCBIN(_index_html,"../webgui/build/index.html");
void embedsite(HttpServer &server){
server.resource["^/robots.txt$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_robots_txtData, g_robots_txtSize)); };

server.resource["^/asset-manifest.json$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_asset_manifest_jsonData, g_asset_manifest_jsonSize)); };

server.resource["^/manifest.json$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_manifest_jsonData, g_manifest_jsonSize)); };

server.resource["^/static/css/main.88b1b6ac.chunk.css$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_main_88b1b6ac_chunk_cssData, g_main_88b1b6ac_chunk_cssSize)); };

server.resource["^/static/css/main.88b1b6ac.chunk.css.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_main_88b1b6ac_chunk_css_mapData, g_main_88b1b6ac_chunk_css_mapSize)); };

server.resource["^/static/js/main.158c5df1.chunk.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_main_158c5df1_chunk_jsData, g_main_158c5df1_chunk_jsSize)); };

server.resource["^/static/js/2.d0675a0e.chunk.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_2_d0675a0e_chunk_jsData, g_2_d0675a0e_chunk_jsSize)); };

server.resource["^/static/js/runtime-main.3abd2872.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_runtime_main_3abd2872_jsData, g_runtime_main_3abd2872_jsSize)); };

server.resource["^/static/js/main.158c5df1.chunk.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_main_158c5df1_chunk_js_mapData, g_main_158c5df1_chunk_js_mapSize)); };

server.resource["^/static/js/runtime-main.3abd2872.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_runtime_main_3abd2872_js_mapData, g_runtime_main_3abd2872_js_mapSize)); };

server.resource["^/static/js/2.d0675a0e.chunk.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_2_d0675a0e_chunk_js_mapData, g_2_d0675a0e_chunk_js_mapSize)); };

server.resource["^/static/js/2.d0675a0e.chunk.js.LICENSE.txt$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_2_d0675a0e_chunk_js_LICENSE_txtData, g_2_d0675a0e_chunk_js_LICENSE_txtSize)); };

server.resource["^/index.html$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_index_htmlData, g_index_htmlSize)); };

server.default_resource["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_index_htmlData, g_index_htmlSize)); };
}
