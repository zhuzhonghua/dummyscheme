#include "dummyscheme.h"

namespace Dummy {

#define MULTI(...) #__VA_ARGS__

typedef void(*TestFunc)();

typedef std::vector<TestFunc> TestFuncList;

class Test{
public:
  static TestFuncList allTestFunc;

  Test(TestFunc func){
    allTestFunc.push_back(func);
  }
};

TestFuncList Test::allTestFunc;

#define FUNCNAME(uniq) TestFunc ## uniq
#define FUNCHELPER(uniq) TestFuncHelper ## uniq

#define RUNTESTUNIQ(uniq)                                                   \
  static void FUNCNAME(uniq) ();                                        \
  static Test FUNCHELPER(uniq) ( FUNCNAME(uniq) );                 \
  void FUNCNAME(uniq) ()

#define RUNTEST() RUNTESTUNIQ(__LINE__)

RUNTEST()
{
  Reader reader;
//  reader.parse("abc");
  reader.parse("123");
  reader.parse("()");
  reader.parse("(1)");
  reader.parse("(1 2)");
  reader.parse("(1 2 \"abc\")");
  reader.parse("(1 2 \"abc\" ())");
  reader.parse("(1 2 \"abc\" (3))");
  reader.parse("(1 2 \"abc\" (3 4))");
}

RUNTEST()
{
  const char* content =
    MULTI((define (fact-helper x res)
           (if (= x 0)
             res
               (fact-helper (- x 1) (* res x))))

          (define (fact x)
           (fact-helper x 1))

          (display "(fact 3) => ")
          (write (fact 3))
          (newline));

  Reader reader;
  reader.parse(content);
}

RUNTEST()
{
  const char* content =
    MULTI((define foo
           (lambda (a b c d e f g h)
            (+ (+ (* a b) (* c d)) (+ (* e f) (* g h)))))

          (define (writeln x)
           (write x)
           (newline))

          (writeln (length (reverse (list 1 2 3 4 5 6 7 8 9 10 11))))
          (writeln (reverse (list 1 2 3 4 5 6 7 8 9 10 11)))
          (writeln (append (list 1 2) (list 3 4)))
          (writeln (foo 1 2 3 4 5 6 7 8))
          (writeln (apply foo (list 1 2 3 4 5 6 7 8)))
          (writeln (apply foo 1 (list 2 3 4 5 6 7 8)))
          (writeln (apply foo 1 2 3 4 (list 5 6 7 8)))
          (writeln (apply foo 1 2 3 4 5 (list 6 7 8))));

  Reader reader;
  reader.parse(content);
}

RUNTEST()
{
  const char * content =
    MULTI((define (make-counter n)
           (lambda ()
            (set! n (+ n 1))
            n))

          (define f (make-counter 0))
          (define g (make-counter 100))

          (write (f)) (newline)
          (write (f)) (newline)
          (write (g)) (newline)
          (write (g)) (newline)
          (write (f)) (newline)
          (write (g)) (newline));

  Reader reader;
  reader.parse(content);
}

RUNTEST()
{
  const char* content =
    MULTI(((lambda (a b)
            ((lambda (c d e)
              (write (+ e (* c 1000) (* a 100) (* b 10) d))
              (newline))
             (- a 2) (+ b 2) 10000))
           3 5));

  Reader reader;
  reader.parse(content);
}

RUNTEST()
{
  const char * content =
    MULTI((let ((a 3)
                (b 5))
           (let ((c (- a 2))
                 (d (+ b 2))
                 (e 10000))
            (write (+ e (* c 1000) (* a 100) (* b 10) d))
            (newline)))

          );
  Reader reader;
  reader.parse(content);
}

RUNTEST()
{
  const char* content =
    MULTI((let ((a 1000))
           (define b (+ a 3))
           (write a)
           (display " ")
           (write b)
           (newline))
          );
  Reader reader;
  reader.parse(content);
}

RUNTEST()
{
  const char* content =
    MULTI((letrec ((add (lambda (a b) (+ a b))))
           (write (add 3 4))
           (newline))

          (letrec ((even? (lambda (n) (if (zero? n) #t (odd? (- n 1)))))
                   (odd? (lambda (n) (if (zero? n) #f (even? (- n 1))))))
           (write (even? 1000))
           (newline)
           (write (even? 1001))
           (newline)
           (write (odd? 1000))
           (newline)
           )

          );

  Reader reader;
  reader.parse(content);
}

RUNTEST()
{
  const char* content =
    MULTI((let ((a 3)
                (b 5))
           (let ((c (- a 2))
                 (d (+ b 2))
                 (e #f))
            (set! e 10000)
            (write (+ e (* c 1000) (* a 100) (* b 10) d))
            (newline))));

  Reader reader;
  reader.parse(content);
}

RUNTEST()
{
  const char* content =
    MULTI((define fail
           (lambda () 999999))

          (define in-range
           (lambda (a b)
            (call-with-current-continuation
             (lambda (cont)
              (enumerate a b cont)))))

          (define enumerate
           (lambda (a b cont)
            (if (< b a)
              (fail)
                (let ((save fail))
                 (begin
                  (set! fail
                   (lambda ()
                    (begin
                     (set! fail save)
                     (enumerate (+ a 1) b cont))))
                  (cont a))))))

          (write
           (let ((x (in-range 2 9))
                 (y (in-range 2 9))
                 (z (in-range 2 9)))
            (if (= (* x x)
                 (+ (* y y) (* z z)))
              (+ (* x 100) (+ (* y 10) z))
                (fail))))

          (newline));

  Reader reader;
  reader.parse(content);
}

void runTest()
{
  for (TestFuncList::iterator itr = Test::allTestFunc.begin();
       itr != Test::allTestFunc.end(); itr++)
  {
    TestFunc test = *itr;
    test();
  }
}

};
