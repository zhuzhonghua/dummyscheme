#pragma once

#include "dummy.h"

namespace Dummy {

class Value : public RefCount {
public:
  virtual String getSymbol() { throw "not a symbol"; return ""; }
  virtual int getNum() { throw "not a number"; return 0; }
  virtual VarValue getEnvSym(VarValue symbol) { throw "not a env, cant get"; return NULL; }
	virtual VarValue findEnv(VarValue symbol) { throw "not a env, cant find"; return NULL; }
	virtual VarValue setEnvSym(VarValue symbol, VarValue value) { throw "not a env, cant set"; return NULL; }
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
	num value
 */
class NumValue : public Value {
public:
	NumValue(int num);

  virtual VarValue eval(VarValue env) { return this; }
	virtual int getNum() { return num; }
	virtual String toString();
protected:
  int num;
};

/*
	string value
 */
class StringValue : public Value {
public:
	StringValue(const String &val);

  virtual VarValue eval(VarValue env) { return this; }
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
	SymbolValue(const String &value);

  virtual VarValue eval(VarValue env);
	virtual String getSymbol() { return symbol; }
	virtual String toString();
protected:
	String symbol;
};

class PairValue : public Value {
protected:
  PairValue() {}
public:
  PairValue(VarValue head, VarValue tail);
	String toString();
  void trace();

public:
  virtual void car(VarValue val) { head = val; }
  virtual void cdr(VarValue val) { tail = val; }

  virtual VarValue car() { return head; }
  virtual VarValue cdr() { return tail; }

protected:
  MemberValue head;
  MemberValue tail;
};

class AssignmentValue: public Value {
public:
  AssignmentValue(VarValue v, VarValue vp);
  VarValue eval(VarValue env);
  String toString();
  void trace();
protected:
  MemberValue var;
  MemberValue vproc;
};

class DefineValue: public AssignmentValue {
public:
  DefineValue(VarValue v, VarValue vp): AssignmentValue(v, vp) {}
  VarValue eval(VarValue env);
  String toString();
};

class IfValue: public Value {
public:
  IfValue(VarValue p, VarValue c, VarValue a);
  VarValue eval(VarValue env);
  String toString();
  void trace();
protected:
  MemberValue pproc;
  MemberValue cproc;
  MemberValue aproc;
};

class ProcedureValue: public Value {
public:
  ProcedureValue(VarValue v, VarValue b);
  VarValue eval(VarValue env);
  String toString();
  void trace();
protected:
  MemberValue vars;
  MemberValue bproc;
};

class ApplicationValue: public Value {
public:
  ApplicationValue(VarValue fproc, VarValue aprocs);
  VarValue eval(VarValue env);
  String toString();
  void trace();
protected:
  MemberValue fproc;
  MemberValue aprocs;
};

class EnvValue : public Value {
public:
  EnvValue(): EnvValue(NULL) {}
	EnvValue(VarValue outer);
  void trace();
public:
  String toString() { return "<env>"; }
	VarValue getEnvSym(VarValue symbol);
	VarValue findEnv(VarValue symbol);
	VarValue setEnvSym(VarValue symbol, VarValue value);
protected:
	VarMap vars;
	MemberValue outer;
};

};
