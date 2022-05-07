#include "dummyscheme.h"
#include "value.h"
#include "env.h"

namespace DummyScheme {

/*
	for convenience
 */
DummyValuePtr opTypeValue(const char* const typeStr, DummyType type, DummyValueList list)
{
	return DummyValuePtr(new DummyValue(typeStr, type, list));
}

DummyValuePtr opTypeValue(const char* const typeStr, DummyType type, DummyValueList::iterator begin, DummyValueList::iterator end)
{
	return opTypeValue(typeStr, type, DummyValueList(begin, end));	
}

DummyValuePtr listValue(DummyValueList& list)
{
	return DummyValuePtr(new DummyValue(list));
}

DummyValuePtr listValue(DummyValueList::iterator begin, DummyValueList::end)
{
	return listValue(DummyValueList(begin, end));
}

DummyValuePtr strValue(const std::string &str)
{
	return DummyValuePtr(new DummyValue(DummyType::DUMMY_STRING, str));
}

DummyValuePtr numValue(int num)
{
	return DummyValuePtr(new DummyValue(num));
}

DummyValuePtr symbolValue(const std::string &symbol)
{
	return DummyValuePtr(new DummyValue(DummyType::DUMMY_SYMBOL, symbol));
}

DummyValuePtr lambdaValue(BindList& binds, DummyValueList& list)
{
	return DummyValuePtr(new DummyValue(binds, list));
}

DummyValuePtr lambdaValue(BindList& binds, DummyValueList::iterator begin, DummyValueList::iterator end)
{
	return lambdaValue(binds, DummyValueList(begin, end));
}

}
