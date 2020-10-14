#include "interpretor.h"
#include <iostream>
#include <filesystem>
void init();

int runmode;

void help(char* prog){
	cout << '\n' << prog << " <file>\n\tRun query from file\n\n"sv
		<< prog << " \"select from 'data.csv'\"\n\tRun query from command line argument\n\n"sv
		<< prog << "\n\tRun server to use graphic interface in web browser\n\n"sv;
	exit(0);
}

int main(int argc, char** argv){

	init();
	string querystring;
	runmode = argc > 1 ? RUN_SINGLE : RUN_SERVER;

	switch (runmode){
	case RUN_SINGLE:
		//show help and exit
		if (!strcmp(argv[1], "help"))
			help(argv[0]);
		//get query from file
		if (filesystem::is_regular_file(argv[1]))
			querystring = st(ifstream(argv[1]).rdbuf());
		//get query from arg text
		else
			querystring = string(argv[1]);
		break;
	case RUN_SERVER:
		runServer();
		return 0;
	}

	try {
		querySpecs q(querystring);
		q.setoutputCsv();
		runquery(q);
	} catch (...) {
		cerr << "Error: "<< handle_err(current_exception()) << endl;
		return 1;
	}
	return 0;
}

//initialize some stuff
void init(){

	regcomp(&leadingZeroString, "^0[0-9]+$", REG_EXTENDED);
	regcomp(&durationPattern, "^([0-9]+|[0-9]+\\.[0-9]+) ?(seconds|second|minutes|minute|hours|hour|days|day|weeks|week|years|year|s|m|h|d|w|y)$", REG_EXTENDED);
	regcomp(&intType, "^-?[0-9]+$", REG_EXTENDED);
	regcomp(&floatType, "^-?[0-9]*\\.[0-9]+$", REG_EXTENDED);
}
