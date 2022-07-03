INCBIN(_manifest_json,"../webgui/build/manifest.json");
INCBIN(_robots_txt,"../webgui/build/robots.txt");
INCBIN(_index_html,"../webgui/build/index.html");
INCBIN(_asset_manifest_json,"../webgui/build/asset-manifest.json");
INCBIN(_main_eeee2503_chunk_css,"../webgui/build/static/css/main.eeee2503.chunk.css");
INCBIN(_main_eeee2503_chunk_css_map,"../webgui/build/static/css/main.eeee2503.chunk.css.map");
INCBIN(_main_04253c5d_chunk_js,"../webgui/build/static/js/main.04253c5d.chunk.js");
INCBIN(_runtime_main_b8d6ee09_js,"../webgui/build/static/js/runtime-main.b8d6ee09.js");
INCBIN(_2_bfb682cb_chunk_js,"../webgui/build/static/js/2.bfb682cb.chunk.js");
INCBIN(_2_bfb682cb_chunk_js_LICENSE_txt,"../webgui/build/static/js/2.bfb682cb.chunk.js.LICENSE.txt");
INCBIN(_main_04253c5d_chunk_js_map,"../webgui/build/static/js/main.04253c5d.chunk.js.map");
INCBIN(_runtime_main_b8d6ee09_js_map,"../webgui/build/static/js/runtime-main.b8d6ee09.js.map");
INCBIN(_2_bfb682cb_chunk_js_map,"../webgui/build/static/js/2.bfb682cb.chunk.js.map");
void embedsite(HttpServer &server){
server.resource["^/manifest.json$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_manifest_jsonData, g_manifest_jsonSize)); };
server.resource["^/robots.txt$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_robots_txtData, g_robots_txtSize)); };
server.resource["^/index.html$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_index_htmlData, g_index_htmlSize)); };
server.resource["^/asset-manifest.json$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_asset_manifest_jsonData, g_asset_manifest_jsonSize)); };
server.resource["^/static/css/main.eeee2503.chunk.css$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_main_eeee2503_chunk_cssData, g_main_eeee2503_chunk_cssSize)); };
server.resource["^/static/css/main.eeee2503.chunk.css.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_main_eeee2503_chunk_css_mapData, g_main_eeee2503_chunk_css_mapSize)); };
server.resource["^/static/js/main.04253c5d.chunk.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_main_04253c5d_chunk_jsData, g_main_04253c5d_chunk_jsSize)); };
server.resource["^/static/js/runtime-main.b8d6ee09.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_runtime_main_b8d6ee09_jsData, g_runtime_main_b8d6ee09_jsSize)); };
server.resource["^/static/js/2.bfb682cb.chunk.js$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_2_bfb682cb_chunk_jsData, g_2_bfb682cb_chunk_jsSize)); };
server.resource["^/static/js/2.bfb682cb.chunk.js.LICENSE.txt$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_2_bfb682cb_chunk_js_LICENSE_txtData, g_2_bfb682cb_chunk_js_LICENSE_txtSize)); };
server.resource["^/static/js/main.04253c5d.chunk.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_main_04253c5d_chunk_js_mapData, g_main_04253c5d_chunk_js_mapSize)); };
server.resource["^/static/js/runtime-main.b8d6ee09.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_runtime_main_b8d6ee09_js_mapData, g_runtime_main_b8d6ee09_js_mapSize)); };
server.resource["^/static/js/2.bfb682cb.chunk.js.map$"]["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_2_bfb682cb_chunk_js_mapData, g_2_bfb682cb_chunk_js_mapSize)); };
server.default_resource["GET"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){ if (rejectNonLocals(request)) return; response->write(string_view((const char*)g_index_htmlData, g_index_htmlSize)); };
}
