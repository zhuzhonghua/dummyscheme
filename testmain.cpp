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

void test3()
{
	DummyEnvPtr env(new DummyEnv(NULL));	
	Tokenize tokenize("(define a 3)");
	tokenize.run(env);
	
	tokenize.init("a");
	tokenize.run(env);
}

void test4()
{
	DummyEnvPtr env(new DummyEnv(NULL));	
	Tokenize tokenize("(define a 3)");
	tokenize.run(env);
	
	tokenize.init("(define b (+ a 2))");
	tokenize.run(env);
	
	tokenize.init("(* a b)");
	tokenize.run(env);
}

void test5()
{
	DummyEnvPtr env(new DummyEnv(NULL));	
	Tokenize tokenize("(let ((c 2)) c)");
	tokenize.run(env);
	
	tokenize.init("(let ((c 2) (d 3)) (* c d))");
	tokenize.run(env);
	
	tokenize.init("(define a 4)");
	tokenize.run(env);	
	
	tokenize.init("(let ((c 2) (d 3)) (+ c d a))");
	tokenize.run(env);
}

void test6()
{
	DummyEnvPtr env(new DummyEnv(NULL));	
	Tokenize tokenize("nil");
	tokenize.run(env);
	
	tokenize.init("t");
	tokenize.run(env);
}

int main()
{
	Tokenize::init();
	RUNTEST(test1);
	RUNTEST(test2);
	RUNTEST(test3);
	RUNTEST(test4);
	RUNTEST(test5);
	RUNTEST(test6);
}
