#pragma once

#include <string>
#include <sstream>
#include <cstdarg>
#include <vector>

enum TokenType{
	TOKEN_UNKNOWN,
	TOKEN_NUM,
	TOKEN_DOUBLE_QUOT,
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_SYMBOL,
};

enum DummyType {
	DUMMY_INT_NUM,
	DUMMY_FLOAT_NUM,
	DUMMY_STRING,
	DUMMY_SYMBOL,
	DUMMY_LIST,
};

class Tokenize;

class DummyValue{
protected:
	friend class Tokenize;
	DummyType type;
	union {
		int intnum;
		double floatnum;	
	} basic;
	std::string strAndSymbol;
	std::vector<DummyValue*> list;	
public:
	DummyValue(int num);
	DummyValue(DummyType type, std::string val);
	DummyValue(std::vector<DummyValue*> list);
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
	Tokenize(std::string &input);
protected:
	DummyValue* readP();
	std::vector<DummyValue*> readList();
	std::vector<DummyValue*> readListP();
	DummyValue* readNum();
	DummyValue* readStr();
	// read a word as symbol
	DummyValue* readSymbol();	
	
public:
	static void error(const char *fmt, ...);
	static void toString(std::stringstream& out, DummyValue * val);
protected:
	bool isNum();
	// space and tab
	// TODO: comment
	void skipBlank();
	bool isBlank();
	TokenType readToken();
	void unexpectedToken();
};
