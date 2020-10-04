#include <ctype.h>
#include <filesystem>
#include <sstream>
#include <string>
#include "interpretor.h"


fileReader::fileReader(string& fname) : filename(fname) {
	if (!filesystem::exists(fname)){
		if (fname.length() <= 4 
				|| fname.compare(fname.length() - 4, 4, ".csv"sv) != 0
				|| fname.compare(fname.length() - 4, 4, ".CSV"sv) != 0){
			fname += ".csv";
			if (!filesystem::exists(fname)){
				error("Could not open file "+fname);
			}
		} else {
			error("Could not open file "+fname);
		}
	}
	//filesize optimizations more beneficial for joined files
	small = filesystem::file_size(fname) < (fileno>0? 100:1)*1024*1024;
	fs.open(fname.c_str());	
}
char fileReader::blank = 0;
fileReader::~fileReader(){
	int i = 0;
	for (auto t: vpTypes){
		if (t == T_STRING)
			for (auto &vp: joinValpos[i])
				free(vp.val.s);
		i++;
	}
	for (auto &a : andchains)
		for(u32 i=0; i<a.values.size(); ++i)
			if (a.functionTypes[i] == 2) //funcTypes[T_STRING]
				for (auto d: a.values[i])
					free(d.s);
}
bool fileReader::readlineat(i64 position){
	pos = position;
	if (inmemory){
		if (position < 0){
			entries = entriesVec.data();
			return 0;
		}
		entries = gotrows[position].data();
		return 0;
	} else {
		if (position < 0){
			fill(entriesVec.begin(), entriesVec.end(), csvEntry{&blank,&blank});
			return 0;
		}
		fs.seekline(position);
		return readline();
	}
}
bool fileReader::readline(){
	if (small){
		if (inmemory){
			if (memidx >= numrows)
				return 1;
			pos = memidx;
			entries = gotrows[memidx++].data();
			return 0;
		} else { //haven't loaded into memory yet (infertypes)
			auto line = fs.getline();
			gotbuffers.emplace_front(new char[fs.linesize]);
			buf = gotbuffers.front().get();
			strcpy(buf, line);
		}
	} else {
		pos = prevpos;
		buf = fs.getline();
	}
	entriesVec.clear();
	fieldsFound = 0;
	pos1 = pos2 = buf;
	equoteCount = 0;
	while (1){
		//trim leading space
		while (*pos2 && isblank(*pos2)) ++pos2;
		pos1 = pos2;
		//non-quoted field
		if (*pos2 != '"'){
			while(*pos2 && *pos2 != delim) ++pos2;
			if (*pos2 == delim){
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
			//escape character
			if (pos2>buf && *(pos2-1) == '\\'){
				compactQuote();
				++pos2;
				goto inquote;
			}
			++pos2;
			switch (*pos2){
			// "" escaped quote
			case '"':
				compactQuote();
				++pos2;
				goto inquote;
			//end of line
			case '\0':
				terminator = pos2-1;
				getField();
				return checkWidth();
			case ' ':
				terminator = pos2-1;
				getField();
				while (*pos2 != delim) ++pos2;
				pos1 = ++pos2;
			}
			//end of field
			if (*pos2 == delim){
				if (equoteCount){
					memmove(escapedQuote-equoteCount, escapedQuote, pos2-escapedQuote);
					terminator = pos2-1-equoteCount;
					equoteCount = 0;
				} else {
					terminator = pos2-1;
				}
				getField();
				pos1 = ++pos2;
			}
		}

	}
	return 0;
}
inline void fileReader::compactQuote(){
	if (equoteCount)
		memmove(escapedQuote-equoteCount, escapedQuote, pos2-escapedQuote);
	escapedQuote = pos2;
	++equoteCount;
}
inline bool fileReader::checkWidth(){
	prevpos += (pos2 - buf + 1);
	//numfields is 0 until first line is done
	if (numFields == 0)
		numFields = entriesVec.size();
	entries = entriesVec.data();
	return fieldsFound != numFields;
}
inline void fileReader::getField(){
	//trim trailing whitespace and push pointer
	while (isspace(*(terminator-1))) --terminator;
	*terminator = '\0';
	entriesVec.emplace_back(pos1, terminator);
	++fieldsFound;
}

//initial scan also loads file into mem if small file
void fileReader::inferTypes() {
	readline();
	auto startData = pos;
	//get col names and initialize blank types
	for (u32 i=0; i<entriesVec.size(); ++i) {
		if (noheader)
			colnames.push_back(st("col",i+1));
		else
			colnames.emplace_back(entriesVec[i].val);
		types.push_back(0);
	}
	//get samples and infer types from them
	if (!noheader){
		readline();
		startData = pos;
	}
	if (small){
		for (int j=0;;) {
			if (j<10000){
				for (u32 i=0; i<entriesVec.size(); ++i)
					types[i] = getNarrowestType(entriesVec[i].val, types[i]);
				++j;
			}
			gotrows.push_back(move(entriesVec));
			++numrows;
			if (readline())
				break;
		}
		inmemory = true;
	} else {
		for (int j=0; j<10000; ++j) {
			for (u32 i=0; i<entriesVec.size(); ++i)
				types[i] = getNarrowestType(entriesVec[i].val, types[i]);
			if (readline())
				break;
		}
	}
	for (u32 i=0; i<entriesVec.size(); ++i)
		if (!types[i])
			types[i] = T_STRING; //maybe come up with better way of handling nulls
	prevpos = startData;
	if (small)
		fill(entriesVec.begin(), entriesVec.end(), csvEntry{&blank,&blank});
	else
		fs.seekfull(startData);
}

int fileReader::getColIdx(string& colname){
	if (noheader) return -1;
	for (u32 i = 0; i < colnames.size(); ++i)
		if (colnames[i] == colname)
			return i;
	return -1;
}

void openfiles(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr)
		return;
	if (n->label == N_FROM || n->label == N_JOIN){
		//initialize and put in map
		string& fpath = n->tok1.val;
		string id = st("_f",q.numFiles);
		q.filevec.push_back(make_shared<fileReader>(fpath));
		auto& fr = q.filevec.back();
		fr->id = id;
		fr->fileno = q.numFiles;
		q.files[id] = fr;
		if (n->tok4.id)
			q.files[n->tok4.val] = fr;
		int a = fpath.find_last_of("/\\") + 1;
		int b = fpath.size()-4-a;
		fpath = fpath.substr(a, b);
		q.files[fpath] = fr;

		if (q.options & O_S)
			fr->delim = ' ';
		else
			fr->delim = ',';

		//header options
		if ((q.options & O_NH) != 0)
			fr->noheader = true;
		else
			fr->noheader = false;
		if (n->tok5.id){
			string s = n->tok5.lower();
			if (s == "nh"sv || s == "noheader"sv)
				fr->noheader = true;
			if (s == "h"sv || s == "header"sv)
				fr->noheader = false;
		}
		++q.numFiles;
		fr->inferTypes();
	}
	openfiles(q, n->node1);
	openfiles(q, n->node2);
	openfiles(q, n->node3);
}

regex_t extPattern;
regex_t hidPattern;
shared_ptr<directory> filebrowse(string dir){

	filesystem::path thisdir(dir);
	if (!filesystem::exists(thisdir) || !filesystem::is_directory(thisdir)){
		error(st(dir," is not a directory"));
	}
	auto resp = make_shared<directory>();
	vector<string> others;
	for (auto& f : filesystem::directory_iterator(thisdir)){
		if (!regexec(&hidPattern, f.path().u8string().c_str(), 0,0,0)){
		} else if (filesystem::is_directory(f.status())){
			resp->dirs.push_back(f.path().u8string());
		} else if (filesystem::is_regular_file(f.status())){
			if (!regexec(&extPattern, f.path().u8string().c_str(), 0,0,0))
				resp->files.push_back(f.path().u8string());
			else
				others.push_back(f.path().u8string());
		}
	}
	sort(resp->dirs.begin(), resp->dirs.end());
	sort(resp->files.begin(), resp->files.end());
	sort(others.begin(), others.end());
	resp->files.insert(resp->files.end(), others.begin(), others.end());
	resp->parent = thisdir.parent_path().u8string();
	resp->fpath = thisdir.u8string();
	return resp;
}
