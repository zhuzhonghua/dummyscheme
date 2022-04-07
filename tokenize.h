#pragma once

#include "dummyscheme.h"

namespace DummyScheme {	
	
enum TokenType{
	TOKEN_UNKNOWN,
	TOKEN_NUM,
	TOKEN_DOUBLE_QUOT,
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_SYMBOL,
};

/*
	P = NUM | LEFT_PAREN LIST RIGHT_PAREN
	LIST = SYMBOL LISTP
	LISTP = P LISTP	
*/
class Tokenize {
protected:
	int index;
	std::string input;
public:
	Tokenize(const std::string &input);
	void init(const std::string &input);
	void run(DummyEnvPtr env);
protected:
	DummyValuePtr readP();
	DummyValuePtr readList();
	DummyValueList readListP();
	DummyValuePtr readNum();
	DummyValuePtr readStr();
	// read a word as symbol
	DummyValuePtr readSymbol();
protected:
	// space and tab
	// TODO: comment
	void skipBlank();
	bool isBlank();
	TokenType readToken();
};
}
