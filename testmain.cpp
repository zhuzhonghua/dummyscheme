#include <cstdio>
#include <string>
#include <iostream>

#include "dummyscheme.h"
#include "tokenize.h"
#include "env.h"
#include "value.h"

using namespace DummyScheme;

typedef void (*TestFunc)();

void runtest(TestFunc testfunc, char* funcname)
{
	testfunc();
	printf("%s run success\n", funcname);
}

#define RUNTEST(func) runtest(func, #func)

#define ISEQUAL(a, b, env) if (!a->isEqualValue(b, env)) throw a->toString() + "not equal" + b->toString();
#define ISTOKENIZEEQUAL(t, env, b) ISEQUAL(t.run(env), b, env)

DummyValueList constructList(int n, ...)
{
	DummyValueList list;
	va_list ap;
	va_start(ap, n);
	for (int i = 0; i < n; i++)
	{
		list.push_back(va_arg(ap, DummyValuePtr));
	}
	va_end(ap);

	return list;
}

#define LISTVALUE(...) listValue(constructList(__VA_ARGS__))
#define LIST(...) constructList(__VA_ARGS__)

void test1()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(define a 2)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(2));
	
	tokenize.init("(+ a 3)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(5));

	tokenize.init("(+ 2 (- 3 2) 4)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(7));

	tokenize.init("a");
	ISTOKENIZEEQUAL(tokenize, env, numValue(2));

	tokenize.init("(define b (+ a 2))");
	ISTOKENIZEEQUAL(tokenize, env, numValue(4));
	
	tokenize.init("(* a b)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(8));
}

void test2()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(let ((c 2)) c)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(2));
	
	tokenize.init("(let ((c 2) (d 3)) (* c d))");
	ISTOKENIZEEQUAL(tokenize, env, numValue(6));	
	
	tokenize.init("(define a 4)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(4));		
	
	tokenize.init("(let ((c 2) (d 3)) (+ c d a))");
	ISTOKENIZEEQUAL(tokenize, env, numValue(9));
}

void test3()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("nil");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::nil);
	
	tokenize.init("#t");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);
	
	tokenize.init("#f");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);
}

void test4()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(begin (+ 2 3) (- 4 5))");
	ISTOKENIZEEQUAL(tokenize, env, numValue(-1));
}

void test5()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(if nil 1 2)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(2));
	
	tokenize.init("(if #t 1 2)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(1));

	tokenize.init("(if nil 1 2 3)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(3));
	
	tokenize.init("(if nil 1)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::nil);
	
	tokenize.init("(when #t 1 2)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(2));
	
	tokenize.init("(when nil 1 2)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::nil);
	
	tokenize.init("(when #f 1 2)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::nil);
	
	tokenize.init("(unless #f 1 2)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(2));
	
	tokenize.init("(unless #t 1 2)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::nil);
}

void test6()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(lambda (a) (+ a 2))");
	tokenize.run(env);
	
	tokenize.init("((lambda (a) (+ a 2)) 4)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(6));

	tokenize.init("(apply (lambda (a) (+ a 2)) 4)");
	ISTOKENIZEEQUAL(tokenize, env, numValue(6));
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
	ISTOKENIZEEQUAL(tokenize, env, numValue(4));

	tokenize.init("(list a 3 4 5)");
	tokenize.run(env);
	
	tokenize.init("(list? (list a 3 4 5))");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);

	tokenize.init("(define b (list 4 5 6))");
	tokenize.run(env);	

	tokenize.init("(list? b (list a 3 4 5))");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);

	tokenize.init("(list? nil b)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);

	tokenize.init("(equal? 1 2)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);

	tokenize.init("(equal? 1 1 1)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);
	
	tokenize.init("(equal? (list 1 2 3) (list 1 2 3))");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);

	tokenize.init("(equal? (list 1 2 3) 1)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);
}

void test9()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(null? (list 1 2 3))");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);
	
	tokenize.init("(null? (list))");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);

	tokenize.init("(length (list 1 2 3))");
	ISTOKENIZEEQUAL(tokenize, env, numValue(3));
}

void test10()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(= 1 1)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);	
	
	tokenize.init("(not nil #f)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);		
	
	tokenize.init("(not #t)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);
	
	tokenize.init("(= 1 1 3)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);
	
	tokenize.init("(< 1 2 3 4 5)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);
	
	tokenize.init("(< 1 1 3 4 5)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);
	
	tokenize.init("(<= 1 2 3 4 4)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);
	
	tokenize.init("(<= 1 2 5 4 4)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);

	tokenize.init("(> 5 4 3 2 1)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);
	
	tokenize.init("(> 5 4 3 2 2)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);

	tokenize.init("(>= 4 4 3 2 1)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::t);
	
	tokenize.init("(>= 4 4 3 2 4)");
	ISTOKENIZEEQUAL(tokenize, env, DummyValue::f);
}

void test11()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(load \"testload.scm\")");
	tokenize.run(env);
}

void test12()
{
	DummyScheme::DummyEnvPtr env(new DummyScheme::DummyEnv(NULL));	
	DummyScheme::Tokenize tokenize("(quote abc)");
	tokenize.run(env);

	tokenize.init("(quote (1 2 3))");
	tokenize.run(env);	

	tokenize.init("'abc");
	tokenize.run(env);	
	
	tokenize.init("'(1 2 3)");
	tokenize.run(env);

	tokenize.init("(define lst (quote (1 2 3)))");
	tokenize.run(env);

	tokenize.init("(quasiquote (a lst d))");
	tokenize.run(env);

	tokenize.init("(quasiquote (a (unquote lst) d))");
	tokenize.run(env);
	
	tokenize.init("(quasiquote (a (unquote-splicing lst) d))");
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
		RUNTEST(test11);
		RUNTEST(test12);

		printf("test all success");
	}
	catch(std::string &excep) {
		printf(excep.c_str());

		printf("test failed");
//		char c;
//		std::cin >> c;
	}
}
