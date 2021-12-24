#pragma once

#define TOKEN_LEFT_PAREN '('
#define TOKEN_RIGHT_PAREN ')'

/*
	P = NUM | LEFT LIST RIGHT
	LIST = SYMBOL LISTP
	LISTP = P LISTP	
*/
class Tokenize{
protected:
	int index;
	std::string input;
public:
	Tokenize(std::string &input);
	void read();
public:
	bool isNum();
	// space and tab
	// TODO: comment
	int skipBlank();
	int readNum();
	// read a word as symbol
	int readSymbol();
	// a lisp form
	int readForm();	
};
