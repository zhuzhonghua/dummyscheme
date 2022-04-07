#include "tokenize.h"
#include "env.h"
#include "value.h"

#define CASE_NUM case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
#define CASE_SYMBOL case '+':case '-':case '*':case '/'

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
	}

	return DummyValuePtr();
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

OpMap Tokenize::opMap;

void Tokenize::init()
{
	opMap["+"] = OpFuncPlus;
	opMap["-"] = OpFuncMinus;
	opMap["*"] = OpFuncMul;
	opMap["/"] = OpFuncDivide;
	opMap["define"] = OpFuncDefine;
	opMap["let"] = OpFuncLet;
}

/*
	Just for first pass to check
*/
void Tokenize::addOpForCheck(const std::string &symbol)
{
//	std::map<std::string, OpFunc>::iterator it = opMap.find(symbol);
//	if (it == opMap.end()) {
//		opMap[symbol] = NULL;
//	} else {
//		// do nothing
//	}
}

/*
	for define function
*/
void Tokenize::addOp(const std::string &symbol, OpFunc func)
{
	//	OpMap::iterator it = opMap.find(symbol);
	opMap[symbol] = func;
}

void Tokenize::unexpectedToken()
{
		std::stringstream ss;
		ss << "unexpected token "
			 << input[index]
			 << " at " << index;
		// can't do this
		// local variable
		throw ss.str().c_str();
}

Tokenize::Tokenize(const std::string &input)
{
	init(input);
}

void Tokenize::init(const std::string &input)
{
	this->input = input;
	this->index = 0;
}

void Tokenize::run(DummyEnvPtr env)
{
	DummyValuePtr val = readP();
	printf("%s\n", val->toString().c_str());
		
	DummyValuePtr evalVal = val->eval(env);
	printf("%s\n", evalVal->toString().c_str());
}

TokenType Tokenize::readToken()
{
	skipBlank();
	switch(input[index])
	{
	CASE_NUM:
		return TokenType::TOKEN_NUM;
	case '\"':
		return TokenType::TOKEN_DOUBLE_QUOT;
	case '(':
		return TokenType::TOKEN_LEFT_PAREN;
	case ')':
		return TokenType::TOKEN_RIGHT_PAREN;	
	CASE_SYMBOL:
		return TokenType::TOKEN_SYMBOL;
	default:
		if ('A' <= input[index] && input[index] <= 'Z') {
			return TokenType::TOKEN_SYMBOL;
		}	else if ('a' <= input[index] && input[index] <= 'z') {
			return TokenType::TOKEN_SYMBOL;
		} else {
			Error("unexpected token=%c index=%d\n", input[index], index);
		}
		break;
	}

	return TokenType::TOKEN_UNKNOWN;
}

/*
	P = NUM | STRING | SYMBOL | LEFT_PAREN LIST RIGHT_PAREN
	add check for p at the first pass
*/
DummyValuePtr Tokenize::readP()
{
	headType = readToken();
	TokenType token;	
	switch(headType)
	{
	case TokenType::TOKEN_NUM:
		return readNum();
		break;
	case TokenType::TOKEN_DOUBLE_QUOT:
		return readStr();
		break;
	case TokenType::TOKEN_SYMBOL:
		return readSymbol();
		break;	
	case TokenType::TOKEN_LEFT_PAREN:{	
		index++;
		DummyValuePtr curValue(readList());
		token = readToken();
		if (token != TokenType::TOKEN_RIGHT_PAREN) {
			Error("unexpected token=%d char=%c, index=%d expected \)", token, getCurChar(), index);
		} else {
			index++;
		}
		return curValue;
		break;
	}
	default:
		Error("unexpected token %d", headType);
		break;
	}
	
	return DummyValuePtr();
}

/*
	LIST = P LISTP	
	It's different from readListP with different return value
*/
DummyValuePtr Tokenize::readList()
{
	headType = readToken();
	switch(headType)
	{
	case TokenType::TOKEN_RIGHT_PAREN:
		// )
		// do nothing
		break;
	default: {
		DummyValueList list;
		list.push_back(readP());
		
		DummyValueList listP = readListP();
		if (listP.size() > 0) {
			list.insert(list.end(), listP.begin(), listP.end());
		}
		return DummyValuePtr(new DummyValue(list));
		break;
	}
	}

	return DummyValuePtr();
}

/*
	LISTP = P LISTP		
*/
DummyValueList Tokenize::readListP()
{
	DummyValueList list;	
	headType = readToken();
	switch(headType)
	{
	case TokenType::TOKEN_RIGHT_PAREN:
		// )
		// DO nothing
		break;	
	default: {
		list.push_back(readP());
		DummyValueList listP = readListP();	
		if (listP.size() > 0) {
			list.insert(list.end(), listP.begin(), listP.end());
		}	
		break;
	}
	}

	return list;
}

bool Tokenize::isNum()
{
	return input[index] >= '0' && input[index] <= '9';
}

DummyValuePtr Tokenize::readStr()
{
	if (input[index] != '\"') {
		Error("begin expect token \" actually %c index %d", input[index], index);
	}
	
	index++;
	
	std::stringstream str;
	while(index < input.length() && input[index] != '\"') {
		str << input[index++];
	}
	
	if (input[index] != '\"') {
		Error("end expect token \" actually %c index %d", input[index], index);
	}
	
	index++;
	
	return DummyValuePtr(new DummyValue(DummyType::DUMMY_STRING, str.str()));	
}

DummyValuePtr Tokenize::readNum()
{
	char num[20] = {0};
	int temp = 0;
	
	bool breakFlag = false;
	while(index < input.length() && !breakFlag)
	{
		switch(input[index])
		{
		CASE_NUM:
			num[temp++] = input[index++];
			break;
		default:
			breakFlag = true;
			break;
		}
	}
	int val = 0;
	sscanf(num, "%d", &val);

	return DummyValuePtr(new DummyValue(val));
}

bool Tokenize::isBlank()
{
	switch(input[index])
	{
	case ' ':
	case '\t':
	case '\n':
	case '\r':
		return true;
	default:
		return false;
	}

	return false;
}

void Tokenize::skipBlank()
{
	while(index < input.size() && isBlank())
		index++;
	
	if(index >= input.size()) {
		Error("something is error index outof range %d", input.size());
	}
}

DummyValuePtr Tokenize::readSymbol()
{
	std::stringstream symbol;
	while(index < input.length() && !isBlank()) {
		char c = input[index];
		if (c == ')')
			break;
		// TODO: other wrong character
		symbol << input[index++];	
	}

	std::string symStr = symbol.str();
	if (0 == symStr.compare(DummyValue::nil->getSymbol())) {
		return DummyValue::nil;
	} else if (0 == symStr.compare(DummyValue::t->getSymbol())) {
		return DummyValue::t;
	} else {
		return DummyValuePtr(new DummyValue(DummyType::DUMMY_SYMBOL, symStr));
	}
}
