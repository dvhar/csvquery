#include<string>
#include<iostream>
#include<map>
#include "interpretor.h"
using namespace std;

int main(){


	//regex_t durationPattern;
	regcomp(&durationPattern, "^([0-9]+|[0-9]+\\.[0-9]+)\\s(seconds|second|minutes|minute|hours|hour|days|day|weeks|week|years|year|s|m|h|d|w|y)$", REG_EXTENDED);
	if (regexec(&durationPattern, "77.9 days", 0, NULL, 0)) {
		cerr << "no reg match\n";
	} else {
		puts("good");
	}
	return 0;
}
