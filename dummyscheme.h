#pragma once

#include <string>
#include <sstream>

enum TokenType{
	TOKEN_UNKNOWN,
	TOKEN_NUM,
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_SYMBOL,
};

/*
	P = NUM | LEFT_PAREN LIST RIGHT_PAREN
	LIST = SYMBOL LISTP
	LISTP = P LISTP	
*/
class Tokenize{
protected:
	TokenType headType;
	int index;
	std::string input;
public:
	Tokenize(std::string &input);
protected:
	void unexpectedToken();
	TokenType readToken();
	void readP();
	void readList();
	void readListP();
	
protected:
	bool isNum();
	// space and tab
	// TODO: comment
	void skipBlank();
	bool isBlank();
	int readNum();
	// read a word as symbol
	void readSymbol();
};
