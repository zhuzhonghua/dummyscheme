#pragma once

#include <string>
#include <sstream>
#include <cstdarg>
#include <vector>
#include <map>

#include "dummyscheme.h"

enum DummyType {
	DUMMY_INT_NUM,
	DUMMY_FLOAT_NUM,
	DUMMY_STRING,
	DUMMY_SYMBOL,
	DUMMY_LIST,
};

class Tokenize;

class DummyValue : public DummyRefCount {
protected:
	friend class Tokenize;
	DummyType type;
	union {
		int intnum;
		double floatnum;	
	} basic;
	std::string strAndSymbol;
	DummyValueList list;	
public:
	int getInt(DummyEnvPtr env);
public:
	DummyValuePtr eval(DummyEnvPtr env);
public:
	bool isInt() { return type == DUMMY_INT_NUM; }
	bool isFloat() { return type == DUMMY_FLOAT_NUM; }
	bool isString() { return type == DUMMY_STRING; }
	bool isSymbol() { return type == DUMMY_SYMBOL; }
	bool isList() { return type == DUMMY_LIST; }
	
	DummyType getType() { return type; }
	std::string getStr() const { return strAndSymbol; }
	std::string getSymbol() const { return strAndSymbol; }
	int getInt() const { return basic.intnum; }
	double getDouble() const { return basic.floatnum; }
	DummyValueList getList() { return list; }
public:
	DummyValue(int num);
	DummyValue(DummyType type, std::string val);
	DummyValue(DummyValueList list);
	DummyValue(DummyValuePtr val);
	~DummyValue();
};

