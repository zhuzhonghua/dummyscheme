#pragma once

#include <string>
#include <sstream>
#include <cstdarg>
#include <vector>
#include <map>

#include "dummyscheme.h"

namespace DummyScheme {
class Tokenize;

class DummyValue : public DummyRefCount {
public:
	static DummyValuePtr nil;
	static DummyValuePtr t;
	static DummyValuePtr f;
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
	static std::string getTypeStr(int type);
	static DummyType getStrType(const std::string& symbol);
	static DummyValuePtr create(DummyValueList& list);
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
	bool isFalse() { return type == DUMMY_FALSE; }
	bool isLambda() { return type == DUMMY_LAMBDA; }
	bool isList() { return type == DUMMY_LIST; }
	
	bool isSame(const std::string& sym) { return 0 == strOrSymOrBind[0].compare(sym); }
	bool isNilValue() { return isNil() || isFalse(); }
	
	DummyType getType() { return type; }
	std::string getStr() { return strOrSymOrBind[0]; }
	std::string getSymbol() { return strOrSymOrBind[0]; }
	std::string getSymbolCheck() { AssertDummyValue(isSymbol(), "", this); return strOrSymOrBind[0]; }
	BindList getBind() { return strOrSymOrBind; }
	int getInt() { AssertDummyValue(isInt(), "", this); return basic.intnum; }
	double getDouble() { AssertDummyValue(isFloat(), "", this); return basic.floatnum; }
	DummyValueList getList() { return list; }
	DummyValueList getListCheck() { AssertDummyValue(isList(), "", this); return list; }
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
