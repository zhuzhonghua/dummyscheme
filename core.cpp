#include "core.h"
#include "dummyscheme.h"
#include "value.h"
#include "env.h"

using namespace DummyScheme;

bool DummyCore::isEqual(const std::string& first, const DummyValuePtr& second)
{
	AssertDummyValue(second->isSymbol(), "compare must be a symbol", second);
	return first.size() > 0 && 0 == first.compare(second->getSymbol());
}

/*
	(+ 1 2 3)
	TODO: check parameter length the first pass	
*/
DummyValuePtr DummyCore::OpEvalPlus(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 
	
	int num = 0;
	DummyValueList::iterator itr = list.begin();	
	for (; itr != list.end(); ++itr)
	{
		DummyValuePtr item = *itr;	
		num += item->getInt(env);
	}
	
	return DummyValuePtr(new DummyValue(num));
}

/*
	(- 1 2 3)
*/
DummyValuePtr DummyCore::OpEvalMinus(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 
	
	DummyValueList::iterator itr = list.begin();
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
DummyValuePtr DummyCore::OpEvalMul(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 

	int num = 1;
	// first is the symbol	
	DummyValueList::iterator itr = list.begin();	
	
	for (; itr != list.end(); ++itr) {
		DummyValuePtr item = *itr;	
		num *= item->getInt(env);
	}
	
	return DummyValuePtr(new DummyValue(num));
}

/*
	(/ 1 2 3)
*/
DummyValuePtr DummyCore::OpEvalDivide(DummyValuePtr value, DummyEnvPtr env)
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
DummyValuePtr DummyCore::OpEvalDefine(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList();	
	
	DummyValuePtr symbol = list.front();
	DummyValuePtr symbolValue	= DummyValue::nil;
	
	if (symbol->isSymbol()) {
		symbolValue = (*(list.begin()+1))->eval(env);		
	} else {
		// must be list
		// checked when construct
		DummyValueList symbolList = symbol->getList();
		symbol = symbolList.front();
		
		// lambda
		DummyValueList bindList(symbolList.begin()+1, symbolList.end());
		DummyValueList::iterator bindItr = bindList.begin();
		BindList binds;
		for (; bindItr != bindList.end(); ++bindItr) {
			binds.push_back((*bindItr)->getSymbol());
		}
		symbolValue = DummyValuePtr(new DummyValue(binds, DummyValueList(list.begin()+1, list.end()))); 
	}
	
	env->set(symbol->getSymbol(), symbolValue);

	return symbolValue;
}

/*
	(let ((c 2)) c)
	(let ((c 2) (d 3)) (+ c d))
	let equals let*
 */
DummyValuePtr DummyCore::OpEvalLet(DummyValuePtr value, DummyEnvPtr env)
{
	DummyEnvPtr letEnv(new DummyEnv(env));
	
	DummyValueList letList = value->getList(); 
	
	DummyValueList::iterator letItr = letList.begin();	
	// first is the all the variables
	// first is a list
	DummyValuePtr vars = *letItr;

	DummyValueList varList = vars->getList();
	DummyValueList::iterator varItr = varList.begin();	
	for (; varItr != varList.end(); ++varItr) {
		DummyValuePtr var = *varItr;
		DummyValueList varList = var->getList();

		DummyValuePtr symbol = varList[0];
		DummyValuePtr value = varList[1];
		letEnv->set(symbol->getSymbol(), value->eval(letEnv));
	}

	// exec the let body
	DummyValuePtr retValue = DummyValue::nil;
	for (letItr++; letItr != letList.end(); letItr++) {
		retValue = (*letItr)->eval(letEnv);
	}

	return retValue;
}

/*
	(begin a b c)
 */
DummyValuePtr DummyCore::OpEvalBegin(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList();
	
	DummyValueList::iterator itr = list.begin();		
	// exec the begin body
	DummyValuePtr retValue = DummyValue::nil;
	for (; itr != list.end(); itr++) {
		retValue = (*itr)->eval(env);
	}

	return retValue;
}

/*
	(if true 1 2)
	(if true 1 2 3)
 */
DummyValuePtr DummyCore::OpEvalIf(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList();
	
	DummyValueList::iterator itr = list.begin();		
	DummyValuePtr condition = *itr;
	DummyValuePtr ifTrueBody = *(++itr);
	
	DummyValuePtr retValue = DummyValue::nil;

	// first check condition
	if (!condition->eval(env)->isNilValue()) {
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

/*
	(when true 1 2)
	(when nil 1 2 3)
 */
DummyValuePtr DummyCore::OpEvalWhen(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList();
	
	DummyValueList::iterator itr = list.begin();		
	DummyValuePtr condition = *itr;
	
	DummyValuePtr retValue = DummyValue::nil;

	// first check condition
	if (!condition->eval(env)->isNilValue()) {
		// Exec other values
		for (itr++; itr != list.end(); itr++) {
			retValue = (*itr)->eval(env);
		}
	}

	return retValue;
}

/*
	(unless true 1 2)
	(unless nil 1 2 3)
 */
DummyValuePtr DummyCore::OpEvalUnless(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList();
	
	DummyValueList::iterator itr = list.begin();		
	DummyValuePtr condition = *itr;
	
	DummyValuePtr retValue = DummyValue::nil;

	// first check condition
	if (condition->eval(env)->isNilValue()) {
		// Exec other values
		for (itr++; itr != list.end(); itr++) {
			retValue = (*itr)->eval(env);
		}
	}

	return retValue;
}

/*
	((lambda (a) (+ a 2)) 3)
	(apply (lambda (a) (+ a 2)) 3)
	(apply f 2 3)
 */
DummyValuePtr DummyCore::OpEvalApply(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList applyList = value->getList(); 
	DummyValueList::iterator applyItr = applyList.begin();	
	DummyValuePtr lambda = *applyItr;
	
	if (!lambda->isLambda()) {
		lambda = env->get(lambda->getSymbol());
		AssertDummyValue(lambda->isLambda(), "op must be lambda", lambda);
	}
	
	DummyEnvPtr applyEnv(new DummyEnv(env));
		
	// set parameter binds
	BindList binds = lambda->getBind();
	BindList::iterator bindItr = binds.begin();
	for (; bindItr != binds.end(); ++bindItr) {
		++applyItr;	
		AssertDummyValue(applyItr != applyList.end(), "parameter less", value);
			
		applyEnv->set(*bindItr, (*applyItr)->eval(env));
	}

	// exec the body
	DummyValuePtr retValue = DummyValue::nil;
	DummyValueList lambdaBody = lambda->getList();
	DummyValueList::iterator bodyItr = lambdaBody.begin();	
	for (; bodyItr != lambdaBody.end(); ++bodyItr) {
		retValue = (*bodyItr)->eval(applyEnv);
	}

	return retValue;
}

/*
	construct plus
 */
DummyValuePtr DummyCore::OpConstructPlus(DummyValueList& list)
{
	return DummyValuePtr(new DummyValue(DummyType::DUMMY_PLUS, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct minus
 */
DummyValuePtr DummyCore::OpConstructMinus(DummyValueList& list)
{
	AssertDummyValueList(list.size() >= 3, "minus parameter >= 3", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_MINUS, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct mul
 */
DummyValuePtr DummyCore::OpConstructMul(DummyValueList& list)
{
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_MUL, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct divide
 */
DummyValuePtr DummyCore::OpConstructDivide(DummyValueList& list)
{
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_DIVIDE, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct define
	(define a 2)
	(define (square x) (* x x))
 */
DummyValuePtr DummyCore::OpConstructDefine(DummyValueList& list)
{
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);
	
	DummyValuePtr second = *(list.begin() + 1);
	AssertDummyValueList(second->isSymbol() || second->isList(), "second must be a symbol or list", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_DEFINE, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct let
 */
DummyValuePtr DummyCore::OpConstructLet(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);
	
	DummyValuePtr var = *(list.begin() + 1);
	AssertDummyValue(var->isList(), "var must be a list", var);
	
	DummyValueList symbolList = var->getList();
	DummyValueList::iterator symbolItr = symbolList.begin();
	for(; symbolItr != symbolList.end(); ++symbolItr) {
		DummyValueList varList = (*symbolItr)->getListCheck();
		AssertDummyValueList(varList.size() == 2, "varList list must =2", varList);
		AssertDummyValueList(varList.front()->isSymbol(), "symbol first must be symbol", varList);
	}

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_LET, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct begin
 */
DummyValuePtr DummyCore::OpConstructBegin(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(list.size() >= 2, "parameter >= 2", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_BEGIN, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct if
 */
DummyValuePtr DummyCore::OpConstructIf(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_IF, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct when
 */
DummyValuePtr DummyCore::OpConstructWhen(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_WHEN, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct unless
 */
DummyValuePtr DummyCore::OpConstructUnless(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_UNLESS, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct lambda
 */
DummyValuePtr DummyCore::OpConstructLambda(DummyValueList& list)
{
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);
	
	DummyValueList::iterator itr= list.begin();	
	DummyValuePtr front = *itr;

	DummyValuePtr binds = *++itr;
	DummyValueList bindList = binds->getList();
	DummyValueList::iterator bindItr = bindList.begin();
	BindList symbols;
	for (; bindItr != bindList.end(); ++bindItr) {
		symbols.push_back((*bindItr)->getSymbol());
	}
	// rest is the body
	return DummyValuePtr(new DummyValue(symbols, DummyValueList(++itr, list.end())));
}

/*
	construct apply
 */
DummyValuePtr DummyCore::OpConstructApply(DummyValueList& list)
{
	DummyValueList::iterator itr = list.begin();
	DummyValuePtr front = *itr;
	if (!front->isLambda()) {
		AssertDummyValueList(list.size() >= 2, "parameter >= 2", list);
		++itr;
	}
	// first maybe the lambda

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_APPLY, DummyValueList(itr, list.end())));
}
