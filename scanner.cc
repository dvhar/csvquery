#include <string>
#include <iostream>
#include <stdio.h>
#include "interpretor.h"

template<class T>
constexpr size_t len(T &a)
{
    return sizeof(a) / sizeof(typename std::remove_all_extents<T>::type);
}

string token::lower() {
	string s = val;
	boost::to_lower(s);
	return s;
}
void token::print() {
	cout << "    id: " << id << " -> " << enumMap[id] << endl
		<< "    val: " << val << endl;
}

const int specials[] = { '*','=','!','<','>','\'','"','(',')',',','+','-','%','/','^' };
const int others[] = { '\\',':','_','.','[',']','~','{','}' };

int table[NUM_STATES][256];
void initable(){
	static bool tabinit = false;
	if (tabinit) return;
	//initialize table to errr
	int i,j;
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

	/*
	for i=0; i< NUM_STATES; i++{
		for j=0; j< 255; j++{
			Printf("[ %d ][ %c ]=%-34s",i,j,enumMap[table[i][j]])
		}
		Printf("\n")
	}
	*/
	tabinit = true;
}

class StringLookahead {
	public:
	string Str;
	int idx;
	int getc();
	int peek();
};
int StringLookahead::getc() {
	if (idx >= Str.length()) { return EOS; }
	idx++;
	return (int) Str[idx-1];
}
int StringLookahead::peek() {
	if (idx >= Str.length()) { return EOS; }
	return (int) Str[idx];
}

token scanner(StringLookahead &s) {
	initable();
	static int lineNo = 1;
	static int colNo = 1;
	static int waitForQuote;
	int state = STATE_INITAL;
	int nextState, nextchar;
	string S;

	while ( (state & FINAL) == 0 && state < NUM_STATES ) {
		nextState = table[state][s.peek()];
		if ((nextState & ERROR_STATE) != 0) {
		//end of string
			if (state == 255) return { 255, "END", lineNo, colNo, false };
			stringstream e;
			e << "line: " << lineNo << ", col: " << colNo << ", char: " << (char) s.peek();
			return { ERROR_STATE, e.str(), lineNo, colNo, false };
		}
		if ((nextState & FINAL) != 0) {
			//see if keyword or regular word
			if (nextState == WORD_TK) {
				string s = S;
				boost::to_lower(s);
				if (keywordMap.count(s) && waitForQuote == 0) {
					//return keyword token
					return { keywordMap[s], S, lineNo, colNo, false };
				} else {
					//return word token
					return { nextState, S, lineNo, colNo, false };
				}
			//see if special type or something else
			} else if (nextState == SPECIAL) {
				if (specialMap.count(S)) {
					int sp = specialMap[S];
					//find out if these tokens are in a quote to preserve whitespace
					if ((sp == SP_SQUOTE || sp == SP_DQUOTE) && waitForQuote == 0) {
						waitForQuote = sp;
					} else if ((sp == SP_SQUOTE || sp == SP_DQUOTE) && waitForQuote == sp) {
						waitForQuote = 0;
					}
					//return special token
					return { sp, S, lineNo, colNo, false };
				} else {
					stringstream e;
					e << "line: " << lineNo << ", col: " << colNo << ", char: " << (char) s.peek();
					return { ERROR_STATE, e.str(), lineNo, colNo, false };
				}
			} else {
				return { nextState, S, lineNo, colNo, false };
			}

		} else {
			state = nextState;
			nextchar = s.getc();
			colNo++;
			//include whitespace in the token when waiting for a closing quote
			if (waitForQuote != 0) {
				S += nextchar;
			} else if (nextchar != ' ' && nextchar != '\t' && nextchar != '\n' && nextchar != ';'){
				S += nextchar;
			}
			if (nextchar == '\n') { lineNo++; colNo=0; }
			if (nextchar == EOS) {
				return { EOS, "END", lineNo, colNo, false };
			}
		}
	}
	return { EOS, "END", lineNo, colNo, false };

}


void scanTokens(querySpecs &q) {
	int lineNo = 1;
	int colNo = 1;
	StringLookahead input = {q.queryString, 0};
	while(1) {
		token t = scanner(input);
		//turn tokens inside quotes into single token
		if (t.id == SP_SQUOTE || t.id == SP_DQUOTE) {
			int quote = t.id;
			string S = "";
			for (token t = scanner(input); t.id != quote && t.id != EOS ; t = scanner(input)) {
				if (t.id == ERROR_STATE) error("scanner error: "+t.val);
				S += t.val;
			}
			if (t.id != quote)  error("Quote was not terminated");
			t = {WORD_TK,S,t.line,t.col,true};
		}
		q.tokArray.push_back(t);
		if (t.id == ERROR_STATE) error("scanner error: "+t.val);
		if (t.id == EOS) break;
	}
}

