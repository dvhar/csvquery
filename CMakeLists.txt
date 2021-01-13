cmake_minimum_required(VERSION 3.12)

project(cql LANGUAGES C CXX)

add_compile_options(-O3 -pipe)
set(is_win "$<BOOL:${WIN32}>")
set(is_unix "$<BOOL:${UNIX}>")

if(UNIX)
	set(THREADS_PREFER_PTHREAD_FLAG YES)
	find_package(Threads REQUIRED)
	find_package(Boost REQUIRED COMPONENTS system)
endif()
find_package(Boost REQUIRED COMPONENTS filesystem)

file(GLOB sources CONFIGURE_DEPENDS "*.cc")
set(sources ${sources}
	deps/dateparse/dateparse.c
	deps/dateparse/musl.c
	deps/crypto/chacha20.c
	deps/crypto/base64.c
	deps/crypto/sha1.c
	deps/crypto/sha256.c
	deps/crypto/md5.c
	deps/crypto/halfsiphash.c
	deps/crypto/siphash.c
	deps/getline/bufreader.cc
	deps/json/escape.cc
)

add_executable(cql ${sources})
target_compile_features(cql PRIVATE cxx_std_17)
target_link_libraries(cql PRIVATE
	Boost::filesystem
	"$<${is_unix}:Boost::system>"
	"$<${is_unix}:Threads::Threads>"
	"$<${is_win}:tre>"
	"$<${is_win}:wsock32>"
	"$<${is_win}:ws2_32>")
