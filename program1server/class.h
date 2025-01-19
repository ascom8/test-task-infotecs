#include <string>

#pragma once

class String {
public:

	String(std::string s) {
		str = s;
	}
	
	std::string getString() { return str; }
	std::string getBuffer() { return buffer; };

	bool checkString();
	void processingString(std::string& buffer);
	void clearBuffer(std::string& buffer);

private:
	std::string str;
	std::string buffer = "";
};
