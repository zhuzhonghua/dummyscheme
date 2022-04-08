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

DummyValue::DummyValue(DummyValuePtr binds, DummyValueList::iterator begin, DummyValueList::iterator end)
	:type(DummyType::DUMMY_LAMBDA)
{
	Assert(binds->isList(), "lambda binds must list %s", binds->toString().c_str());
	
	DummyValueList bindsList = binds->getList();
	DummyValueList::iterator itr = bindsList.begin();
	for(; itr != bindsList.end(); itr++) {
		DummyValuePtr symbol = *itr;
		Assert(symbol->isSymbol(), "lambda binds must be symbol %s", symbol->toString().c_str());
		strOrSymOrBind.push_back(symbol->getSymbol());
	}

	for (; begin != end; ++begin) {
		list.push_back(*begin);
	}

	//list = DummyValueList(begin, end);
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
			return DummyValue::nil;
		}
		DummyValuePtr front = list.front();
		std::string symbolStr;
		if (front->isList()) {
			// TODO: check front must be lambda
			symbolStr = DummyValue::apply;
		} else {
			symbolStr = front->getSymbol();
		}

		OpFunc op = getOpFunc(symbolStr);
		// this list contains symbol value as the first
		return op(DummyValuePtr(this), env);	
		// code down is error
		// this be changed
//		// the first might be a lambda
//		if (front->isSymbol() && 0 == front->getSymbol().compare(DummyValue::lambda)) {
//			return getOpFunc(DummyValue::lambda)(DummyValuePtr(this), env);
//		} else {
//			DummyValuePtr symbol = front->eval(env);
//			Print("func symbol %s\n", symbol->toString().c_str());
//			std::string symbolStr;
//			if (symbol->isLambda()) {
//				symbolStr = DummyValue::apply;
//				list[0] = symbol;
//			} else {
//				symbolStr = symbol->getSymbol();
//			}
//
//			OpFunc op = getOpFunc(symbolStr);
//			// this list contains symbol value as the first
//			return op(DummyValuePtr(this), env);	
		break;
	}
	default:
		Error("unknown dummytype %d", type);	
		return DummyValue::nil;
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
		out << this->strOrSymOrBind[0];
		break;
	case DUMMY_STRING:
		out << "\"" << this->strOrSymOrBind[0] << "\"";
		break;
	case DUMMY_LAMBDA:{
		out << "#<function>";
		out << " [";
		BindList::iterator bindItr = strOrSymOrBind.begin();
		for (; bindItr != strOrSymOrBind.end(); bindItr++) {
			out << (*bindItr);
			if (bindItr + 1 != strOrSymOrBind.end()) {
				out << " ";	
			}
		}
		out << "] ";
		DummyValueList::iterator itr = this->list.begin();	
		for (;itr != this->list.end(); itr++) {
			out << (*itr)->toString();
			if (itr + 1 != this->list.end()) {
				out << " ";	
			}
		}
		break;	
	}
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
