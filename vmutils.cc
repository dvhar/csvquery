#include "interpretor.h"
#include "vmachine.h"

//map for printing opcodes
map<int, string> opMap = { 
{CVER,"CVER"}, {CVNO,"CVNO"}, {CVIF,"CVIF"}, {CVIS,"CVIS"}, {CVFI,"CVFI"}, {CVFS,"CVFS"}, {CVDRS,"CVDRS"}, {CVDTS,"CVDTS"}, {CVSI,"CVSI"}, {CVSF,"CVSF"}, {CVSDR,"CVSDR"}, {CVSDT,"CVSDT"}, {IADD,"IADD"}, {FADD,"FADD"}, {TADD,"TADD"}, {DTADD,"DTADD"}, {DRADD,"DRADD"}, {ISUB,"ISUB"}, {FSUB,"FSUB"}, {DTSUB,"DTSUB"}, {DRSUB,"DRSUB"}, {IMULT,"IMULT"}, {FMULT,"FMULT"}, {DRMULT,"DRMULT"}, {IDIV,"IDIV"}, {FDIV,"FDIV"}, {DRDIV,"DRDIV"}, {INEG,"INEG"}, {FNEG,"FNEG"}, {PNEG,"PNEG"}, {IMOD,"IMOD"}, {IEXP,"IEXP"}, {FEXP,"FEXP"}, {JMP,"JMP"}, {JMPCNT,"JMPCNT"}, {JMPTRUE,"JMPTRUE"}, {JMPFALSE,"JMPFALSE"}, {JMPNOTNULL_ELSEPOP,"JMPNOTNULL_ELSEPOP"}, {RDLINE,"RDLINE"}, {RDLINE_ORDERED,"RDLINE_ORDERED"}, {PREP_REREAD,"PREP_REREAD"}, {PUT,"PUT"}, {LDPUT,"LDPUT"}, {LDPUTALL,"LDPUTALL"}, {PUTVAR,"PUTVAR"}, {LDINT,"LDINT"}, {LDFLOAT,"LDFLOAT"}, {LDTEXT,"LDTEXT"}, {LDDATE,"LDDATE"}, {LDDUR,"LDDUR"}, {LDNULL,"LDNULL"}, {LDLIT,"LDLIT"}, {LDVAR,"LDVAR"}, {IEQ,"IEQ"}, {FEQ,"FEQ"}, {TEQ,"TEQ"}, {LIKE,"LIKE"}, {ILEQ,"ILEQ"}, {FLEQ,"FLEQ"}, {TLEQ,"TLEQ"}, {ILT,"ILT"}, {FLT,"FLT"}, {TLT,"TLT"}, {PRINT,"PRINT"}, {PUSH,"PUSH"}, {POP,"POP"}, {POPCPY,"POPCPY"}, {ENDRUN,"ENDRUN"}, {NULFALSE1,"NULFALSE1"}, {NULFALSE2,"NULFALSE2"}, {NDIST,"NDIST"}, {SDIST,"SDIST"}, {PUTDIST,"PUTDIST"}, {FINC,"FINC"}, {ENCCHA,"ENCCHA"}, {DECCHA,"DECCHA"}, {SAVEPOSI_JMP,"SAVEPOSI_JMP"}, {SAVEPOSF_JMP,"SAVEPOSF_JMP"}, {SAVEPOSS_JMP,"SAVEPOSS_JMP"}, {SORTI,"SORTI"}, {SORTF,"SORTF"}, {SORTS, "SORTS"}
};

void opcode::print(){
	cerr << ft("code: {: <18}  [{: <2}  {: <2}  {: <2}]\n", opMap[code], p1, p2, p3);
}

void dat::appendToBuffer(string &outbuf){
	if (b & NIL) return;
	// need quoting: , \n
	//implement quote escaping after implemented in reader
	static char abnormal[] = {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

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

valPos::valPos(int64 i, int64 p){ val.i = i; pos = p; }
valPos::valPos(double f, int64 p){ val.f = f; pos = p; }
valPos::valPos(char* s, int64 p){ val.s = s; pos = p; }
valPos::valPos(){val={0};pos=0;}

treeCString::treeCString(){
	s = nullptr;
}
treeCString::treeCString(dat& d){
	if (d.b & MAL){
		s = d.u.s;
		DISOWN(d);
	} else {
		s = (char*) malloc(d.z+1);
		memcpy(s, d.u.s, d.z+1);
	}
}

vmachine::vmachine(querySpecs &qs){
	q = &qs;
	for (int i=1; i<=q->numFiles; ++i){
		files.push_back(q->files[str2("_f", i)]);
	}
	if (q->grouping){
		//set group target with opcode
	} else {
		destrow.resize(q->colspec.count);
		torow = destrow.data();
		torowSize = destrow.size();
	}
	//eventually set stack size based on ast
	stack.resize(100);
	posVectors.resize(q->posVecs);
	ops = q->bytecode.data();
	quantityLimit = q->quantityLimit;
	bt_nums.resize(q->btn);
	bt_strings.resize(q->bts);
	distinctVal = {0};
	for (auto &d : stack)   d = {0};
	for (auto &d : destrow) d = {0};
}

vmachine::~vmachine(){
	FREE2(distinctVal);
	for (auto &d : stack)   FREE2(d);
	for (auto &d : destrow) FREE2(d);
	for (auto &b : bt_strings){
		for (auto it = b.begin(); it != b.end(); ++it){
			free(it->s);
		}
	}
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
