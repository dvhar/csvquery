#include "interpretor.h"
#include <iostream>
void init();
int runmode = RUN_CMD;

int main(int argc, char** argv){

	init();
	FILE *fp;
	string qs;
	char cc[1], c;

	while((c = getopt(argc, argv, "c:hs")) != -1)
		switch(c){
		//run query from command line argument
		case 'c':
			qs = string(optarg);
			runmode = RUN_SINGLE;
			break;
		//run http server
		case 's':
			runmode = RUN_SERVER;
			break;
		//help
		case 'h':
			cout << '\n' << argv[0] << " <file>\n\tRun query from file\n\n"sv
				<< argv[0] << " -c \"select from 'data.csv'\"\n\tRun query from command line argument\n\n"sv
				<< argv[0] << "\n\tRun query from stdin. Later this will be changed to run gui server\n\n"sv;
			exit(0);
			break;
		}

	switch (runmode){
	case RUN_CMD:
		//get query from file or stdin
		if (argc == 2)
			fp = fopen(argv[1], "r");
		else
			fp = stdin;
		while (!feof(fp)){
			fread(cc, 1, 1, fp);
			qs.push_back(*cc);
		}
		break;
	case RUN_SINGLE:
		//already got query string from -c
		break;
	case RUN_SERVER:
		runServer();
		return 0;;
	}
	if (runmode != 1){
	}

	try {
		querySpecs q(qs);
		q.setoutputCsv();
		runquery(q);
	} catch (...) {
		auto ia = current_exception();
		cerr << "Error: "sv << handle_err(ia) << endl;
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
