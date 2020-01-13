set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "-Ofast -ltbb")
set(CMAKE_CXX_FLAGS "-ltbb")
cmake_minimum_required(VERSION 3.1)
project(cql)

find_package(fmt CONFIG REQUIRED)
find_package (Boost)

#add_library(tre STATIC IMPORTED)
#set_target_properties(tre PROPERTIES IMPORTED_LOCATION /home/dave/gits/vcpkg/installed/x64-linux/lib/libtre.a)
#set_target_properties(tre PROPERTIES INTERFACE_INCLUDE_DIRECTORIES /home/dave/gits/vcpkg/installed/x64-linux/include/tre)

if (WIN32)
add_library(tre STATIC IMPORTED)
set_target_properties(tre PROPERTIES IMPORTED_LOCATION C:/msys64/home/dog/vcpkg/installed/x86-windows/lib/tre.lib)
set_target_properties(tre PROPERTIES INTERFACE_INCLUDE_DIRECTORIES C:/msys64/home/dog/vcpkg/installed/x86-windows/include/tre)
endif()


file(GLOB CQL_SOURCE "*.cc")
set(CQL_SOURCE ${CQL_SOURCE} deps/dateparse/dateparse.c)
add_executable(cql ${CQL_SOURCE})

if (WIN32)
target_link_libraries(cql PRIVATE fmt::fmt tre C:/msys64/home/dog/vcpkg/installed/x86-windows/lib/getopt.lib -ltre -ltbb -static-libgcc -static-libstdc++)
else ()
target_link_libraries(cql PRIVATE fmt::fmt -ltbb -static-libgcc -static-libstdc++)
endif()
