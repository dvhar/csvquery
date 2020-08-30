#include <stdio.h>
#define BS  (1024*1024*3)

class bufreader {
	char buf[BS+1];
	FILE* f;
	char* end;
	char* nl;
	char* line;
	int biggestline;
	bool single;
	void refresh();
	bufreader& operator=(bufreader);
	public:
	int linesize;
	char* getline();
	void open(const char* fname);
	bufreader(){f = NULL;}
	~bufreader(){if (f) fclose(f);}
	//fill entire buffer when reread
	void seekfull(long long pos){
		fseek(f, pos, SEEK_SET);
		end = line = NULL;
		single = false;
	}
	//fill buffer to size of biggest line when reread
	void seekline(long long pos){
		fseek(f, pos, SEEK_SET);
		end = line = NULL;
		single = true;
	}
};
