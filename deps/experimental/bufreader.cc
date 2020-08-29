#include<string.h>
#include<iostream>
#include "bufreader.h"
using namespace std;
//#define EX

inline void lreader::refresh(){
	auto readb = fread(buf, 1, (single ? biggest : BS), f);
	line = buf;
	end = line + readb;
	nl = (char*) memchr(line, '\n', end - line);
	if (nl){
		*nl = 0;
		linesize = nl - line + 1;
	} else line = NULL;
}
inline void lreader::realign(){
	memmove(buf, line, end-line);
}

char* lreader::readline(){
	if (single || line == NULL){
		refresh();
	} else {
		if (end > line){
			line = nl + 1;
			nl = (char*) memchr(line, '\n', end - line);
			if (nl){
				*nl = 0;
				linesize = nl - line + 1;
			} else {
				if (end > line)
					realign();
				refresh();
			}
		} else {
				refresh();
		}
	}
	if (linesize > biggest)
		biggest = linesize;
	return line;
}

#ifdef EX
int main(int argc, char** argv){
	int count = 0;
	char* line;
	lreader r;
	r.open((char*) "/home/dave/gits/csvquery/build/ptest.csv");
	while ((line = r.readline())){
		//cout << "line: '" << line << "'" << endl;
		cout << "\n------------\n" << line;
	}
	return 0;
}
#endif
