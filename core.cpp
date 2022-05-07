#include "core.h"
#include "dummyscheme.h"
#include "value.h"
#include "env.h"
#include "tokenize.h"

using namespace DummyScheme;

#define DUMMY_OPEVALNAME(uniq) DummyBuiltInOpEval ## uniq
#define DUMMY_OPEVALHELP(uniq) DummyBuiltInOpEvalHelp ## uniq

#define DUMMY_OPCREATENAME(uniq) DummyBuiltInOpCreate ## uniq
#define DUMMY_OPCREATEHELP(uniq) DummyBuiltInOpCreateHelp ## uniq

#define DUMMY_BUILTIN_OP(type, symbol) static DummyOpEval DUMMY_OPEVALNAME(type);	\
 static DummyBuiltInOp DUMMY_OPEVALHELP(type) (type, DUMMY_OPEVALNAME(type));	\
 DummyValuePtr DUMMY_OPEVALNAME(type) (DummyValuePtr ast, DummyEnvPtr env)


#define DUMMY_BUILTIN_OP_CREATE(type, symbol) static DummyOpCreate DUMMY_OPCREATENAME(type); \
 static DummyBuiltInOp DUMMY_OPCREATEHELP(type) (symbol, DUMMY_OPCREATENAME(type)); \
 DummyValuePtr DUMMY_OPCREATENAME(type) (DummyValueList& list)

#define DUMMY_BUILTIN_OP_CREATE_NORMAL(type, symbol, num) \
DUMMY_BUILTIN_OP_CREATE(type, symbol)																		\
{																																				\
	if (num > 0) AssertDummyValueList(list.size() >= num + 1, list, "need more parameters"); \
	return opTypeValue(symbol, type, list.begin()+1, list.end);						\
}

#define AssertArgBigEqual(num) AssertDummyValue(list.size() >= num, ast)
#define AssertArgEqual(num) AssertDummyValue(list.size() == num, ast)

DummyOpEval DummyBuiltInOp::builtInOps[DUMMY_TYPE_MAX] = {0};
std::map<std::string, DummyOpCreate> DummyBuiltInOp::builtInOpsCreate;

DummyBuiltInOp::DummyBuiltInOp(DummyType type, DummyOpEval op)
{
	builtInOps[type] = op;
}

DummyBuiltInOp::DummyBuiltInOp(const std::string &typeStr, DummyOpCreate op)
{
	builtInOpsCreate[typeStr] = op;
}

/*
	Ö´ÐÐÒ»¸öAST
 */
DummyValuePtr DummyCore::Eval(DummyValuePtr ast, DummyEnvPtr env)
{
	while (true)
	{
		DummyType astType = ast->getType();
		switch (astType) {
		case DummyType::DUMMY_LET:{
			// (let ((c 2)) c)
			// (let ((c 2) (d 3)) (+ c d) (- c d))
			// let equals let*	
			DummyEnvPtr letEnv(new DummyEnv(env));
			DummyValueList letList = ast->getList();
	
			ConstructLetEnv(letList.front()->getList(), letEnv);

			// tail call optimization
			ast = opTypeValue("begin", DummyType::DUMMY_BEGIN, letList.begin() + 1, letList.end());
			env = letEnv;
			continue;
			break;
		}
		case DummyType::DUMMY_BEGIN:{
			// tail call optimization
			DummyValueList list = ast->getList();	
			DummyValuePtr ret = list.front()->eval(env);
			if (list.begin() + 1 == list.end())
				return ret;
			else
			{
				ast = opTypeValue("begin", DummyType::DUMMY_BEGIN, list.begin() + 1, list.end());
				// env = env
				continue;
			}
			break;	
		}
		case DummyType::DUMMY_IF:{
			DummyValueList list = ast->getList();
	
			DummyValueList::iterator itr = list.begin();
			DummyValuePtr condition = *itr;
			// first check condition
			if (condition->eval(env)->isTrueValue())
			{
				// TRUE != nil
				if (itr + 1 != list.end())
				{
					ast = *(itr + 1);
					// env = env;
					continue;
				}
				else
					return DummyValue::nil;
			}
			else
			{
				// if true body or if false body is the end
				if (itr + 1 == list.end() || itr + 2 == list.end())
					return DummyValue::nil;
				else
				{
					// tail call optimization
					ast = typeVlaue("begin", DummyType::DUMMY_BEGIN, DummyValueList(itr+2, list.end()));
					// env = env
					continue;
				}
			}
			break;
		}
		case DummyType::DUMMY_WHEN:
		case DummyType::DUMMY_UNLESS: {
			DummyValueList list = ast->getList();
			DummyValueList::iterator itr = list.begin();
			DummyValuePtr condition = *itr;	
			// first check condition
			bool flag = false;
			if (astType == DummyType::DUMMY_WHEN) flag = condition->eval(env)->isTrueValue();
			else if (astType == DummyType::DUMMY_UNLESS) flag = condition->eval(env)->isFalseValue();
			
			if (flag)
				if (itr + 1 == list.end())
					return DummyValue::nil;
				else
				{
					// tail call optimization
					ast = opTypeValue("begin", DummyType::DUMMY_BEGIN, DummyValueList(itr+1, list.end()));
					// env = env;	
					continue;
				}
			else
				return DummyValue::nil;
			break;
		}
		case DummyType::DUMMY_QUASIQUOTE:{
			ast = OpEvalQuasiQuote(ast, env);
			// this result need to be reevaluated
			break;
		}
		default:
			return ast->eval(env);
			break;
		}
	}

	return DummyValue::nil;
}

