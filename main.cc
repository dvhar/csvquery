#include "interpretor.h"
#include "deps/config/config_file.h"
#include <iostream>
#include <boost/filesystem.hpp>

void initregex();
void loadconfig();
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
		"\t-t Output results in terminal as table instead of csv format\n"
		"\t-c Output results in terminal as csv instead of table (default)\n"
		"\t-y Same as -t but with background colors to help see lines\n"
		"\t-w Same as -t but no colors\n"
		"\t-j Return json to stdout (rows limited to 20000 divided by number of columns)\n"
		"\t-h Show this help message and exit\n"
		"\t-v Show version and exit\n\n"
		"Config file is " << globalSettings.configfilepath << "\n";
	exit(0);
}

int main(int argc, char** argv){

	bool jsonstdout = false;
	loadconfig();
	for(char c; (c = getopt(argc, argv, "hxvgdefjtywc")) != -1;)
		switch(c){
		case 'x':
			globalSettings.update = false;
			break;
		case 'g':
			globalSettings.debug = false;
			break;
		case 'd':
			globalSettings.debug = true;
			break;
		case 'e':
			globalSettings.autoexit = false;
			break;
		case 'f':
			globalSettings.autoheader = true;
			break;
		case 't':
			globalSettings.termbox = true;
			break;
		case 'y':
			globalSettings.termbox = true;
			globalSettings.tablelinebg = true;
			break;
		case 'w':
			globalSettings.termbox = true;
			globalSettings.tablecolor = false;
			break;
		case 'c':
			globalSettings.termbox = false;
			break;
		case 'j':
			jsonstdout = true;
			break;
		case 'h':
			help(argv[0]);
		case 'v':
			cout << version << endl;
			exit(0);
		}

	initregex();
	runmode = argc > optind ? RUN_SINGLE : RUN_SERVER;
	if (runmode == RUN_SERVER){
		runServer();
		return 0;
	}

	auto arg1 = argv[optind];
	string querystring;
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

void loadconfig(){
	vector<string> opts{
		"version",
		"show_debug_info",
		"check_update",
		"guess_file_header",
		"exit_automatically",
		"table_or_csv"
	};
	string defaultoutput;
	boost::filesystem::create_directories(globalSettings.configdir);
	if (boost::filesystem::is_regular_file(globalSettings.configfilepath)){
		ifstream cfile(globalSettings.configfilepath);
		if (cfile.good()){
			string confversion;
			CFG::ReadFile(cfile, opts,
					confversion,
					globalSettings.debug,
					globalSettings.update,
					globalSettings.autoheader,
					globalSettings.autoexit,
					defaultoutput);
			if (defaultoutput == "table")
				globalSettings.termbox = true;
			if (confversion >= version){
				return;
			}
		}
		cfile.close();
	}
	defaultoutput = globalSettings.termbox ? "table":"csv";
	ofstream cfile(globalSettings.configfilepath);
	CFG::WriteFile(cfile, opts,
			version,
			globalSettings.debug,
			globalSettings.update,
			globalSettings.autoheader,
			globalSettings.autoexit,
			defaultoutput);
}
