#include "value.h"
#include "dummyscheme.h"
#include "env.h"
#include "tokenize.h"
#include "core.h"

using namespace DummyScheme;

DummyValuePtr DummyValue::nil(new DummyValue(DummyType::DUMMY_NIL, "nil"));
DummyValuePtr DummyValue::t(new DummyValue(DummyType::DUMMY_TRUE, "#t"));
DummyValuePtr DummyValue::f(new DummyValue(DummyType::DUMMY_FALSE, "#f"));

DummyValue::DummyValue(int num)
	:type(DUMMY_TYPE_INT_NUM)
{
	basic.intnum = num;
}

DummyValue::DummyValue(int type, const std::string &val)
	:type(type)
{	
	switch(type) {
	case DUMMY_TYPE_STRING:
	case DUMMY_TYPE_SYMBOL:
	case DUMMY_TYPE_NIL:
	case DUMMY_TYPE_TRUE:
	case DUMMY_TYPE_FALSE:
		strOrSymOrBind.push_back(val);	
		basic.intnum = 0;
		break;
	default:
		Error("wrong dummytype %d with %s", type, val.c_str());
		break;
	}
}

DummyValue::DummyValue(BindList binds, DummyValueList list)
	:type(DUMMY_TYPE_LAMBDA),
	 list(list),
	 strOrSymOrBind(binds)
{
	basic.intnum = 0;
}

DummyValue::DummyValue(const char* const typeStr, int type, DummyValueList list)
	:type(type),
	 list(list)
{
	basic.typeStr = typeStr;
}

DummyValue::DummyValue(DummyValueList list)
	:type(DUMMY_TYPE_LIST),
	 list(list)
{
	basic.typeStr = "list";
}

DummyValue::DummyValue(DummyValuePtr val)
	:type(val->type),
	basic(val->basic),
	list(val->list)
{
}

DummyValue::~DummyValue()
{
}

int DummyValue::getInt(DummyEnvPtr env)
{
	if (this->isInt())
		return this->basic.intnum;
	else if (this->isSymbol())
	{
		DummyValuePtr symbolValue = env->get(this->getSymbol());
		AssertDummyValue(symbolValue->isInt(), this);
		return symbolValue->getInt();
	}
	else
	{
		DummyValuePtr realItem = DummyCore::Eval(this, env);
		AssertDummyValue(realItem->isInt(), this);
		return realItem->getInt();
	}
}

std::string DummyValue::toString()
{
	std::stringstream out;		
	switch(type) {
	case DUMMY_TYPE_TYPE_INT_NUM:
		out << this->basic.intnum;
		break;
	case DUMMY_TYPE_TYPE_FLOAT_NUM:
		out << this->basic.floatnum;	
		break;
	case DUMMY_TYPE_TYPE_STRING:
		out << "\"" << strOrSymOrBind[0] << "\"";	
		break;
	case DUMMY_TYPE_TYPE_TRUE:	
	case DUMMY_TYPE_TYPE_FALSE:
	case DUMMY_TYPE_TYPE_NIL:
	case DUMMY_TYPE_TYPE_SYMBOL:
		out << strOrSymOrBind[0];
		break;
	case DUMMY_TYPE_TYPE_LAMBDA:{
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
		break;
	}
	case DUMMY_TYPE_TYPE_LIST:{
		out << "(";
		DummyValueList::iterator itr = list.begin();
		for (;itr != list.end(); itr++)
		{
			out << (*itr)->toString();
			if (itr + 1 != list.end())
				out << " ";	
		}
		out << ")";	
		break;
	}
	default:{
		out << "(" << basic.typeStr << " ";	
		DummyValueList::iterator itr = list.begin();	
		for (;itr != list.end(); itr++)
		{
			out << (*itr)->toString();
			if (itr + 1 != list.end())
				out << " ";	
		}
		out << ")";	
		break;
	}
	}

	return out.str();
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
	case DUMMY_TYPE_TYPE_INT_NUM:
		return this->basic.intnum == other->basic.intnum;
	case DUMMY_TYPE_TYPE_FLOAT_NUM:
		return isFloatEqual(this->basic.floatnum, other->basic.floatnum);
	case DUMMY_TYPE_TYPE_STRING:
		return 0 == this->strOrSymOrBind[0].compare(other->strOrSymOrBind[0]);
	case DUMMY_TYPE_TYPE_LIST:{
		DummyValueList a = this->list;
		DummyValueList b = other->list;
		
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
		return false;
	}
}


DummyValuePtr DummyValue::eval(DummyEnvPtr env)
{	
	switch(type) {
	case DUMMY_TYPE_TYPE_INT_NUM:
	case DUMMY_TYPE_TYPE_FLOAT_NUM:
	case DUMMY_TYPE_TYPE_STRING:
	case DUMMY_TYPE_TYPE_TRUE:
	case DUMMY_TYPE_TYPE_FALSE:
	case DUMMY_TYPE_TYPE_NIL:
	// the real lambda eval needs apply	
	// may be a returnvalue
	case DUMMY_TYPE_TYPE_LAMBDA: 
		return this;
	case DUMMY_TYPE_TYPE_SYMBOL:
		return env->get(strOrSymOrBind[0]);
	case DUMMY_TYPE_TYPE_QUOTE:
		// (quote abc)
		// (quote (1 2 3))
		return list.front();
	case DUMMY_TYPE_TYPE_UNQUOTE:{
		Error("cannot eval unquote separately");
		return DummyValue::nil;
	}
	case DUMMY_TYPE_TYPE_UNQUOTE_SPLICING:{
		Error("cannot eval unquote-splicing separately");
		return DummyValue::nil;
	}
	default:{
		DummyOpEval op = DummyBuiltInOp::builtInOps[type];
		Assert(op != null, "internal error of no opeval with type %d", type);
		return op(this, env);
		break;
	}
	}

	Error("unexpected type %d", type);
	
	return DummyValue::nil;
}

DummyValuePtr DummyValue::create(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	if (front->isSymbol())
	{
		std::string symbol = front->getSymbol();
		DummyOpCreate::iterator opCreateItr = DummyBuiltInOp::builtInOpsCreate.find(symbol);	
		if (opCreateItr != DummyBuiltInOp::builtInOpsCreate.end())
			return (*opCreateItr)(list);
		// (let ((c 2)) c)
		//	Error("unexpected type %d", type);	
	}
	else if (front->isLambda()) // ((lambda (a) (+ a 2)) 2)
		return DummyCore::OpConstructApply(list);

	return listValue(list);
}
