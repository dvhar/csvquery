#include <ctype.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>
#include "interpretor.h"


fileReader::fileReader(string fname){
	if (!filesystem::exists(fname)){
		if (fname.length() <= 4 
				|| fname.compare(fname.length() - 4, 4, ".csv") != 0
				|| fname.compare(fname.length() - 4, 4, ".CSV") != 0){
			fname += ".csv";
			if (!filesystem::exists(fname)){
				error("Could not open file "+fname);
			}
		} else {
			error("Could not open file "+fname);
		}
	}
	pos = prevpos = numFields = 0;
	filename = fname;
	fs = ifstream(fname);	
}
fileReader::~fileReader(){
	fs.close();
}
void fileReader::print(){
	for (auto &e : entries){
		cerr << "[" << e.val << "]";
	}
	cerr << endl;
}
bool fileReader::readlineat(int64 position){
	fs.clear();
	fs.seekg(position);
	return readline();
}
bool fileReader::readline(){
	pos = prevpos;
	fs.getline(buf, BUFSIZE);
	entries.clear();
	fieldsFound = 0;
	pos1 = pos2 = buf;
	while (1){
		//trim leading space
		while (*pos2 && isblank(*pos2)) ++pos2;
		pos1 = pos2;
		//non-quoted field
		if (*pos2 != '"'){
			while(*pos2 && *pos2 != ',') ++pos2;
			if (*pos2 == ','){
				terminator = pos2;
				getField();
				pos1 = ++pos2;
			}
			if (!(*pos2)){
				terminator = pos2;
				getField();
				return checkWidth();
			}
		//quoted field
		} else {
			++pos1; ++pos2;
			inquote:
			//go to next quote
			while(*pos2 && *pos2 != '"') ++pos2;
			//escape character - should compact this
			++pos2;
			switch (*pos2){
			// "" escaped quote
			case '"':
				++pos2;
				goto inquote;
			//end of field
			case ',':
				terminator = pos2-1;
				getField();
				pos1 = ++pos2;
				break;
			//end of line
			case '\0':
				terminator = pos2-1;
				getField();
				return checkWidth();
			}
		}

	}
	return 0;
}
inline bool fileReader::checkWidth(){
	prevpos += (pos2 - buf + 1);
	//numfields is 0 until first line is done
	if (numFields == 0)
		numFields = entries.size();
	return fieldsFound != numFields;
}
inline void fileReader::getField(){
	//trim trailing whitespace and push pointer
	while (isblank(*(terminator-1))) --terminator;
	*terminator = '\0';
	entries.emplace_back(csvEntry{pos1, terminator});
	++fieldsFound;
}

void fileReader::inferTypes() {
	readline();
	auto startData = pos;
	//get col names and initialize blank types
	for (int i=0; i<entries.size(); ++i) {
		if (noheader)
			colnames.push_back(str2("col",i+1));
		else
			colnames.push_back(string(entries[i].val));
		types.push_back(0);
	}
	//get samples and infer types from them
	if (!noheader){
		readline();
		startData = pos;
	}
	for (int j=0; j<10000; ++j) {
		for (int i=0; i<entries.size(); ++i)
			types[i] = getNarrowestType(entries[i].val, types[i]);
		if (readline()){
			break;
		}
	}
	for (int i=0; i<entries.size(); ++i)
		if (!types[i])
			types[i] = T_STRING; //maybe come up with better way of handling nulls
	fs.clear();
	fs.seekg(startData);
	prevpos = startData;
}

int fileReader::getColIdx(string colname){
	if (noheader) return -1;
	for (int i = 0; i < colnames.size(); ++i)
		if (colnames[i] == colname)
			return i;
	return -1;
}

void fileReader::numlines(){
	auto f = fopen(filename.c_str(),"r");
	int lines=0, nbytes;
	char *bufp, *bufe;
	while((nbytes = fread(buf, 1, BUFSIZE, f))){
		bufp = buf;
		bufe = buf + nbytes;
		while((bufp = (char*) memchr(bufp, '\n', bufe-bufp))){
			++bufp;
			++lines;
		}
	}
	fclose(f);
	linecount = lines - (noheader ? 0 : 1);
}

void openfiles(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr)
		return;
	if (n->label == N_FROM || n->label == N_JOIN){
		//initialize and put in map
		++q.numFiles;
		string path = n->tok1.val;
		string id = str2("_f",q.numFiles);
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
			if (s == "nh" || s == "noheader")
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
