#include <stdio.h>
#include <memory>
#include<boost/filesystem.hpp>
#define DEFAULT_BUFSIZE  (1024*1024*3)

class bufreader {
	FILE* f = NULL;
	char* end = NULL;
	char* nl = NULL;
	char* line = NULL;
	int biggestline = 0;
	long long readsofar = 0;
	long long bufpos = 0;
	bool single = false;
	char* buf;
	std::unique_ptr<char[]> realbuf;
	int linesize = 0;
	void refresh();
	void addrefresh(int);
	bufreader& operator=(bufreader);
	public:
	long long fsize = 0;
	long long buffsize = DEFAULT_BUFSIZE;
	bool done = false;
	char* getline();
	long long addline();
	bufreader(){}
	~bufreader(){if (f) fclose(f);}
	void open(const char* fname, long long wantsize){
		fsize = boost::filesystem::file_size(fname);
		buffsize = fsize < wantsize ? fsize : wantsize;
		open(fname);
	}
	void open(const char* fname){
		if (fsize == 0)
			fsize = boost::filesystem::file_size(fname);
		f = fopen(fname,"rb");
		realbuf.reset(new char[buffsize+4]);
		//padding for safe parsing
		realbuf[0] = realbuf[buffsize+2] =
		realbuf[1] = realbuf[buffsize+3] = 0;
		buf = &realbuf[2];
	};
	//fill entire buffer when reread
	void seekfull(long long pos){
		fseek(f, pos, SEEK_SET);
		done = false;
		end = line = NULL;
		single = false;
		readsofar = pos;
	}
	//fill buffer to size of biggest line when reread
	void seekline(long long pos){
		fseek(f, pos, SEEK_SET);
		done = false;
		end = line = NULL;
		single = true;
		readsofar = pos;
	}
	long long linepos(){
		return (long long)(bufpos + (line - buf));
	}
};
