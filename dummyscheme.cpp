#include "dummyscheme.h"

#define CASE_NUM case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
#define CASE_SYMBOL case '+':case '-':case '*':case '/'

void Tokenize::unexpectedToken()
{
		std::stringstream ss;
		ss << "unexpected token "
			 << input[index]
			 << " at " << index;
		throw ss.str().c_str();
}

Tokenize::Tokenize(std::string &input)
{
	this->input = input;
	this->index = 0;

	try {
		readP();
	}	catch(const char* exception) {
		printf(exception);
	}
}

TokenType Tokenize::readToken()
{
	skipBlank();
	switch(input[index])
	{
	CASE_NUM:
		return TokenType::TOKEN_NUM;
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
	P = NUM | LEFT_PAREN LIST RIGHT_PAREN
*/
void Tokenize::readP()
{
	headType = readToken();
	TokenType token;	
	switch(headType)
	{
	case TokenType::TOKEN_NUM:
		readNum();
		break;
	case TokenType::TOKEN_LEFT_PAREN:
		index++;
		readList();
		token = readToken();
		if (token != TokenType::TOKEN_RIGHT_PAREN)
			unexpectedToken();
		else
			index++;
		break;
	default:
		unexpectedToken();
		break;
	}
}

/*
	LIST = SYMBOL LISTP	
*/
void Tokenize::readList()
{
	headType = readToken();
	switch(headType)
	{
	case TokenType::TOKEN_SYMBOL:
		readSymbol();
		readListP();
		break;
	default:
		unexpectedToken();
		break;
	}
}

/*
	LISTP = P LISTP		
*/
void Tokenize::readListP()
{
	headType = readToken();
	switch(headType)
	{
	case TokenType::TOKEN_NUM:
	case TokenType::TOKEN_LEFT_PAREN:
		readP();
		readListP();
		break;
	case TokenType::TOKEN_RIGHT_PAREN:
		// DO nothing
		break;
	default:
		unexpectedToken();
		break;
	}
}

bool Tokenize::isNum()
{
	return input[index] >= '0' && input[index] <= '9';
}

int Tokenize::readNum()
{
	char num[20] = {0};
	int temp = 0;
	
	while(index < input.length())
	{
		switch(input[index])
		{
		CASE_NUM:
			num[temp++] = input[index++];
			break;
		default:
			return index;
		}
	}
	int val = 0;
	sscanf(num, "%d", &val);

	return index;
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

void Tokenize::readSymbol()
{
	std::stringstream symbol;
	while(index < input.length() && !isBlank())
		symbol << input[index++];	
}

