#include "dummy.h"
#include "value.h"

namespace Dummy {

SymbolMap Dummy::globalSymbols;

const ValuePtr Dummy::Quote = Dummy::intern("quote");
const ValuePtr Dummy::UnQuote = Dummy::intern("unquote");
const ValuePtr Dummy::UnQuoteSplicing = Dummy::intern("unquote-splicing");
const ValuePtr Dummy::QuasiQuote = Dummy::intern("quasiquote");

ValuePtr Dummy::intern(String symbol)
{
  SymbolMapItr itr = globalSymbols.find(symbol);
  if (itr != globalSymbols.end())
    return itr->second;

  ValuePtr value = SymbolValue::create(symbol);
  globalSymbols[symbol] = value;

  return value;
}

ValuePtr Dummy::list2(ValuePtr a, ValuePtr b)
{
  ValuePtr val = cons(b, Value::nil);
  return PairValue::create(a, val);
}

ValuePtr Dummy::cons(ValuePtr a, ValuePtr b)
{
  return PairValue::create(a, b);
}

bool Dummy::pairp(ValuePtr val)
{
  return dynamic_cast<PairValue*>(val.ptr()) ? true : false;
}

ValuePtr Dummy::car(ValuePtr pair, ValuePtr val)
{
  PairValue* tmp = dynamic_cast<PairValue*>(pair.ptr());
  if (!tmp)
    throw "not a pair";

  tmp->car(val);

  return pair;
}

ValuePtr Dummy::cdr(ValuePtr pair, ValuePtr val)
{
  PairValue* tmp = dynamic_cast<PairValue*>(pair.ptr());
  if (!tmp)
    throw "not a pair";

  tmp->cdr(val);

  return pair;
}

ValuePtr Dummy::car(ValuePtr pair)
{
  PairValue* tmp = dynamic_cast<PairValue*>(pair.ptr());
  if (!tmp)
    throw "not a pair";

  return tmp->car();
}

ValuePtr Dummy::cdr(ValuePtr pair)
{
  PairValue* tmp = dynamic_cast<PairValue*>(pair.ptr());
  if (!tmp)
    throw "not a pair";

  return tmp->cdr();
}
}
