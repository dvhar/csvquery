#include "interpretor.h"
#include "deps/config/config_file.h"
#include <iostream>
#include <boost/filesystem.hpp>
void initregex();

void loadconfig(bool);
#ifdef WIN32
auto configpath = gethome() + "\\_cqrc";
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
		"\t-d Show debug info in console\n"
		"\t-s Save options set by the flags before this one to the config and exit\n"
		"\t-h Show this help message and exit\n"
		"\t-v Show version and exit\n\n"
		"Config file is " << configpath << "\n";
	exit(0);
}

int main(int argc, char** argv){

	int shift = 0;
	loadconfig(0);
	for(char c; (c = getopt(argc, argv, "hxvgds")) != -1;)
		switch(c){
		case 'h':
			help(argv[0]);
		case 'v':
			cout << version << endl;
			exit(0);
		case 'x':
			globalOptions.update = false;
			shift++;
			break;
		case 'd':
			globalOptions.debug = true;
			shift++;
			break;
		case 'g':
			globalOptions.debug = false;
			shift++;
			break;
		case 's':
			loadconfig(1);
			exit(0);
		}

	initregex();
	string querystring;
	int arg1 = shift+1;
	runmode = argc > arg1 ? RUN_SINGLE : RUN_SERVER;
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

void loadconfig(bool save){
	vector<string> opts{ "version", "show_debug_info", "check_update", "guess_file_header" };
	if (!save && boost::filesystem::is_regular_file(configpath)){
		ifstream cfile(configpath);
		if (cfile.good()){
			string confversion;
			CFG::ReadFile(cfile, opts,
					confversion,
					globalOptions.debug,
					globalOptions.update,
					globalOptions.autoheader);
			if (confversion >= version){
				return;
			}
		}
		cfile.close();
	}
	ofstream cfile(configpath);
	cfile << "#csvquery config\n";
	CFG::WriteFile(cfile, opts,
			version,
			globalOptions.debug,
			globalOptions.update,
			globalOptions.autoheader);
}
