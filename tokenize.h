#pragma once

#include "dummyscheme.h"

namespace DummyScheme {	
	
enum {
	TOKEN_UNKNOWN,
	TOKEN_END,
	TOKEN_NUM,
	TOKEN_STRING,
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
	int numLexVal;
	String strLexVal;
protected:
	int index;
	String input;
public:
	Tokenize(const String &input);
	void init(const String &input);
	DummyValuePtr run(DummyEnvPtr env);
protected:
	DummyValuePtr readP();
	DummyValuePtr readList();
	DummyValueList readListP();
	
	DummyValuePtr readQuoteRelate(int type, const char* typeStr);

protected:
	DummyValuePtr getShareSymbol(const String &symbol);
protected:
	int readNum();
	int readStr();
	int readSymbol();
protected:
	// space and tab
	// TODO: comment
	bool isBlank();
	int dLex();
	void match(int type);
};
}
