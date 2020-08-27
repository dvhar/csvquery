#include<string.h>
#include<iostream>
#include<fstream>
using namespace std;

#define CUSTOM

const int BS = 1024 * 1024 * 8;
char buf[BS];
char* fname =  (char*) "../../build/ptest.csv";
#ifdef CUSTOM
auto f = fopen(fname,"r");
char* readline(){
	static char* end;
	static char* nl;
	static char* line = NULL;
	if (line == NULL){
		auto readb = fread(buf, 1, BS, f);
		line = buf;
		end = line + readb;
		nl = (char*) memchr(line, '\n', end - line);
		*nl = 0;
	} else {
		if (end - line > 0){
			line = nl + 1;
			nl = (char*) memchr(line, '\n', end - line);
			if (nl){
				*nl = 0;
			} else {
				int rem = end - line;
				memcpy(buf, line, rem);
				auto readb = fread(buf+rem, 1, BS-rem, f);
				line = buf;
				end = line + BS;
				nl = (char*) memchr(line, '\n', end - line);
				*nl = 0;
			}
		} else {
				int rem = end - line;
				memcpy(buf, line, rem);
				auto readb = fread(buf+rem, 1, BS-rem, f);
				line = buf;
				end = line + BS;
				nl = (char*) memchr(line, '\n', end - line);
				*nl = 0;
		}
	}
	return line;
}
#else
ifstream ff(fname);
#endif

int main(int argc, char** argv){
	int count = 0;
#ifdef CUSTOM
	while (!feof(f)){
		readline();
	}
#else
	while (ff.good()){
		ff.getline(buf, BS);
	}
#endif
	return 0;
}
