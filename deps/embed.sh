#!/bin/bash

out=embed_site.h

dostuff(){
	mbin $1
	echo
	filename=`echo $1 | sed 's-webgui/build/--'`
	len=$(echo "`mbin -n $1`_len")
	echo "server.resource[\"^/$filename$\"][\"GET\"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){response->write($(mbin -n $1));};"
}
export -f dostuff

echo -e "#ifndef EMBED_SITE\n#define EMBED_SITE\n" > $out

find webgui/build -regextype posix-egrep -regex ".*(js|css|map|json|png|txt|html)" -type f -exec bash -c 'dostuff "$0"' {} \; >> $out
echo "server.default_resource[\"GET\"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){response->write($(mbin -n webgui/build/index.html));};" >> $out

echo  "#endif" >> $out
