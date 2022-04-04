#include "value.h"
#include "dummyscheme.h"

DummyValue::DummyValue(int num)
	:type(DummyType::DUMMY_INT_NUM)
{
	basic.intnum = num;
}

DummyValue::DummyValue(DummyType type, std::string val)
	:type(type)
{	
	if (type == DummyType::DUMMY_STRING ||
			type == DummyType::DUMMY_SYMBOL) {
		strAndSymbol = val;
	}	else {
		Error("wrong dummytype %d", type);
	}
}

DummyValue::DummyValue(DummyValueList list)
	:type(DummyType::DUMMY_LIST),
	 list(list)
{
}

DummyValue::DummyValue(DummyValuePtr val)
	:type(val->type),
	basic(val->basic),
	list(val->list)
{
}

DummyValue::~DummyValue()
{
	// only check list type
	if (type == DummyType::DUMMY_LIST) {
//		for (DummyValueList::iterator itr = list.begin();
//				 itr != list.end(); ++itr) {
//			delete *itr;
//		}
	}
}

