;; aftertest.scm - regression tests collected from previous AI-generated
;; test files (test.scm / test_div.scm / test_show.scm / testmath.scm /
;; test_dw.scm / tmp_*.scm).  Only the verified-CORRECT content is kept here;
;; files that crashed, hung, or were intentional error probes were left out
;; (tmp_s3, tmp_bad, tmp_rest{,2,4}, tmp_usr2, tmp_usr5, tmp_usr_bad,
;; tmp_dw_cont, tmp_dw_throw, tmp_dw_in_before/after, tmp_dw_err_*,
;; tmp_dw_reenter_before_multi, test_dw's t20 ...).
;;
;; run: ./mkbuild/vm init.scm aftertest.scm
;;
;; The VM writes display/write output to stderr by default.

(define *npass* 0)
(define *nfail* 0)

(define (check name got want)
  (if (equal? got want)
      (begin
        (set! *npass* (+ *npass* 1)))
      (begin
        (set! *nfail* (+ *nfail* 1))
        (display "FAIL ") (write name) (newline)
        (display "     expect: ") (write want) (newline)
        (display "     got:    ") (write got) (newline))))

(define (section line)
  (newline)
  (display line) (newline)
  (display "=") (newline))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 1. dynamic-wind  (test_dw.scm t1..t22; only the non-crashing parts)

(section "1. dynamic-wind")
(check '(dw value) (dynamic-wind (lambda () 'b) (lambda () 42) (lambda () 'a)) 42)

(define *log* '())
(define (add! x) (set! *log* (cons x *log*)))
(set! *log* '())
(dynamic-wind (lambda () (add! 'a)) (lambda () (add! 'b)) (lambda () (add! 'c)))
(check 'dw-order (reverse *log*) '(a b c))

(set! *log* '())
(check 'dw-body-value
       (dynamic-wind (lambda () (add! 'b)) (lambda () (add! 't) 99) (lambda () (add! 'a) 100))
       99)
(check 'dw-after-discarded (reverse *log*) '(b t a))

(set! *log* '())
(check 'dw-nested-value
       (dynamic-wind (lambda () (add! 'b1))
                     (lambda () (dynamic-wind (lambda () (add! 'b2))
                                               (lambda () (add! 't2) 100)
                                               (lambda () (add! 'a2))))
                     (lambda () (add! 'a1)))
       100)
(check 'dw-nested-order (reverse *log*) '(b1 b2 t2 a2 a1))

(set! *log* '())
(define (dw-t7)
  (dynamic-wind (lambda () (add! 'b1))
                (lambda ()
                  (dynamic-wind (lambda () (add! 'b2))
                                (lambda () (add! 'x) 'deep)
                                (lambda () (add! 'a2))))
                (lambda () (add! 'a1))))
(check 'dw-tail-nested-value (dw-t7) 'deep)
(check 'dw-tail-nested-order (reverse *log*) '(b1 b2 x a2 a1))

(define (dw-t9) (dynamic-wind (lambda () 'b) (lambda () 'inner-tail) (lambda () 'a)))
(check 'dw-tail-position (list 'got (dw-t9)) '(got inner-tail))

(set! *log* '())
(define dw-v10 (dynamic-wind (lambda () 'b) (lambda () (add! 'x) 99) (lambda () 'a)))
(check 'dw-define-value dw-v10 99)
(check 'dw-define-log (reverse *log*) '(x))

(check 'dw-native-thunks
       (dynamic-wind current-output-port current-input-port current-output-port)
       (current-input-port))

(set! *log* '())
(check 'dw-3level-value
       (dynamic-wind current-output-port
                     (lambda ()
                       (add! 'l1)
                       (dynamic-wind (lambda () (add! 'l2))
                                     (lambda () (add! 'l3) 7)
                                     (lambda () (add! 'l4))))
                     current-input-port)
       7)
(check 'dw-3level-order (reverse *log*) '(l1 l2 l3 l4))

(set! *log* '())
(check 'dw-result-as-proc
       ((dynamic-wind (lambda () (add! 'b)) (lambda () (add! 'x) car) (lambda () (add! 'a))) '(9 8))
       9)
(check 'dw-in-expression
       (+ 1 (if (input-port? (dynamic-wind current-output-port current-input-port current-output-port)) 100 0))
       101)
(check 'dw-closure-env
       (let ((x 10)) (dynamic-wind (lambda () 'b) (lambda () (+ x 32)) (lambda () 'a)))
       42)

(check 'dw-apply-1 (apply dynamic-wind (list (lambda () 'b) (lambda () 42) (lambda () 'a))) 42)
(define (dw-t18) (apply dynamic-wind (list (lambda () 'b) (lambda () 7) (lambda () 'a))))
(check 'dw-apply-2 (dw-t18) 7)
(check 'dw-apply-3 (apply dynamic-wind (lambda () 'x) (list (lambda () 9) (lambda () 'y))) 9)

(check 'dw-force-delay
       (force (delay (dynamic-wind (lambda () 'b) (lambda () 99) (lambda () 'a))))
       99)

(define (dw-biglist n)
  (let loop ((i 0) (acc '()))
    (if (= i n) acc (loop (+ i 1) (cons i acc)))))
(define dw-gc-r
  (let ((r (dynamic-wind (lambda () #f)
                          (lambda () (dw-biglist 300000))
                          (lambda () (dw-biglist 300000)))))
    (list (length r) (car r) (car (reverse r)))))
(check 'dw-gc-stress dw-gc-r '(300000 299999 0))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 2. dynamic-wind + continuation interaction (tmp_dw_*  passing files)

(section "2. dynamic-wind x call/cc")

;; escape from the body to an outer continuation (tmp_dw_escape.scm)
(define *path* '())
(define (apush s) (set! *path* (cons s *path*)))
(define dw-esc
  (call/cc (lambda (outer)
             (dynamic-wind
               (lambda () (apush 'connect))
               (lambda () (apush 'talk1) (outer 'escaped) (apush 'dead))
               (lambda () (apush 'disconnect))))))
(check 'dw-escape-value dw-esc 'escaped)
(check 'dw-escape-path (reverse *path*) '(connect talk1 disconnect))

;; escape through a helper (tmp_dw_escape2.scm)
(define saved-outer #f)
(set! *path* '())
(define (dw-helper-esc) (saved-outer 'from-helper) (apush 'dead2))
(define dw-esc2
  (call/cc (lambda (outer)
             (set! saved-outer outer)
             (dynamic-wind
               (lambda () (apush 'connect))
               (lambda () (apush 'talk1) (dw-helper-esc))
               (lambda () (apush 'disconnect))))))
(check 'dw-escape2-value dw-esc2 'from-helper)
(check 'dw-escape2-path (reverse *path*) '(connect talk1 disconnect))

;; re-entry repeats before/after each time (tmp_dw_count.scm)
(define dwk #f)
(define nbefore 0)
(define nafter 0)
(define dw-r2
  (dynamic-wind
    (lambda () (set! nbefore (+ nbefore 1)))
    (lambda () (call/cc (lambda (c) (set! dwk c))) 'BODY)
    (lambda () (set! nafter (+ nafter 1)))))
(check 'dw-count-initial (list nbefore nafter dw-r2) '(1 1 BODY))
(dwk 'x)
(check 'dw-count-k1 (list nbefore nafter) '(2 2))
(dwk 'y)
(check 'dw-count-k2 (list nbefore nafter) '(3 3))

;; jump-in/jump-out repeatedly (tmp_dw_pairs.scm)
(define *path3* '())
(define (apush3 s) (set! *path3* (cons s *path3*)))
(define k3 #f)
(define dw-r3
  (dynamic-wind
    (lambda () (apush3 'b))
    (lambda () (call/cc (lambda (c) (set! k3 c))) (apush3 'BODY))
    (lambda () (apush3 'a))))
(set! *path3* '())
(k3 'x)
(check 'dw-pairs-k1 (reverse *path3*) '(b BODY a))
(set! *path3* '())
(k3 'y)
(check 'dw-pairs-k2 (reverse *path3*) '(b BODY a))

;; invocation inside the same extent (tmp_dw_within.scm)
(define *path4* '())
(define (apush4 s) (set! *path4* (cons s *path4*)))
(define dw-within
  (dynamic-wind
    (lambda () (apush4 'connect))
    (lambda () (call/cc (lambda (k) (k 'x))) (apush4 'body))
    (lambda () (apush4 'disconnect))))
(check 'dw-within-path (reverse *path4*) '(connect body disconnect))

;; nested dynamic-wind + capture + outer escape (tmp_dw_nested.scm)
(define *p5* '())
(define (apush5 s) (set! *p5* (cons s *p5*)))
(define k5 #f)
(define dw-rr
  (call/cc (lambda (o)
             (dynamic-wind
               (lambda () (apush5 'B1))
               (lambda ()
                 (dynamic-wind
                   (lambda () (apush5 'B2))
                   (lambda ()
                     (call/cc (lambda (c) (set! k5 c)))
                     (o 'esc))
                   (lambda () (apush5 'A2))))
               (lambda () (apush5 'A1))))))
(check 'dw-nested-rr dw-rr 'esc)
(check 'dw-nested-p1 (reverse *p5*) '(B1 B2 A2 A1))
(set! *p5* '())
(k5 'reenter)
(check 'dw-nested-p2 (reverse *p5*) '(B1 B2 A2 A1))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 3. call/cc and closures

(section "3. call/cc x closures")

(check 'cc-k1 (call/cc (lambda (k) (k 1))) 1)

(define ccsaved #f)
(define cccount 0)
(define (cc-f)
  (define i (call/cc (lambda (k) (set! ccsaved k) (k 0))))
  (set! cccount (+ cccount 1))
  (if (< cccount 3) (ccsaved cccount) i))
(check 'cc-multishot (cc-f) 2)
(check 'cc-multishot-count cccount 3)

;; option-B fix regression: a set!-assigned LOCAL counter must accumulate
;; across multishot continuation re-entries (an unboxed local was restored
;; from the frozen frame snapshot on each re-entry and the loop hung)
(define (cc-f-local)
  (let ((c #f) (n 0))
    (call/cc (lambda (k) (set! c k)))
    (set! n (+ n 1))
    (if (< n 3) (c n) n)))
(check 'cc-multishot-local (cc-f-local) 3)

(define kk #f)
(define cc-basic (call/cc (lambda (c) (set! kk c) 42)))
(check 'cc-basic-value cc-basic 42)
(check 'cc-basic-proc (procedure? kk) #t)

;; resume via re-entry; must NOT wrap the resume in `check`
;; (re-entering a captured continuation replays the capture
;; frame, so keep it a bare display pair, verified in tmp_cc_min.scm)
(define cck #f)
(display "cc-min-first: ")
(display (call/cc (lambda (k) (set! cck k) (k 1))))
(newline)
(display "cc-min-resume: ")
(display (cck 2))
(newline)

(define (gen-counter)
  (let ((n 0))
    (lambda () (set! n (+ n 1)) n)))
(define g1 (gen-counter))
(check 'closure-counter (list (g1) (g1) (g1)) '(1 2 3))
(check 'closure-identity (eqv? (gen-counter) (gen-counter)) #f)

(define compose-2 (lambda (f g) (lambda args (f (apply g args)))))
(define sqt (lambda (x) (sqrt x)))
(check 'compose-apply ((compose-2 sqt *) 12 75) 30)
(check 'apply-mult (apply * '(12 75)) 900)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 4. syntax-rules: ellipsis, custom ellipsis, dotted / trailing vars

(section "4. syntax-rules")

(define-syntax sr-s1 (syntax-rules () ((_ x ...) '(x ...))))
(check 'sr-s1-7 (sr-s1 1 2 3 4 5 6 7) '(1 2 3 4 5 6 7))
(check 'sr-s1-2 (sr-s1 1 2) '(1 2))

(define-syntax sr-s2 (syntax-rules () ((_ a ... b) (list a ... b))))
(check 'sr-trailing-var (sr-s2 1 2 3 4) '(1 2 3 4))
(check 'sr-trailing-single (sr-s2 9) '(9))

(define-syntax sr-s3 (syntax-rules () ((_ a ... b c) (list c b a ...))))
(check 'sr-trailing-vars (sr-s3 1 2 3 4 5) '(5 4 1 2 3))

(define-syntax part-2
  (syntax-rules ()
    ((_ a b (m n) ... x y)
     (vector (list a b) (list m ...) (list n ...) (list x y)))
    ((_ . rest) 'error)))
(check 'sr-vec-pattern
       (part-2 10 (+ 21 22) (31 32) (41 42) (51 52) (+ 61 2) 77)
       '#((10 43) (31 41 51) (32 42 52) (63 77)))

(define-syntax part-2x
  (syntax-rules ()
    ((_ a b (m n) ... x y . rest)
     (vector (list a b) (list m ...) (list n ...) (list x y) (cons "rest:" 'rest)))
    ((_ . rest) 'error)))
(check 'sr-dot-rest-nonempty
       (part-2x 10 (+ 21 22) (31 32) (41 42) (51 52) (+ 61 2) 77 . "tail")
       '#((10 43) (31 41 51) (32 42 52) (63 77) ("rest:" . "tail")))

(check 'sr-usr-2tail
       (let-syntax ((foo (syntax-rules ()
                           ((foo args ... penultimate ultimate)
                            (list ultimate penultimate args ...)))))
         (foo 1 2 3 4 5))
       '(5 4 1 2 3))
(check 'sr-usr-2args
       (let-syntax ((foo (syntax-rules ()
                           ((foo a ... b c) (list c b a ...)))))
         (foo 1 2))
       '(2 1))
(check 'sr-usr-1tail
       (let-syntax ((foo (syntax-rules ()
                           ((foo a ... b) (list b a ...)))))
         (foo 1 2 3))
       '(3 1 2))
(check 'sr-usr-dot
       (let-syntax ((foo (syntax-rules ()
                           ((foo args ... . rest) (list rest args ...)))))
         (foo 1 2 3 . 4))
       '(4 1 2 3))
(check 'sr-usr-dot2
       (let-syntax ((foo (syntax-rules ()
                           ((foo args ... rest) (list rest args ...)))))
         (foo 1 2 3 4))
       '(4 1 2 3))
(check 'sr-dot-rest-len
       (let-syntax ((foo (syntax-rules () ((_ . rest) (length 'rest)))))
         (foo 1 2 3))
       3)

;; custom ellipsis (tmp_ellipsis1.scm)
(check 'sr-ellipsis-custom
       (let-syntax ((foo (syntax-rules ::: ()
                           ((foo a :::) '(a :::)))))
         (foo 1 2 3))
       '(1 2 3))
(check 'sr-ellipsis-atsign
       (let-syntax ((bar (syntax-rules @ ()
                           ((bar a @) (list a @)))))
         (bar 7 8 9))
       '(7 8 9))
(check 'sr-ellipsis-literal
       (let-syntax ((bazz (syntax-rules ... (...)
                            ((bazz x) '(x ...)))))
         (bazz 100))
       '(100 ...))
(check 'sr-ellipsis-custom-2
       (let-syntax ((foo (syntax-rules ::: ()
                           ((foo ... args :::) (args ::: ...)))))
         (foo 3 - 5))
       2)
(check 'sr-ellipsis-literal2
       (let ()
         (define-syntax m (syntax-rules ... (if) ((_ x) 'x)))
         (m 200))
       200)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 5. derived forms (cond / do / let* / define-in-lambda)

(section "5. derived forms")

(check 'cond-2 (cond ((= 1 2) 'no) (#t 'yes)) 'yes)
(check 'cond-no-match (if (cond (#f 'yes)) 't 'f) 't)
(check 'cond-multi-result (cond (#f 'x) (else 1 2)) 2)
(check 'cond-begins (cond (#t (begin 1 2 3))) 3)
(check 'do-result
       (do ((i 0 (+ i 1)) (j 0 (+ j 2))) ((= i 3) (list i j)) i j)
       '(3 6))
(check 'let* (let* ((a 1) (b (+ a 1)) (c (+ b 1))) c) 3)
(check 'intern-def
       (let ((a 1))
         (define (inc x) (+ x 1))
         (inc a))
       2)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 6. bignum arithmetic (test_div.scm / test_show.scm, corrected)

(section "6. bignum arithmetic")

(define b3-3 (string->number "33333333333333333333"))
(define b3-2 (string->number "33333333333333333332"))
(define b1-1 (string->number "11111111111111111111"))
(define b3-0 (string->number "33333333333333333330"))

(check 'bignum-mod-1 (modulo (- b3-2) 3) 1)
(check 'bignum-mod-2 (modulo (- b3-2) -3) -2)
(check 'bignum-rem-1 (remainder (- b3-2) 3) -2)
(check 'bignum-rem-2 (remainder (- b3-2) -3) -2)

(check 'bignum-mod-small-1 (modulo 3 b3-3) 3)
(check 'bignum-mod-small-2 (modulo -3 b3-3) 33333333333333333330)
(check 'bignum-rem-small-1 (remainder 3 b3-3) 3)
(check 'bignum-rem-small-2 (remainder -3 b3-3) -3)

(check 'bignum-mod-negdiv-1 (modulo 3 (- b3-3)) -33333333333333333330)
(check 'bignum-mod-negdiv-2 (modulo -3 (- b3-3)) -3)
(check 'bignum-rem-negdiv-1 (remainder 3 (- b3-3)) 3)
(check 'bignum-rem-negdiv-2 (remainder -3 (- b3-3)) -3)

(check 'bignum-quotient (quotient b3-2 3) 11111111111111111110)
(check 'bignum-remainder (remainder b3-2 3) 2)

(check 'bignum-gcd-1 (gcd b3-2 b1-1) 1)
(check 'bignum-gcd-2 (gcd b3-0 b1-1) 1)
(check 'bignum-gcd-3 (gcd b3-3 b3-0) 3)
(check 'bignum-lcm (lcm -3 b1-1) 33333333333333333333)

(check 'smallquotient-a (quotient -7 2) -3)
(check 'smallquotient-b (quotient 7 -2) -3)
(check 'smallquotient-c (remainder -7 2) -1)
(check 'smallquotient-d (remainder 7 -2) 1)
(check 'smallquotient-e (modulo -7 2) 1)
(check 'smallquotient-f (modulo 7 -2) -1)
(check 'smallquotient-g (modulo -7 -2) -1)
(check 'smallgcd (gcd 32 -36) 4)
(check 'smallgcdn (gcd) 0)
(check 'smalllcm (lcm 32 -36) 288)
(check 'smalllcmn (lcm) 1)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 7. number->string formatting and read-back round trip

(section "7. number->string x round-trip")

(check 'n2s-1e10 (number->string 1e10) "1e+10")
(check 'n2s-1e11 (number->string 1e11) "1e+11")
(check 'n2s-1e12 (number->string 1e12) "1e+12")
(check 'n2s-1e15 (number->string 1e15) "1e+15")
(check 'n2s-1e18 (number->string 1e18) "1e+18")
(check 'n2s-1e20 (number->string 1.0e20) "1e+20")
(check 'n2s-sub (number->string (- 1.0e20 1.0e16)) "9.999e+19")
(check 'n2s-zero (number->string (* 1.0e20 (- 100 100))) "0.0")
(check 'n2s-div (number->string (/ 1.0e20 4)) "2.5e+19")
(check 'n2s-add (number->string (+ 1.0e20 0.5)) "1e+20")
(check 'n2s-10.0 (number->string 10.0) "1e+01")
(check 'n2s-100.0 (number->string 100.0) "1e+02")
(check 'n2s-3.0 (number->string 3.0) "3.0")
(check 'n2s-4.3 (number->string 4.3) "4.3")
(check 'n2s-1e-10 (number->string 1e-10) "1e-10")
(check 'n2s-5e-05 (number->string 0.00005) "5e-05")
(check 'n2s-2.5e-300 (number->string 2.5e-300) "2.5e-300")
(check 'n2s-123456.789 (number->string 123456.789) "123456.789")
(check 'n2s-5e-324 (number->string 5e-324) "5e-324")
(check 'n2s-1.0 (number->string 1.0) "1.0")
(check 'n2s-0.1 (number->string 0.1) "0.1")
(check 'n2s-nan (number->string (string->number "+nan.0")) "+nan.0")
(check 'n2s-inf (number->string (string->number "+inf.0")) "+inf.0")
(check 'n2s-ninf (number->string (string->number "-inf.0")) "-inf.0")
(check 'n2s-0 (number->string 0) "0")
(check 'n2s-0.0 (number->string 0.0) "0.0")
(check 'n2s-n0.0 (number->string -0.0) "-0.0")

(define (rt x) (eqv? x (string->number (number->string x))))
(check 'rt-1e7 (rt 1e7) #t)
(check 'rt-1e10 (rt 1e10) #t)
(check 'rt-1e19 (rt 1e19) #t)
(check 'rt-1e20 (rt 1e20) #t)
(check 'rt-1e30 (rt 1e30) #t)
(check 'rt-pi (rt 3.141592653589793) #t)
(check 'rt-n1e20 (rt -1e20) #t)
(check 'rt-1e-5 (rt 0.00001) #t)
(check 'rt-n0.01 (rt -0.01) #t)
(check 'rt-nan (rt (string->number "+nan.0")) #f)
(check 'rt-inf-str (equal? (number->string (string->number "+inf.0")) "+inf.0") #t)
(check 'rt-nan-str (equal? (number->string (string->number "+nan.0")) "+nan.0") #t)

;; mixed bignum/inexact precision (testmath.scm sections 4-5, all passing)
(define big-ex (expt 2 150))
(define big-inex (exact->inexact big-ex))
(check 'mixed-eq-all (= (+ big-ex 1) big-inex (- big-ex 1)) #f)
(check 'mixed-lt (< (- (inexact->exact big-inex) 1) big-inex (+ (inexact->exact big-inex) 1)) #t)
(check 'mixed-iexact (equal? (inexact->exact (exact->inexact big-ex)) big-ex) #t)
(check 'ie-big (exact? (inexact->exact 1e20)) #t)
(check 'ie-1/2 (inexact->exact 0.5) 1/2)
(check 'ie-1/4 (inexact->exact 0.25) 1/4)
(check 'ie-2.0 (inexact->exact 2.0) 2)
(check 'lt-1e300 (< 1e300 2e300 3e300) #t)
(check 'eq-1e300 (= 1e300 1e300) #t)
(check 'expt-2-150 (eqv? (expt 2 150) (expt 2 150)) #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 8. c*r combos (R5RS)

(section "8. c*r combos")

;; force in tail position: the value is the promise value and the calling
;; frame must return, not be resumed at its last instruction
(define (force-tail-fn) (force (delay 42)))
(check 'force-tail-fn (force-tail-fn) 42)
(check 'force-tail-lambda ((lambda () (force (delay 7)))) 7)
(check 'force-tail-in-def ((lambda (f) (f)) force-tail-fn) 42)

;; chained promise: the thunk of a promise returns another promise, so force
;; has to pick the inner one up (recallforce)
(define pchain (delay (delay 6)))
(check 'force-chain (force pchain) 6)
(check 'force-chain-memo (force pchain) 6)
(check 'force-chain-lambda ((lambda () (force (delay (delay 7))))) 7)
(check 'force-chain-consumed (+ 0 (force pchain)) 6)

;; a body whose last form is an empty begin emits no code, yet the frame must
;; still return its value to the caller
(define (empty-begin-tail) 42 (begin))
(check 'empty-begin-tail (empty-begin-tail) 42)
(check 'empty-begin-tail-nested (list (empty-begin-tail) (empty-begin-tail)) '(42 42))

(check 'cxr-caddr (caddr '(1 2 3)) 3)
(check 'cxr-cadddr (cadddr '(1 2 3 4)) 4)
(check 'cxr-cddddr (cddddr '(1 2 3 4 5)) '(5))
(check 'cxr-cdaar (cdaar '(((1 . 2)) . x)) 2)
(check 'cxr-cdadr (cdadr '(a (1 . 2))) 2)
(check 'cxr-caaadr (caaadr '(a ((b . c)))) 'b)
(check 'cxr-cdddar (cdddar '((a b c . d))) 'd)
(check 'cxr-cadddr (cadddr '(1 2 3 4)) 4)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 9. R5RS gaps implemented: make-promise / letrec-syntax /
;;    call-with-values / with-input-from-file / with-output-to-file

(section "9. R5RS gaps")

;; letrec-syntax: sibling templates are visible inside each other
;; (recursive scope; expansion happens on use, after all bindings are in)
(check 'letrec-syntax-fwd
       (letrec-syntax ((a (syntax-rules () ((a) (b))))
                       (b (syntax-rules () ((b) 42))))
         (a))
       42)
(check 'letrec-syntax-chain
       (letrec-syntax ((a (syntax-rules () ((a) (b))))
                       (b (syntax-rules () ((b) (c))))
                       (c (syntax-rules () ((c) 'deep))))
         (a))
       'deep)
(check 'letrec-syntax-empty-body
       (let () (letrec-syntax () (define internal-def 'ok)) internal-def)
       'ok)

;; call-with-values: multiple / zero / single (non-values) / nesting
(check 'cwv-multi (call-with-values (lambda () (values 1 2)) +) 3)
(check 'cwv-three (call-with-values (lambda () (values 1 2 3)) list) '(1 2 3))
(check 'cwv-zero (call-with-values (lambda () (values)) (lambda () 42)) 42)
(check 'cwv-single (call-with-values (lambda () 7) (lambda (x) x)) 7)
(check 'cwv-add (call-with-values (lambda () (values 10 20)) (lambda (a b) (+ a b))) 30)
(check 'cwv-nested (call-with-values (lambda () (values 1 2))
                                    (lambda (a b)
                                      (call-with-values (lambda () (values b a)) list)))
       '(2 1))

;; with-input-from-file / with-output-to-file: temporarily rebind
;; current-input-port / current-output-port, restore and close on the way
;; out (dynamic-wind after also runs on escape via a continuation)
(define wif-file "/tmp/aftertest_wif_in.txt")
(define wof-file "/tmp/aftertest_wof_out.txt")
(call-with-output-file wif-file (lambda (out) (write 'hello out) (newline out)))
(check 'wif-read
       (with-input-from-file wif-file (lambda () (read)))
       'hello)
(define wif-old (current-input-port))
(with-input-from-file wif-file (lambda () (read)))
(check 'wif-restore (eq? (current-input-port) wif-old) #t)
(with-output-to-file wof-file
  (lambda () (display "wof-line") (newline)))
(define wof-old (current-output-port))
(with-output-to-file wof-file (lambda () (display "x")))
(check 'wof-restore (eq? (current-output-port) wof-old) #t)
(check 'wof-content
       (call-with-input-file wof-file (lambda (in) (read-char in)))
       #\x)
;; escape via continuation: dynamic-wind after closes/restores, re-entry
;; re-opens, and afterwards the current port is the original one again
(define wif-esc #f)
(define wif-esc-res
  (with-input-from-file wif-file
    (lambda ()
      (call/cc (lambda (c) (set! wif-esc c)))
      'esc-body)))
(check 'wif-esc-body wif-esc-res 'esc-body)
(wif-esc 'again)
(check 'wif-esc-restore (eq? (current-input-port) wif-old) #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 10. hash-table  (Lua-style hybrid table: array part + hash part)

(section "10. hash-table")
(define ht (make-hash-table))
;; array part: dense integer keys, direct slot writes
(check 'ht-array-first (begin (hash-table-set! ht 1 'a) (hash-table-ref ht 1)) 'a)
(check 'ht-array-multi
       (begin
         (do ((i 1 (+ i 1))) ((> i 100))
           (hash-table-set! ht i (* i 10)))
         (hash-table-ref ht 100))
       1000)
(check 'ht-array-update
       (begin (hash-table-set! ht 5 'five) (hash-table-ref ht 5)) 'five)
;; hash part: symbols, strings, sparse/big integers
(check 'ht-sym (begin (hash-table-set! ht 'foo 'bar) (hash-table-ref ht 'foo)) 'bar)
(check 'ht-str (begin (hash-table-set! ht "k" 'v) (hash-table-ref ht "k")) 'v)
(check 'ht-bigint (begin (hash-table-set! ht 1000000 'big) (hash-table-ref ht 1000000)) 'big)
(check 'ht-listval (begin (hash-table-set! ht 'l (list 1 2 3)) (hash-table-ref ht 'l)) '(1 2 3))
;; mixed keys coexist
(check 'ht-mixed
       (begin
         (hash-table-set! ht 77 'int)
         (hash-table-set! ht 'sym 's)
         (list (hash-table-ref ht 77) (hash-table-ref ht 'sym) (hash-table-ref ht 1)))
       '(int s 10))
;; repeated rehash + array growth under a dense insert storm
(check 'ht-rehash-storm
       (begin
         (do ((i 1001 (+ i 1))) ((> i 5000))
           (hash-table-set! ht i (modulo i 11)))
         (list (hash-table-ref ht 5000) (hash-table-ref ht 1001) (hash-table-ref ht 100)))
       '(6 0 1000))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 11. hash-table traversal (Lua luaH_next style: kv = #f starts, next (key . val) or #f)

(section "11. hash-table traversal")
;; incremental walk: start / step / end
(check 'htw-step
       (let ((h (make-hash-table)))
         (hash-table-set! h 1 'a)
         (hash-table-set! h 2 'b)
         (let ((w1 (hash-table-next h #f))
               (w2 (hash-table-next h (hash-table-next h #f))))
           (list w1 w2 (hash-table-next h w2))))
       '((1 . a) (2 . b) #f))
;; array part walks in ascending order, then the hash part
(check 'htw-order
       (let ((h (make-hash-table)))
         (hash-table-set! h 3 'c)
         (hash-table-set! h 1 'a)
         (hash-table-set! h 2 'b)
         (hash-table->alist h))
       '((1 . a) (2 . b) (3 . c)))
;; for-each visits every entry exactly once
(check 'htw-for-each
       (let ((h (make-hash-table)))
         (hash-table-set! h 'a 1)
         (hash-table-set! h 'b 2)
         (hash-table-set! h 'c 3)
         (let ((seen '()))
           (hash-table-for-each (lambda (k v) (set! seen (cons (cons k v) seen))) h)
           (length seen)))
       3)
;; map / keys / values over a mixed table: dense ints + sym + str
(check 'htw-mixed
       (let ((h (make-hash-table)))
         (do ((i 1 (+ i 1))) ((> i 100))
           (hash-table-set! h i (* i 10)))
         (hash-table-set! h 'sym 's)
         (hash-table-set! h "str" 't)
         (list (length (hash-table-keys h))
               (hash-table-ref h 77)
               (let ((ks (hash-table-keys h)))
                 (and (member 'sym ks) (member "str" ks) #t))))
       '(102 770 #t))
;; empty table walks to #f immediately
(check 'htw-empty
       (let ((h (make-hash-table)))
         (list (hash-table-next h #f) (hash-table->alist h)))
       '(#f ()))
;; call/cc can escape the middle of for-each (walk is Scheme frames)
(check 'htw-escape
       (let ((h (make-hash-table)))
         (hash-table-set! h 1 'a)
         (hash-table-set! h 2 'b)
         (hash-table-set! h 3 'c)
         (call/cc
          (lambda (out)
            (hash-table-for-each
             (lambda (k v) (when (= k 2) (out 'escaped)))
             h))))
       'escaped)
;; multi-shot: capture inside the walk, then re-enter and resume from k=2.
;; The re-entry flag keeps the resumed path from re-invoking `saved` (after
;; the walk completes, control returns to the instruction after for-each).
(check 'htw-reenter
       (let ((h (make-hash-table))
             (seen '())
             (saved #f)
             (reentered #f))
         (hash-table-set! h 1 'a)
         (hash-table-set! h 2 'b)
         (hash-table-set! h 3 'c)
         (hash-table-for-each
          (lambda (k v)
            (when (= k 2)
              (call/cc (lambda (k2) (set! saved k2) (k2 'x))))
            (set! seen (cons (cons k v) seen)))
          h)
         (when (not reentered)
           (set! reentered #t)
           (saved 'again))
         (length seen))
       5)

(newline)
(section "12. hash-table print/equal")
;; display/write print a label only (no entries)
(check 'htpv-print
       (let ((h (make-hash-table)))
         (hash-table-set! h 1 'a)
         (hash-table-set! h 'sym 42)
         (call-with-output-string
          (lambda (out) (display h out))))
       "#<hash-table>")
(check 'htpv-write
       (let ((h (make-hash-table)))
         (hash-table-set! h 1 'a)
         (call-with-output-string
          (lambda (out) (write h out))))
       "#<hash-table>")
;; equal?/eqv?/eq? compare hashtables by identity (pointer)
(check 'htpv-eq
       (let ((h1 (make-hash-table)) (h2 (make-hash-table)))
         (hash-table-set! h1 1 'a)
         (hash-table-set! h2 1 'a)
         (list (equal? h1 h1) (equal? h1 h2) (eqv? h1 h2) (eq? h1 h2)))
       '(#t #f #f #f))


(display "aftertest passed ") (display *npass*)
(display " failed ") (display *nfail*)
(newline)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 12. hash-table print / equal


(if (= *nfail* 0)
    (begin (display "ALL PASS") (newline))
    (begin (display "SOME FAIL") (newline)))