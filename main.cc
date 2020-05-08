#include "interpretor.h"
#include <iostream>
void init();

int main(int argc, char** argv){

	cerr << ft("running {}!\n", argv[0]);
	init();

	FILE *fp;
	string qs;
	int whatdo = 0;;
	char cc[1], c;

	while((c = getopt(argc, argv, "c:hs")) != -1)
		switch(c){
		//run query from command line argument
		case 'c':
			qs = string(optarg);
			whatdo = 1;
			break;
		//run http server
		case 's':
			whatdo = 2;
			break;
		//help
		case 'h':
			cerr << "\n" << argv[0] << " <file>\n\tRun query from file\n\n"
				<< argv[0] << " -c \"select from 'data.csv'\"\n\tRun query from command line argument\n\n"
				<< argv[0] << "\n\tRun query from stdin. Later this will be changed to run gui server\n\n";
			exit(0);
			break;
		}

	switch (whatdo){
	case 0:
		//get query from file or stdin
		if (argc == 2)
			fp = fopen(argv[1], "r");
		else
			fp = stdin;
		while (!feof(fp)){
			if (fread(cc, 1, 1, fp)) error("input error");
			qs.push_back(*cc);
		}
		break;
	case 1:
		//already got query string from -c
		break;
	case 2:
		runServer();
		return 0;;
	}
	if (whatdo != 1){
	}

	querySpecs q(qs);
	try {
		scanTokens(q);
		parseQuery(q);
		openfiles(q, q.tree);
		applyTypes(q);
		analyzeTree(q);
		//printTree(q.tree, 0);
		codeGen(q);
		runquery(q);
	} catch (const invalid_argument& ia) {
		cerr << "Error: " << ia.what() << '\n';
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
