#include <ctype.h>
#include <fstream>
#include <sstream>
#include <string>
#include "interpretor.h"


fileReader::fileReader(string fname){
	numFields = widestField = 0;
	fs = ifstream(fname);	
	pos = fs.tellg();
}
void fileReader::print(){
	for (auto &l : line){
		cerr << "[" << l << "]";
	}
	cerr << endl;
}
int fileReader::readline(){
	pos = fs.tellg();
	fs.getline(buf, BUFSIZE);
	line.clear();
	sizes.clear();
	fieldsFound = 0;
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
				if (getWidth())
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
			//end of line
			case '\0':
				terminator = pos2-1;
				getField();
				++fieldsFound;
				if (getWidth())
					return -1;
				return 0;
			}
		}

	}
	return 0;
}
int fileReader::getWidth(){
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
	//trim trailing whitespace and push pointer
	while (isblank(*(terminator-1))) --terminator;
	*terminator = '\0';
	line.push_back(pos1);
	sizes.push_back(terminator-pos1);
	return 0;
}

void fileReader::inferTypes() {
	readline();
	auto startData = pos;
	//get col names and initialize blank types
	for (int i=0; i<line.size(); ++i) {
		if (noheader)
			colnames.push_back(nstring("col%d",i+1));
		else
			colnames.push_back(string(line[i]));
		types.push_back(0);
	}
	//get samples and infer types from them
	if (!noheader){
		readline();
		startData = pos;
	}
	for (int j=0; j<10000; ++j) {
		for (int i=0; i<line.size(); ++i)
			types[i] = getNarrowestType(line[i], types[i]);
		if (readline()){
			break;
		}
	}
	fs.clear();
	fs.seekg(startData);
}

int fileReader::getColIdx(string colname){
	if (noheader) return -1;
	for (int i = 0; i < colnames.size(); ++i)
		if (colnames[i] == colname)
			return i;
	return -1;
}

void openfiles(querySpecs &q, unique_ptr<node> &n){
	static int fileNo = 1;
	if (n == nullptr)
		return;
	if (n->label == N_FROM || n->label == N_JOIN){
		//initialize and put in map
		string path = n->tok1.val;
		string id = nstring("_f%d",fileNo++);
		shared_ptr<fileReader> fr(new fileReader(path));
		fr->id = id;
		q.files[id] = fr;
		if (n->tok4.id)
			q.files[n->tok4.val] = fr;
		int a = path.find_last_of("/\\") + 1;
		int b = path.size()-4-a;
		path = path.substr(a, b);
		q.files[path] = fr;

		//header options
		if ((q.options & O_NH) != 0)
			fr->noheader = true;
		else
			fr->noheader = false;
		if (n->tok3.id){
			string s = n->tok3.lower();
			if (s == "n" || s == "noheader")
				fr->noheader = true;
			if (s == "h" || s == "header")
				fr->noheader = false;
		}
		//get types
		fr->inferTypes();
	}
	openfiles(q, n->node1);
	openfiles(q, n->node2);
	openfiles(q, n->node3);
}