/*
	(quasiquote lst)
	`lst
 */
DummyValuePtr DummyCore::OpEvalQuasiQuote(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValuePtr item = value->getList().front();
	// recursive call
	if (!item->isList())
		return item;
	

	DummyValueList list = item->getList();
	DummyValueList::iterator itr = list.begin();
	DummyValueList retValue;
	for (; itr != list.end(); itr++)
	{
		// eval and put
		if ((*itr)->isUnQuote())
			retValue.push_back(Eval((*itr)->getList().front(), env));
		else if ((*itr)->isUnQuoteSplicing())
		{
			// eval and splice put
			DummyValuePtr evalItr = Eval((*itr)->getList().front(), env);
			if (evalItr->isList())
			{
				DummyValueList evalItrList = evalItr->getList();
				retValue.insert(retValue.end(), evalItrList.begin(), evalItrList.end());
			}
			else
				retValue.push_back(evalItr);
		}
		else			// just put
			retValue.push_back(OpEvalQuasiQuote(*itr, env));
	}

	return DummyValue::create(retValue);
}

bool DummyCore::isEqual(const std::string& first, const DummyValuePtr& second)
{
	AssertDummyValue(second->isSymbol(), second);
	return first.size() > 0 && 0 == first.compare(second->getSymbol());
}

/*
	((a 2)
	 (b 3))
 */
void DummyCore::ConstructLetEnv(DummyValueList varList, DummyEnvPtr letEnv)
{
	DummyValueList::iterator varItr = varList.begin();	
	for (; varItr != varList.end(); ++varItr) {
		DummyValueList symbolValue = (*varItr)->getList();

		DummyValuePtr symbol = symbolValue[0];
		DummyValuePtr value = symbolValue[1];
		letEnv->set(symbol->getSymbol(), value->eval(letEnv));
	}
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_BEGIN, "begin", 1);
DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_IF, "if", 2);
DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_WHEN, "when", 2);
DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_UNLESS, "unless", 2);
DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_QUOTE, "quote", 1);
DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_UNQUOTE, "unquote", 1);
DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_UNQUOTE_SPLICING, "unquote-splicing", 1);
DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_QUASIQUOTE, "quasiquote", 1);

/*
	construct let
	(let ((c 2)) (+ c 2))
 */
DUMMY_BUILTIN_OP_CREATE(DUMMY_TYPE_LET, "let")
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(list.size() >= 3, list, "parameter >= 3");
	
	DummyValuePtr var = *(list.begin() + 1);
	AssertDummyValue(var->isList(), var);
	
	DummyValueList symbolList = var->getList();
	DummyValueList::iterator symbolItr = symbolList.begin();
	for(; symbolItr != symbolList.end(); ++symbolItr)
	{
		DummyValueList varList = (*symbolItr)->getListCheck();
		AssertDummyValueList(varList.size() == 2, varList, "varList list must =2");
		AssertDummyValueList(varList.front()->isSymbol(), varList, "first must be symbol");
		// TODO: store the info, symbol and value
	}

	return opTypeValue("let", DUMMY_TYPE_LET, list.begin()+1, list.end());
}

/*
	construct lambda
	(lambda (p1 p2) (+ p1 p2))
 */
