#pragma once

#include "dummy.h"

namespace Dummy {

enum {
	TOKEN_UNKNOWN,
	TOKEN_END,
	TOKEN_NUM,
	TOKEN_STRING,
	TOKEN_DOUBLE_QUOTE,
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_SYMBOL,
	TOKEN_QUOTE,
	TOKEN_UNQUOTE,
	TOKEN_UNQUOTE_SPLICING,
	TOKEN_QUASIQUOTE,
};

/*
  VALUE = NUM | STRING | SYMBOL | QUOTE | UNQUOTE | UNQUOTE_SPLICING | QUASIQUOTE | LEFT_PAREN LIST RIGHT_PAREN
	LIST = VALUE LIST
 */
class Reader {
protected:
	int aheadToken;
	int numLexVal;
	String strLexVal;

	int index;
	String input;

public:
  void parse(const String &input);

protected:
  VarValue readValue();
	VarValue readList();
protected:
  int readNum();
	int readString();
	int readSymbol();

  // space and tab
	// TODO: comment
	bool isBlank(char c);
	int dLex();
	void match(int type);

protected:
  void skipBlankComment();
  bool isEnd();
  char curChar();
  char curCharAdv();
  void advInput();
};
};
