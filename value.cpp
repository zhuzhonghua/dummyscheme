#include "value.h"
#include "dummyscheme.h"
#include "env.h"
#include "tokenize.h"
#include "core.h"

using namespace DummyScheme;

const DummyValuePtr DummyValue::nil(new DummyValue(DUMMY_TYPE_NIL, STR_NIL));
const DummyValuePtr DummyValue::t(new DummyValue(DUMMY_TYPE_TRUE, STR_TRUE));
const DummyValuePtr DummyValue::f(new DummyValue(DUMMY_TYPE_FALSE, STR_FALSE));

DummyValue::DummyValue(int type, const String& val)
	:DummyValue(type)
{
	switch(type) {
	case DUMMY_TYPE_TRUE:
	case DUMMY_TYPE_FALSE:
	case DUMMY_TYPE_NIL:
		// do nothing
		break;
	default:{
		Error("unknown type of value %d", type);
		break;
	}
	}
}

DummyValue::DummyValue(int type)
	:type(type)
{
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

bool DummyValue::isEqualString(const String str)
{
	switch(type) {
	case DUMMY_TYPE_TRUE:
		return 0 == str.compare(STR_TRUE);
	case DUMMY_TYPE_FALSE:
		return 0 == str.compare(STR_FALSE);
	case DUMMY_TYPE_NIL:
		return 0 == str.compare(STR_NIL);
	default:{
		Error("unknown type of value %d", type);
		break;
	}
	}
	return false;
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

DummyOpTypeValue::DummyOpTypeValue(int type, DummyValueList list)
	:DummyValue(type),
	 list(list)
{
}

std::string DummyOpTypeValue::toString()
{
	std::stringstream out;
	out << "(" << DummyCore::GetTypeStr(type) << " ";	
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


/*
	(quasiquote lst)
	`lst
*/
DummyValuePtr OpEvalQuasiQuote(DummyValuePtr value, DummyEnvPtr env)
{
	// because unquote and unquote-splicing can only eval in quasiquote
	if (value->isUnQuote() || value->isUnQuoteSplicing())
	{
		// (define lst (quote b c))
		// (quasiquote ((unquote lst) d))
		// (quasiquote ((unquote (quote b c)) d))
		// (quasiquote (a (unquote-splicing lst)))
		DummyValuePtr quoteItem = DummyCore::Eval(value->getList().front(), env);
		
		// looks like (unquote 1) and (unquote-splicing ) is correct too
		// AssertDummyValue(quoteItem->isQuote(), value, "unquote can only eval on quote item");
		if (quoteItem->isQuote())
			return quoteItem->eval(env);
		else
			return quoteItem;
	}
	
	// recursive call
	// or only have one item
	if (!value->isList())
		return value;// don't eval at this place
	
	DummyValueList list = value->getList();
	DummyValueList::iterator itr = list.begin();
	DummyValueList retValue;
	for (; itr != list.end(); itr++)
	{
		DummyValuePtr item = *itr;
		// eval and put
		if (item->isUnQuote())
		{
			retValue.push_back(OpEvalQuasiQuote(item, env));
		}
		else if (item->isUnQuoteSplicing())
		{
			// (define lst (quote b c))
			// (quasiquote (a (unquote-splicing lst)))
			DummyValuePtr evalUnQuoteItem = OpEvalQuasiQuote(item, env);
			// eval and splice put
			if (evalUnQuoteItem->isList())
			{
				DummyValueList evalItrList = evalUnQuoteItem->getList();
				retValue.insert(retValue.end(), evalItrList.begin(), evalItrList.end());
			}
			else
				retValue.push_back(evalUnQuoteItem);
		}
		else			// just put
			retValue.push_back(OpEvalQuasiQuote(*itr, env));
	}

	return DummyCore::Compile(retValue);
}

DummyValuePtr DummyOpTypeValue::eval(DummyEnvPtr env)
{
	switch(type) {
	case DUMMY_TYPE_QUOTE:
		// (quote abc)
		// (quote (1 2 3))
		return this->getList().front();
	case DUMMY_TYPE_UNQUOTE:{
		Error("cannot eval unquote separately");
		return DummyValue::nil;
	}
	case DUMMY_TYPE_UNQUOTE_SPLICING:{
		Error("cannot eval unquote-splicing separately");
		return DummyValue::nil;
	}
	case DUMMY_TYPE_QUASIQUOTE:{
		return OpEvalQuasiQuote(this->getList().front(), env);
		break;
	}
	default:{
		return DummyCore::EvalOpType(this, env);
		break;
	}
	}
}

