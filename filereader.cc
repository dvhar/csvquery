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
	string err;
	FILE* fp;
	int numFields;
	int widestField;
	char* fpos1;
	char* fpos2;
	char* end;
	char* terminator;
	char fileBuf[BUFSIZE];
	vector<char*> line;
	fileReader(string);
	int readline();
	int getField(int);
	int getWidth(char*, int);
	void trim();
	void refill();
	void print();
};

fileReader::fileReader(string fname){
	numFields = 0;
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
	}
	cerr << "read " << i << " bytes\n";
	if (i < BUFSIZE) fileBuf[i] = 0;
	fpos1 = fpos2 = fileBuf;
	end = fileBuf+BUFSIZE-1;
}
void fileReader::print(){
	//cerr << "line size: " << line.size() << endl;
	for (int i=0; i<line.size(); ++i){
		cerr << "[" << line[i] << "]";
	}
	cerr << endl;
}
int fileReader::readline(){
	line.clear();
	int fieldsFound = 0;
	char* startpos;;
	while (1){
		refill();
		startpos = fpos1;
		//trim leading space
		while (*fpos2 && isblank(*fpos2)){ ++fpos2; }
		fpos1 = fpos2;
		//non-quoted field
		if (*fpos2 != '"'){
			//cerr << "non-quote\n";
			while(*fpos2 && *fpos2 != ',' && *fpos2 != '\n'){
				//cerr << "'" << (int)*fpos;
				++fpos2;
			}
			if (*fpos2 == ','){
				getField(fieldsFound);
				++fieldsFound;
				fpos1 = ++fpos2;
			}
			if (*fpos2 == '\n' || !(*fpos2)){
				getField(fieldsFound);
				++fieldsFound;
				fpos1 = ++fpos2;
				getWidth(startpos, fieldsFound);
				return 0;
			}
		//quoted field
		} else {
			//cerr << "quote\n";
			++fpos1; ++fpos2;
			//go to next quote
			while(*fpos2 && *fpos2 != '"') ++fpos2;
			//backslash escape
			++fpos2;
			switch (*fpos2){
			// "" escaped quote
			case '"':
				++fpos2;
				break;
			//end of field
			case ',':
				--fpos2;
				getField(fieldsFound);
				++fieldsFound;
				fpos1 = ++fpos2;
				break;
			//end of field
			case '\0':
			case '\n':
				--fpos2;
				getField(fieldsFound);
				++fieldsFound;
				fpos1 = ++fpos2;
				getWidth(startpos, fieldsFound);
				return 0;
			}
		}

	}
	return 0;
}
void fileReader::trim(){
	//cerr << "trim\n";
	terminator = fpos2;
	while (isblank(*(terminator-1))) --terminator;
	*terminator = '\0';
}
void fileReader::refill(){
	//cerr << "refill\n";
	char* temp = fpos2;
	while (*temp != '\n'){
		++temp;
		if (temp == end){
			temp = fileBuf;
			//move line to beginning of buffer and refill it
			while (fpos1 < end){
				*(temp++) = *(++fpos1);
			}
			fpos1 = fileBuf;
			int sz = temp-fpos1;
			int i = fread(temp, 1, BUFSIZE-sz, fp);
			if (i==0){
				perror("zero size");
				fclose(fp);
			}
			if (feof(fp)){
				fclose(fp);
			}
			//cerr << "read " << i << " bytes\n";
			fpos2 = temp;
		}
	}
}
int fileReader::getWidth(char* startpos, int fieldsFound){
	//cerr << "getwidth\n";
	int width = fpos2 - startpos;
	if (width > widestField){
		//numfields is 0 until first line is done
		if (numFields == 0)
			numFields = line.size();
		widestField = width;
	}
	if (fieldsFound != numFields){
		stringstream e;
		e << "should have " << numFields << " fields, found " << line.size()-1;
		err = e.str();
		return -1;
	}
	return 0;
}
int fileReader::getField(int n){
	trim();
	//avoid using malloc for all lines but first
	//cerr << "get field " << n << " size: " << terminator-fpos1 << endl;
	line.push_back(fpos1);
	return 0;
}

int main(){
	//fileReader f("/home/dave/Documents/work/parkingTest.csv");
	fileReader f("/home/dave/Documents/work/Parking_Violations_Issued_-_Fiscal_Year_2017.csv");
	int num = 0;
	while(! f.readline() ){
		//fprintf(stderr,"--------------%d--------------------------------------------\n",num++);
		//f.print();
		//cerr << "printed\n";
	}
	cerr << f.err << endl;
	//fprintf(stderr,"_________________________________________\n");
	//puts(f.fileBuf);
}
