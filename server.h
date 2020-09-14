#pragma once

#include "deps/http/server_http.hpp"
#include<memory>
#include<string_view>
#include<forward_list>
#include<iostream>

using namespace std;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

template<typename T>
static void __st(stringstream& ss, T v) {
	ss << v;
}
template<typename T, typename... Args>
static void __st(stringstream& ss, T first, Args... args) {
	ss << first;
	__st(ss, args...);
}
template<typename T, typename... Args>
static string st(T first, Args... args) {
	stringstream ss;
	__st(ss, first, args...);
	return ss.str();
}

//outdated backward compatible stuff:
class singleQueryResult {
	public:
	int numrows;
	int showLimit;
	int numcols;
	vector<int> types;
	vector<string> colnames;
	vector<int> pos;
	forward_list<string> Vals; //each string is whole row
	int status;
	string query;
};

class returnData {
	public:
	forward_list<shared_ptr<singleQueryResult>> entries;
	int status;
	string originalQuery;
	bool clipped;
	string message;
};

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
