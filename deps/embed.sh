#!/bin/bash

out=embed_site.hpp

dostuff(){
	mbin $1
	filename=`echo $1 | sed 's-webgui/build/--'`
	buf=`mbin $1 -n`
	len="`mbin -n $1`_len"
	echo "server.resource[\"^/$filename$\"][\"GET\"] = [$len](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		response->write(string_view($buf, $len));
	};"
	echo
}
export -f dostuff

echo -e "#ifndef EMBED_SITE\n#define EMBED_SITE\n" > $out

find webgui/build -regextype posix-egrep -regex ".*(js|css|map|json|png|txt|html)" -type f -exec bash -c 'dostuff "$0"' {} \; >> $out

buf=`mbin webgui/build/index.html -n`
len="`mbin webgui/build/index.html -n`_len"
echo "server.default_resource[\"GET\"] = [$len](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		response->write(string_view($buf, $len));
	};" >> $out

echo  "#endif" >> $out
