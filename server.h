#pragma once
#include "interpretor.h"



enum legacy_server {
	DAT_BLANK   = 0,
	DAT_ERROR   = 1,
	DAT_GOOD    = 1 << 1,
	DAT_BADPATH = 1 << 2,
	DAT_IOERR   = 1 << 4,
	F_CSV       = 1 << 5,

	SK_MSG       = 0,
	SK_PING      = 1,
	SK_PONG      = 2,
	SK_STOP      = 3,
	SK_DIRLIST   = 4,
	SK_FILECLICK = 5,
	SK_PASS      = 6,
	SK_ID        = 7,
};

void servews();
void returnPassword(i64 sesid, string pass);

class webquery {
	public:
	vector<string> queries;
	int whichone =0;
	string querystring;
	string savepath;
	int qamount =0;
	int fileIO =0;
	i64 sessionId=0;
	bool isSaving(){ return ((fileIO & F_CSV) != 0); }
};
