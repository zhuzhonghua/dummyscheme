#include <cstdio>
#include <string>
#include <iostream>

#include "dummyscheme.h"

int read(std::string &input)
{
	if(getline(std::cin, input))
		return input.size();
	else
		return 0;
}

std::string eval(std::string &input)
{
	Tokenize tokenize(input);
	return "yes";
}

void print(std::string &result)
{
	printf("%s\n", result.c_str());
}

int main()
{
	printf("dummyscheme\n");
	while(true) {
		printf("user>");
		std::string input;	
		if(!read(input))
			break;
		std::string result = eval(input);	
		print(result);
	}
	
	return 0;
}
