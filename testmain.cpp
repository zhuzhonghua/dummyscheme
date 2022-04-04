#include <cstdio>
#include <string>
#include <iostream>

#include "dummyscheme.h"
#include "tokenize.h"
#include "env.h"

typedef void (*TestFunc)();

void runtest(TestFunc testfunc, char* funcname)
{
	try {
		testfunc();
		printf("%s run success\n", funcname);	
	}	catch(const char* exception) {
		printf("%s Failed with exception %s\n", funcname, exception);	
	}
}

#define RUNTEST(func) runtest(func, #func)

void test1()
{
	DummyEnvPtr env(new DummyEnv(NULL));	
	Tokenize tokenize("(define a 2)");
	tokenize.run(env);
	
	tokenize.init("(+ a 3)");
	tokenize.run(env);
}

void test2()
{
	DummyEnvPtr env(new DummyEnv(NULL));	
	Tokenize tokenize("(+ 2 (- 3 2) 4)");
	tokenize.run(env);
}

int main()
{
	Tokenize::init();
	RUNTEST(test1);
	RUNTEST(test2);
}
