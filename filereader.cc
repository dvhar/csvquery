#include <ctype.h>
#include <boost/filesystem.hpp>
#include <sstream>
#include <string>
#include "interpretor.h"


fileReader::fileReader(string& fname, querySpecs &qs) : filename(fname), q(&qs) {
	fileno = qs.numFiles;
	if (!boost::filesystem::exists(fname)){
		if (!regex_match(fname,extPat)){
			fname += ".csv";
			if (!boost::filesystem::exists(fname)){
				error("Could not open file ",fname);
			}
		} else {
			error("Could not open file ",fname);
		}
	}
	i64 optisize = br.buffsize;
	if (fileno > 0){
		int jmegs = max(100, totalram() / 20);
		if (qs.sorting)
			jmegs = max(100, jmegs/2);
		optisize = jmegs * 1024 * 2024;
	}
	br.open(fname.c_str(), optisize);	
	small = br.fsize <= br.buffsize;
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
		entries = gotrows[position].get();
		return 0;
	} else {
		if (position < 0){
			fill(entriesVec.begin(), entriesVec.end(), csvEntry{&blank,&blank});
			return 0;
		}
		br.seekline(position);
		return readline();
	}
}
bool fileReader::readline(){
	if (inmemory){
		if (memidx >= numrows)
			return 1;
		pos = memidx;
		entries = gotrows[memidx++].get();
		return 0;
	} else {
		pos = prevpos;
		buf = br.getline();
		if (br.done) return 1;
	}
	entriesVec.clear();
	fieldsFound = 0;
	pos1 = pos2 = buf;
	equoteCount = 0;
	while (1){
		//trim leading space
		while (*pos2 && isspace(*pos2)) ++pos2;
		pos1 = pos2;
		//non-quoted field
		if (*pos2 != '"'){
			while(*pos2 && *pos2 != delim) ++pos2;
			if (*pos2 == delim){
				terminator = pos2;
				getField();
				pos1 = ++pos2;
			}
			if (!*pos2){
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
			//end of line. TODO: turn this into multi-line field
			if (!*pos2){
				getQuotedField();
				return checkWidth();
			}
			//nonstandard escape character
			if (*(pos2-1) == '\\' && !nextIsDelim()){
				compactQuote();
				++pos2;
				goto inquote;
			}
			++pos2;
			switch (*pos2){
			// line ends after quote
			case '\0':
			case '\n':
			case '\r':
				getQuotedField();
				return checkWidth();
			// "" escaped quote
			case '"':
				compactQuote();
				++pos2;
				goto inquote;
			default:
				//end of field
				if (*pos2 == delim){
					getQuotedField();
					pos1 = ++pos2;
				} else if (auto d = nextIsDelim()){
					getQuotedField();
					pos1 = pos2 = d+1;
				} else {
					//unescaped quote in middle of the damn field
					++pos2;
					goto inquote;
				}
			}
		}

	}
	return 0;
}
inline char* fileReader::nextIsDelim(){
	auto nextc = pos2+1;
	while (*nextc == ' ')
		++nextc;
	if (*nextc == delim || *nextc == '\n' || *nextc == '\r' || *nextc == 0)
		return nextc;
	return nullptr;
}
inline void fileReader::getQuotedField(){
	if (equoteCount){
		memmove(escapedQuote-equoteCount, escapedQuote, pos2-escapedQuote);
		terminator = pos2-1-equoteCount;
		equoteCount = 0;
	} else {
		terminator = pos2-1;
	}
	getField();
}
inline void fileReader::compactQuote(){
	if (equoteCount)
		memmove(escapedQuote-equoteCount, escapedQuote, pos2-escapedQuote);
	escapedQuote = pos2;
	++equoteCount;
}
inline bool fileReader::checkWidth(){
	prevpos += (pos2 - buf + 1);
	if (numFields == 0)
		numFields = entriesVec.size();
	entries = entriesVec.data();
	return fieldsFound != numFields;
}
inline void fileReader::getField(){
	while (isspace(*(terminator-1))) --terminator;
	*terminator = '\0';
	entriesVec.emplace_back(pos1, terminator);
	++fieldsFound;
}

//initial scan also loads file into mem if small file
void fileReader::inferTypes() {
	if (readline())
		error("Error reading first line from ",filename);
	if (autoheader)
		for (auto &e : entriesVec)
			if (isInt(e.val) || isFloat(e.val)){
				noheader = true;
				break;
			}
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
		if (readline())
			error("Error reading first line of data from ",filename);
		startData = pos;
	}
	if (small){
		int samples = 0;
		do {
			if (samples<10000){
				for (u32 i=0; i<entriesVec.size(); ++i)
					types[i] = getNarrowestType(entriesVec[i].val, types[i]);
				++samples;
			}
			gotrows.emplace_back(entriesVec.release());
			++numrows;
		} while (!readline());
		inmemory = true;
	} else {
		for (int samples=0; samples<10000; ++samples) {
			for (u32 i=0; i<entriesVec.size(); ++i)
				types[i] = getNarrowestType(entriesVec[i].val, types[i]);
			if (readline())
				break;
		}
	}
	for (u32 i=0; i<entriesVec.size(); ++i)
		if (!types[i])
			types[i] = T_STRING;
	prevpos = startData;
	if (small)
		fill(entriesVec.begin(), entriesVec.end(), csvEntry{&blank,&blank});
	else
		br.seekfull(startData);
}

