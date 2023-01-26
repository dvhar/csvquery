#pragma once

#include "interpretor.h"

class scanner {
	token ringbuf[2];
	int lastskipped = -2;
	int ringidx = 0;
	bool hasnext = 0;
	bool hascurr = 0;
	int currPos = 0;
	int nextPos = 0;
	int lineNo = 1;
	int colNo = 1;
	int waitForQuote = 0;
	u32 idx =0;
	querySpecs* q;
	int filderedGetc();
	int filderedPeek();
	char* nextCstr();
	token scanAnyToken();
	token scanPlainToken();
	token scanQuotedToken(int);
	public:
		int getPos();
		void setPos(int);
		token currToken();
		token nextToken();
		token peekToken();
		token prevToken();
		scanner(querySpecs&);
};
