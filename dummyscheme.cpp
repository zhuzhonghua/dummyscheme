#include "dummyscheme.h"
#include "value.h"
#include "env.h"

namespace DummyScheme {

/*
	for convenience
 */
DummyValuePtr opTypeValue(const char* const typeStr, int type, const DummyValueList& list)
{
	return new DummyValue(typeStr, type, list);
}

DummyValuePtr opTypeValue(const char* const typeStr, int type, DummyValueList::iterator begin, DummyValueList::iterator end)
{
	return opTypeValue(typeStr, type, DummyValueList(begin, end));	
}

DummyValuePtr listValue(const DummyValueList& list)
{
	return new DummyValue(list);
}

DummyValuePtr listValue(DummyValueList::iterator begin, DummyValueList::iterator end)
{
	return listValue(DummyValueList(begin, end));
}

DummyValuePtr strValue(const std::string &str)
{
	return new DummyValue(DUMMY_TYPE_STRING, str);
}

DummyValuePtr numValue(int num)
{
	return new DummyValue(num);
}

DummyValuePtr symbolValue(const std::string &symbol)
{
	return new DummyValue(DUMMY_TYPE_SYMBOL, symbol);
}

DummyValuePtr lambdaValue(const BindList& binds, const DummyValueList& list)
{
	return new DummyValue(binds, list);
}

DummyValuePtr lambdaValue(const BindList& binds, DummyValueList::iterator begin, DummyValueList::iterator end)
{
	return lambdaValue(binds, DummyValueList(begin, end));
}

}
