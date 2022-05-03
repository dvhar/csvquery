#include<string.h>
#include "bufreader.h"

inline void bufreader::refresh(){
	if (readsofar >= fsize){
		done = true;
		return;
	}
	long long offset = end - line;
	auto readb = fread(buf+offset, 1, (single ? biggestline-offset : buffsize-offset), f);
	readsofar += readb;
	line = buf;
	end = line + offset + readb;
	nl = (char*) strchr(line, '\n');
	if (nl){
		*nl = 0;
		linesize = nl - line + 1;
	} else {
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

//return true if success
bool bufreader::addrefresh(int rem){
	if (readsofar >= fsize){
		done = true;
		return false;
	}
	auto readb = fread(buf+rem, 1, (single ? biggestline-rem : buffsize-rem), f);
	readsofar += readb;
	line = buf;
	end = line + rem + readb;
	nl = (char*) strchr(line+rem, '\n');
	if (nl){
		*nl = 0;
		linesize = nl - line + 1;
	} else {
		done = true;
		return false;
	}
	return true;
}
//return amount needed to add to field pointers to revalidated them
long long bufreader::addline(){
	*nl = '\n';
	nl = (char*) strchr(nl+1, '\n');
	if (nl){
		*nl = 0;
		linesize = nl - line + 1;
		if (linesize > biggestline)
			biggestline = linesize;
		return 0;
	} else if (fsize > buffsize) {
		long long offset = buf - line;
		long long rem = end - line;
		memmove(buf, line, rem);
		addrefresh(rem);
		return offset;
	} else { // TODO: read the rest of malformed line
		done = true;
		return 0;
	}
}
