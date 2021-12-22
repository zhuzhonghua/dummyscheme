#include "dummyscheme.h"

void unexpectedToken(std::string &input, int startIndex)
{
		std::stringstream ss;
		ss << "unexpected token "
			 << input[startIndex]
			 << " at " << startIndex;
		printf(ss.str());
}

int skipBlank(std::string &input, int startIndex)
{
	while(startIndex < input.size())
	{
		switch(input[startIndex])
		{
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			startIndex++;
			break;
		default:
			return startIndex;
		}
	}

	return -1;
}

int readSymbol(std::string &input, int startIndex)
{
	std::stringstream symbol;
	if (!(input[startIndex] >= 'a' && input[startIndex] <= 'z') &&
			!(input[startIndex] >= 'A' && input[startIndex] <= 'Z'))
	{
		unexpectedToken(input, startIndex);
		return -1;
	}

	symbol << input[startIndex++];
	while((input[startIndex] >= 'a' && input[startIndex] <= 'z') ||
				(input[startIndex] >= 'A' && input[startIndex] <= 'Z') ||
				(input[startIndex] >= '0' && input[startIndex] <= '9') ||
				input[startIndex] == '-')
	{
		symbol << input[startIndex++];
	}

	return startIndex;
}

int readForm(std::string &input, int startIndex)
{
	if (input[startIndex] != TOKEN_LEFT_PAREN)
	{
		unexpectedToken(input, startIndex);
		return -1;
	}

	startIndex++;
	startIndex = skipBlank(input, startIndex);
	startIndex = readSymbol(input, startIndex);
}
