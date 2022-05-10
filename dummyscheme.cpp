#include "dummyscheme.h"
#include "value.h"
#include "env.h"

namespace DummyScheme {

/*
	for convenience
 */
DummyValuePtr opTypeValue(int type, const DummyValueList& list)
{
	return new DummyOpTypeValue(type, list);
}

DummyValuePtr opTypeValue(int type, DummyValueList::iterator begin, DummyValueList::iterator end)
{
	return opTypeValue(type, DummyValueList(begin, end));	
}

DummyValuePtr listValue(const DummyValueList& list)
{
	return new DummyListValue(list);
}

DummyValuePtr listValue(DummyValueList::iterator begin, DummyValueList::iterator end)
{
	return listValue(DummyValueList(begin, end));
}

DummyValuePtr strValue(const std::string &str)
{
	return new DummyStringValue(str);
}

DummyValuePtr numValue(int num)
{
	return new DummyNumValue(num);
}

DummyValuePtr symbolValue(const std::string &symbol)
{
	return new DummySymbolValue(symbol);
}

DummyValuePtr lambdaValue(const BindList& binds, const DummyValueList& list)
{
	return new DummyLambdaValue(binds, list);
}

DummyValuePtr lambdaValue(const BindList& binds, DummyValueList::iterator begin, DummyValueList::iterator end)
{
	return lambdaValue(binds, DummyValueList(begin, end));
}

}
