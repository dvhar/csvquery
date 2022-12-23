#include "interpretor.h"
#include "deps/config/config_file.h"
#include <iostream>
#include <boost/filesystem.hpp>

void initregex();
void loadconfig();
int bracecount(string&);
void help(char* prog){
	cout << '\n'
		<< prog << " FILE\n\tRun query from FILE\n"
		<< prog << " version\n\tshow version and exit\n"
		<< prog << " help\n\tshow this help message and exit\n"
		<< prog << " \"select from 'data.csv'\"\n\tRun query from command line argument\n"
		<< prog << " \"select from {}\" data.csv\n\tRun query from command line argument and substitute {} with the following argument\n"
		<< prog << "\n\tRun server to use graphic interface in web browser\n\n"
		"Flags:\n"
		"\t-m    Don't require comma between selections, but do require 'as' for result names\n"
		"\t-g    Don't show debug info in console\n"
		"\t-d    Show debug info in console (default)\n"
		"\t-e    Don't automatically exit 3 minutes after the browser page is closed\n"
		"\t-a    Guess if files have header based on whether or not there are numbers in the first row\n"
		"\t-t    Output results in terminal as table instead of csv format\n"
		"\t-c    Output results in terminal as csv instead of table (default)\n"
		"\t-y    Same as -t but with background colors to help see lines\n"
		"\t-w    Same as -t but no colors\n"
		"\t-j    Print json to stdout (rows limited to 20000 divided by number of columns)\n"
		"\t-h    Show this help message and exit\n"
		"\t-v    Show version and exit\n"
		"\t-f FILE    Run a selectAll query on FILE and output table\n"
		"\t-o FILE    Save query to FILE\n\n"
		"Config file is " << globalSettings.configfilepath << ". These are the settings you can change:\n"
		"\tshow_debug_info:      same as -d and -g\n"
		"\tguess_file_header:    same as -a\n"
		"\texit_automatically:   same as -e\n"
		"\ttable_or_csv:         same as -t and -c\n"
		"\tneed_comma_separator: same as -m\n";
	exit(0);
}

int main(int argc, char** argv){

	bool jsonstdout = false;
	string querystring;
	string savefile;
	loadconfig();
	for(char c; (c = getopt(argc, argv, "hvgdeajtywcmf:o:")) != -1;)
		switch(c){
		case 'o':
			savefile = optarg;
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
		case 'a':
			globalSettings.autoheader = true;
			break;
		case 'f':
			globalSettings.termbox = true;
			querystring = st('"',optarg,'"');
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
			globalSettings.termbox = false;
			break;
		case 'm':
			globalSettings.needcomma = false;
			break;
		case 'h':
			help(argv[0]);
		case 'v':
			cout << version << endl;
			exit(0);
		}

	initregex();
	runmode = argc > optind ? RUN_SINGLE : RUN_SERVER;
	auto arg1 = argv[optind];

	try {

		//run select * on -f arg
		if (!querystring.empty()){
			runmode = RUN_SINGLE;
			for (int i=optind; i<argc; ++i){
				querystring += ' ';
				querystring += argv[i];
			}
		}

		//run gui server and exit when done
		else if (runmode == RUN_SERVER)
			runServer();

		//show version and exit
		else if (!strcmp(arg1, "version")){
			cout << version << endl;
			return 0;
		}

		//show help and exit
		else if (!strcmp(arg1, "help"))
			help(argv[0]);

		//get query from multiple args or substitute {}
		else if (argc > optind+1){
			querystring = argv[optind];
			int bc = bracecount(querystring);
			if (bc && (optind + bc != argc-1))
				error("Query with ", bc, " '{}' pair",(bc>1?"s":"")," must have ",bc," matching argument",(bc>1?"s":""),". Found ",argc-optind-1,'\n');
			int end = argc - bc;
			for (int i=optind+1; i<end; ++i){
				querystring += ' ';
				querystring += argv[i];
			}
			for (int i = end; i < end+bc; i++)
				boost::replace_first(querystring, "{}", st("'",argv[i],"'"));
		}

		//get query from file
		else if (strlen(arg1) < 30 && boost::filesystem::is_regular_file(arg1))
			querystring = st(ifstream(arg1).rdbuf());

		//get query from single arg
		else {
			querystring = string(arg1);
			int bc = bracecount(querystring);
			if (bc)
				error("Query with ", bc, " '{}' pair",(bc>1?"s":"")," must have ",bc," matching argument",(bc>1?"s":""),". Found 0\n");
		}

		querySpecs q(querystring, savefile);
		if (jsonstdout){
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

int bracecount(string& q){
	int cnt = 0;
	i64 idx = -1;
	while ((idx = q.find("{}",idx+1)) > -1)
		cnt++;
	return cnt;
}

void loadconfig(){
	vector<string> opts{
		"version",
		"show_debug_info",
		"guess_file_header",
		"exit_automatically",
		"table_or_csv",
		"need_comma_separator"
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
					globalSettings.autoheader,
					globalSettings.autoexit,
					defaultoutput,
					globalSettings.needcomma);
			if (defaultoutput == "table")
				globalSettings.termbox = true;
			if (confversion >= version){
				return;
			}
		}
		cfile.close();
		defaultoutput = globalSettings.termbox ? "table":"csv";
	} else {
		defaultoutput = globalSettings.termbox ? "table":"csv";
		ofstream cfile(globalSettings.configfilepath);
		CFG::WriteFile(cfile, opts,
				version,
				globalSettings.debug,
				globalSettings.autoheader,
				globalSettings.autoexit,
				defaultoutput,
				globalSettings.needcomma);
	}
}
