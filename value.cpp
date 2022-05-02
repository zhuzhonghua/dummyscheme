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
	case DummyType::DUMMY_FALSE:
		strOrSymOrBind.push_back(val);	
		basic.intnum = 0;
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
	basic.intnum = 0;
}

DummyValue::DummyValue(const char* typeStr, DummyType type, DummyValueList list)
	:type(type),
	 list(list)
{
	basic.typeStr = typeStr;
}

DummyValue::DummyValue(DummyValueList list)
	:type(DummyType::DUMMY_LIST),
	 list(list)
{
	basic.intnum = 0;	
}

DummyValue::DummyValue(DummyValuePtr val)
	:type(val->type),
	basic(val->basic),
	list(val->list)
{
}

DummyValue::~DummyValue()
{
	list.clear();
}

int DummyValue::getInt(DummyEnvPtr env)
{
	if (this->isInt()) {
		return this->basic.intnum;
	} else if (this->isSymbol()) {
		DummyValuePtr symbolValue = env->get(this->getSymbol());
		return symbolValue->getInt();
	} else {
		DummyValuePtr realItem = this->eval(env);
		return realItem->getInt();	
	}
}

std::string DummyValue::toString()
{
	std::stringstream out;		
	switch(type) {
	case DummyType::DUMMY_INT_NUM:
		out << this->basic.intnum;
		break;
	case DummyType::DUMMY_FLOAT_NUM:
		out << this->basic.floatnum;	
		break;
	case DummyType::DUMMY_STRING:
		out << "\"" << strOrSymOrBind[0] << "\"";	
		break;
	case DummyType::DUMMY_TRUE:	
	case DummyType::DUMMY_FALSE:
	case DummyType::DUMMY_NIL:
	case DummyType::DUMMY_SYMBOL:
		out << strOrSymOrBind[0];
		break;
	case DummyType::DUMMY_LAMBDA:{
		out << "#<function>";
		out << " [";
		BindList binds = getBind();
		BindList::iterator bindItr = binds.begin();
		for (; bindItr != binds.end(); bindItr++) {
			out << (*bindItr);
			if (bindItr + 1 != binds.end()) {
				out << " ";	
			}
		}
		out << "] ";
		DummyValueList::iterator itr = list.begin();	
		for (;itr != list.end(); itr++) {
			out << (*itr)->toString();
			if (itr + 1 != list.end()) {
				out << " ";	
			}
		}
		break;
	}
	case DummyType::DUMMY_LIST:{
		out << "(";
		DummyValueList::iterator itr = list.begin();
		for (;itr != list.end(); itr++) {
			out << (*itr)->toString();
			if (itr + 1 != list.end()) {
				out << " ";	
			}
		}
		out << ")";	
		break;
	}
	default:{
		out << "(" << basic.typeStr << " ";	
		DummyValueList::iterator itr = list.begin();	
		for (;itr != list.end(); itr++) {
			out << (*itr)->toString();
			if (itr + 1 != list.end()) {
				out << " ";	
			}
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
	if (other == this) {
		return true;
	}
	
	if(this->type != other->type) {
		if ((this->isNil() || this->isFalse()) && (other->isNil() || other->isFalse())) {
			return true;
		}
		return false;
	}

	switch(this->type) {
	case DummyType::DUMMY_INT_NUM:
		return this->basic.intnum == other->basic.intnum;
	case DummyType::DUMMY_FLOAT_NUM:
		return isFloatEqual(this->basic.floatnum, other->basic.floatnum);
	case DummyType::DUMMY_STRING:
		return 0 == this->strOrSymOrBind[0].compare(other->strOrSymOrBind[0]);
	case DummyType::DUMMY_LIST:{
		DummyValueList a = this->list;
		DummyValueList b = other->list;
		if (a.size() != b.size()) {
			return false;
		}
		DummyValueList::iterator aItr = a.begin();
		DummyValueList::iterator bItr = b.begin();
		for (; aItr != a.end() && bItr != b.end(); ++aItr, ++bItr) {
			DummyValuePtr aEvalValue = (*aItr)->eval(env);
			DummyValuePtr bEvalValue = (*bItr)->eval(env);
			if (!aEvalValue->isEqualValue(bEvalValue, env)) {
				return false;
			}
		}
		return true;
	}
	default:
		return false;
	}
}


DummyValuePtr DummyValue::eval(DummyEnvPtr env)
{
#define CaseReturnEval(type, op) case type: return DummyCore:: op(DummyValuePtr(this), env)
	
	switch(type) {
	case DummyType::DUMMY_INT_NUM:
	case DummyType::DUMMY_FLOAT_NUM:
	case DummyType::DUMMY_STRING:
	case DummyType::DUMMY_TRUE:
	case DummyType::DUMMY_FALSE:
	case DummyType::DUMMY_NIL:
	// the real lambda eval needs apply	
	// may be a returnvalue
	case DummyType::DUMMY_LAMBDA: 
		return DummyValuePtr(this);
	case DummyType::DUMMY_SYMBOL:{
		return env->get(strOrSymOrBind[0]);
	}
	case DummyType::DUMMY_QUOTE:
		// (quote abc)
		// (quote (1 2 3))
		return list.front();
		
	CaseReturnEval(DummyType::DUMMY_PLUS, OpEvalPlus);
	CaseReturnEval(DummyType::DUMMY_MINUS, OpEvalMinus);
	CaseReturnEval(DummyType::DUMMY_MUL, OpEvalMul);
	CaseReturnEval(DummyType::DUMMY_DIVIDE, OpEvalDivide);
	CaseReturnEval(DummyType::DUMMY_DEFINE, OpEvalDefine);
	CaseReturnEval(DummyType::DUMMY_APPLY, OpEvalApply);
	CaseReturnEval(DummyType::DUMMY_DISPLAY, OpEvalDisplay);
	CaseReturnEval(DummyType::DUMMY_LIST, OpEvalList);
	CaseReturnEval(DummyType::DUMMY_LIST_MARK, OpEvalListMark);
	CaseReturnEval(DummyType::DUMMY_NULL_MARK, OpEvalNullMark);
	CaseReturnEval(DummyType::DUMMY_EQUAL_MARK, OpEvalEqualMark);
	CaseReturnEval(DummyType::DUMMY_EQUAL, OpEvalEqual);
	CaseReturnEval(DummyType::DUMMY_NOT, OpEvalNot);
	CaseReturnEval(DummyType::DUMMY_LESS, OpEvalLess);
	CaseReturnEval(DummyType::DUMMY_LESS_EQUAL, OpEvalLessEqual);
	CaseReturnEval(DummyType::DUMMY_BIG, OpEvalBig);
	CaseReturnEval(DummyType::DUMMY_BIG_EQUAL, OpEvalBigEqual);
	CaseReturnEval(DummyType::DUMMY_LENGTH, OpEvalLength);
	CaseReturnEval(DummyType::DUMMY_LOAD, OpEvalLoad);
	}

	Error("unexpected type %d", type);
	
	return DummyValue::nil;
}

DummyValuePtr DummyValue::create(DummyValueList& list)
{
#define CompareReturn(str, type, num)																		\
	if (0 == symbol.compare(str))																					\
		return DummyCore::OpConstructTypeList(str, type, DummyValueList(list.begin()+1, list.end()), num); 
	
#define CompareReturnValue(str, op) \
	if (0 == symbol.compare(str)) return DummyCore:: op(list);

	DummyValuePtr front = list.front();
	if (front->isSymbol()) {
		std::string symbol = front->getSymbol();
		
		CompareReturnValue("define", OpConstructDefine);
		CompareReturnValue("let", OpConstructLet);
		CompareReturnValue("lambda", OpConstructLambda);
		CompareReturnValue("apply", OpConstructApply);
		
		CompareReturn("+", DummyType::DUMMY_PLUS, 1);
		CompareReturn("-", DummyType::DUMMY_MINUS, 2);
		CompareReturn("*", DummyType::DUMMY_MUL, 2);
		CompareReturn("/", DummyType::DUMMY_DIVIDE, 2);
		CompareReturn("let", DummyType::DUMMY_LET, 2);
		CompareReturn("begin", DummyType::DUMMY_BEGIN, 1);
		CompareReturn("if", DummyType::DUMMY_IF, 2);
		CompareReturn("when", DummyType::DUMMY_WHEN, 2);
		CompareReturn("unless", DummyType::DUMMY_UNLESS, 2);
		CompareReturn("display", DummyType::DUMMY_DISPLAY, 1);
		CompareReturn("list", DummyType::DUMMY_LIST, 0);
		CompareReturn("list?", DummyType::DUMMY_LIST_MARK, 1);
		CompareReturn("null?", DummyType::DUMMY_NULL_MARK, 1);
		CompareReturn("equal?", DummyType::DUMMY_EQUAL_MARK, 2);
		CompareReturn("length", DummyType::DUMMY_LENGTH, 1);
		CompareReturn("=", DummyType::DUMMY_EQUAL, 2);
		CompareReturn("not", DummyType::DUMMY_NOT, 1);
		CompareReturn("<", DummyType::DUMMY_LESS, 2);
		CompareReturn("<=", DummyType::DUMMY_LESS_EQUAL, 2);
		CompareReturn(">", DummyType::DUMMY_BIG, 2);
		CompareReturn(">=", DummyType::DUMMY_BIG_EQUAL, 2);
		CompareReturn("load", DummyType::DUMMY_LOAD, 1);
		CompareReturn("quote", DummyType::DUMMY_QUOTE, 1);
		
		// (let ((c 2)) c)
		//	Error("unexpected type %d", type);	
	} else if (front->isLambda()) {
		// ((lambda (a) (+ a 2)) 2)
		return DummyCore::OpConstructApply(list);
	}

	return DummyValuePtr(new DummyValue(list));
}
