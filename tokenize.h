#pragma once

#include "dummyscheme.h"

namespace DummyScheme {	
	
enum {
	TOKEN_UNKNOWN,
	TOKEN_NUM,
	TOKEN_DOUBLE_QUOTE,
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_SYMBOL,
	TOKEN_SINGLE_QUOTE,
	TOKEN_UNQUOTE,
	TOKEN_UNQUOTE_SPLICING,
	TOKEN_QUASIQUOTE,
};

/*
	P = NUM | STRING | SYMBOL | QUOTE | UNQUOTE | UNQUOTE_SPLICING | QUASIQUOTE | LEFT_PAREN LIST RIGHT_PAREN	
	LIST = P LISTP
	LISTP = P LISTP

	difference between LIST and LISTP is the return value
*/
class Tokenize {
protected:
	DummyShareEnvPtr shareEnv;
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
	DummyValuePtr readUnQuote();
	DummyValuePtr readUnQuoteSplicing();
	DummyValuePtr readQuasiQuote();
	// read a word as symbol
	DummyValuePtr readSymbol();
protected:
	// space and tab
	// TODO: comment
	void skipBlank();
	bool isBlank();
	int readToken();
	int look();
};
}
