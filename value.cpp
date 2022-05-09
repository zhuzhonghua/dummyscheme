#include "value.h"
#include "dummyscheme.h"
#include "env.h"
#include "tokenize.h"
#include "core.h"

using namespace DummyScheme;

DummyValuePtr DummyValue::nil(new DummyValue(DUMMY_TYPE_NIL));
DummyValuePtr DummyValue::t(new DummyValue(DUMMY_TYPE_TRUE));
DummyValuePtr DummyValue::f(new DummyValue(DUMMY_TYPE_FALSE));

DummyValue::DummyValue(int type)
	:type(type)
{
}

int DummyValue::getInt(DummyEnvPtr env)
{
	if (this->isInt())
		return this->getInt();
	else if (this->isSymbol())
	{
		DummyValuePtr symbolValue = env->get(this->getSymbol());
		return symbolValue->getInt();
	}
	else
	{
		DummyValuePtr realItem = DummyCore::Eval(this, env);
		return realItem->getInt();
	}
}

std::string DummyValue::toString()
{
	switch(type) {
	case DUMMY_TYPE_TRUE:
		return STR_TRUE;
	case DUMMY_TYPE_FALSE:
		return STR_FALSE;
	case DUMMY_TYPE_NIL:
		return STR_NIL;
	default:{
		Error("unknown type of value %d", type);
		break;
	}
	}
	return "";
}

/*
	(equal? a b)
 */
bool DummyValue::isEqualValue(DummyValuePtr other, DummyEnvPtr env)
{
	if (other == this)
		return true;
	
	if(this->type != other->type)
	{
		if ((this->isNil() || this->isFalse()) && (other->isNil() || other->isFalse()))
			return true;
		return false;
	}

	switch(this->type) {
	case DUMMY_TYPE_INT_NUM:
		return getInt() == other->getInt();
	case DUMMY_TYPE_FLOAT_NUM:
		return isFloatEqual(this->getDouble(), other->getDouble());
	case DUMMY_TYPE_STRING:
		return 0 == this->getStr().compare(other->getStr());
	case DUMMY_TYPE_SYMBOL:
		return env->get(this->getSymbol())->isEqualValue(env->get(other->getSymbol()), env);
	case DUMMY_TYPE_LIST:{
		DummyValueList a = this->getList();
		DummyValueList b = other->getList();
		
		if (a.size() != b.size())
			return false;
		
		DummyValueList::iterator aItr = a.begin();
		DummyValueList::iterator bItr = b.begin();
		for (; aItr != a.end() && bItr != b.end(); ++aItr, ++bItr)
		{
			DummyValuePtr aEvalValue = DummyCore::Eval(*aItr, env);
			DummyValuePtr bEvalValue = DummyCore::Eval(*bItr, env);
			
			if (!aEvalValue->isEqualValue(bEvalValue, env))
				return false;
		}
		return true;
	}
	default:
		// other case don't support compare
		return false;
	}
}

DummyValuePtr DummyValue::eval(DummyEnvPtr env)
{	
	switch(type) {
	case DUMMY_TYPE_INT_NUM:
	case DUMMY_TYPE_FLOAT_NUM:
	case DUMMY_TYPE_STRING:
	case DUMMY_TYPE_TRUE:
	case DUMMY_TYPE_FALSE:
	case DUMMY_TYPE_NIL:
	case DUMMY_TYPE_LAMBDA:// the real lambda eval needs apply, may be a returnvalue
		return this;
	case DUMMY_TYPE_LIST:
		Error("cannot eval a list");
		return DummyValue::nil;
	default:
		Error("unknown type to eval %d", type);
		break;
	}
	
	return DummyValue::nil;
}

DummyValuePtr DummyValue::create(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	if (front->isSymbol())
	{
		std::string symbol = front->getSymbol();
		std::map<std::string, DummyOpCreate>::iterator opCreateItr = DummyBuiltInOp::builtInOpsCreate.find(symbol);	
		if (opCreateItr != DummyBuiltInOp::builtInOpsCreate.end())
			return (opCreateItr->second)(list);
		// (let ((c 2)) c)
		//	Error("unexpected type %d", type);	
	}
	else if (front->isLambda()) // ((lambda (a) (+ a 2)) 2)
		return opTypeValue("apply", DUMMY_TYPE_APPLY, list);

	return listValue(list);
}

