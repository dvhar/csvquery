#!/bin/bash


render(){
	echo $@ >> embed_site.cc
}
export -f render

dostuff(){
	mbin $1 >> embed_site.cc
	filename=`echo $1 | sed 's-webgui/build/--'`
	buf=`mbin $1 -n`
	len="`mbin -n $1`_len"
	render "server.resource[\"^/$filename$\"][\"GET\"] = [$len](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		response->write(string_view($buf, $len)); };"
	render
}
export -f dostuff

find webgui/build -regextype posix-egrep -regex ".*(js|css|map|json|png|txt|html)" -type f -exec bash -c 'dostuff "$0"' {} \;

buf=`mbin webgui/build/index.html -n`
len="`mbin webgui/build/index.html -n`_len"
render "server.default_resource[\"GET\"] = [$len](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		response->write(string_view($buf, $len)); };"
