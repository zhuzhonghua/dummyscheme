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
		return this->basic.intnum;
	} else if (this->isSymbol()) {
		DummyValuePtr symbolValue = env->get(this->getSymbol());
		return symbolValue->getInt();
	} else {
		DummyValuePtr realItem = this->eval(env);
		return realItem->getInt();	
	}
}

#define CaseReturnEval(type, op, value, env) case type: return DummyCore:: op(DummyValuePtr(value), env)

DummyValuePtr DummyValue::eval(DummyEnvPtr env)
{
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
	CaseReturnEval(DummyType::DUMMY_PLUS, OpEvalPlus, this, env);
	CaseReturnEval(DummyType::DUMMY_MINUS, OpEvalMinus, this, env);
	CaseReturnEval(DummyType::DUMMY_MUL, OpEvalMul, this, env);
	CaseReturnEval(DummyType::DUMMY_DIVIDE, OpEvalDivide, this, env);
	CaseReturnEval(DummyType::DUMMY_DEFINE, OpEvalDefine, this, env);
	CaseReturnEval(DummyType::DUMMY_LET, OpEvalLet, this, env);
	CaseReturnEval(DummyType::DUMMY_BEGIN, OpEvalBegin, this, env);
	CaseReturnEval(DummyType::DUMMY_IF, OpEvalIf, this, env);
	CaseReturnEval(DummyType::DUMMY_WHEN, OpEvalWhen, this, env);
	CaseReturnEval(DummyType::DUMMY_UNLESS, OpEvalUnless, this, env);
	CaseReturnEval(DummyType::DUMMY_APPLY, OpEvalApply, this, env);
	CaseReturnEval(DummyType::DUMMY_DISPLAY, OpEvalDisplay, this, env);
	CaseReturnEval(DummyType::DUMMY_LIST, OpEvalList, this, env);
	}

	Error("unexpected type %d", type);
	
	return DummyValue::nil;
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
		out << "(" << getTypeStr(type) << " ";	
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

#define CaseReturn(type, str)	case type: return str

std::string DummyValue::getTypeStr(int type)
{
	switch(type) {
	CaseReturn(DummyType::DUMMY_PLUS, "+");
	CaseReturn(DummyType::DUMMY_MINUS, "-");
	CaseReturn(DummyType::DUMMY_MUL, "*");
	CaseReturn(DummyType::DUMMY_DIVIDE, "/");
	CaseReturn(DummyType::DUMMY_DEFINE, "define");
	CaseReturn(DummyType::DUMMY_LET, "let");
	CaseReturn(DummyType::DUMMY_BEGIN, "begin");
	CaseReturn(DummyType::DUMMY_IF, "if");
	CaseReturn(DummyType::DUMMY_WHEN, "when");
	CaseReturn(DummyType::DUMMY_UNLESS, "unless");
	CaseReturn(DummyType::DUMMY_LAMBDA, "lambda");
	CaseReturn(DummyType::DUMMY_APPLY, "apply");
	CaseReturn(DummyType::DUMMY_DISPLAY, "display");
	CaseReturn(DummyType::DUMMY_LIST, "list");
	}
	Error("unexpected type %d", type);
	return "";	
}

#define CompareReturn(symbol, str, type) if (0 == symbol.compare(str)) return type

DummyType DummyValue::getStrType(const std::string& symbol)
{
	CompareReturn(symbol, "+", DummyType::DUMMY_PLUS);
	CompareReturn(symbol, "-", DummyType::DUMMY_MINUS);
	CompareReturn(symbol, "*", DummyType::DUMMY_MUL);
	CompareReturn(symbol, "/", DummyType::DUMMY_DIVIDE);
	CompareReturn(symbol, "define", DummyType::DUMMY_DEFINE);
	CompareReturn(symbol, "let", DummyType::DUMMY_LET);
	CompareReturn(symbol, "begin", DummyType::DUMMY_BEGIN);
	CompareReturn(symbol, "if", DummyType::DUMMY_IF);
	CompareReturn(symbol, "when", DummyType::DUMMY_WHEN);
	CompareReturn(symbol, "unless", DummyType::DUMMY_UNLESS);
	CompareReturn(symbol, "lambda", DummyType::DUMMY_LAMBDA);
	CompareReturn(symbol, "apply", DummyType::DUMMY_APPLY);
	CompareReturn(symbol, "display", DummyType::DUMMY_DISPLAY);
	CompareReturn(symbol, "list", DummyType::DUMMY_LIST);
	
	return DummyType::DUMMY_MAX;
}

#define CaseReturnValue(type, op, list) case type: return DummyCore:: op(list);

#define CaseReturnValueTypeList(type, list, num) \
case type: {return DummyCore::OpConstructTypeList(type, list, num);}

DummyValuePtr DummyValue::create(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	if (front->isSymbol()) {
		std::string symbol = front->getSymbol();	
		DummyType type = getStrType(symbol);
		switch(type) {
		CaseReturnValueTypeList(DummyType::DUMMY_PLUS, DummyValueList(list.begin()+1, list.end()), 1);
		CaseReturnValueTypeList(DummyType::DUMMY_MINUS, DummyValueList(list.begin()+1, list.end()), 2);
		CaseReturnValueTypeList(DummyType::DUMMY_MUL, DummyValueList(list.begin()+1, list.end()), 2);
		CaseReturnValueTypeList(DummyType::DUMMY_DIVIDE, DummyValueList(list.begin()+1, list.end()), 2);
		CaseReturnValueTypeList(DummyType::DUMMY_BEGIN, DummyValueList(list.begin()+1, list.end()), 1);	
		CaseReturnValueTypeList(DummyType::DUMMY_IF, DummyValueList(list.begin()+1, list.end()), 2);	
		CaseReturnValueTypeList(DummyType::DUMMY_WHEN, DummyValueList(list.begin()+1, list.end()), 2);
		CaseReturnValueTypeList(DummyType::DUMMY_UNLESS, DummyValueList(list.begin()+1, list.end()), 2);	
		CaseReturnValueTypeList(DummyType::DUMMY_DISPLAY, DummyValueList(list.begin()+1, list.end()), 1);
		CaseReturnValueTypeList(DummyType::DUMMY_LIST, DummyValueList(list.begin()+1, list.end()), 0);	
		
		CaseReturnValue(DummyType::DUMMY_DEFINE, OpConstructDefine, list);
		CaseReturnValue(DummyType::DUMMY_LET, OpConstructLet, list);
		CaseReturnValue(DummyType::DUMMY_LAMBDA, OpConstructLambda, list);
		CaseReturnValue(DummyType::DUMMY_APPLY, OpConstructApply, list);

		}
		
		// (let ((c 2)) c)
		//	Error("unexpected type %d", type);	
	} else if (front->isLambda()) {
		// ((lambda (a) (+ a 2)) 2)
		return DummyCore::OpConstructApply(list);
	}

	return DummyValuePtr(new DummyValue(list));
}
