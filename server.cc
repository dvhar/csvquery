#include "server.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
using namespace boost::property_tree;


void runServer(){
	webserver ws;
	ws.serve();
}

void webserver::serve(){
	auto endSemicolon = regex(";\\s*$");
	HttpServer server;
	server.config.port = 8060;
	embed(server);

	server.resource["^/query/$"]["POST"] = [&](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request){

		// compatible with old version
		ptree pt;
		webquery wq;
		shared_ptr<returnData> ret;

		try {

			read_json(request->content, pt);
			wq.savepath = pt.get("Savepath","");
			wq.qamount = pt.get("Qamount",0);
			wq.fileIO = pt.get("FileIO",0);
			wq.querystring = regex_replace(pt.get("Query",""), endSemicolon, "");
			ret = runqueries(wq);
			response->write(ret->tojson().str());

		} catch (...){
			auto e = handle_err(current_exception());
			cerr << "Error: " << e << endl;
			response->write(e);
		}


	}; // end /query/

	perr("starting http server\n");
	server.start();
}

shared_ptr<returnData> webserver::runqueries(webquery &wq){
	boost::split(wq.queries, wq.querystring, boost::is_any_of(";"));
	auto ret = make_shared<returnData>();
	int i = 0;
	for (auto &q: wq.queries){
		wq.whichone = i++;
		ret->entries.push_back(runWebQuery(wq));
	}
	return ret;
}

shared_ptr<singleQueryResult> webserver::runWebQuery(webquery &wq){
	cout << "webquery " << wq.whichone << ": " << wq.queries[wq.whichone] << endl;
	querySpecs q(wq.queries[wq.whichone]);
	if ((wq.fileIO & F_CSV) != 0) {
		q.setoutputCsv();
	}
	return runqueryJson(q);
}
