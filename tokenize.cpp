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

Tokenize::Tokenize(const String &input)
	:shareEnv(new DummyShareEnv())
{
	init(input);
}

void Tokenize::init(const String &input)
{
	this->input = input;
	this->index = 0;

	aheadToken = dLex();
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

void Tokenize::match(int type)
{
	if (type == aheadToken)
		aheadToken = dLex();
	else
		Error("unrecognized token type %d with current %d", type, aheadToken);
}

int Tokenize::dLex()
{
	while(index < input.size() && isBlank())
		index++;
	
	if (index >= input.size())
		return TOKEN_END;
	
	// no need index++
	char c = input[index];
	switch(c){
	CASE_NUM:
		return readNum();
	CASE_SYMBOL:
		return readSymbol();
	case '\"':
		return readStr();
	default:
		if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
			return readSymbol();
	}
	
	// need index++
	switch(input[index++]){
	case '(':
		return TOKEN_LEFT_PAREN;
	case ')':
		return TOKEN_RIGHT_PAREN;	
	case '\'':
		return TOKEN_SINGLE_QUOTE;
	case '`':
		return TOKEN_QUASIQUOTE;
		break;
	case '~':{
		AssertInputEnd(index);
		// need to separate ++ or not
		if ('@' == input[index])
		{
			index++;
			return TOKEN_UNQUOTE_SPLICING;
		}
		else
			return TOKEN_UNQUOTE;
	}
	default:
		return TOKEN_UNKNOWN;	
	}
}

DummyValuePtr Tokenize::readQuoteRelate(int type, const char* typeStr)
{
	Assert(!isBlank(), "illegal");
	match(type);
	
	DummyValueList list;
	list.push_back(symbolValue(typeStr));
	
	switch (aheadToken){
	case TOKEN_LEFT_PAREN:
	case TOKEN_SYMBOL:
		list.push_back(readP());
		break;
	default:
		Error("unknown tokentype %d", aheadToken);
		break;
	}
		
	// TODO: straightly create the dummyvalue with type
	return DummyCore::Compile(list);
}

/*
	P = NUM | STRING | SYMBOL | QUOTE | UNQUOTE | UNQUOTE_SPLICING | QUASIQUOTE | LEFT_PAREN LIST RIGHT_PAREN
	add check for p at the first pass
*/
DummyValuePtr Tokenize::readP()
{
	DummyValuePtr val;
	switch(aheadToken){
	case TOKEN_NUM:
		val = numValue(numLexVal);
		match(TOKEN_NUM);
		return val;
	case TOKEN_STRING:
		val = strValue(strLexVal);
		match(TOKEN_STRING);
		return val;
	case TOKEN_SYMBOL:
		val = getShareSymbol(strLexVal);
		match(TOKEN_SYMBOL);
		return val;
	case TOKEN_SINGLE_QUOTE:
		/*
			'(1 2 3)
			'abc
			TODO: support char like 'c'
		*/
		val = readQuoteRelate(TOKEN_SINGLE_QUOTE, "quote");
		return val;
	case TOKEN_UNQUOTE:
		/*
			~obj
			~(+ 1 2)
		*/
		val = readQuoteRelate(TOKEN_UNQUOTE, "unquote");
		return val;
	case TOKEN_QUASIQUOTE:
		/*
			`lst
			`(a b c)
		*/
		val = readQuoteRelate(TOKEN_QUASIQUOTE, "quasiquote");
		return val;
	case TOKEN_UNQUOTE_SPLICING:
		/*
			~@lst
		*/
		val = readQuoteRelate(TOKEN_UNQUOTE_SPLICING, "unquote-splicing");
		return val;
	case TOKEN_LEFT_PAREN:{
		match(TOKEN_LEFT_PAREN);
		val = readList();
		match(TOKEN_RIGHT_PAREN);
		
		if (!val)
			return DummyValue::nil;
		else
			return val;
	}
	default:{
		AssertToken(false);
		return DummyValue::nil;
	}
	}
}

/*
	LIST = P LISTP	
	It's different from readListP with different return value
*/
DummyValuePtr Tokenize::readList()
{
	switch(aheadToken){
	case TOKEN_RIGHT_PAREN:
		// )
		// do nothing
		return NULL;
	default: {
		DummyValueList list;
		list.push_back(readP());
		
		DummyValueList listP = readListP();
		if (listP.size() > 0)
			list.insert(list.end(), listP.begin(), listP.end());
		
		return DummyCore::Compile(list);
	}
	}
}

/*
	LISTP = P LISTP		
*/
DummyValueList Tokenize::readListP()
{
	DummyValueList list;	
	switch(aheadToken){
	case TOKEN_RIGHT_PAREN:
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
	share same symbol dummyvalue
 */
DummyValuePtr Tokenize::getShareSymbol(const String &symbol)
{
	if (DummyValue::nil->isEqualString(symbol))
		return DummyValue::nil;
	else if (DummyValue::t->isEqualString(symbol))
		return DummyValue::t;
	else if (DummyValue::f->isEqualString(symbol))
		return DummyValue::f;
	else
	{
		DummyValuePtr val = shareEnv->get(symbol);
		if (val != DummyValue::nil)
			return val;
		
		val = symbolValue(symbol);
		shareEnv->add(symbol, val);
		return val;
	}
}

/*
	read a str
	TODO: read doc
 */
int Tokenize::readStr()
{
	index++;
	
	StringStream str;
	while(index < input.length() && input[index] != '\"')
		str << input[index++];
	
	AssertInputEnd(index);
	
	index++;

	strLexVal = str.str();
	
	return TOKEN_STRING;
}

/*
	TODO:	currently only support int
 */
int Tokenize::readNum()
{
	// TODO: read float
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
	sscanf(num, "%d", &numLexVal);

	return TOKEN_NUM;
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

int Tokenize::readSymbol()
{
	StringStream symbol;
	bool breakFlag = false;
	while(index < input.length())
	{
		char c = input[index];
		switch(c){
		CASE_SYMBOL:
			symbol << c;
			break;
		default:
			if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
				symbol << c;
			else
				breakFlag = true;
		}
		if (breakFlag)
			break;
		else
			index++;
	}

	strLexVal = symbol.str();

	return TOKEN_SYMBOL;
}
