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
	DUMMY_PLUS,
	DUMMY_MINUS,
	DUMMY_MUL,
	DUMMY_DIVIDE,
	DUMMY_DEFINE,
	DUMMY_LET,
	DUMMY_BEGIN,
	DUMMY_IF,
	DUMMY_WHEN,
	DUMMY_UNLESS,
	DUMMY_LAMBDA,
	DUMMY_APPLY,
	DUMMY_LIST,
	DUMMY_MAX,
};

class Tokenize;

class DummyValue : public DummyRefCount {
public:
	static DummyValuePtr nil;
	static DummyValuePtr t;
	static std::string apply;
	static std::string lambda;
public:
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
	std::string getStr() { AssertDummyValue(isString(), "", this); return strOrSymOrBind[0]; }
	std::string getSymbol() { AssertDummyValue(isSymbol(), "", this); return strOrSymOrBind[0]; }
	BindList getBind() { return strOrSymOrBind; }
	int getInt() { AssertDummyValue(isInt(), "", this); return basic.intnum; }
	double getDouble() { AssertDummyValue(isFloat(), "", this); return basic.floatnum; }
	DummyValueList getList() { AssertDummyValue(isList(), "", this); return list; }
public:
	DummyValue(int num);
	DummyValue(DummyType type, const std::string &val);
	DummyValue(DummyType type, DummyValueList list);
	DummyValue(BindList binds, DummyValueList list);
	DummyValue(DummyValueList list);
	DummyValue(DummyValuePtr val);
	~DummyValue();
};
}
