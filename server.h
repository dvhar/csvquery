#pragma once

#include "deps/http/server_http.hpp"
#include<memory>
#include<string_view>
#include<forward_list>
#include<iostream>
#include "interpretor.h"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

enum legacy_server {
	DAT_BLANK   = 0,
	DAT_ERROR   = 1,
	DAT_GOOD    = 1 << 1,
	DAT_BADPATH = 1 << 2,
	DAT_IOERR   = 1 << 4,
	F_CSV       = 1 << 5
};

class directory {
	json j;
	string fpath;
	string parent;
	string mode;
	vector<string> files;
	vector<string> dirs;
	public:
	void setDir(json&);
	json& tojson();
};

class webquery {
	public:
	vector<string> queries;
	int whichone =0;
	string querystring;
	string savepath;
	int qamount =0;
	int fileIO =0;
	bool isSaving(){ return ((fileIO & F_CSV) != 0); }
};

class webserver {
	string state;
	HttpServer server;
	public:
	shared_ptr<returnData> runqueries(webquery &wq);
	shared_ptr<singleQueryResult> runWebQuery(webquery &wq);
	void serve();
};


template<typename T>
static T fromjson(json& j, string key){
	try {
		return j[key].get<T>();
	} catch (exception e) {
		return T{};
	}
}

string handle_err(exception_ptr eptr);
