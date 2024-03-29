#include <ctype.h>
#include <boost/filesystem.hpp>
#include <sstream>
#include <string>
#include "interpretor.h"

fileReader::fileReader(string& fname, querySpecs &qs) : filename(fname), q(&qs) {
	spaces[' '] = spaces['\n'] = spaces['\r'] = spaces['\f'] = spaces['\v'] = 1;
	fileno = qs.numFiles;
	i64 optisize = br.buffsize;
	if (fileno > 0){
		i64 jmegs = max((i64)100, totalram() / 20);
		if (qs.sorting)
			jmegs = max((i64)100, jmegs/2);
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
		buf = br.getline();
		pos = br.linepos();
		if (br.done) return 1;
	}
	entriesVec.clear();
	fieldsFound = equoteCount = 0;
	pos1 = pos2 = buf;
	while (1){
		//trim leading space
		while (spaces[(u8)*pos2]) ++pos2;
		pos1 = pos2;
		//non-quoted field, or non-csv file since they generally do not follow csv quoting standards
		if (*pos2 != '"' || delim != ','){
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
			//line ends before reaching closing quote
			if (!*pos2){
				auto offset = br.addline();
				if (offset) {
					for (auto& e : entriesVec){
						e.terminator += offset;
						e.val += offset;
					}
					pos1 += offset;
					pos2 += offset;
				}
				if (br.done){
					//file ends on unfinished field TODO: find better solution
					return checkWidth();
				} else
					goto inquote;
			}
			//nonstandard escape character - only use when haven't found normal excape
			if (*(pos2-1) == '\\' && !nextIsDelim() && !normalescape){
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
				normalescape = true;
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
	if (*nextc == delim || *nextc == 0 || *nextc == '\n' || *nextc == '\r')
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
	if (numFields == 0)
		numFields = entriesVec.size();
	entries = entriesVec.data();
	return fieldsFound != numFields;
}
inline void fileReader::getField(){
	while (spaces[(u8)*(terminator-1)] && terminator>pos1) --terminator;
	*terminator = '\0';
	entriesVec.emplace_back(pos1, terminator);
	++fieldsFound;
}

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
	if (small)
		fill(entriesVec.begin(), entriesVec.capend(), csvEntry{&blank,&blank});
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

class opener {
	i64 totalsize = 0;
	querySpecs *q;
	public:
	void openfiles(astnode&);
	bool checkAliases(astnode&);
	opener(querySpecs &qs): q(&qs){};
};
void opener::openfiles(astnode &n){
	if (n == nullptr)
		return;
	if (n->label == N_FILE){
		if (!checkAliases(n))
			findExtension(n->filename());
		string& fpath = n->filename();
		string id = st("_f",q->numFiles);
		q->filevec.push_back(make_shared<fileReader>(fpath, *q));
		auto& fr = q->filevec.back();
		fr->id = id;
		q->filemap[id] = fr;
		if (n->tok4.id) q->filemap[n->filealias()] = fr;
		if (n->tok2.id) q->filemap[n->tok2.val] = fr;
		if (regex_match(fpath, tsvPat))
			fr->delim = '\t';
		int a = fpath.find_last_of("/\\") + 1;
		int b = fpath.size()-4-a;
		fpath = fpath.substr(a, b);
		q->filemap[fpath] = fr;

		fr->autoheader = globalSettings.autoheader;
		//global file options
		if (q->options & O_S) fr->delim = ' ';
		if (q->options & O_P) fr->delim = '|';
		if (q->options & O_T) fr->delim = '\t';
		if (q->options & O_SC) fr->delim = ';';
		if (q->options & O_H)  fr->noheader = fr->autoheader = false;
		if (q->options & O_NH) fr->noheader = true;
		if (q->options & O_AH) fr->autoheader = true;
		//override with local file options
		int fileopts = n->optionbits();
		if (fileopts & O_S)  fr->delim = ' ';
		if (fileopts & O_P)  fr->delim = '|';
		if (fileopts & O_T)  fr->delim = '\t';
		if (fileopts & O_SC)  fr->delim = ';';
		if (fileopts & O_H)  fr->noheader = fr->autoheader = false;
		if (fileopts & O_NH) fr->noheader = true;
		if (fileopts & O_AH) fr->autoheader = true;
		if (fr->delim != '\t')
			fr->spaces['\t'] = 1;

		++q->numFiles;
		fr->inferTypes();
		totalsize += fr->size();
		return;
	}
	openfiles(n->node1);
	openfiles(n->node2);
	openfiles(n->node3);
}
void openfiles(querySpecs &qs){
	opener o(qs);
	o.openfiles(qs.tree);
}

shared_ptr<directory> filebrowse(string dir){
	boost::filesystem::path thisdir(dir);
	if (!boost::filesystem::exists(thisdir) || !boost::filesystem::is_directory(thisdir)){
		error(dir," is not a directory");
	}
	auto resp = make_shared<directory>();
	vector<string> others;
	for (auto& f : boost::filesystem::directory_iterator(thisdir)){
		auto&& S = f.path().string();
		if (regex_match(S,hidPat)){
		} else if (boost::filesystem::is_directory(f.status())){
			resp->dirs.push_back(S);
		} else if (boost::filesystem::is_regular_file(f.status())){
			if (regex_match(S,csvPat) || regex_match(S,tsvPat))
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

void findExtension(string& fname){
	if (!boost::filesystem::exists(fname)){
		if (!regex_match(fname,csvPat)){
			fname += ".csv";
			if (!boost::filesystem::exists(fname)){
				error("Could not find file ",fname);
			}
		} else {
			error("Could not find file ",fname);
		}
	}
}
bool opener::checkAliases(astnode& n){
	if (regex_match(n->tok1.val,filelike))
		return false;
	string aliasfile = st(globalSettings.configdir,SLASH,"alias-",n->tok1.val,".txt");
	if (!boost::filesystem::exists(aliasfile))
		return false;
	n->tok2 = n->tok1;
	ifstream afile(aliasfile);
	getline(afile, n->tok1.val);
	afile >> n->optionbits();
	return true;
}
