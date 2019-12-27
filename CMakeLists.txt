set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "-Ofast -ltbb")
set(CMAKE_CXX_FLAGS "-ltbb")
cmake_minimum_required(VERSION 3.1)
project(cql)

find_package(fmt CONFIG REQUIRED)
find_package (Boost)

add_library(tre STATIC IMPORTED)
set_target_properties(tre PROPERTIES IMPORTED_LOCATION /home/dave/gits/vcpkg/installed/x64-linux/lib/libtre.a)
set_target_properties(tre PROPERTIES INTERFACE_INCLUDE_DIRECTORIES /home/dave/gits/vcpkg/installed/x64-linux/include/tre)

file(GLOB CQL_SOURCE "*.cc")

add_executable(cql ${CQL_SOURCE})

target_link_libraries(cql PRIVATE fmt::fmt tre -ltbb -static-libgcc -static-libstdc++)
