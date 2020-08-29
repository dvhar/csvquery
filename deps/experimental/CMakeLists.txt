
set(CMAKE_CXX_STANDARD 17)
cmake_minimum_required(VERSION 3.1)
project(br)

set(optimize "-O3")
set(applegcc $ENV{HOME}/stuff/gcc/usr/local)
set(staticlib "-static-libgcc -static-libstdc++")

if (APPLE)
	set(CMAKE_CXX_COMPILER ${applegcc}/bin/g++)
	set(CMAKE_C_COMPILER ${applegcc}/bin/gcc)
	set(CMAKE_CXX_FLAGS "${optimize} -ltbb -I${applegcc}/include")
else()
	set(CMAKE_CXX_FLAGS "${optimize} -ltbb")
endif()

file(GLOB CQL_SOURCE "*.cc")
add_executable(br ${CQL_SOURCE})

if (WIN32)
	target_link_libraries(br PRIVATE ${staticlib} wsock32 ws2_32)
elseif (APPLE)
	target_link_libraries(br PRIVATE ${staticlib} -L${applegcc}/lib)
elseif(UNIX)
	target_link_libraries(br PRIVATE ${staticlib} pthread)
endif()
