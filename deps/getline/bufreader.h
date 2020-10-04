#include <stdio.h>
#define BS  (1024*1024*3)

class bufreader {
	FILE* f = NULL;
	char* end = NULL;
	char* nl = NULL;
	char* line = NULL;
	int biggestline = 0;
	bool single = false;
	char buf[BS+1];
	void refresh();
	bufreader& operator=(bufreader);
	public:
	int linesize = 0;
	char* getline();
	bufreader(){}
	~bufreader(){if (f) fclose(f);}
	void open(const char* fname){
		f = fopen(fname,"r");
	};
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
