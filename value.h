#pragma once

#include <string>
#include <sstream>
#include <cstdarg>
#include <vector>
#include <map>

#include "dummyscheme.h"

namespace DummyScheme {
enum DummyType {
	DUMMY_INT_NUM,
	DUMMY_FLOAT_NUM,
	DUMMY_STRING,
	DUMMY_SYMBOL,
	DUMMY_NIL,
	DUMMY_TRUE,
	DUMMY_LAMBDA,
	DUMMY_LIST,
};

class Tokenize;

class DummyValue : public DummyRefCount {
public:
	static DummyValuePtr nil;
	static DummyValuePtr t;
	static std::string apply;
	static std::string lambda;
protected:
	friend class Tokenize;
	DummyType type;
	union {
		int intnum;
		double floatnum;	
	} basic;
	BindList strOrSymOrBind;
	DummyValueList list;
public:
	int getInt(DummyEnvPtr env);
public:
	DummyValuePtr eval(DummyEnvPtr env);
	std::string toString();
public:
	bool isInt() { return type == DUMMY_INT_NUM; }
	bool isFloat() { return type == DUMMY_FLOAT_NUM; }
	bool isString() { return type == DUMMY_STRING; }
	bool isSymbol() { return type == DUMMY_SYMBOL; }
	bool isNil() { return type == DUMMY_NIL; }
	bool isTrue() { return type == DUMMY_TRUE; }
	bool isLambda() { return type == DUMMY_LAMBDA; }
	bool isList() { return type == DUMMY_LIST; }
	
	DummyType getType() { return type; }
	std::string getStr() const { return strOrSymOrBind[0]; }
	std::string getSymbol() const { return strOrSymOrBind[0]; }
	BindList getBind() const { return strOrSymOrBind; }
	int getInt() const { return basic.intnum; }
	double getDouble() const { return basic.floatnum; }
	DummyValueList getList() { return list; }
public:
	DummyValue(int num);
	DummyValue(DummyType type, const std::string &val);
	DummyValue(DummyValuePtr binds, DummyValueList::iterator begin, DummyValueList::iterator end);
	DummyValue(DummyValueList list);
	DummyValue(DummyValuePtr val);
	~DummyValue();
};
}
