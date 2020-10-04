set(CMAKE_CXX_STANDARD 17)
cmake_minimum_required(VERSION 3.1)
project(cql)

set(optimize "-O3 -pipe")
set(applegcc /usr/local)
set(staticlib "-static-libgcc -static-libstdc++ -lssl -lcrypto")



if (APPLE)
	set(CMAKE_CXX_COMPILER ${applegcc}/bin/g++-10)
	set(CMAKE_C_COMPILER ${applegcc}/bin/gcc-10)
	set(CMAKE_CXX_FLAGS "${optimize} -ltbb -I${applegcc}/include -I/usr/local/Cellar/openssl@1.1/1.1.1g/include")
else()
	set(CMAKE_CXX_FLAGS "${optimize} -ltbb")
endif()

find_package (Boost)
find_package (nlohmann_json)

file(GLOB CQL_SOURCE "*.cc")
set(CQL_SOURCE ${CQL_SOURCE} deps/dateparse/dateparse.c)
set(CQL_SOURCE ${CQL_SOURCE} deps/chacha/chacha20.c)
set(CQL_SOURCE ${CQL_SOURCE} deps/crypto/base64.c)
set(CQL_SOURCE ${CQL_SOURCE} deps/crypto/sha1.c)
set(CQL_SOURCE ${CQL_SOURCE} deps/crypto/sha256.c)
set(CQL_SOURCE ${CQL_SOURCE} deps/crypto/md5.c)
set(CQL_SOURCE ${CQL_SOURCE} deps/getline/bufreader.cc)
add_executable(cql ${CQL_SOURCE})

if (WIN32)
	target_link_libraries(cql PRIVATE libtre.a tbb ${staticlib} wsock32 ws2_32 nlohmann_json::nlohmann_json)
elseif (APPLE)
	target_link_libraries(cql PRIVATE tbb ${staticlib} -L${applegcc}/lib -L/usr/local/Cellar/openssl@1.1/1.1.1g/lib)
elseif(UNIX)
	target_link_libraries(cql PRIVATE tbb ${staticlib} pthread libboost_system.a nlohmann_json::nlohmann_json)
endif()
