#include<string.h>
#include "bufreader.h"

inline void bufreader::refresh(){
	if (readsofar >= fsize){
		line[0] = 0;
		done = true;
		return;
	}
	int offset = end - line;
	auto readb = fread(buf+offset, 1, (single ? biggestline-offset : buffsize-offset), f);
	readsofar += readb;
	line = buf;
	end = line + offset + readb;
	nl = (char*) strchr(line, '\n');
	if (nl){
		*nl = 0;
		linesize = nl - line + 1;
	} else {
		line[0] = 0;
		done = true;
	}
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

