cmake_minimum_required(VERSION 3.18)

project(cql LANGUAGES C CXX)

find_package(TBB REQUIRED)
find_package(nlohmann_json REQUIRED)

if(UNIX)
  set(THREADS_PREFER_PTHREAD_FLAG YES)
  find_package(Threads REQUIRED)
  find_package(Boost REQUIRED COMPONENTS system)
endif()

set(sources
	deps/dateparse/dateparse.c
	deps/crypto/chacha20.c
	deps/crypto/base64.c
	deps/crypto/sha1.c
	deps/crypto/sha256.c
	deps/crypto/md5.c
	deps/crypto/halfsiphash.c
	deps/getline/bufreader.cc
	deps/json/escape.cc
	analyzetree.cc
	codegen.cc
	datatyping.cc
	embed_site.cc
	filereader.cc
	http.cc
	main.cc
	parser.cc
	scanner.cc
	termos.cc
	utils.cc
	vmachine.cc
	vmutils.cc
	ws.cc
)

add_executable(cql ${sources})
target_compile_features(cql PRIVATE cxx_std_17)

set(is_win "$<BOOL:${WIN32}>")
set(is_unix "$<BOOL:${UNIX}>")
target_link_libraries(cql PRIVATE
        TBB::tbb
        "$<$<NOT:$<BOOL:${APPLE}>>:nlohmann_json::nlohmann_json>"
        "$<${is_unix}:Boost::system>"
        "$<${is_unix}:Threads::Threads>"
        # Non target dependencies
        "$<${is_win}:tre>"
        "$<${is_win}:wsock32>"
        "$<${is_win}:ws2_32>")
