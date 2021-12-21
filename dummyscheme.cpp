#include "dummyscheme.h"

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