DUMMY_BUILTIN_OP_CREATE(DUMMY_TYPE_LAMBDA, "lambda")
{
	AssertDummyValueList(list.size() >= 3, list, "parameter >= 3");
	
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
	return lambdaValue(symbols, ++itr, list.end());
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_PLUS, "+", 2);
/*
	(+ 1 2 3)
*/
DUMMY_BUILTIN_OP(DUMMY_TYPE_PLUS, "+")
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);
	
	int num = 0;
	DummyValueList::iterator itr = list.begin();	
	for (; itr != list.end(); ++itr)
	{
		DummyValuePtr item = *itr;	
		num += item->getInt(env);
	}
	
	return numValue(num);
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_MINUS, "-", 2);
/*
	(- 1 2 3)
*/
DUMMY_BUILTIN_OP(DUMMY_TYPE_MINUS, "-")
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);
	
	DummyValueList::iterator itr = list.begin();
	int num = (*itr)->getInt();
	
	for (++itr; itr != list.end(); ++itr)
	{
		DummyValuePtr item = *itr;	
		num -= item->getInt(env);
	}
	
	return DummyValuePtr(new DummyValue(num));
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_MUL, "*", 2);
/*
	(* 1 2 3)
*/
DUMMY_BUILTIN_OP(DUMMY_TYPE_MUL, "*")
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);

	int num = 1;
	DummyValueList::iterator itr = list.begin();	
	
	for (; itr != list.end(); ++itr) {
		DummyValuePtr item = *itr;	
		num *= item->getInt(env);
	}
	
	return numValue(num));
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_DIVIDE, "/", 2)
/*
	(/ 1 2 3)
*/
DUMMY_BUILTIN_OP(DUMMY_TYPE_DIVIDE, "/")
{
	DummyValueList list = ast->getList(); 
	AssertArgBigEqual(2);
	
	DummyValueList::iterator itr = list.begin();	
	// second is the eeeee
	int num = (*itr)->getInt();
	
	for (++itr; itr != list.end(); ++itr)
	{
		DummyValuePtr item = *itr;	
		num /= item->getInt(env);
	}
	return numValue(num);
}

/*
	(define a 2)
	(define (square x) (* x x))
 */
DUMMY_BUILTIN_OP_CREATE(DUMMY_TYPE_DEFINE, "define")
{
	AssertDummyValueList(list.size() >= 3, list, "parameter >= 3");
	
	DummyValuePtr second = *(list.begin() + 1);
	AssertDummyValueList(second->isSymbol() || second->isList(), list, "second must be a symbol or list");

	return opTypeValue("define", DUMMY_TYPE_DEFINE, list.begin()+1, list.end());
}

/*
	(define a 2)
	(define b (+ a 3))
	(define b (lambda (p) (+ p 3)))
	(define (c p1 p2) (+ p1 p2))
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_DEFINE, "define")
{
	DummyValueList list = ast->getList();
	AssertArgEqual(2);
	
	DummyValuePtr symbol = list.front();
	DummyValuePtr symbolValue	= DummyValue::nil;
	
	if (symbol->isSymbol())
		symbolValue = Eval(*(list.begin()+1), env);
	else
	{
		// must be list
		AssertDummyValue(symbol->isList(), ast);
		// TODO: put some checkc on construct
		DummyValueList symbolList = symbol->getList();
		symbol = symbolList.front();
		
		// lambda
		DummyValueList bindList(symbolList.begin()+1, symbolList.end());
		DummyValueList::iterator bindItr = bindList.begin();
		BindList binds;
		for (; bindItr != bindList.end(); ++bindItr)
		{
			DummyValuePtr bindValue = *bindItr;
			AssertDummyValue(bindValue->isSymbol(), ast);
			binds.push_back(bindValue->getSymbol());
		}
		symbolValue = lambdaValue(binds, list.begin()+1, list.end());
	}
	
	env->set(symbol->getSymbol(), symbolValue);

	return symbolValue;
}

/*
	(apply (lambda (a) (+ a 2)) 3)
	(apply f 2 3)
 */
DUMMY_BUILTIN_OP_CREATE(DUMMY_TYPE_APPLY, "apply")
{
	DummyValueList::iterator itr = list.begin();
	DummyValuePtr front = *itr;
	if (!front->isLambda())
	{
		AssertDummyValueList(list.size() >= 2, list, "parameter >= 2");
		++itr;
	}
	// first maybe the lambda

	return opTypeValue("apply", DUMMY_TYPE_APPLY, itr, list.end());
}

