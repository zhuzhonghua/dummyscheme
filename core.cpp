#include "core.h"
#include "value.h"
#include "env.h"
#include "tokenize.h"

using namespace DummyScheme;

DummyOpEval DummyCore::opEval[DUMMY_TYPE_MAX] = {0};
MapOpCompile DummyCore::opCompile;
MapOpNum DummyCore::opNum;

MapOpType DummyCore::opToType;
String DummyCore::typeToOp[DUMMY_TYPE_MAX];

DummyTypeOpHelper::DummyTypeOpHelper(int type, const String op)
{
	DummyCore::typeToOp[type] = op;
	DummyCore::opToType[op] = type;	
}

DummyTypeOpHelper::DummyTypeOpHelper(int type, const String op, int num)
	:DummyTypeOpHelper(type, op)
{
	DummyCore::opNum[op] = num;
}

DummyTypeOpHelper::DummyTypeOpHelper(int type, const String op, DummyOpCompile opCompile)
	:DummyTypeOpHelper(type, op)
{
	DummyCore::opCompile[type] = opCompile;
}

DummyTypeOpHelper::DummyTypeOpHelper(int type, const String op, DummyOpEval eval)
	:DummyTypeOpHelper(type, op)
{
	DummyCore::opEval[type] = eval;
}

#define DUMMY_OP_EVAL_NAME(uniq) DummyOpEval ## uniq
#define DUMMY_OP_EVAL_HELP(uniq) DummyOpEvalHelp ## uniq

#define DUMMY_OP_EVAL(type, symbol)																			\
DummyValuePtr DUMMY_OP_EVAL_NAME(type)(DummyValuePtr ast, DummyEnvPtr env); \
 static DummyTypeOpHelper DUMMY_OP_EVAL_HELP(type) (type, STR(symbol), DUMMY_OP_EVAL_NAME(type)); \
 DummyValuePtr DUMMY_OP_EVAL_NAME(type)(DummyValuePtr ast, DummyEnvPtr env)

#define AssertArgBigEqual(num) AssertDummyValue(list.size() >= num, ast, "parameter need more")
#define AssertArgEqual(num) AssertDummyValue(list.size() == num, ast, "parameter must equal")

/*
	((a 2)
	(b 3))
*/
void ConstructLetEnv(DummyValueList varList, DummyEnvPtr letEnv)
{
	DUMMY_VALUE_LIST_FOR(varList)
	{
		DummyValueList symbolValue = (*itr)->getList();

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
		return typeToOp[type];

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
			if (Eval(condition, env)->isTrueValue())
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
			if (astType == DUMMY_TYPE_WHEN) flag = Eval(condition, env)->isTrueValue();
			else if (astType == DUMMY_TYPE_UNLESS) flag = Eval(condition, env)->isFalseValue();
			
			if (flag)
			{
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
	Assert(type >= 0 && type < DUMMY_TYPE_MAX, "internal type opeval out of range %d", type);
	
	DummyOpEval op = opEval[type];
	Assert(op != NULL, "internal error of no opeval with type %d", type);
	return op(ast, env);
}

/*
	(+ 1 2 3)
*/
DUMMY_OP_EVAL(DUMMY_TYPE_PLUS, +)
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
DUMMY_OP_EVAL(DUMMY_TYPE_MINUS, -)
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
DUMMY_OP_EVAL(DUMMY_TYPE_MUL, *)
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
DUMMY_OP_EVAL(DUMMY_TYPE_DIVIDE, /)
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
	(define b (+ a 3))
	(define b (lambda (p) (+ p 3)))
	(define (c p1 p2) (+ p1 p2))
	
	TODO:
	(define a +)
*/
DUMMY_OP_EVAL(DUMMY_TYPE_DEFINE, define)
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
	((lambda (a) (+ a 2)) 3)
	(apply (lambda (a) (+ a 2)) 3)
	(define f (lambda (a b) (+ a b)))
	(apply f 2 3)
	(apply + 2 3)
*/
DUMMY_OP_EVAL(DUMMY_TYPE_APPLY, apply)
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
DUMMY_OP_EVAL(DUMMY_TYPE_DISPLAY, display)
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
DUMMY_OP_EVAL(DUMMY_TYPE_LIST, list)
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
DUMMY_OP_EVAL(DUMMY_TYPE_LIST_MARK, list?)
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
DUMMY_OP_EVAL(DUMMY_TYPE_NULL_MARK, null?)
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
DUMMY_OP_EVAL(DUMMY_TYPE_EQUAL_MARK, equal?)
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
DUMMY_OP_EVAL(DUMMY_TYPE_NOT, not)
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
DUMMY_OP_EVAL(DUMMY_TYPE_EQUAL, =)
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
DUMMY_OP_EVAL(DUMMY_TYPE_LESS, <)
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
DUMMY_OP_EVAL(DUMMY_TYPE_LESS_EQUAL, <=)
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
DUMMY_OP_EVAL(DUMMY_TYPE_BIG, >)
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
DUMMY_OP_EVAL(DUMMY_TYPE_BIG_EQUAL, >=)
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
DUMMY_OP_EVAL(DUMMY_TYPE_LENGTH, length)
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
DUMMY_OP_EVAL(DUMMY_TYPE_LOAD, load)
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

DUMMY_OP_EVAL(DUMMY_TYPE_DEFINE_MACRO, define-macro)
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

//=============================================================
// compile code
//=============================================================

#define DUMMY_OP_COMPILE_NAME(uniq) DummyOpCompile ## uniq
#define DUMMY_OP_COMPILE_HELP(uniq) DummyOpCompileHelp ## uniq

#define DUMMY_OP_COMPILE(type, symbol)																	\
DummyValuePtr DUMMY_OP_COMPILE_NAME(type)(DummyValueList& list);				\
 static DummyTypeOpHelper DUMMY_OP_COMPILE_HELP(type) (type, STR(symbol), DUMMY_OP_COMPILE_NAME(type)); \
 DummyValueList DUMMY_OP_COMPILE_NAME(type)(DummyValueList& list)

#define DUMMY_OP_COMPILE_NUM(type, symbol, num)													\
static DummyTypeOpHelper DUMMY_OP_COMPILE_HELP(type) (type, STR(symbol), num);

DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_BEGIN, begin, 1);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_IF, if, 2);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_WHEN, when, 2);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_UNLESS, unless, 2);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_PLUS, +, 2);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_MINUS, -, 2);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_MUL, *, 2);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_DIVIDE, /, 2);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_QUOTE, quote, 1);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_UNQUOTE, unquote, 1);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_UNQUOTE_SPLICING, unquote-splicing, 1);
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_QUASIQUOTE, quasiquote, 1);

/*
	construct let
	(let ((c 2)) (+ c 2))
*/
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_LET, let, 2);

/*
	(define a 2)
	(define (square x) (* x x))
*/
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_DEFINE, define, 2)

/*
	(define-macro (f b) `(+ 2 b))
 */
DUMMY_OP_COMPILE_NUM(DUMMY_TYPE_DEFINE_MACRO, define-macro, 2);
