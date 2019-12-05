#include "interpretor.h"
#include <iostream>

int main(){
	int s;
	string qs;
	/*
	while (!feof(stdin)){
		scanf("%c",&s);
		qs.push_back((char)s);
	}
	*/
	//qs = "select dog + cat * count(*) from '/home/dave/Documents/work/cities.csv'";
	qs = "with (c1 * c2  + inc()) as w select dog + (cat * count(*)), case when c1=fat then cat when c1=hat then mad end from '/home/dave/Documents/work/cities.csv' inner join '~/Documents/work/country.csv'";
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
