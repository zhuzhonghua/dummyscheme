#pragma once

#include "dummy.h"

namespace Dummy {

class Value : public RefCount {
protected:
  Value() {}
public:
  virtual String getSymbol() { throw "not a symbol"; return ""; }
  virtual int getNum() { throw "not a number"; return 0; }
  virtual VarValue getEnvSym(VarValue symbol) { throw "not a env, cant get"; return NULL; }
	virtual VarValue findEnv(VarValue symbol) { throw "not a env, cant find"; return NULL; }
	virtual VarValue setEnvSym(VarValue symbol, VarValue value) { throw "not a env, cant set"; return NULL; }

  virtual operator bool() const { return true; }
public:
  virtual String toString() { return ""; }
	virtual ~Value() {}

  virtual void trace() {}

  virtual VarValue eval(VarValue env) { throw "cannot eval"; return NULL; };
public:
  virtual void car(VarValue val) { throw " not a pair "; }
  virtual void cdr(VarValue val) { throw " not a pair "; }
  virtual VarValue car() { throw "not a pair "; return NULL; }
  virtual VarValue cdr() { throw "not a pair"; return NULL; }
};

/*
  null value
 */
class NullValue : public Value {
public:
  static NullValue* create();
  virtual operator bool() const { return false; }
  virtual VarValue eval(VarValue env) { return this; }
  virtual String toString() { return "'()"; }
protected:
  NullValue() {}
};

/*
	num value
 */
class NumValue : public Value {
public:
  static NumValue* create(int n);

  virtual VarValue eval(VarValue env) { return this; }
	virtual int getNum() { return num; }
	virtual String toString() {
    StringStream out;
    out << num;
    return out.str();
  }
protected:
  NumValue(int n) { num = n; }
  int num;
};

/*
  bool value
 */
class BoolValue : public Value {
public:
  static BoolValue* create(bool b);

  virtual VarValue eval(VarValue env) { return this; }
  virtual String toString() {
    StringStream out;
    out << val;
    return out.str();
  }

  virtual operator bool() const { return val; }
protected:
  BoolValue(bool b) { val = b; }
  bool val;
};

/*
	string value
 */
class StringValue : public Value {
public:
  static StringValue* create(const String &val);

  virtual VarValue eval(VarValue env) { return this; }
	virtual String getStr() { return str; }
	virtual String toString() {
    return "\"" + str + "\"";
  }
protected:
  StringValue(const String &val) { str = val; }
	String str;
};

/*
	symbol value
 */
class SymbolValue : public Value {
public:
  static SymbolValue* create(const String &val);

  virtual VarValue eval(VarValue env);
	virtual String getSymbol() { return symbol; }
	virtual String toString() {
    return symbol;
  }
protected:
  SymbolValue(const String &value) { symbol = value; }
	String symbol;
};

class PairValue : public Value {
public:
  static PairValue* create(VarValue h, VarValue t);
	String toString();

  void trace() {
    RefGC::trace(head);
    RefGC::trace(tail);
  }
public:
  virtual void car(VarValue val) { head = val; }
  virtual void cdr(VarValue val) { tail = val; }

  virtual VarValue car() { return head; }
  virtual VarValue cdr() { return tail; }

protected:
  PairValue(VarValue h, VarValue t) { head = h; tail = t; }
  MemberValue head;
  MemberValue tail;
};

class AssignmentValue: public Value {
public:
  static AssignmentValue* create(VarValue v, VarValue vp);

  VarValue eval(VarValue env);
  String toString();
  void trace();
protected:
  AssignmentValue(VarValue v, VarValue vp) { var = v; vproc = vp; }
  MemberValue var;
  MemberValue vproc;
};

class DefineValue: public AssignmentValue {
public:
  static DefineValue* create(VarValue v, VarValue vp);
  VarValue eval(VarValue env);
  String toString();
protected:
  DefineValue(VarValue v, VarValue vp): AssignmentValue(v, vp) { }
};

class IfValue: public Value {
public:
  static IfValue* create(VarValue p, VarValue c, VarValue a);

  VarValue eval(VarValue env);
  String toString();
  void trace();
protected:
  IfValue(VarValue p, VarValue c, VarValue a) { pproc = p; cproc = c; aproc = a; }
  MemberValue pproc;
  MemberValue cproc;
  MemberValue aproc;
};

class ProcedureValue: public Value {
public:
  static ProcedureValue* create(VarValue v, VarValue b);

  VarValue eval(VarValue env);
  String toString();
  void trace();
protected:
  ProcedureValue(VarValue v, VarValue b) { vars = v; bproc = b; }
  MemberValue vars;
  MemberValue bproc;
};

class ApplicationValue: public Value {
public:
  static ApplicationValue* create(VarValue f, VarValue a);

  VarValue eval(VarValue env);
  String toString();
  void trace();
protected:
  ApplicationValue(VarValue f, VarValue a) { fproc = f; aprocs = a; }
  MemberValue fproc;
  MemberValue aprocs;
};

class EnvValue : public Value {
public:
  static EnvValue* create(VarValue o);

  void trace();

  String toString() { return "<env>"; }
	VarValue getEnvSym(VarValue symbol);
	VarValue findEnv(VarValue symbol);
	VarValue setEnvSym(VarValue symbol, VarValue value);
protected:
	EnvValue(VarValue o) { outer = o; }

  typedef std::map<MemberValue, MemberValue> VarMap;
  typedef VarMap::iterator VarMapItr;

	VarMap vars;
	MemberValue outer;
};

};
