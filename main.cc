#include "interpretor.h"
#include <iostream>
void init();

regex_t leadingZeroString;
regex_t durationPattern;

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
	unique_ptr<node> tree;
	try {
		scanTokens(q);
		tree = parseQuery(q);
	} catch (const invalid_argument& ia) {
		cerr << "Error: " << ia.what() << '\n';
	}
	printTree(tree,0);

}

//initialize some stuff
void init(){
	regcomp(&leadingZeroString, "^0\\d+$", REG_EXTENDED);
	regcomp(&durationPattern, "^(\\d+|\\d+\\.\\d+)\\s(seconds|second|minutes|minute|hours|hour|days|day|weeks|week|years|year|s|m|h|d|w|y)$", REG_EXTENDED);
}
