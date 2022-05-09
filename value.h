#pragma once

#include <string>
#include <sstream>
#include <cstdarg>
#include <vector>
#include <map>

#include "dummyscheme.h"


namespace DummyScheme {
#define STR_NIL "nil"
#define STR_TRUE "#t"
#define STR_FALSE "#f"

class Tokenize;

class DummyValue : public DummyRefCount {
public:
	static DummyValuePtr nil;
	static DummyValuePtr t;
	static DummyValuePtr f;
protected:
	DummyValue() {}
	int type;
public:
	static DummyValuePtr create(DummyValueList& list);
public:
	virtual int getInt() { Error("error not a num value"); return 0; }
	virtual double getDouble() { Error("error not a num value"); return 0; }
	virtual std::string getStr() { Error("error not a string value"); return ""; }
	virtual std::string getSymbol() { Error("error not a symbol value"); return ""; }
	virtual BindList getBind() { Error("error not a lambda value"); return BindList(); }
	virtual const char* const getTypeStr() { Error("error not a op type value"); return ""; }
	virtual DummyValueList getList() { Error("error dont have list item"); return DummyValueList(); }
	virtual std::string toString();
	virtual DummyValuePtr eval(DummyEnvPtr env);
public:
	bool isEqualValue(DummyValuePtr other, DummyEnvPtr env);
	int getInt(DummyEnvPtr env);
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
	
	bool isNilValue() { return isNil() || isFalse(); }
	bool isTrueValue() { return !isNilValue(); }
	bool isFalseValue() { return isNilValue(); }
	
	int getType() { return type; }
public:
	DummyValue(int type);
	virtual ~DummyValue() {}
};

/*
	num value
 */
class DummyNumValue : public DummyValue {
public:
	DummyNumValue(int num);
	DummyNumValue(double num);
	
	virtual int getInt() { return basic.intnum; }
	virtual double getDouble() { return basic.floatnum; }
	virtual std::string toString();
protected:
	union {
		int intnum;
		double floatnum;	
	} basic;
};

/*
	string value
 */
class DummyStringValue : public DummyValue {
public:
	DummyStringValue(const std::string &val);
	virtual std::string getStr() { return str; }
	virtual std::string toString();
protected:
	std::string str;
};

/*
	symbol value
	TODO: hash the symbol and share one value when symbol is same
 */
class DummySymbolValue : public DummyValue {
public:
	DummySymbolValue(const std::string &value);
	virtual std::string getSymbol() { return symbol; }
	virtual std::string toString();
	virtual DummyValuePtr eval(DummyEnvPtr env);
protected:
	std::string symbol;
};

/*
	general list type
 */
class DummyListValue : public DummyValue {
public:
	DummyListValue(DummyValueList list);
	virtual DummyValueList getList() { return list; }
	virtual std::string toString();
protected:
	DummyValueList list;
};

/*
	lambda value
 */
class DummyLambdaValue : public DummyValue {
public:
	DummyLambdaValue(BindList binds, DummyValueList list);
	virtual BindList getBind() { return binds; }
	virtual DummyValueList getList() { return list; }
	virtual std::string toString();
protected:
	BindList binds;
	DummyValueList list;
};

/*
	op type like define let
 */
class DummyOpTypeValue : public DummyValue {
public:
	DummyOpTypeValue(const char* const typeStr, int type, DummyValueList list);
	virtual const char* const getTypeStr() { return typeStr; }
	virtual DummyValueList getList() { return list; }
	virtual std::string toString();
	virtual DummyValuePtr eval(DummyEnvPtr env);
protected:
	const char* const typeStr;
	DummyValueList list;
};

}
