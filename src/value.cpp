#include "value.h"

using namespace Dummy;

VarValue Value::nil = SymbolValue::create(STR_NIL);

VarValue NumValue::create(int num)
{
  return new NumValue(num);
}

NumValue::NumValue(int num)
{
  this->num = num;
}

String NumValue::toString()
{
  StringStream out;
  out << num;
	return out.str();
}

VarValue StringValue::create(const String &val)
{
  return new StringValue(val);
}

StringValue::StringValue(const String &val)
{
  str = val;
}

String StringValue::toString()
{
  return "\"" + str + "\"";
}

VarValue SymbolValue::create(const String &val)
{
  return new SymbolValue(val);
}

SymbolValue::SymbolValue(const String &value)
  :symbol(value)
{
}

String SymbolValue::toString()
{
  return symbol;
}

VarValue PairValue::create(VarValue head, VarValue tail)
{
  return new PairValue(head, tail);
}

PairValue::PairValue(VarValue head, VarValue tail)
{
  this->head = head;
  this->tail = tail;
}

String PairValue::toString()
{
  StringStream out;
	out << "(" << head->toString() << " " << tail->toString() << ")";

	return out.str();
}

void PairValue::trace()
{
  RefGC::trace(head);
  RefGC::trace(tail);
}
