#include "tokenize.h"
#include "env.h"
#include "value.h"

#define CASE_NUM case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
#define CASE_SYMBOL case '+':case '-':case '*':case '/'

void errorThrow(const char *fmt, ...)
{
	char buffer[512] = {0};
	va_list args;
	va_start(args, fmt);	
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	perror(buffer);
	va_end(args);

	// don't throw buffer
	throw "error happended";
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

DummyValuePtr OpFuncDefine(DummyValuePtr value, DummyEnvPtr env)
{
	DummyValueList list = value->getList(); 
	
	// first is the symbol	
	DummyValueList::iterator itr = list.begin();	
	// second is the symbol
	DummyValuePtr second = *(++itr);	
	if (second->isSymbol()) {
		DummyValuePtr symbolValue = *(++itr);	
		env->set(second->getSymbol(), symbolValue);

		return DummyValuePtr(new DummyValue(symbolValue));
	} else {
		Error("didn't support type %d", second->getType());
	}

	return DummyValuePtr();
}

OpMap Tokenize::opMap;

void Tokenize::init()
{
	opMap["+"] = OpFuncPlus;
	opMap["-"] = OpFuncMinus;
	opMap["*"] = OpFuncMul;
	opMap["/"] = OpFuncDivide;
	opMap["define"] = OpFuncDefine;
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
	OpMap::iterator it = opMap.find(symbol);
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


void Tokenize::toString(std::stringstream& out, DummyValuePtr val)
{
	switch(val->type) {
	case DUMMY_INT_NUM:
		out << val->basic.intnum;
		break;
	case DUMMY_FLOAT_NUM:
		out << val->basic.floatnum;
		break;
	case DUMMY_SYMBOL:
		out << val->strAndSymbol;	
		break;
	case DUMMY_STRING:
		out << "\"" << val->strAndSymbol << "\"";
		break;
	case DUMMY_LIST:{
		out << "(";
		for (DummyValueList::iterator itr = val->list.begin();
				 itr != val->list.end(); itr++) {
			toString(out, *itr);
			if (itr + 1 != val->list.end()) {
				out << " ";	
			}
		}
		out << ")";
		break;
	}
	default:
		Error("unknown type %d", val->type);
		break;
	}
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
	std::stringstream out;
	toString(out, val);
	printf("%s\n", out.str().c_str());
		
	DummyValuePtr evalVal = val->eval(env);
	std::stringstream evalOut;	
	toString(evalOut, evalVal);
	printf("%s\n", evalOut.str().c_str());
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
			return TokenType::TOKEN_UNKNOWN;
		}
	}

	return TokenType::TOKEN_UNKNOWN;
}

/*
	P = NUM | STRING | LEFT_PAREN LIST RIGHT_PAREN
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
			Error("unexpected token %d expected \)", token);
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
	LIST = SYMBOL LISTP	
*/
DummyValuePtr Tokenize::readList()
{
	headType = readToken();
	switch(headType)
	{
	case TokenType::TOKEN_SYMBOL:{
		DummyValueList list;
		DummyValuePtr symbol = readSymbol();
		//addOpForCheck(symbol->getSymbol());
		list.push_back(symbol);
		
		DummyValueList listP = readListP();
		if (listP.size() > 0) {
			list.insert(list.end(), listP.begin(), listP.end());
		}
		return DummyValuePtr(new DummyValue(list));
		break;
	}
	default:
		Error("unexpected token %d", headType);
		break;
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
	case TokenType::TOKEN_NUM:
	case TokenType::TOKEN_LEFT_PAREN:
	case TokenType::TOKEN_SYMBOL: {
		list.push_back(readP());
		DummyValueList listP = readListP();	
		if (listP.size() > 0) {
			list.insert(list.end(), listP.begin(), listP.end());
		}
		break;
	}
	case TokenType::TOKEN_RIGHT_PAREN:
		// DO nothing
		break;
	default:
		Error("unexpected token %d", headType);
		break;
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
}

DummyValuePtr Tokenize::readSymbol()
{
	std::stringstream symbol;
	while(index < input.length() && !isBlank())
		symbol << input[index++];	

	return DummyValuePtr(new DummyValue(DummyType::DUMMY_SYMBOL, symbol.str()));
}
