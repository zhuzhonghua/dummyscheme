#include "value.h"
#include "dummyscheme.h"
#include "env.h"
#include "tokenize.h"

DummyValuePtr DummyValue::nil(new DummyValue(DummyType::DUMMY_NIL, std::string("nil")));
DummyValuePtr DummyValue::t(new DummyValue(DummyType::DUMMY_TRUE, std::string("t")));

DummyValue::DummyValue(int num)
	:type(DummyType::DUMMY_INT_NUM)
{
	basic.intnum = num;
}

DummyValue::DummyValue(DummyType type, const std::string& val)
	:type(type)
{	
	switch(type) {
	case DummyType::DUMMY_STRING:
	case DummyType::DUMMY_SYMBOL:
	case DummyType::DUMMY_NIL:
	case DummyType::DUMMY_TRUE:
		strAndSymbol = val;	
		break;
	default:
		Error("wrong dummytype %d with %s", type, val.c_str());
		break;
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
	DummyType type = getType();
	switch(type) {
	case DummyType::DUMMY_SYMBOL:
		return env->get(this->getSymbol());	
		break;
	case DummyType::DUMMY_INT_NUM:
	case DummyType::DUMMY_FLOAT_NUM:
	case DummyType::DUMMY_STRING:
	case DummyType::DUMMY_NIL:
	case DummyType::DUMMY_TRUE:
		return DummyValuePtr(this);
		break;
	case DummyType::DUMMY_LIST:{
		DummyValueList list = getList();
		if (list.empty()) {
			return DummyValuePtr();
		}
		DummyValuePtr symbol = list.front();
		OpMap::iterator it = Tokenize::opMap.find(symbol->getSymbol());
		if (it == Tokenize::opMap.end())
		{
			Error("didn't define symbol=%s", symbol->getSymbol().c_str());
			return DummyValuePtr();
		}

		OpFunc op = it->second;
		// this list contains symbol value as the first
		return op(DummyValuePtr(this), env);	
		break;
	}
	default:
		Error("unknown dummytype %d", type);	
		return DummyValuePtr();
	}
}

std::string DummyValue::toString()
{
	std::stringstream out;
	switch(this->type) {
	case DUMMY_INT_NUM:
		out << this->basic.intnum;
		break;
	case DUMMY_FLOAT_NUM:
		out << this->basic.floatnum;
		break;
	case DUMMY_SYMBOL:
	case DUMMY_NIL:
	case DUMMY_TRUE:
		out << this->strAndSymbol;	
		break;
	case DUMMY_STRING:
		out << "\"" << this->strAndSymbol << "\"";
		break;
	case DUMMY_LIST:{
		out << "(";
		DummyValueList::iterator itr = this->list.begin();	
		for (;itr != this->list.end(); itr++) {
			out << (*itr)->toString();
			if (itr + 1 != this->list.end()) {
				out << " ";	
			}
		}
		out << ")";
		break;
	}
	default:
		Error("unknown type %d", this->type);
		break;
	}	

	return out.str();
}
