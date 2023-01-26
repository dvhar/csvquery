#include "interpretor.h"
#include "scanner.h"
#include <boost/algorithm/string/case_conv.hpp>

template<class T>
constexpr u32 len(T &a) {
    return sizeof(a) / sizeof(typename std::remove_all_extents<T>::type);
}

string token::lower() {
	return boost::to_lower_copy(val);
}

const int specials[] = { '*','=','!','<','>','\'','"','(',')',',','+','-','%','/','^' };
const int others[] = { '\\',':','_','.','[',']','~','{','}' };

int table[NUM_STATES][256];
void initable(){
	static bool tabinit = false;
	if (tabinit) return;
	//initialize table to errr
	u32 i,j;
	for (i=0; i<NUM_STATES; i++) {
		for (j=0; j<255; j++) { table[i][j] = ERROR_STATE; }
	}
	//next state from initial
	for (i=0; i<len(others); i++) { table[0][others[i]] = STATE_WORD; }
	for (i=0; i<len(specials); i++) { table[0][specials[i]] = STATE_SSPECIAL; }
	table[0][255] =  EOS;
	table[0][' '] =  STATE_INITAL;
	table[0]['\n'] = STATE_INITAL;
	table[0]['\t'] = STATE_INITAL;
	table[0][';'] =  STATE_INITAL;
	table[0][0] =	STATE_INITAL;
	table[0]['<'] =  STATE_MBSPECIAL;
	table[0]['>'] =  STATE_MBSPECIAL;
	for (i='a'; i<='z'; i++) { table[0][i] = STATE_WORD; }
	for (i='A'; i<='Z'; i++) { table[0][i] = STATE_WORD; }
	for (i='0'; i<='9'; i++) { table[0][i] = STATE_WORD; }
	//next state from single-char special
	for (i='a'; i<='z'; i++) { table[STATE_SSPECIAL][i] = SPECIAL; }
	for (i='A'; i<='Z'; i++) { table[STATE_SSPECIAL][i] = SPECIAL; }
	for (i='0'; i<='9'; i++) { table[STATE_SSPECIAL][i] = SPECIAL; }
	for (i=0; i<len(others); i++) { table[STATE_SSPECIAL][others[i]] = SPECIAL; }
	for (i=0; i<len(specials); i++) { table[STATE_SSPECIAL][specials[i]] = SPECIAL; }
	table[STATE_SSPECIAL][';'] =  SPECIAL;
	table[STATE_SSPECIAL][' '] =  SPECIAL;
	table[STATE_SSPECIAL]['\n'] = SPECIAL;
	table[STATE_SSPECIAL]['\t'] = SPECIAL;
	table[STATE_SSPECIAL][EOS] =  SPECIAL;
	table[STATE_MBSPECIAL]['='] =  STATE_MBSPECIAL;
	table[STATE_MBSPECIAL]['>'] =  STATE_MBSPECIAL;
	table[STATE_MBSPECIAL][';'] =  STATE_SSPECIAL;
	table[STATE_MBSPECIAL][' '] =  STATE_SSPECIAL;
	table[STATE_MBSPECIAL]['\n'] = STATE_SSPECIAL;
	table[STATE_MBSPECIAL]['\t'] = STATE_SSPECIAL;
	for (i='a'; i<='z'; i++) { table[STATE_MBSPECIAL][i] = SPECIAL; }
	for (i='A'; i<='Z'; i++) { table[STATE_MBSPECIAL][i] = SPECIAL; }
	for (i='0'; i<='9'; i++) { table[STATE_MBSPECIAL][i] = SPECIAL; }
	for (i=0; i<len(others); i++) { table[STATE_MBSPECIAL][others[i]] = SPECIAL; }
	//next state from word
	for (i=0; i<len(specials); i++) { table[STATE_WORD][specials[i]] = WORD_TK; }
	for (i=0; i<len(others); i++) { table[STATE_WORD][others[i]] = STATE_WORD; }
	table[STATE_WORD][' '] =  WORD_TK;
	table[STATE_WORD]['\n'] = WORD_TK;
	table[STATE_WORD]['\t'] = WORD_TK;
	table[STATE_WORD][';'] =  WORD_TK;
	table[STATE_WORD][EOS] =  WORD_TK;
	for (i='a'; i<='z'; i++) { table[STATE_WORD][i] = STATE_WORD; }
	for (i='A'; i<='Z'; i++) { table[STATE_WORD][i] = STATE_WORD; }
	for (i='0'; i<='9'; i++) { table[STATE_WORD][i] = STATE_WORD; }

	tabinit = true;
}

