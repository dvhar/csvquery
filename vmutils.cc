#include "interpretor.h"
#include "vmachine.h"

//map for printing opcodes
map<int, string> opMap = { {CVNO,"CVNO"}, {CVER,"CVER"}, {CVIF,"CVIF"}, {CVIS,"CVIS"}, {CVFI,"CVFI"}, {CVFS,"CVFS"}, {CVDRS,"CVDRS"}, {CVDTS,"CVDTS"}, {CVSI,"CVSI"}, {CVSF,"CVSF"}, {CVSDT,"CVSDT"}, {CVSDR,"CVSDR"}, {IADD,"IADD"}, {FADD,"FADD"}, {TADD,"TADD"}, {DTADD,"DTADD"}, {DRADD,"DRADD"}, {ISUB,"ISUB"}, {FSUB,"FSUB"}, {DRSUB,"DRSUB"}, {DTSUB,"DTSUB"}, {IMULT,"IMULT"}, {FMULT,"FMULT"}, {DRMULT,"DRMULT"}, {IDIV,"IDIV"}, {FDIV,"FDIV"}, {DRDIV,"DRDIV"}, {INEG,"INEG"}, {FNEG,"FNEG"}, {DNEG,"DNEG"}, {IMOD,"IMOD"}, {IEXP,"IEXP"}, {FEXP,"FEXP"}, {JMP,"JMP"}, {JMPCNT,"JMPCNT"}, {JMPTRUE,"JMPTRUE"}, {JMPFALSE,"JMPFALSE"}, {POP,"POP"}, {RDLINE,"RDLINE"}, {RDLINEAT,"RDLINEAT"}, {PRINT,"PRINT"}, {PUT,"PUT"}, {LDPUT,"LDPUT"}, {LDPUTALL,"LDPUTALL"}, {PUTVAR,"PUTVAR"}, {LDINT,"LDINT"}, {LDFLOAT,"LDFLOAT"}, {LDTEXT,"LDTEXT"}, {LDDATE,"LDDATE"}, {LDDUR,"LDDUR"}, {LDNULL,"LDNULL"}, {LDLIT,"LDLIT"}, {LDVAR,"LDVAR"}, {IEQ,"IEQ"}, {FEQ,"FEQ"}, {TEQ,"TEQ"}, {NEQ,"NEQ"}, {ILEQ,"ILEQ"}, {FLEQ,"FLEQ"}, {TLEQ,"TLEQ"}, {ILT,"ILT"}, {FLT,"FLT"}, {TLT,"TLT"}, {ENDRUN,"ENDRUN"}, {NULFALSE1,"NULFALSE1"}, {NULFALSE2,"NULFALSE2"}, {POPCPY,"POPCPY"}, {LIKE,"LIKE"}, {NDIST,"NDIST"}, {SDIST,"SDIST"}, {PUTDIST,"PUTDIST"}, {JMPNOTNULL_ELSEPOP,"JMPNOTNULL_ELSEPOP"}, {FINC,"FINC"}
};

void opcode::print(){
	cerr << ft("code: {: <18}  [{: <2}  {: <2}  {: <2}]\n", opMap[code], p1, p2, p3);
}

string dat::tostring(){
	if (b & NIL) return "";
	switch ( b & 0b00000111 ) {
	case I:  return ft("{}",u.i); break;
	case F:  return ft("{:.10g}",u.f); break;
	case DT: return ft("{}",datestring(u.i)); break;
	case DR: return ft("{}",durstring(u.i, nullptr)); break;
	case T:  return ft("{}",u.s); break;
	case R:  return ft("regex"); break;
	}
	return "";
}
void dat::print(){
	if (b & NIL) return;
	switch ( b & 0b00000111 ) {
	case I:  fmt::print("{}",u.i); break;
	case F:  fmt::print("{:.10g}",u.f); break;
	case DT: fmt::print("{}",datestring(u.i)); break;
	case DR: fmt::print("{}",durstring(u.i, nullptr)); break;
	case T:  fmt::print("{}",u.s); break;
	case R:  fmt::print("regex"); break;
	}
}

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
		//initialize midrow and point torow to it
	} else {
		destrow.resize(q->colspec.count);
		torow = destrow.data();
		torowSize = destrow.size();
	}
	vars.resize(q->vars.size());
	//eventually set stack size based on ast
	stack.resize(100);
	ops = q->bytecode.data();
	quantityLimit = q->quantityLimit;
	bt_nums.resize(q->btn);
	bt_strings.resize(q->bts);
	distinctVal = {0};
	for (auto &d : stack)   d = {0};
	for (auto &d : vars)    d = {0};
	for (auto &d : destrow) d = {0};
	for (auto &d : midrow)  d = {0};
}

vmachine::~vmachine(){
	FREE2(distinctVal);
	for (auto &d : stack)   FREE2(d);
	for (auto &d : vars)    FREE2(d);
	for (auto &d : destrow) FREE2(d);
	for (auto &d : midrow)  FREE2(d);
	for (auto &b : bt_strings){
		for (auto it = b.begin(); it != b.end(); ++it){
			free(it->s);
		}
	}
}
querySpecs::~querySpecs(){
	for (auto &d : literals){
		FREE2(d);
		if (d.b & RMAL){
			regfree(d.u.r);
			delete d.u.r;
		}
	}
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
		ch.key[i] = pass[i%pass.length()];
	}
	memset(&ch.nonce, 0, sizeof(ch.nonce));
	ctxs.push_back(ch);
	return ctxs.size()-1;
}
pair<char*, int> crypter::chachaEncrypt(int i, int len, char* input){
	auto ch = ctxs.data()+i;
	auto rawResult = (uint8_t*) alloca(len+3);
	memcpy(rawResult+3, input, len);
	memcpy(&ch->ctx.key, ch->key, sizeof(ch->ctx.key));
	auto nonce = rand(); //replace with secure version
	rawResult[0] = ch->nonce[0] = nonce;
	rawResult[1] = ch->nonce[1] = nonce >> 8;
	rawResult[2] = ch->nonce[2] = nonce >> 16;
	chacha20_init_context(&ch->ctx, ch->key, ch->nonce, 1); //find out what counter param does
	chacha20_xor(&ch->ctx, rawResult+3, len);
	int finalSize = encsize(len+3);
	auto finalResult = (char*) malloc(finalSize);
	b64_encode(rawResult, (unsigned char*)finalResult, len+3);
	return pair<char*,int>(finalResult, finalSize);
}
