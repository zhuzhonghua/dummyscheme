#include <cstdio>
#include <string>
#include <iostream>

#include "dummyscheme.h"
#include "tokenize.h"
#include "env.h"

typedef void (*TestFunc)();

void runtest(TestFunc testfunc, char* funcname)
{
	testfunc();
	printf("%s run success\n", funcname);
}

#define RUNTEST(func) runtest(func, #func)

void test1()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(define a 2)");
	tokenize.run(env);
	
	tokenize.init("(+ a 3)");
	tokenize.run(env);

	tokenize.init("(+ 2 (- 3 2) 4)");
	tokenize.run(env);	

	tokenize.init("a");
	tokenize.run(env);	

	tokenize.init("(define b (+ a 2))");
	tokenize.run(env);
	
	tokenize.init("(* a b)");
	tokenize.run(env);	
}

void test2()
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

void test3()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("nil");
	tokenize.run(env);
	
	tokenize.init("#t");
	tokenize.run(env);
	
	tokenize.init("#f");
	tokenize.run(env);	
}

void test4()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(begin (+ 2 3) (- 4 5))");
	tokenize.run(env);
}

void test5()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(if nil 1 2)");
	tokenize.run(env);
	
	tokenize.init("(if #t 1 2)");
	tokenize.run(env);	

	tokenize.init("(if nil 1 2 3)");
	tokenize.run(env);		
	
	tokenize.init("(if nil 1)");
	tokenize.run(env);
	
	tokenize.init("(when #t 1 2)");
	tokenize.run(env);	
	
	tokenize.init("(when nil 1 2)");
	tokenize.run(env);		
	
	tokenize.init("(when #f 1 2)");
	tokenize.run(env);
	
	tokenize.init("(unless #f 1 2)");
	tokenize.run(env);
	
	tokenize.init("(unless #t 1 2)");
	tokenize.run(env);
}

void test6()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(lambda (a) (+ a 2))");
	tokenize.run(env);
	
	tokenize.init("((lambda (a) (+ a 2)) 4)");
	tokenize.run(env);

	tokenize.init("(apply (lambda (a) (+ a 2)) 4)");
	tokenize.run(env);	
}

void test7()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(display \"a b c\")");
	tokenize.run(env);
	
	tokenize.init("(define a 4)");
	tokenize.run(env);

	tokenize.init("(display a)");
	tokenize.run(env);	
}

void test8()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(list 1 2 3)");
	tokenize.run(env);
	
	tokenize.init("(define a 4)");
	tokenize.run(env);

	tokenize.init("(list a 3 4 5)");
	tokenize.run(env);
	
	tokenize.init("(list? (list a 3 4 5))");
	tokenize.run(env);

	tokenize.init("(define b (list 4 5 6))");
	tokenize.run(env);	

	tokenize.init("(list? b (list a 3 4 5))");
	tokenize.run(env);	

	tokenize.init("(list? nil b)");
	tokenize.run(env);

	tokenize.init("(equal? 1 2)");
	tokenize.run(env);

	tokenize.init("(equal? 1 1 1)");
	tokenize.run(env);
	
	tokenize.init("(equal? (list 1 2 3) (list 1 2 3))");
	tokenize.run(env);	

	tokenize.init("(equal? (list 1 2 3) 1)");
	tokenize.run(env);
}

void test9()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(null? (list 1 2 3))");
	tokenize.run(env);
	
	tokenize.init("(null? (list))");
	tokenize.run(env);

	tokenize.init("(length (list 1 2 3))");
	tokenize.run(env);
}

void test10()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(= 1 1)");
	tokenize.run(env);
	
	tokenize.init("(= 1 1 3)");
	tokenize.run(env);
	
	tokenize.init("(< 1 2 3 4 5)");
	tokenize.run(env);
	
	tokenize.init("(< 1 1 3 4 5)");
	tokenize.run(env);
	
	tokenize.init("(<= 1 2 3 4 4)");
	tokenize.run(env);
	
	tokenize.init("(<= 1 2 5 4 4)");
	tokenize.run(env);		

	tokenize.init("(> 5 4 3 2 1)");
	tokenize.run(env);
	
	tokenize.init("(> 5 4 3 2 2)");
	tokenize.run(env);

	tokenize.init("(>= 4 4 3 2 1)");
	tokenize.run(env);
	
	tokenize.init("(>= 4 4 3 2 4)");
	tokenize.run(env);	
}

int main()
{
	try{
		RUNTEST(test1);
		RUNTEST(test2);
		RUNTEST(test3);
		RUNTEST(test4);
		RUNTEST(test5);
		RUNTEST(test6);
		RUNTEST(test7);
		RUNTEST(test8);
		RUNTEST(test9);
		RUNTEST(test10);

		printf("test all success");
	}
	catch(const char* excep) {
		printf(excep);

		printf("test failed");
		char c;
		std::cin >> c;
	}
}
