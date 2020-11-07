#include <stdio.h>
#include<filesystem>
#define BUFSIZE  (1024*1024*3)

class bufreader {
	FILE* f = NULL;
	char* end = NULL;
	char* nl = NULL;
	char* line = NULL;
	int biggestline = 0;
	unsigned long long readsofar = 0;
	bool single = false;
	char* buf;
	char realbuf[BUFSIZE+4];
	void refresh();
	bufreader& operator=(bufreader);
	public:
	unsigned long long fsize = 0;
	int buffsize = BUFSIZE;
	bool done = false;
	int linesize = 0;
	char* getline();
	bufreader(){}
	~bufreader(){if (f) fclose(f);}
	void open(const char* fname){
		fsize = std::filesystem::file_size(fname);
		f = fopen(fname,"rb");
		//padding for safe parsing
		realbuf[0] = realbuf[buffsize+2] =
		realbuf[1] = realbuf[buffsize+3] = 0;
		buf = realbuf+2;
	};
	//fill entire buffer when reread
	void seekfull(long long pos){
		fseek(f, pos, SEEK_SET);
		done = false;
		end = line = NULL;
		single = false;
		readsofar = 0;
	}
	//fill buffer to size of biggest line when reread
	void seekline(long long pos){
		fseek(f, pos, SEEK_SET);
		done = false;
		end = line = NULL;
		single = true;
		readsofar = 0;
	}
};
