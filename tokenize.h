#pragma once

#include "dummyscheme.h"

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
	TokenType headType;
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
	
public:
	static OpMap opMap;
public:
	static void addOp(const std::string &symbol, OpFunc func);
	static void addOpForCheck(const std::string &symbol);
	static void init();
protected:
	bool isNum();
	// space and tab
	// TODO: comment
	void skipBlank();
	bool isBlank();
	TokenType readToken();
	void unexpectedToken();
	char getCurChar() { return input[index]; }
};
