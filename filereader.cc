#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#define BUFSIZE  1024*1024

class fileReader {
	public:
	string filename;
	string err;
	FILE* fp;
	int numFields;
	int widestField;
	int quotes;
	int isquote;
	int endfile;
	char* pos1; //begining of field
	char* pos2; //find end of field
	char* fpos; //scan input buffer
	char* endfb;
	char fileBuf[BUFSIZE]; //read from file
	char lineBuf[BUFSIZE]; //put processed line here
	int lastPos;
	vector<char*> line;
	fileReader(string);
	int readline();
	void findquotes();
	void print();
};

fileReader::fileReader(string fname){
	filename = fname;
	numFields = quotes = endfile = 0;
	fp = fopen(fname.c_str(), "r");
	if (fp == NULL) {
		err = "could not open file "+fname;
		return;
	}
	int i = fread(fileBuf, 1, BUFSIZE, fp);
	if (i==0){
		perror("zero size");
		fclose(fp);
	}
	if (feof(fp)){
		fclose(fp);
		endfile = 1;
	}
	cerr << "read " << i << " bytes\n";
	endfb = fileBuf + BUFSIZE;
	fpos = fileBuf;
	pos1 = pos2 = lineBuf;
}
//is non-escaped quote
void fileReader::findquotes(){
	if (*fpos == '"' &&
		(fpos < fileBuf + BUFSIZE - 1 && *(fpos+1) != '"') &&
		(fpos > fileBuf && *(fpos-1) != '\\')) {
		quotes ^= 1;
		isquote = 1;
	}
	isquote = 0;
}
void fileReader::print(){
	for (int i=0; i<line.size()-1; ++i){
		cerr << "[" << line[i] << "]";
		cerr << "(" << line[i+1]-line[i] << ")";
	}
	cerr << endl;
}
int fileReader::readline(){
	int start = 1;
	int fieldsFound = 0;
	char* startpos = fpos;
	while (1){
		//read whole buffer, refill from file
		if (*fpos == '\n') cerr << "got n before inc\n";
		if (fpos == endfb){
			int i = fread(fileBuf, 1, BUFSIZE, fp);
			if (i==0){
				perror("zero size");
				err = "read error";
				return -1;
			}
			cerr << "read " << i << " bytes\n";
			if (feof(fp)){
				fclose(fp);
				err = "end of file";
			}
			fpos = fileBuf;
		}
		//trim leading space
		while (start && isblank(*fpos)){
			++fpos;
			continue;
		}
		//non-quoted field
		if (*fpos != '"'){
			start = 0;
			while(*fpos != ',' && *fpos != '\n' && *fpos != 0 && fpos < endfb){
				//cerr << "'" << (int)*fpos;
				*pos2 = *fpos;
				++pos2; ++fpos;
			}
			if (*fpos == ','){
				*pos2 = '\0';
				//avoid using malloc for all lines but first
				if (numFields == 0)
					line.push_back(pos1);
				else {
					if (fieldsFound > numFields){
						stringstream e;
						e << "should have " << numFields << " fields, found " << fieldsFound;
						err = e.str();
						return -1;
					}
					line[fieldsFound] = pos1;
				}
				++fieldsFound;
				pos1 = pos2+1;
				pos2 = pos1;
				++fpos;
				start = 1;
			}
			if (*fpos == '\n' || *fpos == 0){
				*pos2 = '\0';
				//avoid using malloc for all lines but first
				if (numFields == 0) {
					line.push_back(pos1);
					line.push_back(pos2+1); //don't dereference, used to calculate size of last value
				} else {
					if (fieldsFound > numFields){
						stringstream e;
						e << "should have " << numFields << " fields, found " << fieldsFound;
						err = e.str();
						return -1;
					}
					line[fieldsFound] = pos1;
					line[fieldsFound+1] = pos2+1; //don't dereference, used to calculate size of last value
				}
				++fieldsFound;
				pos1 = pos2 = lineBuf;
				++fpos;
				int width = fpos - startpos;
				if (width > widestField){
					//numfields is 0 until first line is done
					if (numFields == 0)
						numFields = line.size() - 1;
					widestField = width;
				}
				if (line.size()-1 != numFields){
					stringstream e;
					e << "should have " << numFields << " fields, found " << line.size()-1;
					err = e.str();
					return -1;
				}
				return 0;
			}
		//quoted field
		} else {
			cerr << "quotes\n";
			return -1;
		}

	}
	return 0;
}

int main(){
	fileReader f("/home/dave/Documents/work/parkingTest.csv");
	int num = 0;
	while(! f.readline() ){
		fprintf(stderr,"--------------%d--------------------------------------------\n",num++);
		f.print();
	}
	f.print();
	cerr << f.err << endl;
	//fprintf(stderr,"_________________________________________\n");
	//puts(f.fileBuf);
}
