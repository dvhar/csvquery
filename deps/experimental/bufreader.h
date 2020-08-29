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
	public:
	char* readline();
	lreader(char* fname){
		f = fopen(fname,"r");
		line = NULL;
		bholder.reset(new char[BS]);
		buf = bholder.get();
	}
};
