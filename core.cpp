#include "core.h"
#include "dummyscheme.h"
#include "value.h"
#include "env.h"
#include "tokenize.h"

using namespace DummyScheme;

DummyOpEval DummyCore::builtInOpEval[DUMMY_TYPE_MAX] = {0};
MapOpCompile DummyCore::builtInOpCompile;
MapOpNum DummyCore::builtInOpNum;

MapOpType DummyCore::builtInOpToType;
String DummyCore::builtInTypeToOp[DUMMY_TYPE_MAX];

DummyBuiltInHelper::DummyBuiltInHelper(int type, const String& op)
{
	DummyCore::builtInTypeToOp[type] = op;
	DummyCore::builtInOpToType[op] = type;	
}

DummyBuiltInHelper::DummyBuiltInHelper(int type, const String& op, int num, DummyOpEval opEval)
	:DummyBuiltInHelper(type, op, num)
{
	DummyCore::builtInOpEval[type] = opEval;
}

DummyBuiltInHelper::DummyBuiltInHelper(int type, const String& op, int num)
	:DummyBuiltInHelper(type, op)
{
	DummyCore::builtInOpNum[op] = num;
}

DummyBuiltInHelper::DummyBuiltInHelper(int type, const String& op, DummyOpCompile opCompile)
	:DummyBuiltInHelper(type, op)
{
	DummyCore::builtInOpCompile[op] = opCompile;
}

DummyBuiltInHelper::DummyBuiltInHelper(const String& op, DummyOpCompile opCompile)
{
	DummyCore::builtInOpCompile[op] = opCompile;
}

#define STR(s) #s

#define DUMMY_OPEVALNAME(uniq) DummyBuiltInEval ## uniq
#define DUMMY_OPEVALHELP(uniq) DummyBuiltInEvalHelp ## uniq

#define DUMMY_OPCOMPILENAME(uniq) DummyBuiltInCompile ## uniq
#define DUMMY_OPCOMPILEHELP(uniq) DummyBuiltInCompileHelp ## uniq

#define DUMMY_BUILTIN_OP_NUM(type, symbol, num)													\
DummyValuePtr DUMMY_OPEVALNAME(type)(DummyValuePtr ast, DummyEnvPtr env); \
 static DummyBuiltInHelper DUMMY_OPEVALHELP(type) (type, STR(symbol), num, DUMMY_OPEVALNAME(type)); \
 DummyValuePtr DUMMY_OPEVALNAME(type)(DummyValuePtr ast, DummyEnvPtr env)

#define DUMMY_BUILTIN_OP_COMPILE_DEF(symbol, key)												\
DummyValuePtr DUMMY_OPCOMPILENAME(key)(DummyValueList& list);						\
 static DummyBuiltInHelper DUMMY_OPCOMPILEHELP(key) (STR(symbol), DUMMY_OPCOMPILENAME(key)); \
 DummyValuePtr DUMMY_OPCOMPILENAME(key)(DummyValueList& list)

#define DUMMY_BUILTIN_OP_COMPILE(symbol) DUMMY_BUILTIN_OP_COMPILE_DEF(symbol, symbol)

#define DUMMY_BUILTIN_OP_COMPILE_NUM(type, symbol, num)									\
static DummyBuiltInHelper DUMMY_OPCOMPILEHELP(type) (type, STR(symbol), num);

#define DUMMY_BUILTIN_OP_COMPILE_TYPE(type, symbol)											\
DummyValuePtr DUMMY_OPCOMPILENAME(type)(DummyValueList& list);					\
 static DummyBuiltInHelper DUMMY_OPCOMPILEHELP(type) (type, STR(symbol), DUMMY_OPCOMPILENAME(type)); \
 DummyValuePtr DUMMY_OPCOMPILENAME(type)(DummyValueList& list)

#define DUMMY_BUILTIN_OP(type, symbol) DUMMY_BUILTIN_OP_NUM(type, symbol, 0)

#define AssertArgBigEqual(num) AssertDummyValue(list.size() >= num, ast, "parameter need more")
#define AssertArgEqual(num) AssertDummyValue(list.size() == num, ast, "parameter must equal")

