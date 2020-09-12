#pragma once

#include "deps/http/server_http.hpp"
#include<memory>
#include<string_view>
#include<iostream>

using namespace std;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

void embed(HttpServer&);
