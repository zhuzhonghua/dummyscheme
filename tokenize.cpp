#include "tokenize.h"
#include "env.h"
#include "value.h"
#include "core.h"

using namespace DummyScheme;

#define CASE_NUM case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
#define CASE_SYMBOL case '+':case '-':case '*':case '/':case '#':case '?':case '>':case '=':case '<':case '_'

#define AssertToken(condition) \
Assert(condition, "unexpected char=%c, index=%d", input[index], index)

#define AssertInputEnd(idx) \
Assert(idx < input.size(), "reached end of input")

Tokenize::Tokenize(const std::string &input)
{
	init(input);
}

void Tokenize::init(const std::string &input)
{
	this->input = input;
	this->index = 0;
}

/*
	read dummyvalue and evaluate it
 */
DummyValuePtr Tokenize::run(DummyEnvPtr env)
{
	DummyValuePtr val = readP();
	Print("%s\n", val->toString().c_str());
		
	DummyValuePtr evalVal = DummyCore::Eval(val, env);
	Print("%s\n", evalVal->toString().c_str());
	return evalVal;
}

/*
	look at token type
 */
TokenType Tokenize::look()
{
	AssertInputEnd(index);
	char c = input[index];
	switch(c)
	{
	CASE_NUM:
		return TokenType::TOKEN_NUM;
		break;
	case '\"':
		return TokenType::TOKEN_DOUBLE_QUOTE;
		break;
	case '(':
		return TokenType::TOKEN_LEFT_PAREN;
		break;
	case ')':
		return TokenType::TOKEN_RIGHT_PAREN;	
		break;
	case '\'':
		return TokenType::TOKEN_SINGLE_QUOTE;
		break;
	case '`':
		return TokenType::TOKEN_QUASIQUOTE;
		break;
	case '~':{
		AssertInputEnd(index + 1);
		if ('@' == input[index + 1])
			return TokenType::TOKEN_UNQUOTE_SPLICING;
		else
			return TokenType::TOKEN_UNQUOTE;
		break;
	}
	CASE_SYMBOL:
		return TokenType::TOKEN_SYMBOL;
		break;
	default:
		if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
			return TokenType::TOKEN_SYMBOL;
		}	else {
			return TokenType::TOKEN_UNKNOWN;	
		}
		break;
	}

	return TokenType::TOKEN_UNKNOWN;	
}

/*
	skip blank and look the next token
 */
TokenType Tokenize::readToken()
{
	// TODO: call skip separately
	skipBlank();
	return look();
}

/*
	'(1 2 3)
	'abc
	TODO: support char like 'c'
 */
DummyValuePtr Tokenize::readQuote()
{
	// TODO: how to detect this error ' abc	
	// DONE
	DummyValueList list;
	list.push_back(symbolValue("quote"));
	index++;
	TokenType token = look();
	AssertToken(token == TokenType::TOKEN_LEFT_PAREN || token == TokenType::TOKEN_SYMBOL);

	list.push_back(readP());
		
	// TODO: straightly create the dummyvalue with type
	return DummyCore::Compile(list);
}

/*
	~obj
	~(+ 1 2)
 */
DummyValuePtr Tokenize::readUnQuote()
{
	DummyValueList list;
	list.push_back(symbolValue("unquote"));
	index++;
	TokenType token = look();
	AssertToken(token == TokenType::TOKEN_LEFT_PAREN || token == TokenType::TOKEN_SYMBOL);

	list.push_back(readP());
		
	// TODO: straightly create the dummyvalue with type
	return DummyCore::Compile(list);
}

/*
	~@lst
 */
DummyValuePtr Tokenize::readUnQuoteSplicing()
{
	DummyValueList list;
	list.push_back(symbolValue("unquote-splicing"));
	index += 2;
	TokenType token = look();
	AssertToken(token == TokenType::TOKEN_LEFT_PAREN || token == TokenType::TOKEN_SYMBOL);

	list.push_back(readP());
		
	// TODO: straightly create the dummyvalue with type
	return DummyCore::Compile(list);
}

