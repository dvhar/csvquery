#include "interpretor.h"
#include "deps/config/config_file.h"
#include <iostream>
#include <boost/filesystem.hpp>
void initregex();

void loadconfig(bool);

void help(char* prog){
	cout << '\n'
		<< prog << " <file>\n\tRun query from file\n"
		<< prog << " version\n\tshow version and exit\n"
		<< prog << " help\n\tshow this help message and exit\n"
		<< prog << " \"select from 'data.csv'\"\n\tRun query from command line argument\n"
		<< prog << "\n\tRun server to use graphic interface in web browser\n\n"
		"Flags:\n"
		"\t-x Don't check for updates when clicking the gui's help button\n"
		"\t-g Don't show debug info in console\n"
		"\t-d Show debug info in console (default)\n"
		"\t-e Don't automatically exit 3 minutes after the browser page is closed\n"
		"\t-f Guess if files have header based on whether or not there are numbers in the first row\n"
		"\t-s Save options set by the flags before this one to the config and exit\n"
		"\t-t Print results in terminal as table instead of csv format\n"
		"\t-y Same as -t but with background colors to help see lines\n"
		"\t-w Don't print colors when printing table to terminal\n"
		"\t-j Return json to stdout (rows limited to 20000 divided by number of columns)\n"
		"\t-h Show this help message and exit\n"
		"\t-v Show version and exit\n\n"
		"Config file is " << globalSettings.configpath << "\n";
	exit(0);
}

int main(int argc, char** argv){

	bool jsonstdout = false;
	int shift = 0;
	loadconfig(0);
	for(char c; (c = getopt(argc, argv, "hxvgdsefjtyw")) != -1;)
		switch(c){
		case 'x':
			globalSettings.update = false;
			shift++;
			break;
		case 'g':
			globalSettings.debug = false;
			shift++;
			break;
		case 'd':
			globalSettings.debug = true;
			shift++;
			break;
		case 'e':
			globalSettings.autoexit = false;
			shift++;
			break;
		case 'f':
			globalSettings.autoheader = true;
			shift++;
			break;
		case 't':
			globalSettings.termbox = true;
			shift++;
			break;
		case 'y':
			globalSettings.termbox = true;
			globalSettings.tablelinebg = true;
			shift++;
			break;
		case 'w':
			globalSettings.tablecolor = false;
			shift++;
			break;
		case 's':
			loadconfig(1);
			exit(0);
		case 'j':
			jsonstdout = true;
			shift++;
			break;
		case 'h':
			help(argv[0]);
		case 'v':
			cout << version << endl;
			exit(0);
		}

	initregex();
	string querystring;
	auto arg1 = argv[shift+1];
	runmode = argc > shift+1 ? RUN_SINGLE : RUN_SERVER;
	switch (runmode){
	case RUN_SINGLE:
		//show version and exit
		if (!strcmp(arg1, "version")){
			cout << version << endl;
			return 0;
		}
		//show help and exit
		if (!strcmp(arg1, "help"))
			help(argv[0]);
		//get query from file
		if (strlen(arg1) < 30 && boost::filesystem::is_regular_file(arg1))
			querystring = st(ifstream(arg1).rdbuf());
		//get query from arg text
		else
			querystring = string(arg1);
		break;
	case RUN_SERVER:
		runServer();
		return 0;
	}

	try {
		querySpecs q(querystring);
		if (jsonstdout){
			q.setoutputJson();
			cout << runJsonQuery(q)->tojson().str();
		} else {
			q.setoutputCsv();
			runPlainQuery(q);
		}
	} catch (...) {
		cerr << "Error: "<< EX_STRING << endl;
		return 1;
	}
	return 0;
}

void loadconfig(bool save){
	vector<string> opts{
		"version",
		"show_debug_info",
		"check_update",
		"guess_file_header",
		"exit_automatically"
	};
	if (!save && boost::filesystem::is_regular_file(globalSettings.configpath)){
		ifstream cfile(globalSettings.configpath);
		if (cfile.good()){
			string confversion;
			CFG::ReadFile(cfile, opts,
					confversion,
					globalSettings.debug,
					globalSettings.update,
					globalSettings.autoheader,
					globalSettings.autoexit);
			if (confversion >= version){
				return;
			}
		}
		cfile.close();
	}
	ofstream cfile(globalSettings.configpath);
	CFG::WriteFile(cfile, opts,
			version,
			globalSettings.debug,
			globalSettings.update,
			globalSettings.autoheader,
			globalSettings.autoexit);
}
