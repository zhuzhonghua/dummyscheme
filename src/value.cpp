#include "value.h"

using namespace Dummy;

ValuePtr Value::nil = SymbolValue::create(STR_NIL);

ValuePtr NumValue::create(int num)
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

ValuePtr StringValue::create(const String &val)
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

ValuePtr SymbolValue::create(const String &val)
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

ValuePtr PairValue::create(ValuePtr head, ValuePtr tail)
{
  return new PairValue(head, tail);
}

PairValue::PairValue(ValuePtr head, ValuePtr tail)
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

ValuePtr ListValue::create(const ValueList& list)
{
  return new ListValue(list);
}

ListValue::ListValue(ValueList list)
{
  this->list = list;
}

String ListValue::toString()
{
  StringStream out;
	out << "(";
	ValueListItr itr = list.begin();
	for (;itr != list.end(); itr++)
	{
		out << (*itr)->toString();
		if (itr + 1 != list.end())
			out << " ";
	}
	out << ")";

	return out.str();
}