/*
	`lst
	`(a b c)
 */
DummyValuePtr Tokenize::readQuasiQuote()
{
	DummyValueList list;
	list.push_back(symbolValue("quasiquote"));
	index++;
	TokenType token = look();
	AssertToken(token == TokenType::TOKEN_LEFT_PAREN || token == TokenType::TOKEN_SYMBOL);

	list.push_back(readP());
		
	// TODO: straightly create the dummyvalue with type
	return DummyCore::Compile(list);
}

/*
	P = NUM | STRING | SYMBOL | QUOTE | UNQUOTE | UNQUOTE_SPLICING | QUASIQUOTE | LEFT_PAREN LIST RIGHT_PAREN
	add check for p at the first pass
*/
DummyValuePtr Tokenize::readP()
{
	TokenType headType = readToken();
	switch(headType)
	{
	case TokenType::TOKEN_NUM:
		return readNum();
	case TokenType::TOKEN_DOUBLE_QUOTE:
		return readStr();
	case TokenType::TOKEN_SYMBOL:
		return readSymbol();
	case TokenType::TOKEN_SINGLE_QUOTE:
		return readQuote();
	case TokenType::TOKEN_UNQUOTE:
		return readUnQuote();
	case TokenType::TOKEN_QUASIQUOTE:
		return readQuasiQuote();
	case TokenType::TOKEN_UNQUOTE_SPLICING:
		return readUnQuoteSplicing();
	case TokenType::TOKEN_LEFT_PAREN:{
		index++;
		DummyValuePtr curValue(readList());
		TokenType token = readToken();
		AssertToken(token == TokenType::TOKEN_RIGHT_PAREN);
		index++;
		return curValue;
		break;
	}
	default:{
		Error("unexpected token %d", headType);
		return DummyValue::nil;
		break;
	}
	}	
}

/*
	LIST = P LISTP	
	It's different from readListP with different return value
*/
DummyValuePtr Tokenize::readList()
{
	TokenType headType = readToken();
	switch(headType){
	case TokenType::TOKEN_RIGHT_PAREN:
		// )
		// do nothing
		break;
	default: {
		DummyValueList list;
		list.push_back(readP());
		
		DummyValueList listP = readListP();
		if (listP.size() > 0)
			list.insert(list.end(), listP.begin(), listP.end());
		
		return DummyCore::Compile(list);
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
	switch(headType){
	case TokenType::TOKEN_RIGHT_PAREN:
		// )
		// DO nothing
		break;	
	default: {
		list.push_back(readP());
		
		DummyValueList listP = readListP();	
		if (listP.size() > 0)
			list.insert(list.end(), listP.begin(), listP.end());
		
		break;
	}
	}

	return list;
}

/*
	read a str
	TODO: read doc
 */
DummyValuePtr Tokenize::readStr()
{
	index++;
	
	std::stringstream str;
	while(index < input.length() && input[index] != '\"')
		str << input[index++];
	
	AssertInputEnd(index);
//	AssertToken(input[index] == '\"');
	
	index++;
	
	return strValue(str.str());
}

/*
	TODO:	currently only support int
 */
DummyValuePtr Tokenize::readNum()
{
	// TODO: read other nums
	char num[20] = {0};
	int temp = 0;
	
	bool breakFlag = false;
	while(index < input.length() && !breakFlag)
	{
		switch(input[index]){
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

	return numValue(val);
}

bool Tokenize::isBlank()
{
	switch(input[index]){
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	case '\0':
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
	
	AssertInputEnd(index);
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
	if (DummyValue::nil->isEqualString(symStr))
		return DummyValue::nil;
	else if (DummyValue::t->isEqualString(symStr))
		return DummyValue::t;
	else if (DummyValue::f->isEqualString(symStr))
		return DummyValue::f;
	else
		return symbolValue(symStr);
}
