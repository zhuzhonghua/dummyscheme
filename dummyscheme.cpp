#include "dummyscheme.h"

#define CASE_NUM case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'

void unexpectedToken(std::string &input, int startIndex)
{
		std::stringstream ss;
		ss << "unexpected token "
			 << input[startIndex]
			 << " at " << startIndex;
		printf(ss.str());
}

Tokenize::Tokenize(std::string &input)
{
	this->input = input;
	this->index = 0;
}

/*
NUM | LEFT FORM RIGHT
*/
void Tokenize::read()
{
	index = skipBlank();
	switch(input[index])
	{
	case TOKEN_LEFT_PAREN:
		//readForm(input, index);
		break;
	CASE_NUM:
		readNum();
		break;
	default:{
		std::stringstream ss;
		ss << "unexpected token " << input[index] << " at " << index;	
		return ss.str();
	}
	}
	return input;
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

int Tokenize::skipBlank()
{
	while(index < input.size())
	{
		switch(input[index])
		{
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			index++;
			break;
		default:
			return index;
		}
	}

	return -1;
}

int Tokenize::readSymbol()
{
	std::stringstream symbol;
	if (!(input[index] >= 'a' && input[index] <= 'z') &&
			!(input[index] >= 'A' && input[index] <= 'Z'))
	{
		unexpectedToken(input, index);
		return -1;
	}

	symbol << input[index++];
	while((input[index] >= 'a' && input[index] <= 'z') ||
				(input[index] >= 'A' && input[index] <= 'Z') ||
				(input[index] >= '0' && input[index] <= '9') ||
				input[index] == '-')
	{
		symbol << input[index++];
	}

	return index;
}

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
