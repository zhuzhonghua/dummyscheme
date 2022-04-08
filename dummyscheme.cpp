#include "dummyscheme.h"
#include "value.h"
#include "env.h"

namespace DummyScheme {

OpMap opMap;
OpFunc getOpFunc(const std::string& symbol)
{
	OpMap::iterator it = opMap.find(symbol);
	if (it == opMap.end())
	{
		Error("didn't define symbol=%s", symbol.c_str());
		return NULL;
	}

	return it->second;
}

void addOpFunc(const std::string& symbol, OpFunc func)
{
	opMap[symbol] = func;
}

DummyValuePtr OpFuncPlus(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpFuncMinus(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpFuncMul(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpFuncDivide(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpFuncDefine(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpFuncLet(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpFuncBegin(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpFuncIf(DummyValuePtr value, DummyEnvPtr env);
	
void init()
{
	opMap["+"] = OpFuncPlus;
	opMap["-"] = OpFuncMinus;
	opMap["*"] = OpFuncMul;
	opMap["/"] = OpFuncDivide;
	opMap["define"] = OpFuncDefine;
	opMap["let"] = OpFuncLet;
	opMap["begin"] = OpFuncBegin;
	opMap["if"] = OpFuncIf;
}

/*
	(+ 1 2 3)
	TODO: check parameter length the first pass	
*/
DummyValuePtr OpFuncPlus(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 
	
	int num = 0;
	DummyValueList::iterator itr = list.begin();	
	// first is the symbol
	for (itr++; itr != list.end(); ++itr)
	{
		DummyValuePtr item = *itr;	
		num += item->getInt(env);
	}
	return DummyValuePtr(new DummyValue(num));
}

/*
	(- 1 2 3)
*/
DummyValuePtr OpFuncMinus(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 
	
	// first is the symbol	
	DummyValueList::iterator itr = list.begin();	
	// *itr = "-"
	// second is the eeeee
	++itr;
	int num = (*itr)->getInt();
	
	for (++itr; itr != list.end(); ++itr) {
		DummyValuePtr item = *itr;	
		num -= item->getInt(env);
	}
	return DummyValuePtr(new DummyValue(num));
}

/*
	(* 1 2 3)
*/
DummyValuePtr OpFuncMul(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 

	int num = 1;
	// first is the symbol	
	DummyValueList::iterator itr = list.begin();	
	
	for (itr++; itr != list.end(); ++itr) {
		DummyValuePtr item = *itr;	
		num *= item->getInt(env);
	}
	return DummyValuePtr(new DummyValue(num));
}

/*
	(/ 1 2 3)
*/
DummyValuePtr OpFuncDivide(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 
	
	// first is the symbol	
	DummyValueList::iterator itr = list.begin();	
	// second is the eeeee
	int num = (*(++itr))->getInt();
	
	for (++itr; itr != list.end(); ++itr) {
		DummyValuePtr item = *itr;	
		num /= item->getInt(env);
	}
	return DummyValuePtr(new DummyValue(num));
}

/*
	(define a 2)
	(define b (+ a 3))
 */
DummyValuePtr OpFuncDefine(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 
	
	// first is the symbol	
	DummyValueList::iterator itr = list.begin();	
	// second is the symbol
	DummyValuePtr second = *(++itr);	
	if (second->isSymbol()) {
		DummyValuePtr symbolValue = (*(++itr))->eval(env);	
		env->set(second->getSymbol(), symbolValue);

		return symbolValue;
	} else {
		Error("didn't support type %d", second->getType());
		return DummyValue::nil;
	}
}

/*
	(let ((c 2)) c)
	(let ((c 2) (d 3)) (+ c d))
	let equals let*
 */
DummyValuePtr OpFuncLet(DummyValuePtr value, DummyEnvPtr env)
{
	DummyEnvPtr letEnv(new DummyEnv(env));
	
	DummyValueList letList = value->getList(); 
	
	// first is the let 
	DummyValueList::iterator letItr = letList.begin();	
	// second is the all the variables
	// second is a list
	DummyValuePtr second = *(++letItr);	
	if (!second->isList()) {
		Error("let error needs list but given %d with value %s", second->getType(), second->toString().c_str());
	}
	DummyValueList secondList = second->getList(); 	
	DummyValueList::iterator secondItr = secondList.begin();	
	for (; secondItr != secondList.end(); ++secondItr) {
		DummyValuePtr var = *secondItr;
		if (!var->isList()) {
			Error("var needs list but given %d with value %s", var->getType(), var->toString().c_str());
		}
		DummyValueList varList = var->getList();
		if (varList.size() != 2) {
			Error("varlist needs 2 items but given %d with value %s", varList.size(), var->toString().c_str());
		}
		DummyValuePtr symbol = varList[0];
		if (!symbol->isSymbol()) {
			Error("var must be a symbol but given %s", symbol->toString().c_str());
		}
		DummyValuePtr value = varList[1];
		letEnv->set(symbol->getSymbol(), value->eval(letEnv));
	}

	// exec the let body
	DummyValuePtr retValue;
	for (letItr++; letItr != letList.end(); letItr++) {
		retValue = (*letItr)->eval(letEnv);
	}

	return retValue;
}

/*
	(begin a b c)
 */
DummyValuePtr OpFuncBegin(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList();
	
	// first is the begin 
	DummyValueList::iterator itr = list.begin();		
	// exec the begin body
	DummyValuePtr retValue;
	for (itr++; itr != list.end(); itr++) {
		retValue = (*itr)->eval(env);
	}

	return retValue;
}

/*
	(if true 1 2)
	(if true 1 2 3)
 */
DummyValuePtr OpFuncIf(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList();

	if (list.size() < 2) {
		Error("if syntax error too little items with %s", value->toString().c_str());
	}
	
	// first is the if 
	DummyValueList::iterator itr = list.begin();		
	// second is the condtion to check nil
	DummyValuePtr condition = *(++itr);
	DummyValuePtr ifTrueBody = DummyValue::nil;
	if (list.size() >= 3) {
		ifTrueBody = *(++itr);
	}
	
	DummyValuePtr retValue = DummyValue::nil;

	// first check condition
	if (condition->eval(env) != DummyValue::nil) {
		// TRUE != nil
		retValue = ifTrueBody->eval(env);
	} else {
		// Exec other values
		for (itr++; itr != list.end(); itr++) {
			retValue = (*itr)->eval(env);
		}
	}

	return retValue;	
}
}
