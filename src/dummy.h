#pragma once

#include "util.h"
#include "refcount.h"

namespace Dummy {

class Value;
typedef RefCountPtr<Value>  ValuePtr;
typedef std::vector<ValuePtr> ValueList;
typedef std::set<ValuePtr> ValueSet;
typedef ValueList::iterator ValueListItr;

class Env;
typedef RefCountPtr<Env>  EnvPtr;

typedef std::map<String, ValuePtr> SymbolMap;
typedef SymbolMap::iterator SymbolMapItr;

class Dummy{
public:
  static SymbolMap globalSymbols;
public:
  static const ValuePtr Quote;
  static const ValuePtr UnQuote;
  static const ValuePtr UnQuoteSplicing;
  static const ValuePtr QuasiQuote;
public:
  static ValuePtr intern(String symbol);
  static ValuePtr list2(ValuePtr a, ValuePtr b);
  static ValuePtr cons(ValuePtr a, ValuePtr b);
  static bool pairp(ValuePtr val);
  static ValuePtr car(ValuePtr pair, ValuePtr val);
  static ValuePtr car(ValuePtr pair);
  static ValuePtr cdr(ValuePtr pair, ValuePtr val);
  static ValuePtr cdr(ValuePtr pair);
};
};
