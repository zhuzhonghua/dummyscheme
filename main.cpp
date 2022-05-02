#include <cstdio>
#include <string>
#include <iostream>

#include "dummyscheme.h"
#include "tokenize.h"
#include "env.h"
#include "value.h"

int read(std::string &input)
{
	if(getline(std::cin, input))
		return input.size();
	else
		return 0;
}

std::string eval(std::string &input, DummyScheme::DummyEnvPtr env)
{
	DummyScheme::Tokenize tokenize(input);
	tokenize.run(env);
	
	return "yes";
}

void print(std::string &result)
{
	printf("%s\n", result.c_str());
}

int main()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));
	
	printf("dummyscheme\n");
	while(true) {
		try {
			printf("user>");
			std::string input;	
			if(!read(input))
				break;
			std::string result = eval(input, env);	
			print(result);
		}	catch(std::string &exception) {
			printf("%s\n", exception.c_str());	
		}
	}
	
	return 0;
}
