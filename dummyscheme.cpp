#include "dummyscheme.h"
#include "value.h"
#include "env.h"

namespace DummyScheme {

bool isEqual(const std::string& first, const DummyValuePtr& second)
{
	AssertDummyValue(second->isSymbol(), "compare must be a symbol", second);
	return first.size() > 0 && 0 == first.compare(second->getSymbol());
}

// operator function
typedef DummyValuePtr (*OpEval)(DummyValuePtr, DummyEnvPtr);
typedef DummyValuePtr (*OpConstruct)(DummyValueList);
typedef std::string (*ValueToString)(DummyValuePtr);

DummyValuePtr OpConstructSelf(DummyValueList list);
DummyValuePtr OpConstructPlus(DummyValueList list);
DummyValuePtr OpConstructMinus(DummyValueList list);
DummyValuePtr OpConstructMul(DummyValueList list);
DummyValuePtr OpConstructDivide(DummyValueList list);
DummyValuePtr OpConstructDefine(DummyValueList list);
DummyValuePtr OpConstructLet(DummyValueList list);
DummyValuePtr OpConstructBegin(DummyValueList list);
DummyValuePtr OpConstructIf(DummyValueList list);
DummyValuePtr OpConstructWhen(DummyValueList list);
DummyValuePtr OpConstructUnless(DummyValueList list);
DummyValuePtr OpConstructLambda(DummyValueList list);
DummyValuePtr OpConstructApply(DummyValueList list);

DummyValuePtr OpEvalSelf(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalEnvGet(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalPlus(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalMinus(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalMul(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalDivide(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalDefine(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalLet(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalBegin(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalIf(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalWhen(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalUnless(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalLambda(DummyValuePtr value, DummyEnvPtr env);
DummyValuePtr OpEvalApply(DummyValuePtr value, DummyEnvPtr env);

std::string IntToString(DummyValuePtr value);
std::string FloatToString(DummyValuePtr value);
std::string StrToString(DummyValuePtr value);
std::string SymbolToString(DummyValuePtr value);
std::string LambdaToString(DummyValuePtr value);
std::string ListToString(DummyValuePtr value);
std::string TypeToString(DummyValuePtr value);

struct OpConstructEval{
	std::string key;
	int dummyType;
	OpConstruct create;
	OpEval eval;
	ValueToString str;
};

// attention the dummytype order
const OpConstructEval allOp[] = {
	{"", DummyType::DUMMY_INT_NUM, NULL, OpEvalSelf, IntToString},
	{"", DummyType::DUMMY_FLOAT_NUM, NULL, OpEvalSelf, FloatToString},
	{"", DummyType::DUMMY_STRING, NULL, OpEvalSelf, StrToString},
	{"", DummyType::DUMMY_SYMBOL, NULL, OpEvalEnvGet, SymbolToString},
	{"", DummyType::DUMMY_NIL, NULL, OpEvalSelf, SymbolToString},
	{"", DummyType::DUMMY_TRUE, NULL, OpEvalSelf, SymbolToString},
	{"+", DummyType::DUMMY_PLUS, OpConstructPlus, OpEvalPlus, TypeToString},
	{"-", DummyType::DUMMY_MINUS, OpConstructMinus, OpEvalMinus, TypeToString},
	{"*", DummyType::DUMMY_MUL, OpConstructMul, OpEvalMul, TypeToString},
	{"/", DummyType::DUMMY_DIVIDE, OpConstructDivide, OpEvalDivide, TypeToString},
	{"define", DummyType::DUMMY_DEFINE, OpConstructDefine, OpEvalDefine, TypeToString},
	{"let", DummyType::DUMMY_LET, OpConstructLet, OpEvalLet, TypeToString},
	{"begin", DummyType::DUMMY_BEGIN, OpConstructBegin, OpEvalBegin, TypeToString},
	{"if", DummyType::DUMMY_IF, OpConstructIf, OpEvalIf, TypeToString},
	{"when", DummyType::DUMMY_WHEN, OpConstructWhen, OpEvalWhen, TypeToString},
	{"unless", DummyType::DUMMY_UNLESS,	OpConstructUnless, OpEvalUnless, TypeToString},
	{"lambda", DummyType::DUMMY_LAMBDA,	OpConstructLambda, NULL, TypeToString},
	{"apply", DummyType::DUMMY_APPLY, OpConstructApply, OpEvalApply, TypeToString},
	{"", DummyType::DUMMY_LIST, OpConstructSelf, NULL, ListToString}
};

/*
	create dummyvalue by key
 */
DummyValuePtr createDummyValue(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	if (front->isSymbol()) {
		std::string symbol = front->getSymbol();	
		for (int i = 0; i < DummyType::DUMMY_MAX; i++) {
			const OpConstructEval& op = allOp[i];
			if (isEqual(symbol, op.key)) {
				return op.create(list);
			}
		}
	}

	return DummyValuePtr(new DummyValue(list));
}

/*
	eval by type
 */
DummyValuePtr valueEval(DummyValuePtr value, DummyEnvPtr env)
{
	const OpConstructEval& op = allOp[value->getType()];	
	return op.eval(value, env);
}

/*
	value to string by different type
 */
std::string valueToString(DummyValuePtr value)
{
	const OpConstructEval& op = allOp[value->getType()];	
	return op.str(value);
}

void init()
{
	// Just check
	for (int i = 0; i < DummyType::DUMMY_MAX - 1; i++) {
		const OpConstructEval& op = allOp[i];
		const OpConstructEval& opNext = allOp[i+1];
		Assert(op.dummyType+1 == opNext.dummyType, "internal error");
	}
}

/*
	(+ 1 2 3)
	TODO: check parameter length the first pass	
*/
DummyValuePtr OpEvalPlus(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 
	
	int num = 0;
	DummyValueList::iterator itr = list.begin();	
	for (; itr != list.end(); ++itr)
	{
		DummyValuePtr item = *itr;	
		AssertDummyValue(item->isInt(), "just support int", item);
		num += item->getInt(env);
	}
	
	return DummyValuePtr(new DummyValue(num));
}

/*
	(- 1 2 3)
*/
DummyValuePtr OpEvalMinus(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 
	
	DummyValueList::iterator itr = list.begin();
	int num = (*itr)->getInt();
	
	for (++itr; itr != list.end(); ++itr) {
		DummyValuePtr item = *itr;	
		AssertDummyValue(item->isInt(), "just support int", item);
		num -= item->getInt(env);
	}
	
	return DummyValuePtr(new DummyValue(num));
}

/*
	(* 1 2 3)
*/
DummyValuePtr OpEvalMul(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 

	int num = 1;
	// first is the symbol	
	DummyValueList::iterator itr = list.begin();	
	
	for (; itr != list.end(); ++itr) {
		DummyValuePtr item = *itr;	
		AssertDummyValue(item->isInt(), "just support int", item);
		num *= item->getInt(env);
	}
	
	return DummyValuePtr(new DummyValue(num));
}

/*
	(/ 1 2 3)
*/
DummyValuePtr OpEvalDivide(DummyValuePtr value, DummyEnvPtr env)
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
DummyValuePtr OpEvalDefine(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 
	
	DummyValueList::iterator itr = list.begin();	
	DummyValuePtr symbol = *itr;	
	AssertDummyValue(symbol->isSymbol(), "must be symbol", symbol);
	
	DummyValuePtr symbolValue = (*++itr)->eval(env);	
	env->set(symbol->getSymbol(), symbolValue);

	return symbolValue;
}

/*
	(let ((c 2)) c)
	(let ((c 2) (d 3)) (+ c d))
	let equals let*
 */
DummyValuePtr OpEvalLet(DummyValuePtr value, DummyEnvPtr env)
{
	DummyEnvPtr letEnv(new DummyEnv(env));
	
	DummyValueList letList = value->getList(); 
	
	DummyValueList::iterator letItr = letList.begin();	
	// first is the all the variables
	// first is a list
	DummyValuePtr vars = *letItr;
	AssertDummyValue(vars->isList(), "vars must be list", vars);

	DummyValueList varList = vars->getList();
	DummyValueList::iterator varItr = varList.begin();	
	for (; varItr != varList.end(); ++varItr) {
		DummyValuePtr var = *varItr;
		AssertDummyValue(var->isList(), "var must be list", var);
		DummyValueList varList = var->getList();
		AssertDummyValueList(varList.size() == 2, "var list must =2", varList);

		DummyValuePtr symbol = varList[0];
		AssertDummyValue(symbol->isSymbol(), "must be a symbol", symbol);

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
DummyValuePtr OpEvalBegin(DummyValuePtr value, DummyEnvPtr env)
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
DummyValuePtr OpEvalIf(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList();
	
	DummyValueList::iterator itr = list.begin();		
	DummyValuePtr condition = *itr;
	DummyValuePtr ifTrueBody = *(++itr);
	
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

/*
	(when true 1 2)
	(when nil 1 2 3)
 */
DummyValuePtr OpEvalWhen(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList();
	
	DummyValueList::iterator itr = list.begin();		
	DummyValuePtr condition = *itr;
	
	DummyValuePtr retValue = DummyValue::nil;

	// first check condition
	if (condition->eval(env) != DummyValue::nil) {
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
DummyValuePtr OpEvalUnless(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList();
	
	DummyValueList::iterator itr = list.begin();		
	DummyValuePtr condition = *itr;
	
	DummyValuePtr retValue = DummyValue::nil;

	// first check condition
	if (condition->eval(env) == DummyValue::nil) {
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
DummyValuePtr OpEvalApply(DummyValuePtr value, DummyEnvPtr env)
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
	just return self
 */
DummyValuePtr OpEvalSelf(DummyValuePtr value, DummyEnvPtr env)
{
	return value;
}

/*
	just return self
 */
DummyValuePtr OpEvalEnvGet(DummyValuePtr value, DummyEnvPtr env)
{
	return env->get(value->getSymbol());
}

/*
	return list itself
 */
DummyValuePtr OpConstructSelf(DummyValueList list)
{
	return DummyValuePtr(new DummyValue(list));
}

/*
	construct plus
 */
DummyValuePtr OpConstructPlus(DummyValueList list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(isEqual("+", front->getSymbol()), "first must be +", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_PLUS, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct minus
 */
DummyValuePtr OpConstructMinus(DummyValueList list)
{
	AssertDummyValueList(list.size() >= 3, "minus parameter >= 3", list);
	DummyValuePtr front = list.front();
	AssertDummyValueList(isEqual("-", front->getSymbol()), "first must be -", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_MINUS, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct mul
 */
DummyValuePtr OpConstructMul(DummyValueList list)
{
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);
	DummyValuePtr front = list.front();
	AssertDummyValueList(isEqual("*", front->getSymbol()), "first must be *", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_MUL, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct divide
 */
DummyValuePtr OpConstructDivide(DummyValueList list)
{
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);
	DummyValuePtr front = list.front();
	AssertDummyValueList(isEqual("/", front->getSymbol()), "first must be /", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_DIVIDE, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct define
 */
DummyValuePtr OpConstructDefine(DummyValueList list)
{
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);	
	DummyValuePtr front = list.front();
	AssertDummyValueList(isEqual("define", front->getSymbol()), "first must be define", list);
	DummyValuePtr symbol = *(list.begin() + 1);
	AssertDummyValue(symbol->isSymbol(), "second must be symbol", symbol);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_DEFINE, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct let
 */
DummyValuePtr OpConstructLet(DummyValueList list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(isEqual("let", front->getSymbol()), "first must belet" , list);
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);
	DummyValuePtr symbolList = *(list.begin() + 1);
	AssertDummyValue(symbolList->isList(), "second must be list", symbolList);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_LET, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct begin
 */
DummyValuePtr OpConstructBegin(DummyValueList list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(isEqual("begin", front->getSymbol()), "first must be begin" , list);
	AssertDummyValueList(list.size() >= 2, "parameter >= 2", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_BEGIN, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct if
 */
DummyValuePtr OpConstructIf(DummyValueList list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(isEqual("if", front->getSymbol()), "first must be if" , list);
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_IF, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct when
 */
DummyValuePtr OpConstructWhen(DummyValueList list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(isEqual("when", front->getSymbol()), "first must be when" , list);
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_WHEN, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct unless
 */
DummyValuePtr OpConstructUnless(DummyValueList list)
{
	DummyValuePtr front = list.front();
	AssertDummyValueList(isEqual("unless", front->getSymbol()), "first must be unless" , list);
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_UNLESS, DummyValueList(list.begin()+1, list.end())));
}

/*
	construct lambda
 */
DummyValuePtr OpConstructLambda(DummyValueList list)
{
	AssertDummyValueList(list.size() >= 3, "parameter >= 3", list);
	
	DummyValueList::iterator itr= list.begin();	
	DummyValuePtr front = *itr;
	AssertDummyValueList(isEqual("lambda", front->getSymbol()), "first must be lambda" , list);

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
DummyValuePtr OpConstructApply(DummyValueList list)
{
	DummyValueList::iterator itr = list.begin();
	DummyValuePtr front = *itr;
	if (!front->isLambda()) {
		AssertDummyValueList(isEqual("apply", front->getSymbol()), "first must be apply" , list);
		AssertDummyValueList(list.size() >= 2, "parameter >= 2", list);
		++itr;
	}
	// first maybe the lambda

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_APPLY, DummyValueList(itr, list.end())));
}

std::string IntToString(DummyValuePtr value)
{
	std::stringstream out;	
	out << value->getInt();
	return out.str();
}

std::string FloatToString(DummyValuePtr value)
{
	std::stringstream out;	
	out << value->getDouble();
	return out.str();
}

std::string StrToString(DummyValuePtr value)
{
	std::stringstream out;	
	out << "\"" << value->getStr() << "\"";
	return out.str();	
}

std::string SymbolToString(DummyValuePtr value)
{
	std::stringstream out;	
	out << value->getSymbol();
	return out.str();
}

std::string LambdaToString(DummyValuePtr value)
{	
	std::stringstream out;	
	out << "#<function>";
	out << " [";
	BindList binds = value->getBind();
	BindList::iterator bindItr = binds.begin();
	for (; bindItr != binds.end(); bindItr++) {
		out << (*bindItr);
		if (bindItr + 1 != binds.end()) {
			out << " ";	
		}
	}
	out << "] ";
	DummyValueList list = value->getList();
	DummyValueList::iterator itr = list.begin();	
	for (;itr != list.end(); itr++) {
		out << (*itr)->toString();
		if (itr + 1 != list.end()) {
			out << " ";	
		}
	}
	return out.str();
}

std::string ListToString(DummyValuePtr value)
{
	std::stringstream out;
	out << "(";
	DummyValueList list = value->getList();
	DummyValueList::iterator itr = list.begin();
	for (;itr != list.end(); itr++) {
		out << (*itr)->toString();
		if (itr + 1 != list.end()) {
			out << " ";	
		}
	}
	out << ")";
	return out.str();	
}

std::string TypeToString(DummyValuePtr value)
{
	const OpConstructEval& op = allOp[value->getType()];
	std::stringstream out;	
	out << "(" << op.key << " ";	
	DummyValueList list = value->getList();
	DummyValueList::iterator itr = list.begin();	
	for (;itr != list.end(); itr++) {
		out << (*itr)->toString();
		if (itr + 1 != list.end()) {
			out << " ";	
		}
	}
	out << ")";
	return out.str();		
}
}
