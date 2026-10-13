;; aftertest.scm - regression tests collected from previous AI-generated
;; test files (test.scm / test_div.scm / test_show.scm / testmath.scm /
;; test_dw.scm / tmp_*.scm).  Only the verified-CORRECT content is kept here;
;; files that crashed, hung, or were intentional error probes were left out
;; (tmp_s3, tmp_bad, tmp_rest{,2,4}, tmp_usr2, tmp_usr5, tmp_usr_bad,
;; tmp_dw_cont, tmp_dw_throw, tmp_dw_in_before/after, tmp_dw_err_*,
;; tmp_dw_reenter_before_multi, test_dw's t20 ...).
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



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 13. coroutine: basics (co_a..co_b12, co_min, cogc)
;;    Lua-style asymmetric coroutines: create builds an independent VM;
;;    resume is like a call, yield like a return; when the body ends,
;;    resume returns the final value and status becomes dead.

(section "13. coroutine: basics")

;; co_a: single yield; the second resume's value becomes the yield
;;       expression result (body ends, returning that value)
(define co13a (coroutine-create (lambda (n) (coroutine-yield (list 'y n)))))
(check 'co13a (list (coroutine-resume co13a 7) (coroutine-resume co13a 0))
       '((y 7) 0))

;; co_b: summing loop with yield; resumed once (suspended, not finished)
(define co13b (coroutine-create
  (lambda (n)
    (let loop ((i 0) (sum 0))
      (if (>= i n)
          (coroutine-yield 'done)
          (begin
            (set! sum (+ sum i))
            (coroutine-yield (list 'yield i sum))
            (loop (+ i 1) sum)))))))
(check 'co13b (coroutine-resume co13b 3) '(yield 0 0))

;; co_b2: yields i each round (resume values ignored, loop increments)
(define co13c (coroutine-create
  (lambda (n)
    (let loop ((i 0) (sum 0))
      (if (>= i n)
          (coroutine-yield 'done)
          (begin
            (set! sum (+ sum i))
            (coroutine-yield i)
            (loop (+ i 1) sum)))))))
(check 'co13c (list (coroutine-resume co13c 3) (coroutine-resume co13c 0))
       '(0 1))

;; co_b3: two yields
(define co13d (coroutine-create
  (lambda (n)
    (let loop ((i 0))
      (if (>= i n)
          (coroutine-yield 'done)
          (begin
            (coroutine-yield (list 'yield i))
            (loop (+ i 1))))))))
(check 'co13d (list (coroutine-resume co13d 3) (coroutine-resume co13d 0))
       '((yield 0) (yield 1)))

;; co_b4: main keeps running after resume returns (side-effect order)
(set! *log* '())
(define co13e (coroutine-create
  (lambda (n)
    (let loop ((i 0))
      (if (>= i n)
          (coroutine-yield 'done)
          (begin
            (coroutine-yield (list 'yield i))
            (loop (+ i 1))))))))
(add! (coroutine-resume co13e 3))
(add! 'resume-returned)
(add! (coroutine-resume co13e 0))
(check 'co13e (reverse *log*) '((yield 0) resume-returned (yield 1)))

;; co_b6: no yield, returns directly
(define co13f (coroutine-create (lambda () (list 'a 'b))))
(check 'co13f (coroutine-resume co13f) '(a b))

;; co_b7: after yield the body ends; second resume's value becomes the result
(define co13g (coroutine-create (lambda () (coroutine-yield 'y))))
(check 'co13g (list (coroutine-resume co13g) (coroutine-resume co13g 0))
       '(y 0))

;; co_b9: pure loop without yield (one resume runs to the end)
(define co13h (coroutine-create
  (lambda (n)
    (let loop ((i 0))
      (if (< i n)
          (begin (list 'a i) (loop (+ i 1)))
          'done)))))
(check 'co13h (coroutine-resume co13h 3) 'done)

;; co_b10: yield mixed with tail recursion
(define co13i (coroutine-create
  (lambda (n)
    (let loop ((i 0))
      (if (< i n)
          (begin
            (coroutine-yield i)
            (list 'a i)
            (loop (+ i 1)))
          'done)))))
(check 'co13i (list (coroutine-resume co13i 3) (coroutine-resume co13i 0))
       '(0 1))

;; co_b11/co_b12: named external functions as coroutine bodies
(define (co13j-f n) (let loop ((i 0)) (if (< i n) (begin (list 'a i) (loop (+ i 1))) 'done)))
(define co13j (coroutine-create co13j-f))
(check 'co13j (coroutine-resume co13j 3) 'done)
(define (co13k-f) (let loop ((i 0)) (if (< i 3) (begin (list 'a i) (loop (+ i 1))) 'done)))
(define co13k (coroutine-create co13k-f))
(check 'co13k (coroutine-resume co13k) 'done)

;; co_min: multi-round yields + finish + status
(define co13m (coroutine-create
  (lambda (n)
    (let loop ((i 0) (sum 0))
      (if (>= i n)
          (begin (coroutine-yield 'done) sum)
          (begin
            (set! sum (+ sum i))
            (coroutine-yield (list 'yield i sum))
            (loop (+ i 1) sum)))))))
(check 'co13m (list (coroutine-resume co13m 3) (coroutine-resume co13m 0)
                    (coroutine-resume co13m 0) (coroutine-resume co13m 0))
       '((yield 0 0) (yield 1 1) (yield 2 3) done))
(check 'co13m-st (coroutine-status co13m) 'suspended)   ; stops at (yield 'done), not finished

;; cogc: suspended coroutine survives GC pressure (30000 loops);
;;       the coroutine stack is kept alive as a GC root
(define (gcstress)
  (let loop ((i 0))
    (if (< i 30000)
        (loop (+ i 1)))))
(define co13n (coroutine-create
  (lambda (n)
    (let loop ((i 0) (sum 0))
      (if (>= i n)
          (begin
            (coroutine-yield 'done)
            sum)
          (begin
            (set! sum (+ sum i))
            (if (= (modulo i 7) 0) (coroutine-yield (list 'yield i sum)))
            (loop (+ i 1) sum)))))))
(check 'co13n-r1 (coroutine-resume co13n 100) '(yield 0 0))
(gcstress)
(check 'co13n-r2 (coroutine-resume co13n 0) '(yield 7 28))
(gcstress)
(check 'co13n-r3 (coroutine-resume co13n 0) '(yield 14 105))
(gcstress)
(check 'co13n-r4 (coroutine-resume co13n 0) '(yield 21 231))
(check 'co13n-st (coroutine-status co13n) 'suspended)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 14. coroutine: status / running / nesting
;;    status inference: SUSPENDED/DEAD are explicit; CO_OK is derived from
;;    the execution context (current stack = running, has frames = normal,
;;    otherwise initial = suspended)

(section "14. coroutine: status / running / nesting")

;; co3nest: initially suspended; while A is suspended, B is still suspended
(define c-status (lambda (c) (coroutine-status c)))
(define co14a-B (coroutine-create (lambda () (coroutine-yield 'b-yield) 'b-done)))
(define co14a-A (coroutine-create (lambda ()
  (coroutine-yield 'a-yield1)
  (add! (c-status co14a-B))
  (coroutine-yield 'a-yield2)
  'a-done)))
(check 'co14a-initA (c-status co14a-A) 'suspended)
(check 'co14a-initB (c-status co14a-B) 'suspended)
(check 'co14a-r1 (coroutine-resume co14a-A) 'a-yield1)
(check 'co14a-stA (c-status co14a-A) 'suspended)
(set! *log* '())
(check 'co14a-r2 (coroutine-resume co14a-A) 'a-yield2)
(check 'co14a-log (reverse *log*) '(suspended))

;; co3nest2: nested resume of B to dead; A records B's status each step
(define co14b-B (coroutine-create (lambda () (coroutine-yield 'b-yield) 'b-done)))
(set! *log* '())
(define co14b-A (coroutine-create (lambda ()
  (add! (c-status co14b-B))                 ; suspended
  (add! (coroutine-resume co14b-B))         ; b-yield
  (add! (c-status co14b-B))                 ; suspended
  (add! (coroutine-resume co14b-B))         ; b-done
  (add! (c-status co14b-B))                 ; dead
  (coroutine-yield 'a-back)
  'a-done)))
(check 'co14b-r1 (coroutine-resume co14b-A) 'a-back)
(check 'co14b-log (reverse *log*) '(suspended b-yield suspended b-done dead))
(check 'co14b-stA (c-status co14b-A) 'suspended)
(check 'co14b-r2 (coroutine-resume co14b-A) 'a-done)
(check 'co14b-stA2 (c-status co14b-A) 'dead)

;; costate: running/normal inference (checking self = running;
;;          nested-and-suspended = normal)
(define co14c (coroutine-create (lambda ()
  (add! (coroutine-status (coroutine-running)))    ; running (self executing)
  (define inner (coroutine-create (lambda ()
    (add! (coroutine-status co14c))                ; normal (co14c suspended)
    (coroutine-yield 'iy))))
  (add! (coroutine-resume inner))                  ; iy
  (add! (coroutine-status co14c))                  ; running (after inner returns)
  (coroutine-yield 'y1))))
(check 'co14c-init (c-status co14c) 'suspended)
(set! *log* '())
(check 'co14c-r1 (coroutine-resume co14c) 'y1)
(check 'co14c-log (reverse *log*) '(running normal iy running))

;; coidentity: running identity / eq? / ends returning void / print format
(define co14d (coroutine-create (lambda ()
  (add! (eq? co14d (coroutine-running)))
  (coroutine-yield 'y1))))
(check 'co14d-main-running (coroutine-running) #f)
(set! *log* '())
(check 'co14d-r1 (coroutine-resume co14d) 'y1)
(check 'co14d-log (reverse *log*) '(#t))
(check 'co14d-st (c-status co14d) 'suspended)
(coroutine-resume co14d)                            ; body ends (no value, unchecked)
(check 'co14d-st2 (c-status co14d) 'dead)
(check 'co14d-print
       (call-with-output-string (lambda (o) (write co14d o)))
       "#<coroutine>")

;; coapply: resume/yield via apply
(define co14e (coroutine-create (lambda ()
  (let ((v (apply coroutine-yield '(first))))
    (list 'got v)))))
(check 'co14e-r1 (apply coroutine-resume (list co14e)) 'first)
(check 'co14e-r2 (apply coroutine-resume (list co14e 'hello)) '(got hello))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 15. coroutine x dynamic-wind
;;    before/after follow the execution context: yield while suspended does
;;    not run after; when the coroutine body ends (root frame popped),
;;    after runs once.

(section "15. coroutine with dynamic-wind")

;; codw1: single-layer DW; yield does not trigger after, end does
(set! *log* '())
(define co15a (coroutine-create
  (lambda ()
    (dynamic-wind (lambda () (add! 'b))
                  (lambda () (coroutine-yield 'mid) 'end)
                  (lambda () (add! 'a))))))
(check 'co15a-r1 (coroutine-resume co15a) 'mid)
(check 'co15a-log1 (reverse *log*) '(b))       ; before ran, after did not
(check 'co15a-r2 (coroutine-resume co15a) 'end)
(check 'co15a-log2 (reverse *log*) '(b a))     ; popping the root frame advances after

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 16. coroutine: error boundary (Lua-style)
;;    An error inside a coroutine is caught at the resume boundary: the
;;    coroutine dies (status dead), resume delivers undefined, and the
;;    caller keeps running.  DW afters of the dying stack are run during
;;    teardown.  Errors never cross a resume boundary.

(section "16. coroutine: error boundary")

;; undefined predicate: the error-delivered value prints as <void>
(define (undef? v)
  (equal? (call-with-output-string (lambda (o) (display v o))) "<void>"))

;; 16a body error: resume delivers undefined, coroutine dead, caller continues
(define co16a (coroutine-create (lambda () (coroutine-yield 'y) (car 99))))
(check 'co16a-r1 (coroutine-resume co16a) 'y)
(check 'co16a-r2 (undef? (coroutine-resume co16a)) #t)
(check 'co16a-st (coroutine-status co16a) 'dead)
(check 'co16a-r3 (undef? (coroutine-resume co16a)) #t)   ; resume after dead
(display "after-error") (newline)

;; 16b nested: B dies at its own boundary, A keeps going and sees dead
(define co16b-a (coroutine-create (lambda ()
  (define b (coroutine-create (lambda ()
    (coroutine-yield 'b-y)
    (car 99))))
  (coroutine-yield (list 'b-st0 (coroutine-status b) (coroutine-resume b)))
  (coroutine-yield (list 'b-r2 (undef? (coroutine-resume b)) (coroutine-status b)))
  'a-done)))
(check 'co16b-r1 (coroutine-resume co16b-a) '(b-st0 suspended b-y))
(check 'co16b-r2 (coroutine-resume co16b-a) '(b-r2 #t dead))
(check 'co16b-r3 (coroutine-resume co16b-a) 'a-done)

;; 16c nested resume then own yield: A suspended between B errors
(define co16c-a (coroutine-create (lambda ()
  (define b (coroutine-create (lambda ()
    (coroutine-yield 'b-y)
    (car 99))))
  (coroutine-resume b)
  (coroutine-yield (list 'a-y1 (coroutine-status b)))
  (coroutine-resume b)
  'a-done)))
(check 'co16c-r1 (coroutine-resume co16c-a) '(a-y1 suspended))
(check 'co16c-r2 (coroutine-resume co16c-a) 'a-done)

;; 16d nested DW: C dies at its boundary, B sees undefined and finishes;
;;     B-a runs once on B's normal end
(set! *log* '())
(define co16d-c (coroutine-create (lambda () (coroutine-yield 'c-y) (car 99))))
(define co16d-b (coroutine-create (lambda ()
  (dynamic-wind (lambda () (add! 'B-b))
                (lambda () (coroutine-yield 'b-y)
                          (coroutine-yield (coroutine-resume co16d-c))
                          (coroutine-yield (undef? (coroutine-resume co16d-c)))
                          'done)
                (lambda () (add! 'B-a))))))
(check 'co16d-r1 (coroutine-resume co16d-b) 'b-y)
(check 'co16d-r2 (coroutine-resume co16d-b) 'c-y)
(check 'co16d-r3 (coroutine-resume co16d-b) #t)
(check 'co16d-r4 (coroutine-resume co16d-b) 'done)
(check 'co16d-log (reverse *log*) '(B-b B-a))

;; 16e main-stack DW: coroutine error caught at the boundary, DW body resumes
(set! *log* '())
(define co16e (coroutine-create (lambda ()
  (dynamic-wind (lambda () (add! 'C-b))
                (lambda () (car 1))
                (lambda () (add! 'C-a))))))
(dynamic-wind (lambda () (add! 'M-b))
              (lambda () (coroutine-resume co16e) (add! 'M-t))
              (lambda () (add! 'M-a)))
(check 'co16e-log (reverse *log*) '(M-b C-b C-a M-t M-a))
(check 'co16e-st (coroutine-status co16e) 'dead)

;; 16f three-level nesting: C's error is contained; C-a, B-a, A-a run in order
(set! *log* '())
(define co16f-c (coroutine-create (lambda ()
  (dynamic-wind (lambda () (add! 'C-b))
                (lambda () (coroutine-yield 'c-y1) (coroutine-yield 'c-y2) (car 99))
                (lambda () (add! 'C-a))))))
(define co16f-b (coroutine-create (lambda ()
  (dynamic-wind (lambda () (add! 'B-b))
                (lambda () (coroutine-yield 'b-y1)
                          (coroutine-yield (coroutine-resume co16f-c))
                          (coroutine-yield (coroutine-resume co16f-c))
                          (coroutine-resume co16f-c)
                          'b-done)
                (lambda () (add! 'B-a))))))
(define co16f-a (coroutine-create (lambda ()
  (dynamic-wind (lambda () (add! 'A-b))
                (lambda () (coroutine-yield 'a-y1)
                          (coroutine-yield (coroutine-resume co16f-b))
                          (coroutine-yield (coroutine-resume co16f-b))
                          (coroutine-yield (coroutine-resume co16f-b))
                          (coroutine-resume co16f-b)
                          'a-done)
                (lambda () (add! 'A-a))))))
(check 'co16f-r1 (coroutine-resume co16f-a) 'a-y1)
(check 'co16f-r2 (coroutine-resume co16f-a) 'b-y1)
(check 'co16f-r3 (coroutine-resume co16f-a) 'c-y1)
(check 'co16f-r4 (coroutine-resume co16f-a) 'c-y2)
(check 'co16f-r5 (coroutine-resume co16f-a) 'a-done)
(check 'co16f-log (reverse *log*) '(A-b B-b C-b C-a B-a A-a))
(check 'co16f-stA (coroutine-status co16f-a) 'dead)

;; 16g DW after raises during teardown: caught and printed, A still finishes
(define co16g-c (coroutine-create
  (lambda ()
    (dynamic-wind
      (lambda () #f)
      (lambda () (coroutine-yield 'c-y1) (car 99))
      (lambda () (car 99))))))
(define co16g-a (coroutine-create
  (lambda ()
    (dynamic-wind
      (lambda () #f)
      (lambda () (coroutine-yield 'a-y1)
                (coroutine-resume co16g-c)
                (coroutine-resume co16g-c)
                'a-done)
      (lambda () (display "A-a"))))))
(check 'co16g-r1 (coroutine-resume co16g-a) 'a-y1)
(check 'co16g-r2 (coroutine-resume co16g-a) 'a-done)
(check 'co16g-stA (coroutine-status co16g-a) 'dead)
(display "end") (newline)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 18. coroutines: Lua-style multi-value resume/yield (values packing)
;; (merged from coroutine-test.scm)

(section "18. coroutine multi-value")

;; 1. basic round-trip + argument passing
(define mco1 (coroutine-create (lambda (x)
  (let ((y (coroutine-yield (+ x 1))))
    (coroutine-yield (+ y 1))))))
(check 'co-status-created (coroutine-status mco1) 'suspended)
(check 'co-resume1 (coroutine-resume mco1 10) 11)
(check 'co-status-yielded (coroutine-status mco1) 'suspended)
(check 'co-resume2 (coroutine-resume mco1 20) 21)
(check 'co-status-yielded2 (coroutine-status mco1) 'suspended)
(check 'co-resume3 (coroutine-resume mco1 30) 30)
(check 'co-status-done (coroutine-status mco1) 'dead)

;; 2. generator: loop yield
(define (co-gen n)
  (coroutine-create (lambda ()
    (let loop ((i 0))
      (if (< i n)
        (begin (coroutine-yield i) (loop (+ i 1)))
        'done)))))
(define co-g (co-gen 3))
(check 'co-gen1 (coroutine-resume co-g) 0)
(check 'co-gen2 (coroutine-resume co-g) 1)
(check 'co-gen3 (coroutine-resume co-g) 2)
(check 'co-gen4 (coroutine-resume co-g) 'done)
(check 'co-gen-dead (coroutine-status co-g) 'dead)

;; 3. resume a dead coroutine returns void (error sentinel)
(define co-d (coroutine-create (lambda () 42)))
(check 'co-dead-res1 (coroutine-resume co-d) 42)
(check 'co-dead-status (coroutine-status co-d) 'dead)

;; 4. coroutine-running / status inside the body
(define co-r (coroutine-create (lambda ()
  (check 'co-running-in (coroutine-running) co-r)
  (check 'co-status-in (coroutine-status co-r) 'running)
  (coroutine-yield 'x)
  (check 'co-status-in2 (coroutine-status co-r) 'running))))
(coroutine-resume co-r)
(check 'co-status-running (coroutine-status co-r) 'suspended)

;; 5. nested resume: A resumes B, B yields back, A yields back
(define co-b (coroutine-create (lambda ()
  (coroutine-yield 'from-b)
  (coroutine-yield 'from-b2))))
(define co-a (coroutine-create (lambda ()
  (check 'co-nested-stb (coroutine-status co-b) 'suspended)
  (check 'co-nested-rb (coroutine-resume co-b) 'from-b)
  (check 'co-nested-stb2 (coroutine-status co-b) 'suspended)
  (check 'co-nested-sta (coroutine-status co-a) 'running)
  (coroutine-yield 'from-a)
  (coroutine-yield 'from-a2))))
(check 'co-nested-ra (coroutine-resume co-a) 'from-a)
(check 'co-nested-sta2 (coroutine-status co-a) 'suspended)
(check 'co-nested-ra2 (coroutine-resume co-a) 'from-a2)
(check 'co-nested-ra3 (coroutine-resume co-a 0) 0)
(check 'co-nested-adead (coroutine-status co-a) 'dead)
(check 'co-nested-rb2 (coroutine-resume co-b) 'from-b2)
(check 'co-nested-rb3 (coroutine-resume co-b 0) 0)
(check 'co-nested-bdead (coroutine-status co-b) 'dead)

;; 6. call/cc inside a coroutine
(define co-cc (coroutine-create (lambda ()
  (call/cc (lambda (k)
    (coroutine-yield 'cc-enter)))
  (coroutine-yield 'cc-after))))
(check 'co-cc-r1 (coroutine-resume co-cc) 'cc-enter)
(check 'co-cc-r2 (coroutine-resume co-cc) 'cc-after)

;; 7. dynamic-wind: yield does not run after; coroutine end does
(define co-w (coroutine-create (lambda ()
  (dynamic-wind
    (lambda () (display "wind-before\n"))
    (lambda ()
      (coroutine-yield 'w-yield)
      'w-body-done)
    (lambda () (display "wind-after\n"))))))
(display "co-dw-r1: ") (display (coroutine-resume co-w)) (newline)
(display "co-dw-r2: ") (display (coroutine-resume co-w)) (newline)
(display "co-dw-status: ") (display (coroutine-status co-w)) (newline)

;; 8. tail-call yield: last expression of the body
(define co-t (coroutine-create (lambda (x)
  (display "tail-body-start\n")
  (coroutine-yield (+ x 100)))))
(check 'co-tail-r1 (coroutine-resume co-t 1) 101)
(check 'co-tail-r2 (coroutine-resume co-t 2) 2)
(check 'co-tail-st (coroutine-status co-t) 'dead)

;; 9. nested tail-call yield: resume = inner fn returns, coroutine continues
(define co-nt (coroutine-create (lambda ()
  (define (inner)
    (coroutine-yield 'inner-yield))
  (inner)
  'coro-done)))
(check 'co-nt-r1 (coroutine-resume co-nt) 'inner-yield)
(check 'co-nt-r2 (coroutine-resume co-nt) 'coro-done)
(check 'co-nt-st (coroutine-status co-nt) 'dead)

;; 10. multi-value yield -> resume returns multiple values
(define co-m (coroutine-create (lambda () (coroutine-yield 'a 'b 'c))))
(check 'co-multi-yield (call-with-values (lambda () (coroutine-resume co-m)) list) '(a b c))
(check 'co-multi-st (coroutine-status co-m) 'suspended)
(check 'co-multi-2 (call-with-values (lambda () (coroutine-resume co-m)) list) '())
(check 'co-multi-st2 (coroutine-status co-m) 'dead)

;; 11. first resume with multiple args: laid out as body lambda params
(define co-f (coroutine-create (lambda (a b c) (coroutine-yield (+ a b c)))))
(check 'co-first-resume-3 (coroutine-resume co-f 1 2 3) 6)

;; 12. later resume with multiple args -> yield returns multiple values
(define co-r2 (coroutine-create (lambda ()
  (call-with-values (lambda () (coroutine-yield 'ignored))
                    (lambda (p q) (coroutine-yield (+ p q)))))))
(check 'co-r2-first (coroutine-resume co-r2) 'ignored)
(check 'co-r2-multi (call-with-values (lambda () (coroutine-resume co-r2 10 20)) list) '(30))

;; 13. zero args: yield with no values -> resume returns 0 values
(define co-z (coroutine-create (lambda () (coroutine-yield))))
(check 'co-zero-yield (call-with-values (lambda () (coroutine-resume co-z)) list) '())

;; 14. zero args: first and later resumes with no args
(define co-z2 (coroutine-create (lambda ()
  (call-with-values (lambda () (coroutine-yield 'x))
                    (lambda () (coroutine-yield 'done))))))
(check 'co-zero-first (coroutine-resume co-z2) 'x)
(check 'co-zero-r2 (call-with-values (lambda () (coroutine-resume co-z2)) list) '(done))
(check 'co-zero-r3 (call-with-values (lambda () (coroutine-resume co-z2)) list) '())

;; 15. error sentinel: error inside coroutine is swallowed, resume returns void
(define co-e (coroutine-create (lambda () (car '()))))
(display "co-err-void: ")
(display (coroutine-resume co-e))
(display " [expect <void>]")
(newline)
(check 'co-err-status (coroutine-status co-e) 'dead)

;; 17. rest body lambda: excess first-resume args collected off the source
;;     (apply tail referenced / plain slots consed), no unbounded laying out
(define (co-mk n acc) (if (= n 0) acc (co-mk (- n 1) (cons n acc))))
(define co-rl (coroutine-create (lambda args (apply + args))))
(check 'co-rest-apply (apply coroutine-resume co-rl (co-mk 100 '())) 5050)
(define co-rl2 (coroutine-create (lambda args (apply + args))))
(check 'co-rest-plain (coroutine-resume co-rl2 1 2 3 4 5 6 7 8 9 10) 55)

;; 16. apply form: resume with multiple args / zero args
(define co-ac (coroutine-create (lambda ()
  (call-with-values (lambda () (coroutine-yield 'go))
                    (lambda (p q) (coroutine-yield (+ p q)))))))
(check 'co-apply-first (coroutine-resume co-ac) 'go)
(check 'co-apply-multi (call-with-values (lambda () (apply coroutine-resume co-ac '(5 7))) list) '(12))
(define co-ao (coroutine-create (lambda ()
  (call-with-values (lambda () (coroutine-yield 'go))
                    (lambda (p q) (apply coroutine-yield (list p q)))))))
(check 'co-apply-first-o (coroutine-resume co-ao) 'go)
(check 'co-apply-order (call-with-values (lambda () (apply coroutine-resume co-ao '(5 7))) list) '(5 7))
(define co-ac2 (coroutine-create (lambda ()
  (call-with-values (lambda () (coroutine-yield 'go))
                    (lambda () (coroutine-yield 'done2))))))
(check 'co-apply-first2 (coroutine-resume co-ac2) 'go)
(check 'co-apply-zero (call-with-values (lambda () (apply coroutine-resume co-ac2 '())) list) '(done2))

;; 17. apply form: yield with multiple args
(define co-y (coroutine-create (lambda ()
  (call-with-values (lambda () (apply coroutine-yield '(a b)))
                    (lambda (x y) (coroutine-yield (list x y)))))))
(check 'co-yield-apply (call-with-values (lambda () (coroutine-resume co-y)) list) '(a b))
(check 'co-yield-apply2 (call-with-values (lambda () (coroutine-resume co-y 'x 'y)) list) '((x y)))
(define co-yl (coroutine-create (lambda ()
  (apply coroutine-yield (co-mk 100 '()))
  'done)))
(check 'co-yield-long (call-with-values (lambda () (coroutine-resume co-yl))
                                      (lambda vs (length vs))) 100)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 19. error / pcall / xpcall  (moved from try-error-test.scm)

(section "19. error / pcall / xpcall")

;; xpcall: protected call with error handler (Lua xpcall semantics)
(check 'xpcall-normal (xpcall (lambda () 42) (lambda (e) 'oops)) 42)
(check 'xpcall-error (xpcall (lambda () (error "boom")) (lambda (e) e)) "boom")
(check 'xpcall-error-args (xpcall (lambda () (error "a" "b" 42)) (lambda (e) e))
       "a b 42")
(check 'xpcall-write-obj (xpcall (lambda () (error '(1 2))) (lambda (e) e)) "(1 2)")
(check 'xpcall-internal (xpcall (lambda () (car 1)) (lambda (e) e)) "car: not a pair")
(check 'xpcall-nested
       (xpcall (lambda ()
                 (xpcall (lambda () (error "inner"))
                         (lambda (e) (string-append "got:" e))))
               (lambda (e) 'outer-error))
       "got:inner")
(check 'xpcall-handler-error
       (xpcall (lambda ()
                 (xpcall (lambda () (error "x"))
                         (lambda (e) (error "handler-fail"))))
               (lambda (e) e))
       "handler-fail")

;; dynamic-wind after thunk still runs when thunk raises
(set! *log* '())
(xpcall (lambda ()
          (dynamic-wind (lambda () #f)
                        (lambda () (error "dw"))
                        (lambda () (set! *log* (cons 'after *log*)))))
        (lambda (e) #f))
(check 'xpcall-dw-after *log* '(after))

;; (error) with no argument is itself an error (caught by outer xpcall)
(check 'error-noargs
       (xpcall (lambda () (xpcall (lambda () (error)) (lambda (e) e)))
               (lambda (e) e))
       "error: needs at least 1 argument")

;; xpcall requires closures / arity
(check 'xpcall-nonclosure
       (xpcall (lambda () (xpcall 1 (lambda (e) e))) (lambda (e) e))
       "xpcall: not a closure")
(check 'xpcall-thunk-arity
       (xpcall (lambda () (xpcall (lambda (x) x) (lambda (e) e))) (lambda (e) e))
       "xpcall: thunk must take no arguments")
(check 'xpcall-handler-arity
       (xpcall (lambda () (xpcall (lambda () 1) (lambda (a b) b))) (lambda (e) e))
       "xpcall: handler must take exactly 1 argument")

;; xpcall inside a coroutine
(check 'xpcall-in-coroutine
       (coroutine-resume
        (coroutine-create
         (lambda ()
           (coroutine-yield
            (xpcall (lambda () (error "in-co")) (lambda (e) e))))))
       "in-co")

;; rest lambda as thunk is allowed (no fixed args)
(check 'xpcall-rest-thunk
       (xpcall (lambda args (if (null? args) 'ok 'bad)) (lambda (e) e))
       'ok)

;; xpcall result can be a string built by handler
(check 'xpcall-handler-string
       (xpcall (lambda () (error "k1" "k2")) (lambda (e) (string-length e)))
       5)  ; "k1 k2" length 5

;; pcall (Lua-style): (ok . result) / (false . err-string)
(check 'pcall-ok (pcall (lambda () 42)) '(#t . 42))
(check 'pcall-ok-list (pcall (lambda () (list 1 2 3))) '(#t 1 2 3))
(check 'pcall-err (pcall (lambda () (error "boom"))) '(#f . "boom"))
(check 'pcall-internal (pcall (lambda () (car 1))) '(#f . "car: not a pair"))
(check 'pcall-recover
       (begin
         (pcall (lambda () (error "x")))
         (pcall (lambda () (+ 1 2))))
       '(#t . 3))
(check 'pcall-nonclosure
       (xpcall (lambda () (pcall 7)) (lambda (e) e))
       "pcall: not a closure")
(check 'pcall-thunk-arity
       (xpcall (lambda () (pcall (lambda (x) x))) (lambda (e) e))
       "pcall: thunk must take no arguments")
(check 'pcall-err-len
       (let ((res (pcall (lambda () (error "k1" "k2")))))
         (if (car res) 'bad (string-length (cdr res))))
       5)
(check 'pcall-in-coroutine
       (coroutine-resume
        (coroutine-create
         (lambda ()
           (coroutine-yield
            (pcall (lambda () (error "in-co")))))))
       '(#f . "in-co"))

;; 20. Language-layer additions: R7RS bitwise/bytevector/format + Lua string-format/pack
;;     20a. bytevector (R7RS-small)
(check 'bytevector-len (bytevector-length (bytevector 1 2 3)) 3)
(check 'bytevector-ref (bytevector-u8-ref (bytevector 10 20 30) 1) 20)
(check 'bytevector-set! (let ((b (make-bytevector 2 7)))
                          (bytevector-u8-set! b 0 9)
                          (bytevector-u8-ref b 0))
       9)
(check 'bytevector=? (bytevector=? (bytevector 1 2) (bytevector 1 2)) #t)
(check 'bytevector-copy-range (bytevector-copy (bytevector 1 2 3 4) 1 3)
       (bytevector 2 3))
(check 'bytevector-append (bytevector-append (bytevector 1) (bytevector 2 3))
       (bytevector 1 2 3))
(check 'bytevector-utf8-roundtrip (utf8->string (string->utf8 "héllo")) "héllo")
(check 'bytevector-equal (equal? (bytevector 1 2) (bytevector 1 2)) #t)
(check 'bytevector-copy!
       (let ((to (make-bytevector 4 0)))
         (bytevector-copy! to 1 (bytevector 10 20))
         (list (bytevector-u8-ref to 0) (bytevector-u8-ref to 1)
               (bytevector-u8-ref to 2) (bytevector-u8-ref to 3)))
       '(0 10 20 0))

;;     20b. bitwise (SRFI-151 / R7RS-large (scheme bitwise))
(check 'bitwise-not-0 (bitwise-not 0) -1)
(check 'bitwise-and (bitwise-and #xff #x0f) 15)
(check 'bitwise-ior (bitwise-ior #x0f #xf0) 255)
(check 'bitwise-xor (bitwise-xor #xff #x0f) 240)
(check 'logand-alias (logand 12 10) 8)
(check 'ash-left (ash 1 16) 65536)
(check 'ash-right-arithmetic (ash -8 -1) -4)
(check 'ash-overflow (ash 1 64) 0)
(check 'shift-left (bitwise-arithmetic-shift-left 3 2) 12)
(check 'shift-right (bitwise-arithmetic-shift-right -16 2) -4)
(check 'bit-count (bit-count 255) 8)
(check 'bit-count-neg (bit-count -1) 0)
(check 'bit-length (bit-length 255) 8)
(check 'bit-set?-true (bit-set? 3 8) #t)
(check 'bit-set?-false (bit-set? 0 8) #f)
(check 'bit-field (bit-field #b101010 2 5) 2)
(check 'first-set-bit (first-set-bit 12) 2)
(check 'first-set-bit-zero (first-set-bit 0) -1)
(check 'bitwise-and-variadic (bitwise-and 7 6 4) 4)
(check 'bitwise-ior-variadic (bitwise-ior 1 2 4) 7)
(check 'bitwise-not-neg1 (bitwise-not -1) 0)
(check 'logcount-alias (logcount 255) 8)
(check 'integer-length-alias (integer-length 255) 8)

;;     20c. random (SRFI-27 names; reproducible via random-seed)
(check 'random-integer-range
       (let ((n (begin (random-seed 1) (random-integer 100))))
         (and (>= n 0) (< n 100)))
       #t)
(check 'random-integer-one (begin (random-seed 1) (random-integer 1)) 0)
(check 'random-reproducible
       (begin (random-seed 42) (let ((a (random-integer 1000000)))
                                 (random-seed 42)
                                 (= a (random-integer 1000000))))
       #t)
(check 'random-real-range
       (let ((r (begin (random-seed 1) (random-real))))
         (and (>= r 0) (< r 1)))
       #t)
(check 'random-real-reproducible
       (begin (random-seed 42) (let ((a (random-real)))
                                 (random-seed 42)
                                 (= a (random-real))))
       #t)

;;     20d. format / string-format (SRFI-28 + SRFI-159 numeric subset)
(check 'format-a (format "hello ~a" "world") "hello world")
(check 'format-a-vs-s (format "~a vs ~s" "str" "str") "str vs \"str\"")
(check 'format-hex-zero-pad (format ":~08x-~d" 123456 42) ":0001e240-42")
(check 'format-base (format "d=~d x=~x o=~o b=~b" 255 255 255 255)
       "d=255 x=ff o=377 b=11111111")
(check 'format-float-prec (format "GC ~,2f Kb" 1.5) "GC 1.50 Kb")
(check 'format-exp (format "exp=~e" 0.001) "exp=1.000000e-03")
(check 'format-char-newline (format "~c~%" #\a) "a\n")
(check 'format-percent (format "100~~ sure") "100~ sure")
(check 'format-f (format "~f" 3.14) "3.140000")
(check 'format-g (format "~g" 100.0) "100")
(check 'string-format-name (string-format "a=%s" "(1 2)") "a=(1 2)")

;;     20e. string-pack / string-unpack / packsize + char/byte
(check 'pack-s4
       (let ((s (string-pack "<s4" "hello")))
         (list (string-length s) (%values->list (string-unpack "<s4" s))))
       '(9 ("hello" 9)))
(check 'pack-I2 (string-pack "<I2" 4660)
       (string-char 52 18))
(check 'unpack-I2 (%values->list (string-unpack "<I2" (string-pack "<I2" 4660)))
       '(4660 2))
(check 'unpack-ws-mixed
       (let ((s (string-pack "I1I1>I2" 1 2 500)))
         (%values->list (string-unpack "I1I1>I2" s)))
       '(1 2 500 4))
(check 'unpack-I8 (%values->list (string-unpack ">I8" (string-pack ">I8" 123456789)))
       '(123456789 8))
(check 'unpack-negative-pos
       (let ((msg (string-pack ">I4xI4" 11 22)))
         (%values->list (string-unpack ">I4" msg -4)))
       '(22 9))
(check 'unpack-dns-style
       (let ((d (string-pack ">HHHHHH" 1 2 3 4 5 6)))
         (%values->list (string-unpack ">HHHHHH" d)))
       '(1 2 3 4 5 6 12))
(check 'unpack-s2 (%values->list (string-unpack ">HHI4s2"
                                                (string-pack ">HHI4s2" 1 2 70000 "ab")))
       '(1 2 70000 "ab" 12))
(check 'packsize (string-packsize ">HHI4") 8)
(check 'unpack-signed (%values->list (string-unpack "<h" (string-pack "<h" -2)))
       '(-2 2))
(check 'unpack-double (%values->list (string-unpack "<d" (string-pack "<d" 3.25)))
       '(3.250000 8))
(check 'unpack-z (%values->list (string-unpack "z" (string-pack "z" "hi")))
       '("hi" 3))
(check 'string-char (string-char 65 66 67) "ABC")
(check 'string-byte-one (%values->list (string-byte "ABC" 1)) '(66))
(check 'string-byte-range (%values->list (string-byte "ABC" 0 2)) '(65 66 67))
(check 'string-byte-all (%values->list (string-byte "ABC" 0 -1)) '(65 66 67))
(check 'string-byte-default (%values->list (string-byte "ABC")) '(65))
(check 'pack-X-align (string-length (string-pack "bX4b" 1 2)) 5)
(check 'unpack-X-align (%values->list (string-unpack "bX4b" (string-pack "bX4b" 1 2)))
       '(1 2 5))
(check 'unpack-c (%values->list (string-unpack "c3" (string-pack "c3" "abcdef")))
       '("abc" 3))
(check 'unpack-f-float
       (let ((got (car (%values->list (string-unpack "<f" (string-pack "<f" 1.5)))))
             (nxt (cadr (%values->list (string-unpack "<f" (string-pack "<f" 1.5))))))
         (and (= got 1.5) (= nxt 4)))
       #t)
(check 'pack-native-endian (equal? (string-pack "=I2" 4660) (string-pack "<I2" 4660)) #t)
(check 'unpack-J (%values->list (string-unpack ">J" (string-pack ">J" 123456789)))
       '(123456789 8))

;;     20f. string-format (Lua printf-style, distinct from R7RS format)
(check 'lua-s (string-format "%s" "hi") "hi")
(check 'lua-d (string-format "%d" -7) "-7")
(check 'lua-x (string-format "%x" 255) "ff")
(check 'lua-X (string-format "%X" 255) "FF")
(check 'lua-08x (string-format "%08x" 255) "000000ff")
(check 'lua-02X (string-format "%02X" 10) "0A")
(check 'lua-03d (string-format "%03d" 5) "005")
(check 'lua-left (string-format "%-5s" "ab") "ab   ")
(check 'lua-2f (string-format "%.2f" 3.14159) "3.14")
(check 'lua-8.2f (string-format "%8.2f" 3.14159) "    3.14")
(check 'lua-g (string-format "%g" 100.0) "100")
(check 'lua-c (string-format "%c" 65) "A")
(check 'lua-u (string-format "%u" -1) "18446744073709551615")
(check 'lua-o (string-format "%o" 8) "10")
(check 'lua-pct (string-format "100%%") "100%")
(check 'lua-multi (string-format "%s:%d" "a" 1) "a:1")
(check 'lua-prec-s (string-format "%.3s" "abcdef") "abc")
(check 'lua-httpd (string-format "HTTP/1.1 %03d %s" 200 "OK") "HTTP/1.1 200 OK")
(check 'lua-i (string-format "%i" -7) "-7")
(check 'lua-e (string-format "%e" 100.1) "1.001000e+02")
(check 'lua-E (string-format "%E" 100.1) "1.001000E+02")
(check 'lua-G (string-format "%G" 100.1) "100.1")
(check 'lua-f (string-format "%f" 3.14) "3.140000")
(check 'lua-plus-d (string-format "%+d" 5) "+5")
(check 'lua-sharp-x (string-format "%#x" 255) "0xff")

;;     20g. type (Lua `type` counterpart; returns type symbol)
(check 'type-number (type 42) 'number)
(check 'type-float (type 3.14) 'number)
(check 'type-string (type "hi") 'string)
(check 'type-boolean (type #t) 'boolean)
(check 'type-char (type #\a) 'char)
(check 'type-symbol (type 'sym) 'symbol)
(check 'type-pair (type '(1 2)) 'pair)
(check 'type-null (type '()) 'null)
(check 'type-vector (type (vector 1 2)) 'vector)
(check 'type-hash (type (make-hash-table)) 'hash-table)
(check 'type-bytes (type (bytevector 1 2)) 'bytevector)
(check 'type-closure (type (lambda (x) x)) 'closure)

(display "aftertest passed ") (display *npass*)
(display " failed ") (display *nfail*)
(newline)
(if (= *nfail* 0)
    (begin (display "ALL PASS") (newline))
    (begin (display "SOME FAIL") (newline)))