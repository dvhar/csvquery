#!/bin/bash


[ -f embed_site.hpp ] && rm embed_site.hpp
render(){
	echo $@ >> embed_site.hpp
}

makebin(){
	#cp $1 build/
	base=`basename $1`
	sym=_`echo $base | sed 's/[\./-]/_/g'`
	render "IMPORT_BIN(\"${base}\",${sym});"
	render "extern const char ${sym}[], _sizeof_${sym}[];"
	render "const size_t ${sym}_len = (size_t)_sizeof_${sym};"
}
makeserver(){
	base=`basename $1`
	sym=_`echo $base | sed 's/[\./-]/_/g'`
	path=`echo $1 | sed 's|webgui/build/||g'`
	render "server.resource[\"^/${path}$\"][\"GET\"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		if (rejectNonLocals(request)) return;
		response->write(string_view(${sym}, ${sym}_len));
		};"
	render
}

#echo '
cat << EOF >> embed_site.hpp
#define IMPORT_BIN(file, sym) asm (\\
    ".section .rodata\n"                 \\
    ".balign 4\n"                        \\
    ".global " #sym "\n"                 \\
    #sym ":\n"                           \\
    ".incbin \"" file "\"\n"             \\
    ".global _sizeof_" #sym "\n"         \\
    ".set _sizeof_" #sym ", . - " #sym "\n"\\
    ".balign 4\n"                        \\
    ".section \".text\"\n")
EOF
#'


files=`find webgui/build -regextype posix-egrep -regex ".*(js|css|map|json|png|txt|html)" -type f`

for f in $files; do
makebin $f
done

render 'void embedsite(HttpServer &server){'
for f in $files; do
makeserver $f
done

render "server.default_resource[\"GET\"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){
		if (rejectNonLocals(request)) return;
		response->write(string_view(_index_html, _index_html_len)); };"

render '}'
