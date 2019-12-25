set(CMAKE_CXX_STANDARD 17)
#set(ENV{fmt_DIR} /home/dave/gits/vcpkg/installed/x64-linux/share/fmt/)
#set(CMAKE_CXX_FLAGS "-Ofast -ltbb")
set(CMAKE_CXX_FLAGS "-ltbb")
cmake_minimum_required(VERSION 3.1)
project(cql)

find_package(fmt CONFIG REQUIRED)
find_package (Boost)

#figure out how to use tre
#find_package(tre CONFIG REQUIRED)

file(GLOB CQL_SOURCE "*.cc")

add_executable(cql ${CQL_SOURCE})

#lower one not tested on windows yet
#target_link_libraries(cql PRIVATE fmt::fmt -static)
target_link_libraries(cql PRIVATE fmt::fmt -ltbb -static-libgcc -static-libstdc++)
