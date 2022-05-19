#include "dummyscheme.h"
#include "value.h"
#include "env.h"

namespace DummyScheme {

/*
	for convenience
 */
DummyValuePtr opTypeValue(int type, const DummyValuePtr &value)
{
	DummyValueList list;
	list.push_back(value);
	return new DummyOpTypeValue(type, list);
}

DummyValuePtr opTypeValue(int type, const DummyValueList& list)
{
	return new DummyOpTypeValue(type, list);
}

DummyValuePtr opTypeValue(int type, DummyValueListItr begin, DummyValueListItr end)
{
	return opTypeValue(type, DummyValueList(begin, end));	
}

DummyValuePtr listValue(const DummyValueList& list)
{
	return new DummyListValue(list);
}

DummyValuePtr listValue(DummyValueListItr begin, DummyValueListItr end)
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
	return new DummyLambdaValue(binds, list, false);
}

DummyValuePtr lambdaValue(const BindList& binds, DummyValueListItr begin, DummyValueListItr end)
{
	return lambdaValue(binds, DummyValueList(begin, end));
}

DummyValuePtr macroValue(const BindList& binds, const DummyValuePtr &item)
{
	DummyValueList list;
	list.push_back(item);
	return new DummyLambdaValue(binds, list, true);
}

DummyValuePtr macroValue(const BindList& binds, DummyValueListItr begin, DummyValueListItr end)
{
	return new DummyLambdaValue(binds, DummyValueList(begin, end), true);
}

}
