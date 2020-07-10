#include "interpretor.h"
#include "vmachine.h"

//map for printing opcodes
map<int, string> opMap = {
	{CVER,"CVER"}, {CVNO,"CVNO"}, {CVIF,"CVIF"}, {CVIS,"CVIS"}, {CVFI,"CVFI"}, {CVFS,"CVFS"}, {CVDRS,"CVDRS"}, {CVDTS,"CVDTS"}, {CVSI,"CVSI"}, {CVSF,"CVSF"}, {CVSDR,"CVSDR"}, {CVSDT,"CVSDT"}, {IADD,"IADD"}, {FADD,"FADD"}, {TADD,"TADD"}, {DTADD,"DTADD"}, {DRADD,"DRADD"}, {ISUB,"ISUB"}, {FSUB,"FSUB"}, {DTSUB,"DTSUB"}, {DRSUB,"DRSUB"}, {IMULT,"IMULT"}, {FMULT,"FMULT"}, {DRMULT,"DRMULT"}, {IDIV,"IDIV"}, {FDIV,"FDIV"}, {DRDIV,"DRDIV"}, {INEG,"INEG"}, {FNEG,"FNEG"}, {PNEG,"PNEG"}, {IMOD,"IMOD"}, {FMOD,"FMOD"}, {IEXP,"IEXP"}, {FEXP,"FEXP"}, {JMP,"JMP"}, {JMPCNT,"JMPCNT"}, {JMPTRUE,"JMPTRUE"}, {JMPFALSE,"JMPFALSE"}, {JMPNOTNULL_ELSEPOP,"JMPNOTNULL_ELSEPOP"}, {RDLINE,"RDLINE"}, {RDLINE_ORDERED,"RDLINE_ORDERED"}, {PREP_REREAD,"PREP_REREAD"}, {PUT,"PUT"}, {LDPUT,"LDPUT"}, {LDPUTALL,"LDPUTALL"}, {PUTVAR,"PUTVAR"}, {PUTVAR2,"PUTVAR2"}, {LDINT,"LDINT"}, {LDFLOAT,"LDFLOAT"}, {LDTEXT,"LDTEXT"}, {LDDATE,"LDDATE"}, {LDDUR,"LDDUR"}, {LDNULL,"LDNULL"}, {LDLIT,"LDLIT"}, {LDVAR,"LDVAR"}, {HOLDVAR,"HOLDVAR"}, {IEQ,"IEQ"}, {FEQ,"FEQ"}, {TEQ,"TEQ"}, {LIKE,"LIKE"}, {ILEQ,"ILEQ"}, {FLEQ,"FLEQ"}, {TLEQ,"TLEQ"}, {ILT,"ILT"}, {FLT,"FLT"}, {TLT,"TLT"}, {PRINT,"PRINT"}, {PUSH,"PUSH"}, {PUSH_0,"PUSH_0"}, {POP,"POP"}, {POPCPY,"POPCPY"}, {ENDRUN,"ENDRUN"}, {NULFALSE1,"NULFALSE1"}, {NULFALSE2,"NULFALSE2"}, {NDIST,"NDIST"}, {SDIST,"SDIST"}, {PUTDIST,"PUTDIST"}, {LDDIST,"LDDIST"}, {FINC,"FINC"}, {ENCCHA,"ENCCHA"}, {DECCHA,"DECCHA"}, {SAVESORTN,"SAVESORTN"}, {SAVESORTS,"SAVESORTS"}, {SAVEPOS,"SAVEPOS"}, {SORT,"SORT"}, {GETGROUP,"GETGROUP"}, {ONEGROUP,"ONEGROUP"}, {SUMI,"SUMI"}, {SUMF,"SUMF"}, {AVGI,"AVGI"}, {AVGF,"AVGF"}, {STDVI,"STDVI"}, {STDVF,"STDVF"}, {COUNT,"COUNT"}, {MINI,"MINI"}, {MINF,"MINF"}, {MINS,"MINS"}, {MAXI,"MAXI"}, {MAXF,"MAXF"}, {MAXS,"MAXS"}, {NEXTMAP,"NEXTMAP"}, {NEXTVEC,"NEXTVEC"}, {ROOTMAP,"ROOTMAP"}, {LDMID,"LDMID"}, {LDPUTMID,"LDPUTMID"}, {LDPUTGRP,"LDPUTGRP"}, {LDSTDVI,"LDSTDVI"}, {LDSTDVF,"LDSTDVF"}, {LDAVGI,"LDAVGI"}, {LDAVGF,"LDAVGF"}, {ADD_GROUPSORT_ROW,"ADD_GROUPSORT_ROW"}, {FREEMIDROW,"FREEMIDROW"}, {GSORT,"GSORT"}, {READ_NEXT_GROUP,"READ_NEXT_GROUP"}, {NUL_TO_STR,"NUL_TO_STR"}
};