int fileReader::getColIdx(string& colname){
	if (noheader) return -1;
	for (u32 i = 0; i < colnames.size(); ++i)
		if (colnames[i] == colname)
			return i;
	return -1;
}

void openfiles(querySpecs &q, astnode &n){
	if (n == nullptr)
		return;
	if (n->label == N_FROM || n->label == N_JOIN){
		string& fpath = n->tok1.val;
		string id = st("_f",q.numFiles);
		q.filevec.push_back(make_shared<fileReader>(fpath, q));
		auto& fr = q.filevec.back();
		fr->id = id;
		q.filemap[id] = fr;
		if (n->tok4.id)
			q.filemap[n->tok4.val] = fr;
		int a = fpath.find_last_of("/\\") + 1;
		int b = fpath.size()-4-a;
		fpath = fpath.substr(a, b);
		q.filemap[fpath] = fr;

		if (q.options & O_S)
			fr->delim = ' ';
		else if (q.options & O_P)
			fr->delim = '|';
		else if (q.options & O_T)
			fr->delim = '\t';
		else
			fr->delim = ',';

		//header options
		fr->autoheader = globalSettings.autoheader;
		if ((q.options & O_H) != 0)
			fr->noheader = fr->autoheader = false;
		if ((q.options & O_NH) != 0)
			fr->noheader = true;
		if ((q.options & O_AH) != 0)
			fr->autoheader = true;
		//file opts override global opts
		if (n->tok5.id){
			string s = n->tok5.lower();
			if (s == "nh"){
				fr->noheader = true;
			} if (s == "h") {
				fr->noheader = fr->autoheader = false;
			} if (s == "ah") {
				fr->autoheader = true;
			}
		}
		++q.numFiles;
		fr->inferTypes();
	}
	openfiles(q, n->node1);
	openfiles(q, n->node2);
	openfiles(q, n->node3);
}

shared_ptr<directory> filebrowse(string dir){

	boost::filesystem::path thisdir(dir);
	if (!boost::filesystem::exists(thisdir) || !boost::filesystem::is_directory(thisdir)){
		error(st(dir," is not a directory"));
	}
	auto resp = make_shared<directory>();
	vector<string> others;
	for (auto& f : boost::filesystem::directory_iterator(thisdir)){
		auto&& S = f.path().string();
		if (regex_match(S,hidPat)){
		} else if (boost::filesystem::is_directory(f.status())){
			resp->dirs.push_back(S);
		} else if (boost::filesystem::is_regular_file(f.status())){
			if (regex_match(S,extPat))
				resp->files.push_back(S);
			else
				others.push_back(S);
		}
	}
	sort(resp->dirs.begin(), resp->dirs.end());
	sort(resp->files.begin(), resp->files.end());
	sort(others.begin(), others.end());
	resp->files.insert(resp->files.end(), others.begin(), others.end());
	resp->parent = thisdir.parent_path().string();
	resp->fpath = thisdir.string();
	return resp;
}
