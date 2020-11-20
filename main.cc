#include "interpretor.h"
#include "deps/config/config_file.h"
#include <iostream>
#include <boost/filesystem.hpp>
void initregex();

void loadconfig();
#ifdef WIN32
auto configpath = gethome() + "\_cqrc";
#else
auto configpath = gethome() + "/.cqrc";
#endif 

void help(char* prog){
	cout << '\n'
		<< prog << " <file>\n\tRun query from file\n"sv
		<< prog << " version\n\tshow version and exit\n"sv
		<< prog << " help\n\tshow this help message and exit\n"sv
		<< prog << " \"select from 'data.csv'\"\n\tRun query from command line argument\n"sv
		<< prog << "\n\tRun server to use graphic interface in web browser\n\n"
		"Flags:\n\t-x Don't check for updates when using gui\n"
		"\t-g Don't show debug info in console\n"
		"\t-h Show this help message and exit\n"
		"\t-v Show version and exit\n\n";
		//"Config file is " << configpath << ". It will be created if one doesn't exist\n"
	exit(0);
}

int main(int argc, char** argv){

	initregex();

	char c;
	int shift = 0;
	bool makeconf = false;
	//loadconfig();
	while((c = getopt(argc, argv, "hxvg")) != -1)
		switch(c){
		//help
		case 'h':
			help(argv[0]);
		case 'v':
			cout << version << endl;
			return 0;
		case 'x':
			globalOptions.update = false;
			shift++;
			break;
		case 'g':
			globalOptions.debug = false;
			shift++;
			break;
		}

	string querystring;
	runmode = argc > shift+1 ? RUN_SINGLE : RUN_SERVER;
	int arg1 = shift+1;
	switch (runmode){
	case RUN_SINGLE:
		//show version and exit
		if (!strcmp(argv[arg1], "version")){
			cout << version << endl;
			return 0;
		}
		//show help and exit
		if (!strcmp(argv[arg1], "help"))
			help(argv[0]);
		//get query from file
		if (strlen(argv[arg1]) < 30 && boost::filesystem::is_regular_file(argv[arg1]))
			querystring = st(ifstream(argv[arg1]).rdbuf());
		//get query from arg text
		else
			querystring = string(argv[arg1]);
		break;
	case RUN_SERVER:
		runServer();
		return 0;
	}

	try {
		querySpecs q(querystring);
		q.setoutputCsv();
		runPlainQuery(q);
	} catch (...) {
		cerr << "Error: "<< handle_err(current_exception()) << endl;
		return 1;
	}
	return 0;
}

void loadconfig(){
#ifndef WIN32
	vector<string> opts{ "show_debug_info", "check_update" };
	if (boost::filesystem::is_regular_file(configpath)){
		ifstream cfile(configpath);
		if (cfile.good()){
			CFG::ReadFile(cfile, opts,
					globalOptions.debug,
					globalOptions.update);
		}
	} else {
		ofstream cfile(configpath);
		CFG::WriteFile(cfile, opts,
				globalOptions.debug,
				globalOptions.update);
	}
#endif
}
