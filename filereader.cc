#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#define BUFSIZE  1024*1024

class fileReader {
	public:
	string err;
	ifstream fs;
	int numFields;
	int widestField;
	char* pos1;
	char* pos2;
	char* terminator;
	char buf[BUFSIZE];
	vector<char*> line;
	fileReader(string);
	int readline();
	int getField();
	int getWidth(int);
	void print();
};

fileReader::fileReader(string fname){
	numFields = widestField = 0;
	fs = ifstream(fname);	
}
void fileReader::print(){
	for (int i=0; i<line.size(); ++i){
		cerr << "[" << line[i] << "]";
	}
	cerr << endl;
}
int fileReader::readline(){
	fs.getline(buf, BUFSIZE);
	line.clear();
	int fieldsFound = 0;
	pos1 = pos2 = buf;
	while (1){
		//trim leading space
		while (*pos2 && isblank(*pos2)){ ++pos2; }
		pos1 = pos2;
		//non-quoted field
		if (*pos2 != '"'){
			while(*pos2 && *pos2 != ','){
				++pos2;
			}
			if (*pos2 == ','){
				terminator = pos2;
				getField();
				++fieldsFound;
				pos1 = ++pos2;
			}
			if (!(*pos2)){
				terminator = pos2;
				getField();
				++fieldsFound;
				if (getWidth(fieldsFound))
					return -1;
				return 0;
			}
		//quoted field
		} else {
			++pos1; ++pos2;
			//go to next quote
			while(*pos2 && *pos2 != '"') ++pos2;
			//escape character - should compact this
			++pos2;
			switch (*pos2){
			// "" escaped quote
			case '"':
				++pos2;
				break;
			//end of field
			case ',':
				terminator = pos2-1;
				getField();
				++fieldsFound;
				pos1 = ++pos2;
				break;
			//end of field
			case '\0':
				terminator = pos2-1;
				getField();
				++fieldsFound;
				if (getWidth(fieldsFound))
					return -1;
				return 0;
			}
		}

	}
	return 0;
}
int fileReader::getWidth(int fieldsFound){
	int width = pos2 - buf;
	if (width > widestField){
		//numfields is 0 until first line is done
		if (numFields == 0)
			numFields = line.size();
		widestField = width;
	}
	if (fieldsFound != numFields){
		stringstream e;
		e << "should have " << numFields << " fields, found " << fieldsFound;
		err = e.str();
		return -1;
	}
	return 0;
}
int fileReader::getField(){
	//trime trailing whitespace and push pointer
	while (isblank(*(terminator-1))) --terminator;
	*terminator = '\0';
	line.push_back(pos1);
	return 0;
}

int main(){
	//fileReader f("/home/dave/Documents/work/parkingTest.csv");
	fileReader f("/home/dave/Documents/work/Parking_Violations_Issued_-_Fiscal_Year_2017.csv");
	int num = 0;
	while(! f.readline() ){
		//fprintf(stderr,"%d\n",num++);
		f.print();
		//cerr << "printed\n";
	}
	cerr << f.err << endl;
	//fprintf(stderr,"_________________________________________\n");
	//puts(f.buf);
}
