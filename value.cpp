#include "value.h"
#include "dummyscheme.h"
#include "env.h"
#include "tokenize.h"

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

int DummyValue::getInt(DummyEnvPtr env)
{
	if (this->isInt()) {
		return this->getInt();
	} else if (this->isSymbol()) {
		DummyValuePtr symbolValue = env->get(this->getSymbol());
		if (symbolValue->isInt()) {
			return symbolValue->getInt();
		} else {
				Error("currently don't support type %d", symbolValue->getType());
		}
	} else if (this->isList()) {
		DummyValuePtr realItem = this->eval(env);
			// TODO: check type
		return realItem->getInt();	
	} else {
		Error("unknown type %d", this->getType());	
	}

	return 0;
}

DummyValuePtr DummyValue::eval(DummyEnvPtr env)
{
	if (getType() != DummyType::DUMMY_LIST) {
		return DummyValuePtr(this);
	} else {
		DummyValueList list = getList();
		if (list.empty()) {
			return DummyValuePtr();
		}
		DummyValuePtr symbol = list.front();
		OpMap::iterator it = Tokenize::opMap.find(symbol->getSymbol());
		if (it == Tokenize::opMap.end())
		{
			Error("symbol error %s", symbol->getSymbol().c_str());
			return DummyValuePtr();
		}

		OpFunc op = it->second;
		// this list contains symbol value as the first
		return op(DummyValuePtr(this), env);
	}
}
