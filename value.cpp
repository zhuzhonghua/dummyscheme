#include "value.h"
#include "dummyscheme.h"
#include "env.h"
#include "tokenize.h"

using namespace DummyScheme;

DummyValuePtr DummyValue::nil(new DummyValue(DummyType::DUMMY_NIL, "nil"));
DummyValuePtr DummyValue::t(new DummyValue(DummyType::DUMMY_TRUE, "t"));
std::string DummyValue::apply("apply");
std::string DummyValue::lambda("lambda");

DummyValue::DummyValue(int num)
	:type(DummyType::DUMMY_INT_NUM)
{
	basic.intnum = num;
}

DummyValue::DummyValue(DummyType type, const std::string &val)
	:type(type)
{	
	switch(type) {
	case DummyType::DUMMY_STRING:
	case DummyType::DUMMY_SYMBOL:
	case DummyType::DUMMY_NIL:
	case DummyType::DUMMY_TRUE:
		strOrSymOrBind.push_back(val);	
		break;
	default:
		Error("wrong dummytype %d with %s", type, val.c_str());
		break;
	}
}

DummyValue::DummyValue(BindList binds, DummyValueList list)
	:type(DummyType::DUMMY_LAMBDA),
	 list(list),
	 strOrSymOrBind(binds)
{

}

DummyValue::DummyValue(DummyType type, DummyValueList list)
	:type(type),
	 list(list)
{
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
	return valueEval(this, env);
}

std::string DummyValue::toString()
{
	return valueToString(this);
}
