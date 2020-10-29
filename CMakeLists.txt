cmake_minimum_required(VERSION 3.12)

project(cql LANGUAGES C CXX)

find_package(TBB REQUIRED)
find_package(nlohmann_json REQUIRED)

set(CMAKE_CXX_FLAGS "-O3 -pipe")

if(UNIX)
  set(THREADS_PREFER_PTHREAD_FLAG YES)
  find_package(Threads REQUIRED)
  find_package(Boost REQUIRED COMPONENTS system)
endif()

file(GLOB sources "*.cc")
set(sources ${sources}
	deps/dateparse/dateparse.c
	deps/crypto/chacha20.c
	deps/crypto/base64.c
	deps/crypto/sha1.c
	deps/crypto/sha256.c
	deps/crypto/md5.c
	deps/crypto/halfsiphash.c
	deps/getline/bufreader.cc
	deps/json/escape.cc
)

add_executable(cql ${sources})
target_compile_features(cql PRIVATE cxx_std_17)

set(is_win "$<BOOL:${WIN32}>")
set(is_unix "$<BOOL:${UNIX}>")
target_link_libraries(cql PRIVATE
        TBB::tbb
        "$<${is_unix}:Boost::system>"
        "$<${is_unix}:Threads::Threads>"
        "$<${is_win}:tre>"
        "$<${is_win}:wsock32>"
        "$<${is_win}:ws2_32>")