DummyNumValue::DummyNumValue(int num)
	:DummyValue(DUMMY_TYPE_INT_NUM)
{
	basic.intnum = num;
}

DummyNumValue::DummyNumValue(double num)
	:DummyValue(DUMMY_TYPE_FLOAT_NUM)
{
	basic.floatnum = num;
}

std::string DummyNumValue::toString()
{
	std::stringstream out;		
	switch(type) {
	case DUMMY_TYPE_INT_NUM:
		out << this->basic.intnum;
		break;
	case DUMMY_TYPE_FLOAT_NUM:
		out << this->basic.floatnum;	
		break;
	}
	return out.str();
}

DummyStringValue::DummyStringValue(const std::string &val)
	:DummyValue(DUMMY_TYPE_STRING),
	 str(val)
{
}

std::string DummyStringValue::toString()
{
	return "\"" + str + "\"";
}

DummySymbolValue::DummySymbolValue(const std::string &val)
	:DummyValue(DUMMY_TYPE_SYMBOL),
	 symbol(val)
{
}

std::string DummySymbolValue::toString()
{
	return symbol;
}

DummyValuePtr DummySymbolValue::eval(DummyEnvPtr env)
{
	return env->get(symbol);
}

DummyListValue::DummyListValue(DummyValueList list)
	:DummyValue(DUMMY_TYPE_LIST),
	 list(list)
{
}

std::string DummyListValue::toString()
{
	std::stringstream out;
	out << "(";
	DummyValueList::iterator itr = list.begin();
	for (;itr != list.end(); itr++)
	{
		out << (*itr)->toString();
		if (itr + 1 != list.end())
			out << " ";
	}
	out << ")";	

	return out.str();
}

DummyLambdaValue::DummyLambdaValue(BindList binds, DummyValueList list)
	:DummyValue(DUMMY_TYPE_LAMBDA),
	 list(list),
	 binds(binds)
{
}

std::string DummyLambdaValue::toString()
{
	std::stringstream out;
	out << "#<function>";
	out << " [";
	BindList binds = getBind();
	BindList::iterator bindItr = binds.begin();
	for (; bindItr != binds.end(); bindItr++)
	{
		out << (*bindItr);
		if (bindItr + 1 != binds.end())
			out << " ";	
	}
	out << "] ";
	DummyValueList::iterator itr = list.begin();	
	for (;itr != list.end(); itr++)
	{
		out << (*itr)->toString();
		if (itr + 1 != list.end())
			out << " ";
	}

	return out.str();
}

DummyOpTypeValue::DummyOpTypeValue(const char* const typeStr, int type, DummyValueList list)
	:DummyValue(type),
	 list(list),
	 typeStr(typeStr)
{
}

std::string DummyOpTypeValue::toString()
{
	std::stringstream out;
	out << "(" << typeStr << " ";	
	DummyValueList::iterator itr = list.begin();	
	for (;itr != list.end(); itr++)
	{
		out << (*itr)->toString();
		if (itr + 1 != list.end())
			out << " ";
	}
	out << ")";
	return out.str();
}

DummyValuePtr DummyOpTypeValue::eval(DummyEnvPtr env)
{
	switch(type) {
	case DUMMY_TYPE_QUOTE:
		// (quote abc)
		// (quote (1 2 3))
		return list.front();
	case DUMMY_TYPE_UNQUOTE:{
		Error("cannot eval unquote separately");
		return DummyValue::nil;
	}
	case DUMMY_TYPE_UNQUOTE_SPLICING:{
		Error("cannot eval unquote-splicing separately");
		return DummyValue::nil;
	}
	default:{
		DummyOpEval op = DummyBuiltInOp::builtInOps[type];
		Assert(op != NULL, "internal error of no opeval with type %d", type);
		return op(this, env);
		break;
	}
	}
}

