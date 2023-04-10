#pragma once

#include "dummy.h"

namespace Dummy {

class Value : public RefCount {
protected:
  Value() {}
public:
  virtual String getSymbol() { throw "not a symbol"; return ""; }
  virtual int getNum() { throw "not a number"; return 0; }
  virtual VarValue getEnvSym(VarValue symbol, int lexAddr) { throw "not a env, cant getlexsym"; return NULL; }

	virtual VarValue findEnv(VarValue symbol, int lexAddr) { throw "not a env, cant find lex addr "; return NULL; }
	virtual VarValue setEnvSym(VarValue symbol, VarValue value) { throw "not a env, cant set"; return NULL; }

  virtual int getSymLexAddr(VarValue symbol) { throw "not a env, cant getsymlexaddr"; return NULL; }
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

	virtual String getSymbol() { return symbol; }
	virtual String toString() {
    return symbol;
  }
protected:
  SymbolValue(const String &value) { symbol = value; }
	String symbol;
};

/*
	symbol value
 */
class LexSymbolValue : public Value {
public:
  static LexSymbolValue* create(VarValue value, VarValue env);

	virtual String toString();
  virtual VarValue eval(VarValue env);
protected:
  LexSymbolValue(VarValue value, int addr);
  VarValue variable;
  int lexAddr;
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
  static AssignmentValue* create(VarValue exp, VarValue env);

  virtual VarValue eval(VarValue env);
  virtual String toString() {
    StringStream out;
    out << "(set! " << var << " " << vproc->toString() << ")";

    return out.str();
  }
  void trace() {
    RefGC::trace(var);
    RefGC::trace(vproc);
  }
protected:
  AssignmentValue(VarValue v, VarValue vp, int addr) { lexAddr = addr; var = v; vproc = vp; }
  MemberValue var;
  MemberValue vproc;
  int lexAddr;
};

class DefineValue: public AssignmentValue {
public:
  static DefineValue* create(VarValue exp, VarValue env);
  VarValue eval(VarValue env);
  String toString() {
    StringStream out;
    out << "(define " << var << " " << vproc->toString() << ")";
    return out.str();
  }
protected:
  static VarValue def_variable(VarValue val);
  static VarValue def_value(VarValue val);
protected:
  DefineValue(VarValue v, VarValue vp, int addr)
    :AssignmentValue(v, vp, addr) { }
};

class IfValue: public Value {
public:
  static IfValue* create(VarValue exp, VarValue env);
  static IfValue* create(VarValue p, VarValue c, VarValue a);

  VarValue eval(VarValue env);
  String toString() {
    StringStream out;
    out << "(if "
        << pproc->toString() << " "
        << cproc->toString() << " "
        << aproc->toString() << ")";

    return out.str();
  }

  void trace() {
    RefGC::trace(pproc);
    RefGC::trace(cproc);
    RefGC::trace(aproc);
  }
protected:
  static VarValue predicate(VarValue exp);
  static VarValue consequent(VarValue exp);
  static VarValue alternative(VarValue exp);
protected:
  IfValue(VarValue p, VarValue c, VarValue a) { pproc = p; cproc = c; aproc = a; }
  MemberValue pproc;
  MemberValue cproc;
  MemberValue aproc;
};

class ProcedureValue: public Value {
public:
  static ProcedureValue* create(VarValue exp, VarValue env);

  VarValue eval(VarValue env);
  String toString() {
    StringStream out;
    out << "#<procedure>";

    return out.str();
  }
  void trace() {
    RefGC::trace(vars);
    RefGC::trace(bproc);
  }
protected:
  static VarValue parameters(VarValue exp);
  static VarValue body(VarValue exp);
protected:
  ProcedureValue(VarValue v, VarValue b) { vars = v; bproc = b; }
  MemberValue vars;
  MemberValue bproc;
};

class ApplicationValue: public Value {
public:
  static ApplicationValue* create(VarValue exp, VarValue env);

  VarValue eval(VarValue env);
  String toString() {
    StringStream out;
    out << "#<application>:" << fproc->toString();

    return out.str();
  }
  void trace() {
    RefGC::trace(fproc);
    RefGC::trace(aprocs);
  }
protected:
  ApplicationValue(VarValue f, VarValue a) { fproc = f; aprocs = a; }
  MemberValue fproc;
  MemberValue aprocs;
};

class EnvValue : public Value {
protected:
  typedef std::map<MemberValue, MemberValue> VarMap;
  typedef VarMap::iterator VarMapItr;
public:
  static EnvValue* create(VarValue o);

  virtual void trace() {
    for (VarMapItr itr = vars.begin(); itr != vars.end(); itr++)
    {
      VarValue key = itr->first;
      VarValue value = itr->second;
      RefGC::trace(key);
      RefGC::trace(value);
    }

    RefGC::trace(outer);
  }

  String toString() { return "<env>"; }
	VarValue setEnvSym(VarValue symbol, VarValue value);

  virtual VarValue getEnvSym(VarValue symbol, int lexAddr);
  virtual VarValue findEnv(VarValue symbol, int lexAddr);
protected:
  EnvValue* findEnvLex(VarValue symbol, int lexAddr);
protected:
	EnvValue(VarValue o);

	VarMap vars;
	MemberValue outer;
};

class CompileEnvValue : public Value {
protected:
  typedef std::set<MemberValue> VarSet;
  typedef VarSet::iterator VarSetItr;
public:
  static CompileEnvValue* create(VarValue o, VarValue variables);

  virtual void trace() {
    for (VarSetItr itr = vars.begin(); itr != vars.end(); itr++) {
      VarValue var = *itr;
      RefGC::trace(var);
    }

    RefGC::trace(outer);
  }

  String toString() { return "<compile-env>"; }
protected:
  virtual int getSymLexAddr(VarValue symbol);
protected:
	CompileEnvValue(VarValue o, VarValue v);

	VarSet vars;
	MemberValue outer;
};

};