void opcode::print(){
	cerr << ft("code: {: <18}  [{: <2}  {: <2}  {: <2}]\n", opMap[code], p1, p2, p3);
}

void dat::appendToBuffer(string &outbuf){
	if (b & NIL) return;
	// need quoting: , \n, "
	//implement quote escaping after implemented in reader
	static char abnormal[] = {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	static char buf[40];
	char a = 0;
	switch ( b & 7 ) {
	case T_INT:
		sprintf(buf,"%lld",u.i);
		outbuf += buf;
		return;
	case T_FLOAT:
		sprintf(buf,"%.10g",u.f);
		outbuf += buf;
		return;
	case T_DATE: outbuf += datestring(u.i); return;
	case T_DURATION: outbuf += durstring(u.i, nullptr); return;
	case T_STRING:
		for (auto c = (unsigned char*)u.s; *c; c++) a |= abnormal[*c];
		if (!a){
			outbuf += u.s;
		} else {
			outbuf += '"';
			outbuf += u.s;
			outbuf += '"';
		}
		return;
	}
}

bool isTrivial(unique_ptr<node> &n){
	if (n == nullptr) return false;
	if (n->label == N_VALUE && n->tok3.id)
		return true;
	return isTrivial(n->node1);
}
dat parseIntDat(const char* s){
	char* end = NULL;
	dat idat = { { i: strtol(s, &end, 10) }, T_INT };
	if (*end != 0)
		error(str3("Could not parse ", s, " as a number."));
	return idat;
}
dat parseFloatDat(const char* s){
	char* end = NULL;
	dat fdat = { { f: strtof(s, &end) }, T_FLOAT };
	if (*end != 0)
		error(str3("Could not parse ", s, " as a number."));
	return fdat;
}
dat parseDurationDat(const char* s) {
	date_t dur;
	if (parseDuration((char*)s, &dur))
		error(str3("Could not parse ", s, " as duration."));
	if (dur < 0) dur *= -1;
	dat ddat = { { i: dur }, T_DURATION };
	return ddat;
}
dat parseDateDat(const char* s) {
	date_t date;
	if (dateparse_2(s, &date))
		error(str3("Could not parse ", s, " as date."));
	dat ddat = { { i: date }, T_DATE };
	return ddat;
}
dat parseStringDat(const char* s) {
	//may want to malloc
	dat ddat = { { s: (char*)s }, T_STRING, (unsigned int) strlen(s) };
	return ddat;
}
int addBtree(int type, querySpecs *q){
	//returns index of btree
	switch (type){
	case T_INT:
	case T_DATE:
	case T_DURATION:
	case T_FLOAT:
		return q->btn++;
	case T_STRING:
		return q->bts++;
	default:
		error("invalid btree type");
	}
	return 0;
}

vmachine::vmachine(querySpecs &qs){
	q = &qs;
	for (int i=1; i<=q->numFiles; ++i){
		files.push_back(q->files[str2("_f", i)]);
	}

	destrow.resize(q->colspec.count, {0});
	torow = destrow.data();
	torowSize = q->colspec.count;
	if (q->grouping == 1){
		onegroup = vector<dat>(q->midcount, {{0},NIL});
		torow = onegroup.data();
	}
	if (q->grouping)
		groupTree.reset(new rowgroup());
	if (q->sorting){
		if (q->grouping)
			sortgroupsize = q->colspec.count + q->sortcount;
		else
			normalSortVals.resize(q->sortcount);
	}

	stack.resize(50, {0});
	ops = q->bytecode.data();
	quantityLimit = q->quantityLimit;
	bt_nums.resize(q->btn);
	bt_strings.resize(q->bts);
	distinctVal = {0};
}

vmachine::~vmachine(){
	/* enable when ready for release
	if (runmode == RUN_SINGLE) //skip garbage collection if one-off query
		return;
	*/
	FREE2(distinctVal);
	for (auto &d : stack)   FREE2(d);
	for (auto &d : destrow) FREE2(d);
	for (auto &btree : bt_strings)
		for (auto &tcs : btree)
			free(tcs.s);
	for (auto row : groupSorter) free(row);
	for (auto var : groupSortVars) free(var);
	int i = 0;
	for (auto &vec : normalSortVals)
		if (q->sortInfo[i++].second == T_STRING)
			for (auto u : vec)
				free(u.s);
}
querySpecs::~querySpecs(){
	for (auto &d : dataholder){
		FREE2(d);
		if (d.b & RMAL){
			regfree(d.u.r);
			delete d.u.r;
		}
	}
}

varScoper* varScoper::setscope(int f, int p, int s){
	filter = f;
	policy = p;
	scope = s;
	return this;
}
bool varScoper::neededHere(int i, int f){
	int match;
	switch (policy){
	case V_INCLUDES:
		match = f & filter; break;
	case V_EQUALS:
		match = f == filter; break;
	case V_ANY:
		match = 1; break;
	}
	return match && checkDuplicates(i);
}
bool varScoper::checkDuplicates(int i){
	if (duplicates.count(scope) && duplicates[scope].count(i)){
		return false;
	}
	duplicates[scope][i] = 1;
	return true;
}

//add s2 to s1
void strplus(dat &s1, dat &s2){
	if (ISNULL(s1)) { s1 = s2; DISOWN(s2); return; }
	if (ISNULL(s2)) return;
	int newlen = s1.z+s2.z+1;
	if (ISMAL(s1)){
		s1.u.s = (char*) realloc(s1.u.s, newlen);
		strcat(s1.u.s+s1.z-1, s2.u.s);
	} else {
		char* ns = (char*) malloc(newlen);
		strcpy(ns, s1.u.s);
		strcat(ns+s1.z-1, s2.u.s);
		s1.u.s = ns;
	}
	FREE2(s2);
	s1.b |= MAL;
	s1.z = newlen;
}

int crypter::newChacha(string pass){
	chactx ch;
	for (int i=0; i<sizeof(ch.key); i++){
		//encryption only as strong as password, no need to sha256 to get 256 bit key.
		ch.key[i] = pass[i%pass.length()];
	}
	memset(&ch.nonce, 0, sizeof(ch.nonce));
	ctxs.push_back(ch);
	return ctxs.size()-1;
}
//each row has own nonce so need to reinitialize chacha cipher each time
pair<char*, int> crypter::chachaEncrypt(int i, int len, char* input){
	len++; //null terminator
	auto ch = ctxs.data()+i;
	auto rawResult = (uint8_t*) alloca(len+3);
	memcpy(rawResult+3, input, len);
	auto nonce = rand(); //replace with secure version
	rawResult[0] = ch->nonce[0] = nonce;
	rawResult[1] = ch->nonce[1] = nonce >> 8;
	rawResult[2] = ch->nonce[2] = nonce >> 16;
	memcpy(&ch->ctx.key, ch->key, sizeof(ch->ctx.key));
	chacha20_init_context(&ch->ctx, ch->key, ch->nonce, 1); //find out what counter param does
	chacha20_xor(&ch->ctx, rawResult+3, len);
	int finalSize = encsize(len+3);
	auto finalResult = (char*) malloc(finalSize+1);
	b64_encode(rawResult, (unsigned char*)finalResult, len+3);
	finalResult[finalSize]=0;
	return pair<char*,int>(finalResult, finalSize);
}
pair<char*, int> crypter::chachaDecrypt(int i, int len, char* input){
	len++; //null terminator
	auto ch = ctxs.data()+i;
	auto rawResult = (char*) alloca(len);
	int finalSize;
	b64_decode(input, rawResult, len, &finalSize);
	finalSize -= 4;
	ch->nonce[0] = rawResult[0];
	ch->nonce[1] = rawResult[1];
	ch->nonce[2] = rawResult[2];
	memcpy(&ch->ctx.key, ch->key, sizeof(ch->ctx.key));
	chacha20_init_context(&ch->ctx, ch->key, ch->nonce, 1); //find out what counter param does
	chacha20_xor(&ch->ctx, (uint8_t*) rawResult+3, finalSize);
	auto finalResult = (char*) malloc(finalSize+1);
	memcpy(finalResult, rawResult+3, finalSize);
	finalResult[finalSize]=0;
	return pair<char*,int>(finalResult, finalSize);
}


int getSortComparer(querySpecs *q, int i){
	auto info = q->sortInfo[i];
	switch (info.second + info.first * 10){
	case T_INT:
	case T_DATE:
	case T_DURATION:
		return 0;
	case T_INT + 10:
	case T_DATE + 10:
	case T_DURATION + 10:
		return 3;
	case T_FLOAT:
		return 1;
	case T_FLOAT + 10:
		return 4;
	case T_STRING:
		return 2;
	case T_STRING + 10:
		return 5;
	}
	error("invalid sort function");
};

//debug group traverser
/*
void trav(rowgroup &r){
	if (r.meta.rowOrGroup == 2){
		auto&& m = r.getMap();
		cerr << "new map node\n";
		for (auto &e : m){
			auto&& d = (dat) e.first;
			cerr << "    key: " << d.str() << endl;
		}
		for (auto &e : m){
			trav(e.second);
		}
	}
	if (r.meta.rowOrGroup == 1){
		auto&& v = r.getVec();
		for (auto &e : v){
			cerr << "  v: " << e.str() << endl;
		}
	}
}
*/
