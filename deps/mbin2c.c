#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

unsigned char *ok = "{}[]()',.+=_-*&^$#@!~`<>?|;: \t";
int oklen;
int escape(unsigned char);

int main(int argc, char** argv) {
	int debug = 0, shift = 0, names = 0;
	int cc;
	while ((cc = getopt (argc, argv, "phn")) != -1)
		switch (cc){
		case 'p':
			debug = 1;
			shift++;
			break;
		case 'n':
			names = 1;
			shift++;
			break;
		case 'h':
			puts("-p flag to ouput c file with main() function for debugging\n"
			     "-n flag to ouput just the byte array names");
			return 0;
		}
	if (debug) puts("#include<unistd.h>\nint main(){");
	char* fn = argv[1+shift];
	FILE* f = fopen(fn, "r");
	int n = 0;
	for (n=0; fn[n] != 0; ++n)
		if (fn[n] == '.' || fn[n] == '/')
			fn[n] = '_';
	if (names) {
		printf("b2c_%s\n", fn);
		return(0);
	} else {
		printf("const unsigned char *b2c_%s = \"", fn);
	}
	n = 0;
	unsigned char c;
	oklen = strlen(ok);
	int lastx = 0;
	while(!feof(f)) {
		if(fread(&c, 1, 1, f) == 0) break;
		if (escape(c)){
			if (c == '"' || c == '\\')
				printf("\\%c", c);
			else
				printf("\\x%02x", c);
			lastx = 1;
		} else {
			if (lastx && isalnum(c))
				printf("\" \"");
			printf("%c", c);
			lastx = 0;
		}
		++n;
	}
	fclose(f);
	printf("\";\n");
	printf("int b2c_%s_len = %d;\n", fn, n);
	if (debug) printf("write(1,b2c_%s,b2c_%s_len);}\n", fn, fn);
}
int escape(unsigned char c){
	if (!isalnum(c)){
		for (int i=0; i<oklen; ++i)
			if (c == ok[i])
				return 0;
		return 1;
	}
	return 0;
}
