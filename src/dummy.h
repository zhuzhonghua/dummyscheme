#pragma once

#include "util.h"
#include "refcount.h"

namespace Dummy {

class Value;
typedef RefVarPtr<Value>  VarValue;
typedef RefMemberPtr<Value>  MemberValue;

class Env;
typedef RefVarPtr<Env>  VarEnv;

typedef std::map<String, VarValue> SymbolMap;
typedef SymbolMap::iterator SymbolMapItr;

class Dummy{
public:
  static SymbolMap globalSymbols;
public:
  static const VarValue Quote;
  static const VarValue UnQuote;
  static const VarValue UnQuoteSplicing;
  static const VarValue QuasiQuote;
public:
  static VarValue intern(String symbol);
  static VarValue list2(VarValue a, VarValue b);
  static VarValue cons(VarValue a, VarValue b);
  static bool pairp(VarValue val);
  static VarValue car(VarValue pair, VarValue val);
  static VarValue car(VarValue pair);
  static VarValue cdr(VarValue pair, VarValue val);
  static VarValue cdr(VarValue pair);
};
};
