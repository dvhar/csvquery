#include<string.h>
#include<iostream>
#include "bufreader.h"
using namespace std;

inline void lreader::refresh(){
	auto readb = fread(buf, 1, BS, f);
	line = buf;
	end = line + readb;
	nl = (char*) memchr(line, '\n', end - line);
	if (nl) *nl = 0;
	else line = NULL;
}
inline void lreader::realign(){
	memmove(buf, line, end-line);
}

char* lreader::readline(){
	if (line == NULL){
		refresh();
	} else {
		if (end > line){
			line = nl + 1;
			nl = (char*) memchr(line, '\n', end - line);
			if (nl){
				*nl = 0;
			} else {
				realign();
				refresh();
			}
		} else {
				realign();
				refresh();
		}
	}
	return line;
}

int main(int argc, char** argv){
	int count = 0;
	char* line;
	auto r = lreader((char*) "/home/dave/gits/csvquery/build/test/country.csv");
	while ((line = r.readline())){
		cout << "line: '" << line << "'" << endl;
	}
	return 0;
}
