#pragma once

#include "dummy.h"

namespace Dummy {

#define STR_NIL "nil"
#define STR_TRUE "#t"
#define STR_FALSE "#f"

class Value : public RefCount {
public:
	static ValuePtr nil;
	static ValuePtr t;
	static ValuePtr f;

public:
  virtual String toString() { return ""; }
	virtual ~Value() {}
};

/*
	num value
 */
class NumValue : public Value {
public:
  static ValuePtr create(int num);
public:
	NumValue(int num);

	virtual int getInt() { return num; }
	virtual String toString();
protected:
  int num;

};

/*
	string value
 */
class StringValue : public Value {
public:
  static ValuePtr create(const String &val);
public:
	StringValue(const String &val);
	virtual String getStr() { return str; }
	virtual String toString();
protected:
	String str;
};

/*
	symbol value
 */
class SymbolValue : public Value {
public:
  static ValuePtr create(const String &val);
public:
	SymbolValue(const String &value);
	virtual String getSymbol() { return symbol; }
	virtual String toString();
protected:
	String symbol;
};

class PairValue : public Value {
public:
  static ValuePtr create(ValuePtr head, ValuePtr tail);
public:
  PairValue(ValuePtr head, ValuePtr tail);
	virtual String toString();

  void car(ValuePtr val) { head = val; }
  ValuePtr car() { return head; }
  void cdr(ValuePtr val) { tail = val; }
  ValuePtr cdr() { return tail; }
protected:
  ValuePtr head;
  ValuePtr tail;
};

/*
	list value
 */
class ListValue : public Value {
public:
  static ValuePtr create(const ValueList& list);
public:
	ListValue(ValueList list);
	virtual ValueList getList() { return list; }
  virtual ValuePtr getListItem(int n) { return list[n]; }
//  virtual bool memq(ValuePtr a);
	virtual String toString();
  virtual bool isNull() { return list.empty(); }
//	virtual ValuePtr eval(EnvPtr env);
protected:
	ValueList list;
  ValueSet set;
};

};
