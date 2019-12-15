#include "interpretor.h"
#include <iostream>
void init();

int main(int argc, char** argv){

	FILE *fp;
	string qs;
	char c[1];
    if (argc == 2)
        fp = fopen(argv[1], "r");
    else
		fp = stdin;
	while (!feof(fp)){
		fread(c, 1, 1, fp);
		qs.push_back(*c);
	}


	init();
	querySpecs q;
	q.init(qs);
	try {
		scanTokens(q);
		parseQuery(q);
		openfiles(q, q.tree);
		applyTypes(q);
	} catch (const invalid_argument& ia) {
		cerr << "Error: " << ia.what() << '\n';
	}
	return 0;
}

//initialize some stuff
void init(){
	regcomp(&leadingZeroString, "^0[0-9]+$", REG_EXTENDED);
	regcomp(&durationPattern, "^([0-9]+|[0-9]+\\.[0-9]+)\\s(seconds|second|minutes|minute|hours|hour|days|day|weeks|week|years|year|s|m|h|d|w|y)$", REG_EXTENDED);
	regcomp(&intType, "^-?[0-9]+$", REG_EXTENDED);
	regcomp(&floatType, "^-?[0-9]*\\.[0-9]+$", REG_EXTENDED);
}
