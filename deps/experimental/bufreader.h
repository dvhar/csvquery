#include <ios>
#include <stdio.h>
#include <memory>

class lreader {
	size_t BS = 1024*1024 * 8;
	std::unique_ptr<char> bholder;
	char* buf;
	FILE* f;
	char* end;
	char* nl;
	char* line;
	void refresh();
	void realign();
	int biggest;
	bool findbiggest;
	bool single;
	public:
	int linesize;
	char* readline();
	lreader(){f = NULL;}
	~lreader(){if (f) fclose(f);}
	void open(const char* fname){
		line = NULL;
		linesize = 0;
		biggest = 0;
		single = false;
		bholder.reset(new char[BS]);
		buf = bholder.get();
		f = fopen(fname,"r");
	}
	//fill entire buffer when reread
	void seekfull(long long pos){
		fseek(f, pos, SEEK_SET);
		single = false;
	}
	//fill buffer to size of biggest line when reread
	void seekline(long long pos){
		fseek(f, pos, SEEK_SET);
		single = true;
	}
};
