#include "tokenize.h"
#include "env.h"
#include "value.h"
#include "core.h"

using namespace DummyScheme;

#define CASE_NUM case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
#define CASE_SYMBOL case '+':case '-':case '*':case '/':case '#':case '?':case '>':case '=':case '<'

Tokenize::Tokenize(const std::string &input)
{
	init(input);
}

void Tokenize::init(const std::string &input)
{
	this->input = input;
	this->index = 0;
}

DummyValuePtr Tokenize::run(DummyEnvPtr env)
{
	DummyValuePtr val = readP();
	Print("%s\n", val->toString().c_str());
		
	DummyValuePtr evalVal = DummyCore::Eval(val, env);
	Print("%s\n", evalVal->toString().c_str());
	return evalVal;
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
	TokenType headType = readToken();
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
		TokenType token = readToken();
		Assert(token == TokenType::TOKEN_RIGHT_PAREN, "unexpected token=%d char=%c, index=%d expected \)", token, input[index], index);
		index++;
		return curValue;
		break;
	}
	default:
		Error("unexpected token %d", headType);
		return DummyValue::nil;
		break;
	}	
}

/*
	LIST = P LISTP	
	It's different from readListP with different return value
*/
DummyValuePtr Tokenize::readList()
{
	TokenType headType = readToken();
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
		return DummyValue::create(list);
		break;
	}
	}
}

/*
	LISTP = P LISTP		
*/
DummyValueList Tokenize::readListP()
{
	DummyValueList list;	
	TokenType headType = readToken();
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
	if (DummyValue::nil->isSame(symStr)) {
		return DummyValue::nil;
	} else if (DummyValue::t->isSame(symStr)) {
		return DummyValue::t;
	} else if (DummyValue::f->isSame(symStr)) {
		return DummyValue::f;
	} else {
		return DummyValuePtr(new DummyValue(DummyType::DUMMY_SYMBOL, symStr));
	}
}
