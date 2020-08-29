#include<string.h>
#include "bufreader.h"
using namespace std;
//#define EX

static int c(char* s){
	int i = 0;
	while (*s) 
		if (*s++ == ',')
			++i;
	return i;
}

inline void lreader::refresh(){
	int offset = end - line;
	auto readb = fread(buf+offset, 1, (single ? biggest : BS-offset), f);
	line = buf;
	end = line + offset + readb;
	nl = (char*) memchr(line, '\n', end - line);
	if (nl){
		*nl = 0;
		linesize = nl - line + 1;
	} else line = NULL;
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
				linesize = nl - line + 1;
			} else {
				if (end > line)
					memmove(buf, line, end-line);
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
#include<iostream>
int main(int argc, char** argv){
	int count = 0;
	char* line;
	lreader r;
	r.open((char*) "/home/dave/gits/csvquery/build/ptest.csv");
	while ((line = r.readline())){
		count++;
		if (count == 100)break;
		cout << "c: " <<c(line)<<" count: " << count<<endl;
		if (count == 19)
			cout << "line: '" << line << "'" << endl;
		//cout << "\n----"<<count<<"--------\n" << line;
	}
	cout << "line: " << count<<endl;
	return 0;
}
#endif