int scanner::filderedGetc() {
	if (idx >= q->queryString.length()) { return EOS; }
	if (idx+1 < q->queryString.length()){
		auto maybecomment = string_view(q->queryString.data()+idx, 2);
		if (maybecomment == "--"){
			q->canskip = true;
			while (idx < q->queryString.length() && q->queryString[idx] != '\n') {
				lastskipped = idx;
				idx++;
				colNo++;
			}
		}
		if (maybecomment == "/*"){
			q->canskip = true;
			int oldidx = idx;
			int endcomment = q->queryString.find("*/",idx);
			if (endcomment == -1) error("Comment starting with '/*' was not terminated");
			lastskipped = endcomment+1;
			colNo += endcomment - oldidx;
			idx = lastskipped+1;
		}
	}
	if (idx >= q->queryString.length()) { return EOS; }
	idx++;
	return (int) q->queryString[idx-1];
}
int scanner::filderedPeek() {
	if (idx >= q->queryString.length()) { return EOS; }
	int next = this->filderedGetc();
	if (lastskipped != idx-1)
		idx--;
	return next;
}
char* scanner::nextCstr() {
	if (idx >= q->queryString.length()) { return nullptr; }
	return &q->queryString[idx];
}
scanner::scanner(querySpecs &qs) : q(&qs) {
	initable();
}

int scanner::getPos() {
	return currPos;
}
void scanner::setPos(int newpos){
	currPos = idx = newpos;
	hascurr = hasnext = false;
}
token scanner::peekToken() {
	int peekidx = ringidx^1;
	if (!hasnext){
		nextPos = idx;
		ringbuf[peekidx] = scanAnyToken();
		hasnext = true;
	}
	return ringbuf[peekidx];
}
token scanner::nextToken() {
	ringidx ^= 1;
	if (!hasnext){
		currPos = idx;
		ringbuf[ringidx] = scanAnyToken();
	} else {
		currPos = nextPos;
	}
	hasnext = false;
	hascurr = true;
	return ringbuf[ringidx];
}
token scanner::currToken() {
	if (!hascurr){
		currPos = idx;
		ringbuf[ringidx] = scanAnyToken();
	hascurr = true;
	}
	return ringbuf[ringidx];
}
token scanner::prevToken() {
	return ringbuf[ringidx^1]; // TODO: edge case where setPos or peek invalidates this
}

token scanner::scanPlainToken() {
	int state = STATE_INITAL;
	int nextState, nextchar;
	string S;

	while ( (state & FINAL) == 0 && state < NUM_STATES ) {
		nextState = table[state][filderedPeek()];
		if ((nextState & ERROR_STATE) != 0) {
		//end of string
			if (state == 255) return { 255, "END", lineNo, colNo, false };
			auto err = st("line: ",lineNo," col: ",colNo," char: ",(char)filderedPeek());
			return { ERROR_STATE, err, lineNo, colNo, false };
		}
		if ((nextState & FINAL) != 0) {
			//see if keyword or regular word
			if (nextState == WORD_TK) {
				string tokStr = boost::to_lower_copy(S);
				if (keywordMap.count(tokStr) && waitForQuote == 0) {
					//return keyword token
					return { getkeyword(tokStr), S, lineNo, colNo, false };
				} else {
					//return word token
					return { nextState, S, lineNo, colNo, false };
				}
			//see if special type or something else
			} else if (nextState == SPECIAL) {
				if (specialMap.count(S)) {
					int sp = getspecial(S);
					//find out if these tokens are in a quote to preserve whitespace
					if ((sp == SP_SQUOTE || sp == SP_DQUOTE) && waitForQuote == 0) {
						waitForQuote = sp;
					} else if ((sp == SP_SQUOTE || sp == SP_DQUOTE) && waitForQuote == sp) {
						waitForQuote = 0;
					}
					//return special token
					return { sp, S, lineNo, colNo, false };
				} else {
					auto err = st("line: ",lineNo," col: ",colNo," char: ",(char)filderedPeek());
					return { ERROR_STATE, err, lineNo, colNo, false };
				}
			} else {
				return { nextState, S, lineNo, colNo, false };
			}

		} else {
			state = nextState;
			nextchar = filderedGetc();
			colNo++;
			//include whitespace in the token when waiting for a closing quote
			if (waitForQuote != 0) {
				S += nextchar;
			} else if (nextchar != ' ' && nextchar != '\t' && nextchar != '\n' && nextchar != ';'){
				S += nextchar;
			}
			if (nextchar == '\n') { lineNo++; colNo=0; }
			if (nextchar == EOS) {
				return { EOS, "END Of INPUT", lineNo, colNo, false };
			}
		}
	}
	return { EOS, "END Of INPUT", lineNo, colNo, false };

}

token scanner::scanQuotedToken(int qtype) {
	string S;
	if (auto tokstart = nextCstr(); tokstart){
		int toklen = 0;
		auto c = tokstart;
		for (; *c && *c != qtype; ++c, ++toklen)
			if (*c == '\n'){ lineNo++; colNo=0; }
		if (*c != qtype)
			error("Quote was not terminated");
		idx += toklen;
		return {WORD_TK, string(tokstart, toklen), lineNo, colNo, true };
	}
	error("Quote was not terminated");
	return {};
}

token scanner::scanAnyToken() {
	token t = scanPlainToken();
	if (t.id == SP_SQUOTE || t.id == SP_DQUOTE) {
		t = scanQuotedToken(t.val[0]);
		scanPlainToken(); //end quote
	}
	return t;
}

