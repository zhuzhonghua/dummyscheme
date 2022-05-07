#pragma once

#include <string>
#include <sstream>
#include <cstdarg>
#include <vector>
#include <map>

#include "dummyscheme.h"

namespace DummyScheme {
class Tokenize;
typedef std::vector<std::string> BindList;

class DummyValue : public DummyRefCount {
public:
	static DummyValuePtr nil;
	static DummyValuePtr t;
	static DummyValuePtr f;
protected:
	friend class Tokenize;
	int type;
	union {
		int intnum;
		double floatnum;	
		const char* const typeStr;
	} basic;
	// TODO: all same symbols share one value
	BindList strOrSymOrBind;
	DummyValueList list;
public:
	static DummyValuePtr create(DummyValueList& list);
public:
	int getInt(DummyEnvPtr env);
public:
	DummyValuePtr eval(DummyEnvPtr env);
	std::string toString();
	bool isEqualValue(DummyValuePtr other, DummyEnvPtr env);
public:
	bool isInt() { return type == DUMMY_TYPE_INT_NUM; }
	bool isFloat() { return type == DUMMY_TYPE_FLOAT_NUM; }
	bool isString() { return type == DUMMY_TYPE_STRING; }
	bool isSymbol() { return type == DUMMY_TYPE_SYMBOL; }
	bool isNil() { return type == DUMMY_TYPE_NIL; }
	bool isTrue() { return type == DUMMY_TYPE_TRUE; }
	bool isFalse() { return type == DUMMY_TYPE_FALSE; }
	bool isLambda() { return type == DUMMY_TYPE_LAMBDA; }
	bool isList() { return type == DUMMY_TYPE_LIST; }
	bool isUnQuote() { return type == DUMMY_TYPE_UNQUOTE; }
	bool isUnQuoteSplicing() { return type == DUMMY_TYPE_UNQUOTE_SPLICING; }
	
	bool isSame(const std::string& sym) { return 0 == strOrSymOrBind[0].compare(sym); }
	bool isNilValue() { return isNil() || isFalse(); }
	bool isTrueValue() { return !isNilValue(); }
	bool isFalseValue() { return isNilValue(); }
	
	int getType() { return type; }
	const char* getTypeStr() { return basic.typeStr; }
	std::string getStr() { return strOrSymOrBind[0]; }
	std::string getSymbol() { return strOrSymOrBind[0]; }
	BindList getBind() { return strOrSymOrBind; }
	int getInt() { return basic.intnum; }
	double getDouble() { return basic.floatnum; }
	DummyValueList getList() { return list; }
public:
	DummyValue(int num);
	DummyValue(int type, const std::string &val);
	DummyValue(const char* typeStr, int type, DummyValueList list);
	DummyValue(BindList binds, DummyValueList list);
	DummyValue(DummyValueList list);
	DummyValue(DummyValuePtr val);
	~DummyValue();
};
}
