#include "vm.h"
#include "scminit.h"

namespace Scheme {

void SCMInit::init(VM* vm)
{
  static const char init_scm[] =
";; init\n\
(define-syntax cond\n\
  (syntax-rules (else =>)\n\
    ((_ (else result1 result2 ...))\n\
     (begin result1 result2 ...))\n\
    ((_ (test => result))\n\
     (let ((temp test))\n\
       (if temp\n\
           (result temp))))\n\
    ((_ (test => result) clause1 clause2 ...)\n\
     (let ((temp test))\n\
       (if temp\n\
           (result temp)\n\
           (_ clause1 clause2 ...))))\n\
    ((_ (test))\n\
     test)\n\
    ((_ (test) clause1 clause2 ...)\n\
     (let ((temp test))\n\
       (if temp\n\
           temp\n\
           (_ clause1 clause2 ...))))\n\
    ((_ (test result1 result2 ...))\n\
     (if test\n\
         (begin result1 result2 ...)))\n\
    ((_ (test result1 result2 ...)\n\
        clause1 clause2 ...)\n\
     (if test\n\
         (begin result1 result2 ...)\n\
         (_ clause1 clause2 ...)))))\n\
(define-syntax let\n\
  (syntax-rules ()\n\
    ((_ () body ...)\n\
     ((lambda () body ...)))\n\
    ((_ ((var val) ...) body ...)\n\
     ((lambda (var ...) body ...) val ...))\n\
    ((_ label ((var val) ...) body ...)\n\
     ((letrec ((label (lambda (var ...) body ...)))\n\
        label)\n\
      val ...))\n\
    ))\n\
(define-syntax let*\n\
  (syntax-rules ()\n\
    ((_ () body ...)\n\
     (let () body ...))\n\
    ((_ ((var val) rest ...) body ...)\n\
     (let ((var val))\n\
       (_ (rest ...) body ...)))\n\
    ))\n\
(define-syntax letrec\n\
  (syntax-rules ()\n\
    ((_ () body ...)\n\
     (let () body ...))\n\
    ((_ ((var val) ...) body ...)\n\
     ((lambda ()\n\
        (define var val) ...\n\
        (let () body ...))))))\n\
(define letrec* letrec)\n\
(define-syntax let-syntax\n\
  (syntax-rules ()\n\
    ((_ () body ...)\n\
     (begin body ...))\n\
    ((_ ((k spec) ...) body ...)\n\
     (let ()\n\
       (define-syntax k spec) ...\n\
       body ...))))\n\
(define letrec-syntax let-syntax)\n\
(define-syntax and\n\
  (syntax-rules ()\n\
    ((_) #t)\n\
    ((_ test) test)\n\
    ((_ test1 test2 ...)\n\
     (if test1 (and test2 ...) #f))))\n\
(define-syntax or\n\
  (syntax-rules ()\n\
    ((_) #f)\n\
    ((_ test) test)\n\
    ((_ test1 test2 ...)\n\
     (let ((x test1))\n\
       (if x x (or test2 ...))))))\n\
(define-syntax when\n\
  (syntax-rules ()\n\
    ((_ test result1 result2 ...)\n\
     (if test\n\
         (begin result1 result2 ...)))))\n\
(define-syntax unless\n\
  (syntax-rules ()\n\
    ((unless test result1 result2 ...)\n\
     (when (not test)\n\
       (begin result1 result2 ...)))))\n\
(define (map proc ls . lol)\n\
  (define (map1 proc ls res)\n\
    (if (pair? ls)\n\
        (map1 proc (cdr ls) (cons (proc (car ls)) res))\n\
        (reverse res)))\n\
  (define (mapn proc lol res)\n\
    (if (every? pair? lol)\n\
        (mapn proc\n\
              (map1 cdr lol '())\n\
              (cons (apply proc (map1 car lol '())) res))\n\
        (reverse res)))\n\
  (if (null? lol)\n\
      (map1 proc ls '())\n\
      (mapn proc (cons ls lol) '())))\n\
(define (for-each f ls . lol)\n\
  (define (for1 f ls)\n\
    (if (not (null? ls))\n\
        (begin\n\
          (f (car ls))\n\
          (for1 f (cdr ls)))))\n\
  (define (for2 f ls1 ls2)\n\
    (if (not (null? ls1))\n\
        (begin\n\
          (f (car ls1) (car ls2))\n\
          (for2 f (cdr ls1) (cdr ls2)))))\n\
  (cond ((null? lol)\n\
         (for1 f ls))\n\
        ((null? (cdr lol))\n\
         (for2 f ls (car lol)))\n\
        (else\n\
         (let mapn ((ls (cons ls lol)))\n\
           (when (not (null? (car ls)))\n\
             (apply f (map car ls))\n\
             (mapn (map cdr ls)))))))\n\
(define (every? pred? l)\n\
  (let loop ((l l))\n\
    (or (null? l)\n\
        (and (pred? (car l))\n\
             (loop (cdr l))))))\n\
(define (member obj ls . o)\n\
  (let ((eq (if (pair? o)\n\
                (car o)\n\
                equal?)))\n\
    (let lp ((ls ls))\n\
      (and (pair? ls)\n\
           (if (eq obj (car ls))\n\
               ls\n\
               (lp (cdr ls)))))))\n\
(define-syntax case\n\
  (syntax-rules (else =>)\n\
    ((_ (key ...)\n\
        clauses ...)\n\
     (let ((atom-key (key ...)))\n\
       (_ atom-key clauses ...)))\n\
    ((_ key\n\
        (else => result))\n\
     (result key))\n\
    ((_ key\n\
        (else result1 result2 ...))\n\
     (begin result1 result2 ...))\n\
    ((_ key\n\
        ((atoms ...) result1 result2 ...))\n\
     (if (memv key '(atoms ...))\n\
         (begin result1 result2 ...)\n\
         #f))\n\
    ((_ key\n\
        ((atoms ...) => result))\n\
     (if (memv key '(atoms ...))\n\
         (result key)\n\
         #f))\n\
    ((_ key\n\
        ((atoms ...) => result)\n\
        clause clauses ...)\n\
     (if (memv key '(atoms ...))\n\
         (result key)\n\
         (_ key clause clauses ...)))\n\
    ((_ key\n\
        ((atoms ...) result1 result2 ...)\n\
        clause clauses ...)\n\
     (if (memv key '(atoms ...))\n\
         (begin result1 result2 ...)\n\
         (_ key clause clauses ...)))))\n\
(define-syntax do\n\
  (syntax-rules ()\n\
    ((_ ((var init step ...) ...)\n\
        (test expr ...)\n\
        command ...)\n\
     (letrec\n\
         ((loop\n\
           (lambda (var ...)\n\
             (if test\n\
                 (begin\n\
                   (if #f #f)\n\
                   expr ...)\n\
                 (begin\n\
                   command ...\n\
                   (loop (_ \"step\" var step ...)\n\
                         ...))))))\n\
       (loop init ...)))\n\
    ((_ \"step\" x)\n\
     x)\n\
    ((_ \"step\" x y)\n\
     y)))\n\
(define (call-with-values producer consumer)\n\
  (apply consumer (%values->list (producer))))\n\
;; hash-table traversal on top of the C primitive hash-table-next\n\
(define (hash-table-for-each proc ht)\n\
  (let loop ((kv #f))\n\
    (let ((next (hash-table-next ht kv)))\n\
      (if next\n\
          (begin\n\
            (proc (car next) (cdr next))\n\
            (loop next))\n\
          (if #f #f)))))\n\
(define (hash-table-map proc ht)\n\
  (let loop ((kv #f) (acc '()))\n\
    (let ((next (hash-table-next ht kv)))\n\
      (if next\n\
          (loop next (cons (proc (car next) (cdr next)) acc))\n\
          (reverse acc)))))\n\
(define (hash-table->alist ht)\n\
  (hash-table-map cons ht))\n\
(define (hash-table-keys ht)\n\
  (hash-table-map (lambda (k v) k) ht))\n\
(define (hash-table-values ht)\n\
  (hash-table-map (lambda (k v) v) ht))\n";

  ReaderS reader(init_scm);
  Lexer lex(vm, &reader);
  bool flag = false;
  do {
    flag = vm->dolex(&lex, NULL);
  } while (flag);
}

}