/*
	((a 2)
	(b 3))
*/
void ConstructLetEnv(DummyValueList varList, DummyEnvPtr letEnv)
{
	DummyValueListItr varItr = varList.begin();	
	for (; varItr != varList.end(); ++varItr) {
		DummyValueList symbolValue = (*varItr)->getList();

		DummyValuePtr symbol = symbolValue[0];
		DummyValuePtr value = symbolValue[1];
		letEnv->set(symbol->getSymbol(), DummyCore::Eval(value, letEnv));
	}
}

/*
	get type string
*/
String DummyCore::GetTypeStr(int type)
{
	if (type >= 0 && type <= DUMMY_TYPE_MAX)
		return builtInTypeToOp[type];

	return "unknown";
}

DummyValuePtr DummyCore::Eval(DummyValueListItr begin, DummyValueListItr end, DummyEnvPtr env)
{
	DummyValuePtr retValue = DummyValue::nil;
	for (; begin != end; begin++)
		retValue = Eval(*begin, env);
	return retValue;
}

/*
	eval list of values
	TODO: check if front is a lambda or lambda symbol
 */
DummyValuePtr DummyCore::Eval(DummyValueList list, DummyEnvPtr env)
{
	return Eval(list.begin(), list.end(), env);
}

/*
	eval one AST
*/
DummyValuePtr DummyCore::Eval(DummyValuePtr ast, DummyEnvPtr env)
{
	while (true)
	{
		int astType = ast->getType();
		switch (astType) {
		case DUMMY_TYPE_LET:{
			// (let ((c 2)) c)
			// (let ((c 2) (d 3)) (+ c d) (- c d))
			// let equals let*	
			DummyEnvPtr letEnv(new DummyEnv(env));
			DummyValueList letList = ast->getList();
	
			ConstructLetEnv(letList.front()->getList(), letEnv);

//			// tail call optimization
//			ast = opTypeValue(DUMMY_TYPE_BEGIN, letList.begin() + 1, letList.end());
//			env = letEnv;
//			continue;
			
			return Eval(letList.begin() + 1, letList.end(), letEnv);
			break;
		}
		case DUMMY_TYPE_BEGIN:{
			// tail call optimization
//			DummyValueList list = ast->getList();	
//			DummyValuePtr ret = list.front()->eval(env);
//			if (list.begin() + 1 == list.end())
//				return ret;
//			else
//			{
//				ast = opTypeValue(DUMMY_TYPE_BEGIN, list.begin() + 1, list.end());
//				// env = env
//				continue;
//			}
			return Eval(ast->getList(), env);
			break;	
		}
		case DUMMY_TYPE_IF:{
			DummyValueList list = ast->getList();
	
			DummyValueListItr itr = list.begin();
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
//					ast = opTypeValue(DUMMY_TYPE_BEGIN, DummyValueList(itr+2, list.end()));
					return Eval(itr+2, list.end(), env);
					// env = env
					continue;
				}
			}
			break;
		}
		case DUMMY_TYPE_WHEN:
		case DUMMY_TYPE_UNLESS: {
			DummyValueList list = ast->getList();
			DummyValueListItr itr = list.begin();
			DummyValuePtr condition = *itr;	
			// first check condition
			bool flag = false;
			if (astType == DUMMY_TYPE_WHEN) flag = condition->eval(env)->isTrueValue();
			else if (astType == DUMMY_TYPE_UNLESS) flag = condition->eval(env)->isFalseValue();
			
			if (flag)
				if (itr + 1 == list.end())
					return DummyValue::nil;
				else
				{
					// tail call optimization
//					ast = opTypeValue(DUMMY_TYPE_BEGIN, DummyValueList(itr+1, list.end()));
					return Eval(itr+1, list.end(), env);
					// env = env;	
					continue;
				}
			else
				return DummyValue::nil;
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
	eval the optypevalue
 */
DummyValuePtr DummyCore::EvalOpType(DummyValuePtr ast, DummyEnvPtr env)
{
	int type = ast->getType();
	DummyOpEval op = builtInOpEval[type];
	Assert(op != NULL, "internal error of no opeval with type %d", type);
	return op(ast, env);
}

/*
	create optype or normal list type
*/
DummyValuePtr DummyCore::Compile(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	if (front->isSymbol())
	{
		// specified constructor
		String symbol = front->getSymbol();
		MapOpCompile::iterator opCompileItr = builtInOpCompile.find(symbol);
		// attention here is the full list
		if (opCompileItr != builtInOpCompile.end())
			return (opCompileItr->second)(list);

		// default constructor
		MapOpType::iterator opTypeItr = builtInOpToType.find(symbol);
		if (opTypeItr != builtInOpToType.end())
		{
			MapOpNum::iterator opNumItr = builtInOpNum.find(symbol);
			if (opNumItr != builtInOpNum.end() && opNumItr->second > 0)
				AssertDummyValueList(list.size() >= opNumItr->second + 1, list, "need more parameters");
			
			// attention: here is the begin + 1
			return opTypeValue(opTypeItr->second, list.begin()+1, list.end());
		}
		
		// (let ((c 2)) c)
		//	Error("unexpected type %d", type);	
	}
	else if (front->isLambda()) // ((lambda (a) (+ a 2)) 2)
		return opTypeValue(DUMMY_TYPE_APPLY, list);

	return listValue(list);
}

DUMMY_BUILTIN_OP_COMPILE_NUM(DUMMY_TYPE_BEGIN, begin, 1);
DUMMY_BUILTIN_OP_COMPILE_NUM(DUMMY_TYPE_IF, if, 2);
DUMMY_BUILTIN_OP_COMPILE_NUM(DUMMY_TYPE_WHEN, when, 2);
DUMMY_BUILTIN_OP_COMPILE_NUM(DUMMY_TYPE_UNLESS, unless, 2);

DUMMY_BUILTIN_OP_COMPILE_TYPE(DUMMY_TYPE_QUOTE, quote)
{
	AssertDummyValueList(list.size() == 2, list, "quote can only have on item");
	return opTypeValue(DUMMY_TYPE_QUOTE, list.begin()+1, list.end());
}

DUMMY_BUILTIN_OP_COMPILE_TYPE(DUMMY_TYPE_UNQUOTE, unquote)
{
	AssertDummyValueList(list.size() == 2, list, "unquote can only have on item");
	return opTypeValue(DUMMY_TYPE_UNQUOTE, list.begin()+1, list.end());
}

DUMMY_BUILTIN_OP_COMPILE_TYPE(DUMMY_TYPE_UNQUOTE_SPLICING, unquote-splicing)
{
	AssertDummyValueList(list.size() == 2, list, "unquote-splicing can only have on item");
	return opTypeValue(DUMMY_TYPE_UNQUOTE_SPLICING, list.begin()+1, list.end());
}

DUMMY_BUILTIN_OP_COMPILE_TYPE(DUMMY_TYPE_QUASIQUOTE, quasiquote)
{
	AssertDummyValueList(list.size() == 2, list, "quasiquote can only have on item");
	return opTypeValue(DUMMY_TYPE_QUASIQUOTE, list.begin()+1, list.end());
}

/*
	construct let
	(let ((c 2)) (+ c 2))
*/
DUMMY_BUILTIN_OP_COMPILE(let)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(list.size() >= 3, list, "parameter >= 3");
	
	DummyValuePtr var = *(list.begin() + 1);
	
	DummyValueList symbolList = var->getList();
	DummyValueListItr symbolItr = symbolList.begin();
	for(; symbolItr != symbolList.end(); ++symbolItr)
	{
		DummyValueList varList = (*symbolItr)->getList();
		AssertDummyValueList(varList.size() == 2, varList, "varList list must =2");
		AssertDummyValueList(varList.front()->isSymbol(), varList, "first must be symbol");
		// TODO: store the info, symbol and value
	}

	return opTypeValue(DUMMY_TYPE_LET, list.begin()+1, list.end());
}

/*
	construct lambda
	(lambda (p1 p2) (+ p1 p2))
*/
DUMMY_BUILTIN_OP_COMPILE(lambda)
{
	AssertDummyValueList(list.size() >= 3, list, "parameter >= 3");
	
	DummyValueListItr itr= list.begin();	
	DummyValuePtr front = *itr;

	DummyValuePtr binds = *++itr;
	DummyValueList bindList = binds->getList();
	DummyValueListItr bindItr = bindList.begin();
	BindList symbols;
	for (; bindItr != bindList.end(); ++bindItr) {
		symbols.push_back((*bindItr)->getSymbol());
	}
	// rest is the body
	return lambdaValue(symbols, ++itr, list.end());
}

/*
	(+ 1 2 3)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_PLUS, +, 2)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);
	
	int num = 0;
	DummyValueListItr itr = list.begin();	
	for (; itr != list.end(); ++itr)
		num += DummyCore::Eval(*itr, env)->getInt();
	
	return numValue(num);
}

/*
	(- 1 2 3)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_MINUS, -, 2)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);
	
	DummyValueListItr itr = list.begin();
	int num = DummyCore::Eval(*itr, env)->getInt();
	
	for (++itr; itr != list.end(); ++itr)
		num -= DummyCore::Eval(*itr, env)->getInt();
	
	return numValue(num);
}

/*
	(* 1 2 3)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_MUL, *, 2)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);

	int num = 1;
	DummyValueListItr itr = list.begin();	
	
	for (; itr != list.end(); ++itr)
		num *= DummyCore::Eval(*itr, env)->getInt();
	
	return numValue(num);
}

/*
	(/ 1 2 3)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_DIVIDE, /, 2)
{
	DummyValueList list = ast->getList(); 
	AssertArgBigEqual(2);
	
	DummyValueListItr itr = list.begin();	
	// second is the eeeee
	int num = DummyCore::Eval(*itr, env)->getInt();
	
	for (++itr; itr != list.end(); ++itr)
		num /= DummyCore::Eval(*itr, env)->getInt();
	
	return numValue(num);
}

/*
	(define a 2)
	(define (square x) (* x x))
*/
DUMMY_BUILTIN_OP_COMPILE(define)
{
	AssertDummyValueList(list.size() >= 3, list, "parameter >= 3");
	
	DummyValuePtr second = *(list.begin() + 1);
	AssertDummyValueList(second->isSymbol() || second->isList(), list, "second must be a symbol or list");

	return opTypeValue(DUMMY_TYPE_DEFINE, list.begin()+1, list.end());
}

/*
	(define a 2)
	(define b (+ a 3))
	(define b (lambda (p) (+ p 3)))
	(define (c p1 p2) (+ p1 p2))
	
	TODO:
	(define a +)
*/
DUMMY_BUILTIN_OP(DUMMY_TYPE_DEFINE, define)
{
	DummyValueList list = ast->getList();
	AssertArgEqual(2);
	
	DummyValuePtr symbol = list.front();
	DummyValuePtr symbolValue	= DummyValue::nil;
	
	if (symbol->isSymbol())
		symbolValue = DummyCore::Eval(*(list.begin()+1), env);
	else
	{
		// must be list
		// TODO: put some checkc on construct
		// DONE
		DummyValueList symbolList = symbol->getList();
		symbol = symbolList.front();
		
		// lambda
		DummyValueList bindList(symbolList.begin()+1, symbolList.end());
		DummyValueListItr bindItr = bindList.begin();
		BindList binds;
		for (; bindItr != bindList.end(); ++bindItr)
		{
			DummyValuePtr bindValue = *bindItr;
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
	
	TODO:
	(apply + 2 3)
	(define a +)
	(apply a 2 3)
*/
DUMMY_BUILTIN_OP_COMPILE(apply)
{
	DummyValueListItr itr = list.begin();
	DummyValuePtr front = *itr;
	if (!front->isLambda())
	{
		AssertDummyValueList(list.size() >= 2, list, "parameter >= 2");
		++itr;
	}
	// first maybe the lambda

	return opTypeValue(DUMMY_TYPE_APPLY, itr, list.end());
}

/*
	((lambda (a) (+ a 2)) 3)
	(apply (lambda (a) (+ a 2)) 3)
	(define f (lambda (a b) (+ a b)))
	(apply f 2 3)
	(apply + 2 3)
*/
DUMMY_BUILTIN_OP(DUMMY_TYPE_APPLY, apply)
{
	DummyValueList list = ast->getList(); 
	DummyValueListItr applyItr = list.begin();	
	DummyValuePtr lambda = *applyItr;
	
	if (!lambda->isLambda())
	{
		lambda = env->get(lambda->getSymbol());
		// TODO: builtin types
//		AssertDummyValue(lambda->isLambda(), lambda, "must be a lambda");
	}

	return lambda->apply(++applyItr, list.end(), env);
}

/*
	(display a b c)
*/
DUMMY_BUILTIN_OP(DUMMY_TYPE_DISPLAY, display)
{
	DummyValueList list = ast->getList();
	
	DummyValueListItr itr = list.begin();		
	// exec the begin body
	for (; itr != list.end(); itr++)
	{
		String result = DummyCore::Eval(*itr, env)->toString();
		Print(result.c_str());
	}

	Print("\n");
	return DummyValue::nil;
}

/*
	(list 1 a b)
*/
DUMMY_BUILTIN_OP(DUMMY_TYPE_LIST, list)
{
	DummyValueList list = ast->getList();
	DummyValueList evalList;
	
	DummyValueListItr itr = list.begin();		
	// exec the begin body
	for (; itr != list.end(); itr++)
	{
		evalList.push_back(DummyCore::Eval(*itr, env));
	}
	
	return listValue(evalList);		
}

/*
	(list? (list 1 2 3))
	support multi item
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_LIST_MARK, list?, 1)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(1);
	
	DummyValueListItr itr = list.begin();		
	for (; itr != list.end(); itr++)
	{
		if(!DummyCore::Eval(*itr, env)->isList())
			return DummyValue::nil;
	}
	
	return DummyValue::t;
}

/*
	(null? (list 1 2 3))
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_NULL_MARK, null?, 1)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(1);
	
	DummyValueListItr itr = list.begin();
	for (; itr != list.end(); itr++)
	{
		DummyValuePtr evalVal = DummyCore::Eval(*itr, env);
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

/*
	(equal? 1 2)
	(equal? 1 1 1)
	(equal? (list 1 2 3) (list 1 2 3))
	(equal? (list 1 2 3) 1)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_EQUAL_MARK, equal?, 1)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(1);
	
	DummyValueListItr itr = list.begin();		
	DummyValuePtr first = *itr;
	for (++itr; itr != list.end(); itr++)
	{
		if (!first->isEqualValue(*itr, env))
			return DummyValue::nil;
	}
	
	return DummyValue::t;	
}

/*
	(not nil #f)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_NOT, not, 1)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(1);
	
	DummyValueListItr itr = list.begin();		
	for (; itr != list.end(); itr++)
	{
		DummyValuePtr eval = DummyCore::Eval(*itr, env);
		if (eval != DummyValue::nil && eval != DummyValue::f)
			return DummyValue::nil;
	}
	
	return DummyValue::t;	
}

/*
	(= 1 2)
	(= 1 2 3)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_EQUAL, =, 2)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);
	
	DummyValueListItr itr = list.begin();		
	DummyValuePtr first = *itr;
	int firstInt = DummyCore::Eval(first, env)->getInt();
	for (++itr; itr != list.end(); itr++)
	{
		if (firstInt != DummyCore::Eval(*itr, env)->getInt())
			return DummyValue::nil;
	}
	
	return DummyValue::t;
}

/*
	(< 1 2 3 4 5)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_LESS, <, 2)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);
	
	DummyValueListItr itr = list.begin();		
	DummyValuePtr first = *itr;
	int firstInt = DummyCore::Eval(first, env)->getInt();
	for (++itr; itr != list.end(); itr++)
	{
		if (firstInt >= DummyCore::Eval(*itr, env)->getInt())
			return DummyValue::nil;
	}
	
	return DummyValue::t;
}

/*
	(<= 1 2 3 4 4)
	(<= 1 2 5 4 4)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_LESS_EQUAL, <=, 2)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);
	
	DummyValueListItr itr = list.begin();		
	int firstInt = DummyCore::Eval(*itr, env)->getInt();
	for (++itr; itr != list.end(); itr++)
	{
		int evalInt = DummyCore::Eval(*itr, env)->getInt();
		if (firstInt > evalInt)
			return DummyValue::nil;
		else
			firstInt = evalInt;
	}
	
	return DummyValue::t;
}

/*
	(> 5 4 3 2 1)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_BIG, >, 2)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);
	
	DummyValueListItr itr = list.begin();		
	
	int firstInt = DummyCore::Eval(*itr, env)->getInt();
	for (++itr; itr != list.end(); itr++)
	{
		int evalInt = DummyCore::Eval(*itr, env)->getInt();
		if (firstInt <= evalInt)
			return DummyValue::nil;
		else
			firstInt = evalInt;
	}
	
	return DummyValue::t;
}

/*
	(>= 4 4 3 2 1)
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_BIG_EQUAL, >=, 2)
{
	DummyValueList list = ast->getList();
	AssertArgBigEqual(2);
	
	DummyValueListItr itr = list.begin();		
	
	int firstInt = DummyCore::Eval(*itr, env)->getInt();
	for (++itr; itr != list.end(); itr++)
	{
		int evalInt = DummyCore::Eval(*itr, env)->getInt();
		if (firstInt < evalInt)
			return DummyValue::nil;
		else
			firstInt = evalInt;
	}
	
	return DummyValue::t;
	
}

/*
	(length (list 1 2 3))
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_LENGTH, length, 1)
{
	DummyValueList list = ast->getList();
	AssertArgEqual(1);
	
	DummyValuePtr evalVal = DummyCore::Eval(list.front(), env);
	// must do the check
	// other type value also has the method getlist
	AssertDummyValue(evalVal->isList(), ast, "must be list");
	
	return numValue(evalVal->getList().size());
}

/*
	(load "test.scm")
*/
DUMMY_BUILTIN_OP_NUM(DUMMY_TYPE_LOAD, load, 1)
{
	DummyValueList list = ast->getList();	
	AssertArgBigEqual(1);
	
	// TODO: use a filetype dummyvalue dynamicaly read file content
	DummyValueListItr itr = list.begin();	
	for (; itr != list.end(); ++itr)
	{
		String fileName = DummyCore::Eval(*itr, env)->getStr();

		FILE* file = fopen(fileName.c_str(), "rb");
		Assert(file != NULL, "file not found");
		
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		fseek(file, 0, SEEK_SET);
		String content;
		content.resize(size+1);
		fread((void*)content.data(), 1, size, file);
		content[size] =  '\0';

		StringStream toEval;
		toEval << "(begin " << content << "  )";
		Tokenize tokenize(toEval.str());
		tokenize.run(env);
		// TODO: check exception
		// check re open same file
		fclose(file);
	}
	
	return DummyValue::nil;
}

/*
	(define-macro (f b) `(+ 2 b))
 */
DUMMY_BUILTIN_OP_COMPILE_TYPE(DUMMY_TYPE_DEFINE_MACRO, define-macro)
{
	AssertDummyValueList(list.size() == 3, list, "parameter >= 3");
	
	DummyValuePtr second = *(list.begin() + 1);
	AssertDummyValueList(second->isList(), list, "second must be a list");
	AssertDummyValueList(second->getList().front()->isSymbol(), list, "the first of second must be a symbol");

	return opTypeValue(DUMMY_TYPE_DEFINE_MACRO, list.begin()+1, list.end());
}
DUMMY_BUILTIN_OP(DUMMY_TYPE_DEFINE_MACRO, define-macro)
{
	DummyValueList list = ast->getList();
	AssertArgEqual(2);
	
	DummyValuePtr symbol = list.front();
	
	DummyValueList symbolList = symbol->getList();
	symbol = symbolList.front();
		
	// lambda
	DummyValueList bindList(symbolList.begin()+1, symbolList.end());
	DummyValueListItr bindItr = bindList.begin();
	BindList binds;
	for (; bindItr != bindList.end(); ++bindItr)
	{
		DummyValuePtr bindValue = *bindItr;
		binds.push_back(bindValue->getSymbol());
	}
	DummyValuePtr symbolValue = macroValue(binds, list.begin()+1, list.end());
	
	env->set(symbol->getSymbol(), symbolValue);
	return symbolValue;
}
