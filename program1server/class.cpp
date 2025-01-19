#include <iostream>
#include <sstream>
#include "class.h"

bool String::checkString() {
	std::string s = this->getString();
	bool flag = true;
	for (char letter : s) {
		if (!std::isalpha(static_cast<unsigned char>(letter))) {
			flag = false;
			break;
		}
	}
	return s.size() <= 64 && flag;
}

void String::processingString(std::string& buffer) {
	if (this->checkString() == true) {
		std::string s = this->getString();
		while (s.size() > 0) {
			unsigned char symbol = s[0];
			int count = std::count(s.begin(), s.end(), symbol);
			buffer += s[0];
			buffer += ":";
			buffer += std::to_string(count);
			buffer += ";";
			s.erase(std::remove(s.begin(), s.end(), symbol), s.end());
		}
	}
}

void String::clearBuffer(std::string& buffer) {
	buffer.clear();
}