/*
	((lambda (a) (+ a 2)) 3)
	(apply (lambda (a) (+ a 2)) 3)
	(apply f 2 3)
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_APPLY, "apply")
{
	DummyValueList list = ast->getList(); 
	DummyValueList::iterator applyItr = list.begin();	
	DummyValuePtr lambda = *applyItr;
	
	if (!lambda->isLambda()) {
		lambda = env->get(lambda->getSymbol());
		AssertDummyValue(lambda->isLambda(), lambda);
	}
	
	DummyEnvPtr applyEnv(new DummyEnv(env));
		
	// set parameter binds
	BindList binds = lambda->getBind();
	BindList::iterator bindItr = binds.begin();
	for (; bindItr != binds.end(); ++bindItr)
	{
		++applyItr;	
		AssertDummyValue(applyItr != list.end(), value);
			
		applyEnv->set(*bindItr, Eval(*applyItr, env));
	}

	// exec the body
	DummyValuePtr retValue = DummyValue::nil;
	DummyValueList lambdaBody = lambda->getList();
	DummyValueList::iterator bodyItr = lambdaBody.begin();	
	for (; bodyItr != lambdaBody.end(); ++bodyItr)
	{
		retValue = Eval(*bodyItr, applyEnv);
	}

	return retValue;
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_DISPLAY, "display", 1);
/*
	(display a b c)
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_DISPLAY, "display")
{
	DummyValueList list = ast->getList();
	
	DummyValueList::iterator itr = list.begin();		
	// exec the begin body
	for (; itr != list.end(); itr++) {
		std::string result = Eval(*itr, env)->toString();
		Print(result.c_str());
	}

	Print("\n");
	return DummyValue::nil;
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_LIST, "list", 0);
/*
	(list 1 a b)
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_LIST, "list")
{
	DummyValueList list = ast->getList();
	DummyValueList evalList;
	
	DummyValueList::iterator itr = list.begin();		
	// exec the begin body
	for (; itr != list.end(); itr++)
	{
		evalList.push_back(Eval(*itr, env));
	}
	
	return listValue(evalList);		
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_LIST_MARK, "list?", 1);
/*
	(list? (list 1 2 3))
	support multi item
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_LIST_MARK, "list?")
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(1);
	
	DummyValueList::iterator itr = list.begin();		
	for (; itr != list.end(); itr++)
	{
		if(!Eval(*itr, env)->isList())
			return DummyValue::nil;
	}
	
	return DummyValue::t;
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_NULL_MARK, "null?", 1);
/*
	(null? (list 1 2 3))
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_NULL_MARK, "null?")
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(1);
	
	DummyValueList::iterator itr = list.begin();		
	for (; itr != list.end(); itr++)
	{
		DummyValuePtr evalVal = (*itr)->eval(env);
		if (evalVal == DummyValue::nil)
			;// do nohting
		else if (evalVal->isList())
			if (evalVal->getList().size() <= 0)
				;// do nohting
			else 
				return DummyValue::nil;	
		else 
			return DummyValue::nil;
	}
	
	return DummyValue::t;	
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_EQUAL_MARK, "equal?", 2);
/*
	(equal? 1 2)
	(equal? 1 1 1)
	(equal? (list 1 2 3) (list 1 2 3))
	(equal? (list 1 2 3) 1)
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_EQUAL_MARK, "equal?")
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(1);
	
	DummyValueList::iterator itr = list.begin();		
	DummyValuePtr first = *itr;
	for (++itr; itr != list.end(); itr++)
	{
		if (!first->isEqualValue(*itr, env))
			return DummyValue::nil;
	}
	
	return DummyValue::t;	
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_NOT, "not", 1);
/*
	(not nil #f)
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_NOT, "not")
{
	DummyValueList list = value->getList();
	AssertArgBigEqual(1);
	
	DummyValueList::iterator itr = list.begin();		
	for (; itr != list.end(); itr++)
	{
		DummyValuePtr eval = (*itr)->eval(env);
		if (eval != DummyValue::nil && eval != DummyValue::f)
			return DummyValue::nil;
	}
	
	return DummyValue::t;	
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_EQUAL, "=", 2);
/*
	(= 1 2)
	(= 1 2 3)
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_EQUAL, "=")
{
	DummyValueList list = value->getList();
	AssertArgBigEqual(2);
	
	DummyValueList::iterator itr = list.begin();		
	DummyValuePtr first = *itr;
	int firstInt = first->getInt(env);
	for (++itr; itr != list.end(); itr++)
	{
		DummyValuePtr evalVal = Eval(*itr, env);
		AssertDummyValue(evalVal->isInt(), ast);
		
		int evalInt = evalVal->getInt(env);
		if (firstInt != evalInt)
			return DummyValue::nil;
	}
	
	return DummyValue::t;
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_LESS, "<", 2);
/*
	(< 1 2 3 4 5)
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_LESS, "<")
{
	DummyValueList list = value->getList();
	AssertArgBigEqual(2);
	
	DummyValueList::iterator itr = list.begin();		
	DummyValuePtr first = *itr;
	int firstInt = first->getInt(env);
	for (++itr; itr != list.end(); itr++)
	{
		DummyValuePtr evalVal = Eval(*itr, env);
		AssertDummyValue(evalVal->isInt(), ast);
		
		int evalInt = evalVal->getInt(env);
		if (firstInt >= evalInt)
			return DummyValue::nil;
	}
	
	return DummyValue::t;
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_LESS_EQUAL, "<=", 2);
/*
	(<= 1 2 3 4 4)
	(<= 1 2 5 4 4)
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_LESS_EQUAL, "<=")
{
	DummyValueList list = value->getList();
	AssertArgBigEqual(2);
	
	DummyValueList::iterator itr = list.begin();		
	int firstInt = (*itr)->getInt(env);
	for (++itr; itr != list.end(); itr++)
	{
		DummyValuePtr evalVal = Eval(*itr, env);
		AssertDummyValue(evalVal->isInt(), ast);
		
		int evalInt = evalVal->getInt(env);
		if (firstInt > evalInt)
			return DummyValue::nil;
		else
			firstInt = evalInt;
	}
	
	return DummyValue::t;
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_BIG, ">", 2);
/*
	(> 5 4 3 2 1)
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_BIG, ">")
{
	DummyValueList list = value->getList();
	AssertArgBigEqual(2);
	
	DummyValueList::iterator itr = list.begin();		
	AssertDummyValue((*itr)->isInt(), ast);
	
	int firstInt = (*itr)->getInt(env);
	for (++itr; itr != list.end(); itr++)
	{
		DummyValuePtr evalVal = Eval(*itr, env);
		AssertDummyValue(evalVal->isInt(), ast);
		
		int evalInt = evalVal->getInt(env);
		if (firstInt <= evalInt)
			return DummyValue::nil;
		else
			firstInt = evalInt;
	}
	
	return DummyValue::t;
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_BIG_EQUAL, ">=", 2);
/*
	(>= 4 4 3 2 1)
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_BIG_EQUAL, ">=")
{
	DummyValueList list = value->getList();
	AssertArgBigEqual(2);
	
	DummyValueList::iterator itr = list.begin();		
	AssertDummyValue((*itr)->isInt(), ast);
	
	int firstInt = (*itr)->getInt(env);
	for (++itr; itr != list.end(); itr++)
	{
		DummyValuePtr evalVal = Eval(*itr, env);
		AssertDummyValue(evalVal->isInt(), ast);
		
		int evalInt = evalVal->getInt(env);
		if (firstInt < evalInt)
			return DummyValue::nil;
		else
			firstInt = evalInt;
	}
	
	return DummyValue::t;
	
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_LENGTH, "length", 1);
/*
	(length (list 1 2 3))
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_LENGTH, "length")
{
	DummyValueList list = ast->getList();
	AssertArgEqual(1);
	
	DummyValuePtr evalVal = Eval(list.front(), env);
	AssertDummyValue(evalVal->isList(), ast);

	return numValue(evalVal->getList().size());
}

DUMMY_BUILTIN_OP_CREATE_NORMAL(DUMMY_TYPE_LOAD, "load", 1);
/*
	(load "test.scm")
 */
DUMMY_BUILTIN_OP(DUMMY_TYPE_LOAD, "load")
{
	DummyValueList list = value->getList();	
	AssertArgBigEqual(1);
	
	// TODO: use a filetype dummyvalue dynamicaly read file content
	DummyValueList::iterator itr = list.begin();	
	for (; itr != list.end(); ++itr)
	{
		DummyValuePtr fileValue = Eval(*itr, env);
		AssertDummyValue(fileValue->isString(), ast);

		FILE* file = fopen(fileValue->getStr().c_str(), "rb");
		Assert(file != NULL, "file not found");
		
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		fseek(file, 0, SEEK_SET);
		std::string content;
		content.resize(size+1);
		fread((void*)content.data(), 1, size, file);
		content[size] =  '\0';

		std::stringstream toEval;
		toEval << "(begin " << content << "  )";
		Tokenize tokenize(toEval.str());
		tokenize.run(env);
		// TODO: check exception
		// check re open same file
		fclose(file);
	}
	
	return DummyValue::nil;
}

