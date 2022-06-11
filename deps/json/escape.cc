#include "escape.h"
#include <string>
using namespace std;
int linelen = 100;

void addEscapedJSON(const basic_string_view<char>& input, string& output) {
	for (string::size_type i = 0; i < input.length(); ++i)
	{
		switch (input[i]) {
			case '"':
				output += "\\\"";
				break;
			case '\b':
				output += "\\b";
				break;
			case '\f':
				output += "\\f";
				break;
			case '\n':
				output += "\\n";
				break;
			case '\r':
				output += "\\r";
				break;
			case '\t':
				output += "\\t";
				break;
			case '\\':
				output += "\\\\";
				break;
			default:
				output += input[i];
				break;
		}

	}
}
string escapeJSON(const basic_string_view<char>& input){
	string output;
	output.reserve(input.length());
	addEscapedJSON(input, output);
	return output;
}
void shorten(string_view input, string& output){
	while (1) {
		if (input.length() <= linelen){
			addEscapedJSON(input, output);
			return;
		}
		auto lineend = input.rfind(' ', linelen);
		if (lineend == string::npos){
			lineend = input.find(' ');
			if (lineend == string::npos){
				addEscapedJSON(input, output);
				return;
			}
		}
		addEscapedJSON(input.substr(0, lineend), output);
		output += "\\n";
		input = input.substr(lineend+1);
	}
}
string chopAndEscapeJson(basic_string_view<char>&& input){
	string output;
	output.reserve(input.length());
	if (input.length() <= linelen){
		addEscapedJSON(input, output);
		return output;
	}
	while (1) {
		auto lineend = input.find('\n');
		if (lineend != string_view::npos){
			shorten(input.substr(0, lineend+1), output);
			input = input.substr(lineend+1);
		} else {
			shorten(input, output);
			return output;
		}
	}
}
