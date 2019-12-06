#include "interpretor.h"
#include <iostream>

int main(int argc, char** argv){

	FILE *fp;
	string qs;
	int s;
    if (argc == 2)
        fp = fopen(argv[1], "r");
    else
		fp = stdin;
	while (!feof(fp)){
		fscanf(fp,"%c",&s);
		qs.push_back((char)s);
	}


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
