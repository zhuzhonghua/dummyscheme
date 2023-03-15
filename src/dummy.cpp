#include "dummy.h"
#include "value.h"

namespace Dummy {

SymbolMap Dummy::globalSymbols;

const VarValue Dummy::Quote = Dummy::intern("quote");
const VarValue Dummy::UnQuote = Dummy::intern("unquote");
const VarValue Dummy::UnQuoteSplicing = Dummy::intern("unquote-splicing");
const VarValue Dummy::QuasiQuote = Dummy::intern("quasiquote");

VarValue Dummy::intern(String symbol)
{
  SymbolMapItr itr = globalSymbols.find(symbol);
  if (itr != globalSymbols.end())
    return itr->second;

  VarValue value = SymbolValue::create(symbol);
  globalSymbols[symbol] = value;

  return value;
}

VarValue Dummy::list2(VarValue a, VarValue b)
{
  VarValue val = cons(b, Value::nil);
  return PairValue::create(a, val);
}

VarValue Dummy::cons(VarValue a, VarValue b)
{
  return PairValue::create(a, b);
}

bool Dummy::pairp(VarValue val)
{
  return dynamic_cast<PairValue*>(val.ptr()) ? true : false;
}

VarValue Dummy::car(VarValue pair, VarValue val)
{
  PairValue* tmp = dynamic_cast<PairValue*>(pair.ptr());
  if (!tmp)
    throw "not a pair";

  tmp->car(val);

  return pair;
}

VarValue Dummy::cdr(VarValue pair, VarValue val)
{
  PairValue* tmp = dynamic_cast<PairValue*>(pair.ptr());
  if (!tmp)
    throw "not a pair";

  tmp->cdr(val);

  return pair;
}

VarValue Dummy::car(VarValue pair)
{
  PairValue* tmp = dynamic_cast<PairValue*>(pair.ptr());
  if (!tmp)
    throw "not a pair";

  return tmp->car();
}

VarValue Dummy::cdr(VarValue pair)
{
  PairValue* tmp = dynamic_cast<PairValue*>(pair.ptr());
  if (!tmp)
    throw "not a pair";

  return tmp->cdr();
}

}
