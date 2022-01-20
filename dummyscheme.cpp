#include "dummyscheme.h"

#define CASE_NUM case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
#define CASE_SYMBOL case '+':case '-':case '*':case '/'

void Tokenize::unexpectedToken()
{
		std::stringstream ss;
		ss << "unexpected token "
			 << input[index]
			 << " at " << index;
		printf(ss.str());
}

Tokenize::Tokenize(std::string &input)
{
	this->input = input;
	this->index = 0;

	readP();
}

TokenType Tokenize::readToken()
{
	skipBlank();
	switch(input[index])
	{
	CASE_NUM:
		return TokenType::TOKEN_NUM;
	case:'('
		return TokenType::TOKEN_LEFT_PAREN;
	case:')'
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
	TokenType token = readToken();
	switch(token)
	{
	case TokenType::TOKEN_NUM:
		readNum();
		break;
	case TokenType::TOKEN_LEFT_PAREN:
		readList();
		token = readToken();
		if (token != TokenType::TOKEN_RIGHT_PAREN)
			unexpectedToken();
		else
			index++;
		break;
	default:{
		unexpectedToken();
		break
	}
	}
}

/*
	LIST = SYMBOL LISTP	
*/
void Tokenize::readList()
{
	TokenType token = readToken();
	switch(token)
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
	readP();
	
}

bool Tokenize::isNum()
{
	return input[index] >= '0' && input[index] <= '9';
}

int Tokenize::readNum()
{
	int num = 0;
	while(index < input.length())
	{
		switch(input[index])
		{
		CASE_NUM:
			num *= 10;			
			num += (input[index++] - '0');		
			break;
		default:
			return index;
		}
	}

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

/*
 STAR BLANK 
*/
int Tokenize::readForm()
{
	switch(input[index])
	{
	case TOKEN_LEFT_PAREN:
		index++;	
		skipBlank();
		readSymbol();
		break;
	default:
		unexpectedToken(input, index);	
		return -1;
	}
	return -1;
}
