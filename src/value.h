#pragma once

#include "dummy.h"

namespace Dummy {

#define STR_NIL "nil"
#define STR_TRUE "#t"
#define STR_FALSE "#f"

class Value : public RefCount {
public:
	static VarValue nil;

public:
  virtual String toString() { return ""; }
	virtual ~Value() {}

  void trace() {}
};

/*
	num value
 */
class NumValue : public Value {
public:
  static VarValue create(int num);
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
  static VarValue create(const String &val);
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
  static VarValue create(const String &val);
public:
	SymbolValue(const String &value);
	virtual String getSymbol() { return symbol; }
	virtual String toString();
protected:
	String symbol;
};

class PairValue : public Value {
public:
  static VarValue create(VarValue head, VarValue tail);
public:
  PairValue(VarValue head, VarValue tail);
	virtual String toString();

  void car(VarValue val) { head = val; }
  VarValue car() { return head; }
  void cdr(VarValue val) { tail = val; }
  VarValue cdr() { return tail; }

  void trace();
protected:
  MemberValue head;
  MemberValue tail;
};
};
