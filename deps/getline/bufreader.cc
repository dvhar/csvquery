#include<string.h>
#include "bufreader.h"

void bufreader::open(const char* fname){
	end = line = NULL;
	linesize = 0;
	biggestline = 0;
	single = false;
	buf[BS] = 0;
	f = fopen(fname,"r");
}

inline void bufreader::refresh(){
	int offset = end - line;
	auto readb = fread(buf+offset, 1, (single ? biggestline : BS-offset), f);
	line = buf;
	end = line + offset + readb;
	nl = (char*) strchr(line, '\n');
	if (nl){
		*nl = 0;
		linesize = nl - line + 1;
	} else line[0] = 0;
}

char* bufreader::getline(){
	if (line == NULL){
		refresh();
	} else {
		if (end > line){
			line = nl + 1;
			nl = (char*) strchr(line, '\n');
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
	if (linesize > biggestline)
		biggestline = linesize;
	return line;
}

