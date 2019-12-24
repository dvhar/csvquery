set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "-Ofast -ltbb")
cmake_minimum_required(VERSION 3.1)
project(cql)

find_package(fmt CONFIG REQUIRED)

#figure out how to use tre
#find_package(tre CONFIG REQUIRED)

file(GLOB CQL_SOURCE "*.cc")

add_executable(cql ${CQL_SOURCE})

target_link_libraries(cql PRIVATE fmt::fmt -static)
