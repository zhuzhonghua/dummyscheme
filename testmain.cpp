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
		throw "failed test";
	}
}

#define RUNTEST(func) runtest(func, #func)

void test1()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(define a 2)");
	tokenize.run(env);
	
	tokenize.init("(+ a 3)");
	tokenize.run(env);
}

void test2()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(+ 2 (- 3 2) 4)");
	tokenize.run(env);
}

void test3()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(define a 3)");
	tokenize.run(env);
	
	tokenize.init("a");
	tokenize.run(env);
}

void test4()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(define a 3)");
	tokenize.run(env);
	
	tokenize.init("(define b (+ a 2))");
	tokenize.run(env);
	
	tokenize.init("(* a b)");
	tokenize.run(env);
}

void test5()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(let ((c 2)) c)");
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
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("nil");
	tokenize.run(env);
	
	tokenize.init("t");
	tokenize.run(env);
}

void test7()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(begin (+ 2 3) (- 4 5))");
	tokenize.run(env);
}

void test8()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(if nil 1 2)");
	tokenize.run(env);
	
	tokenize.init("(if t 1 2)");
	tokenize.run(env);	

	tokenize.init("(if nil 1 2 3)");
	tokenize.run(env);		
	
	tokenize.init("(if nil 1)");
	tokenize.run(env);
}

void test9()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(lambda (a) (+ a 2))");
	tokenize.run(env);
	
	tokenize.init("((lambda (a) (+ a 2)) 4)");
	tokenize.run(env);

	tokenize.init("(apply (lambda (a) (+ a 2)) 4)");
	tokenize.run(env);	
}

int main()
{
	try{
		DummyScheme::init();
		RUNTEST(test1);
		RUNTEST(test2);
		RUNTEST(test3);
		RUNTEST(test4);
		RUNTEST(test5);
		RUNTEST(test6);
		RUNTEST(test7);
		RUNTEST(test8);
		RUNTEST(test9);
	}
	catch(const char* excep) {
		printf(excep);
	}
}
