#include "interpretor.h"
#include <iostream>
void init();

int runmode;

int main(int argc, char** argv){

	init();
	FILE *fp;
	string querystring;
	runmode = argc > 1 ? RUN_FILE : RUN_SERVER;

	char c;
	while((c = getopt(argc, argv, "c:hs")) != -1)
		switch(c){
		//run query from command line argument
		case 'c':
			querystring = string(optarg);
			runmode = RUN_SINGLE;
			break;
		//run query from stdin
		case 's':
			runmode = RUN_FILE;
			break;
		//help
		case 'h':
			cout << '\n' << argv[0] << " <file>\n\tRun query from file\n\n"sv
				<< argv[0] << " -c \"select from 'data.csv'\"\n\tRun query from command line argument\n\n"sv
				<< argv[0] << " -s \n\tRun query from stdin\n\n"sv
				<< argv[0] << "\n\tRun server to use graphic interface in web browser\n\n"sv;
			return 0;
			break;
		}

	switch (runmode){
	case RUN_FILE:
		//get query from file or stdin
		if (argc == 2)
			fp = fopen(argv[1], "r");
		else
			fp = stdin;
		while (!feof(fp))
			querystring.push_back((char)getc(fp));
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
