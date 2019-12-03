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
	qs = "select dog + cat * count(*) from '/home/dave/Documents/work/cities.csv'";
	querySpecs q;
	q.init(qs);
	scanTokens(q);
	parseQuery(q);

}
