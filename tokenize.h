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
	TOKEN_QUOTE,
};

/*
	P = NUM | STRING | SYMBOL | QUOTE| LEFT_PAREN LIST RIGHT_PAREN	
	LIST = P LISTP
	LISTP = P LISTP

	difference between LIST and LISTP is the return value
*/
class Tokenize {
protected:
	int aheadToken;
	int index;
	std::string input;
public:
	Tokenize(const std::string &input);
	void init(const std::string &input);
	DummyValuePtr run(DummyEnvPtr env);
protected:
	DummyValuePtr readP();
	DummyValuePtr readList();
	DummyValueList readListP();
	DummyValuePtr readNum();
	DummyValuePtr readStr();
	DummyValuePtr readQuote();
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
