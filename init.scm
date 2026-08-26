;; init

(define-syntax cond
  (syntax-rules (else =>)
    ((_ (else result1 result2 ...))
     (begin result1 result2 ...))

    ((_ (test => result))
     (let ((temp test))
       (if temp
           (result temp))))

    ((_ (test => result) clause1 clause2 ...)
     (let ((temp test))
       (if temp
           (result temp)
           (_ clause1 clause2 ...))))

    ((_ (test))
     test)

    ((_ (test) clause1 clause2 ...)
     (let ((temp test))
       (if temp
           temp
           (_ clause1 clause2 ...))))

    ((_ (test result1 result2 ...))
     (if test
         (begin result1 result2 ...)))

    ((_ (test result1 result2 ...)
        clause1 clause2 ...)
     (if test
         (begin result1 result2 ...)
         (_ clause1 clause2 ...)))))


(define-syntax let
  (syntax-rules ()

    ((_ () body ...)
     ((lambda () body ...)))

    ((_ ((var val) ...) body ...)
     ((lambda (var ...) body ...) val ...))

    ((_ label ((var val) ...) body ...)
     ((letrec ((label (lambda (var ...) body ...)))
        label)
      val ...))
    ))

(define-syntax let*
  (syntax-rules ()

    ((_ () body ...)
     (let () body ...))

    ((_ ((var val) rest ...) body ...)
     (let ((var val))
       (_ (rest ...) body ...)))
    ))

(define-syntax letrec
  (syntax-rules ()

    ((_ () body ...)
     (let () body ...))

    ((_ ((var val) ...) body ...)
     ((lambda ()
        (define var val) ...
        (let () body ...))))))

(define letrec* letrec)

(define-syntax and
  (syntax-rules ()
    ((_) #t)

    ((_ test) test)

    ((_ test1 test2 ...)
     (if test1 (and test2 ...) #f))))

(define-syntax or
  (syntax-rules ()
    ((_) #f)

    ((_ test) test)

    ((_ test1 test2 ...)
     (let ((x test1))
       (if x x (or test2 ...))))))

(define-syntax when
  (syntax-rules ()
    ((_ test result1 result2 ...)
     (if test
         (begin result1 result2 ...)))))

(define-syntax unless
  (syntax-rules ()
    ((unless test result1 result2 ...)
     (when (not test)
       (begin result1 result2 ...)))))

(define (map proc ls . lol)
  (define (map1 proc ls res)
    (if (pair? ls)
        (map1 proc (cdr ls) (cons (proc (car ls)) res))
        (reverse res)))
  (define (mapn proc lol res)
    (if (every? pair? lol)
        (mapn proc
              (map1 cdr lol '())
              (cons (apply proc (map1 car lol '())) res))
        (reverse res)))
  (if (null? lol)
      (map1 proc ls '())
      (mapn proc (cons ls lol) '())))

(define (for-each f ls . lol)
  (define (for1 f ls)
    (if (not (null? ls))
        (begin
          (f (car ls))
          (for1 f (cdr ls)))))
  (define (for2 f ls1 ls2)
    (if (not (null? ls1))
        (begin
          (f (car ls1) (car ls2))
          (for2 f (cdr ls1) (cdr ls2)))))
  (cond ((null? lol)
         (for1 f ls))
        ((null? (cdr lol))
         (for2 f ls (car lol)))
        (else
         (let mapn ((ls (cons ls lol)))
           (when (not (null? (car ls)))
             (apply f (map car ls))
             (mapn (map cdr ls)))))))

(define (every? pred? l)
  (let loop ((l l))
    (or (null? l)
        (and (pred? (car l))
             (loop (cdr l))))))

(define (member obj ls . o)
  (let ((eq (if (pair? o)
                (car o)
                equal?)))
    (let lp ((ls ls))
      (and (pair? ls)
           (if (eq obj (car ls))
               ls
               (lp (cdr ls)))))))

(define-syntax case
  (syntax-rules (else =>)
    ((_ (key ...)
        clauses ...)
     (let ((atom-key (key ...)))
       (_ atom-key clauses ...)))

    ((_ key
        (else => result))
     (result key))

    ((_ key
        (else result1 result2 ...))
     (begin result1 result2 ...))

    ((_ key
        ((atoms ...) result1 result2 ...))
     (if (memv key '(atoms ...))
         (begin result1 result2 ...)
         #f))

    ((_ key
        ((atoms ...) => result))
     (if (memv key '(atoms ...))
         (result key)
         #f))

    ((_ key
        ((atoms ...) => result)
        clause clauses ...)
     (if (memv key '(atoms ...))
         (result key)
         (_ key clause clauses ...)))

    ((_ key
        ((atoms ...) result1 result2 ...)
        clause clauses ...)
     (if (memv key '(atoms ...))
         (begin result1 result2 ...)
         (_ key clause clauses ...)))))

(define-syntax do
  (syntax-rules ()
    ((_ ((var init step ...) ...)
        (test expr ...)
        command ...)
     (letrec
         ((loop
           (lambda (var ...)
             (if test
                 (begin
                   (if #f #f)
                   expr ...)
                 (begin
                   command ...
                   (loop (_ "step" var step ...)
                         ...))))))
       (loop init ...)))
    ((_ "step" x)
     x)
    ((_ "step" x y)
     y)))
