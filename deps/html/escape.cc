#include "escape.h"
#include <string>
using namespace std;
static int linelen = 100;


#define UTF_INVALID 0xFFFD
#define UTF_SIZ     4
#define BETWEEN(X, A, B)        ((A) <= (X) && (X) <= (B))
static const unsigned char utfbyte[UTF_SIZ + 1] = {0x80,    0, 0xC0, 0xE0, 0xF0};
static const unsigned char utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long utfmin[UTF_SIZ + 1] = {       0,    0,  0x80,  0x800,  0x10000};
static const long utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};
long utf8decodebyte(const char c, size_t *i) {
	for (*i = 0; *i < (UTF_SIZ + 1); ++(*i))
		if (((unsigned char)c & utfmask[*i]) == utfbyte[*i])
			return (unsigned char)c & ~utfmask[*i];
	return 0;
}

size_t utf8validate(long *u, size_t i) {
	if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
		*u = UTF_INVALID;
	for (i = 1; *u > utfmax[i]; ++i)
		;
	return i;
}

size_t utf8decode(const char *c, long *u, size_t clen) {
	size_t i, j, len, type;
	long udecoded;

	*u = UTF_INVALID;
	if (!clen)
		return 0;
	udecoded = utf8decodebyte(c[0], &len);
	if (!BETWEEN(len, 1, UTF_SIZ))
		return 1;
	for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
		udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
		if (type)
			return j;
	}
	if (j < len)
		return 0;
	*u = udecoded;
	utf8validate(u, len);

	return len;
}

void addEscapedHTML(const basic_string_view<char>& input, string& output) {
	int utf8charlen;
	long utf8codepoint = 0;
	char* end = (char*)input.data() + input.length();
	for (char* c = (char*)input.data(); c < end;) {
		if (isascii(*c)) {
			switch (*c) {
				case '<': output += "&lt;"; break;
				case '>': output += "&gt;"; break;
				case '"': output += "&quot;"; break;
				case '\'':output += "&#39;"; break;
				case '&': output += "&amp;"; break;
				default:  output += *c;
			}
			++c;
		} else {
			utf8charlen = utf8decode(c, &utf8codepoint, UTF_SIZ);
			output += "&#" + to_string(utf8codepoint) + ';';
			c += utf8charlen;
		}

	}
}
string escapeHTML(const basic_string_view<char>& input){
	string output;
	output.reserve(input.length());
	addEscapedHTML(input, output);
	return output;
}
static void shorten(string_view input, string& output){
	while (1) {
		if (input.length() <= linelen){
			addEscapedHTML(input, output);
			return;
		}
		auto lineend = input.rfind(' ', linelen);
		if (lineend == string::npos){
			lineend = input.find(' ');
			if (lineend == string::npos){
				addEscapedHTML(input, output);
				return;
			}
		}
		addEscapedHTML(input.substr(0, lineend), output);
		output += "<br>";
		input = input.substr(lineend+1);
	}
}
string chopAndEscapeHTML(basic_string_view<char>&& input){
	string output;
	output.reserve(input.length());
	if (input.length() <= linelen){
		addEscapedHTML(input, output);
		return output;
	}
	while (1) {
		auto lineend = input.find('\n');
		if (lineend != string_view::npos){
			shorten(input.substr(0, lineend), output);
			output += "<br>";
			input = input.substr(lineend+1);
		} else {
			shorten(input, output);
			return output;
		}
	}
}
