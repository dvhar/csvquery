#pragma once

#include "deps/http/server_http.hpp"
#include<memory>
#include<string_view>
#include<forward_list>
#include<iostream>
#include "interpretor.h"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

class webquery {
	public:
	vector<string> queries;
	int whichone;
	string querystring;
	string savepath;
	int qamount;
	int fileIO;
};

class webserver {
	public:
	shared_ptr<returnData> runqueries(webquery &wq);
	shared_ptr<singleQueryResult> runquery(webquery &wq);
	void embed(HttpServer&);
	void serve();
};

const int F_CSV = 1 << 5;

string handle_err(exception_ptr eptr);
