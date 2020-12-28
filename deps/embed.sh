#!/bin/bash


[ -f embed_site.hpp ] && rm embed_site.hpp
render(){
	echo $@ >> embed_site.hpp
}

makebin(){
	base=`basename $1`
	sym=_`echo $base | sed 's/[\./-]/_/g'`
	render "INCBIN(${sym},\"../${1}\");"
}
makeserver(){
	base=`basename $1`
	sym=_`echo $base | sed 's/[\./-]/_/g'`
	path=`echo $1 | sed 's|webgui/build/||g'`
	render "server.resource[\"^/${path}$\"][\"GET\"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		if (rejectNonLocals(request)) return;
			response->write(string_view((const char*)g${sym}Data, g${sym}Size));
		};"
	render
}

files=`find webgui/build -regextype posix-egrep -regex ".*(js|css|map|json|txt|html)" -type f`

for f in $files; do
makebin $f
done

render 'void embedsite(HttpServer &server){'
for f in $files; do
makeserver $f
done

render "server.default_resource[\"GET\"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		if (rejectNonLocals(request)) return;
			response->write(string_view((const char*)g_index_htmlData, g_index_htmlSize)); };"

render '}'
