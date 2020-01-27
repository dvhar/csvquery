#!/bin/bash

out=embed_site.hpp

dostuff(){
	mbin $1
	filename=`echo $1 | sed 's-webgui/build/--'`
	buf=`mbin $1 -n`
	len="`mbin -n $1`_len"
	echo "server.resource[\"^/$filename$\"][\"GET\"] = [$len](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		sbuf.resize($len);
		vp = (void*) sbuf.data();
		memcpy(vp, $buf, $len);
		response->write(sbuf);
	};"
	echo
}
export -f dostuff

echo -e "#ifndef EMBED_SITE\n#define EMBED_SITE\n" > $out

#void* and string workaround for ostreaming binary data that includes null characters
echo "static string sbuf;" >> $out
echo "static void* vp;" >> $out

find webgui/build -regextype posix-egrep -regex ".*(js|css|map|json|png|txt|html)" -type f -exec bash -c 'dostuff "$0"' {} \; >> $out

buf=`mbin webgui/build/index.html -n`
len="`mbin webgui/build/index.html -n`_len"
echo "server.default_resource[\"GET\"] = [$len](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		sbuf.resize($len);
		vp = (void*) sbuf.data();
		memcpy(vp, $buf, $len);
		response->write(sbuf);
	};" >> $out

echo  "#endif" >> $out
