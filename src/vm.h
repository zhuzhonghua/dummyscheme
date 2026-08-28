#pragma once

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstddef>
#include <cfloat>
#include <cmath>
#include <cctype>
#include <ctime>
#include <climits>
#include <algorithm>

namespace Scheme {

static void *salloc(void *ptr, size_t nsize) {
  if (nsize == 0) {
    free(ptr);
    return NULL;
  }
  else
    return realloc(ptr, nsize);
}

#define CASE_I 'i':case 'I'
#define CASE_EXPMARK 'd':case 'D':case 'e':case 'E':case 'f':case 'F':case 'l':case 'L':case 's': case 'S'
#define CASE_AFDIGIT 'a':case 'A':case 'b':case 'B':case 'c':case 'C':case 'd':case 'D':case 'e':case 'E':case 'f':case 'F'
#define CASE_09DIGIT '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9'
// isspace
#define CASE_BLANK '\n':case ' ':case '\t':case '\r':case '\0':case '\v':case '\f'

#define hygiene_id_post "hygiene-var"

typedef unsigned int Instruction;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char byte;

#define MAXSHORTLEN 40
#define SYMBOLCASEI

// precompiled code ('<esc>scheme')
#define SCM_SIGNATURE	"\x1bscheme"

typedef long scm_int;
typedef unsigned long scm_uint;
#define scm_int_fmt "%ld"
#define INT_BITS (8*sizeof(scm_int))
#define INT_BITS_HALF (4*sizeof(scm_int))
#define UINT_MASK_LOW 0xFFFFFFFFULL
#define UINT_MASK_HIGH 0xFFFFFFFF00000000ULL
#define UINT_BASE 1000000000ULL // 10^9 (must be < 2^32 for div128by64)
#define UINT_BASE_LEN 9
#define SCM_INT_MAX LONG_MAX
#define SCM_INT_MIN LONG_MIN

typedef double scm_float;

#define Debug(x)
#define DebugMem(x)
#define DebugReg(x)
#define DebugVT
//#define DebugSRule
//#define DebugCCode
//#define DebugQQuote
#define DebugPrintLine Print("\n%s:%d\n", __FILE__, __LINE__);

#define DebugAssertStop(x) DebugPrintLine; x
//#define DebugAssertStop(x)

#define DebugAssert(vm) DebugAssertStop(vm->printframe();*((int*)0) = 0)

#define TRY try

#define CATCH(err) catch(const char* err)

#define STR(s) #s

#define Print0(file, fmt, ...) fprintf(file, fmt, ##__VA_ARGS__)
#define Print(fmt, ...) Print0(stderr, fmt, ##__VA_ARGS__)

#define Assert(vm, cond, fmt, ...) do{                  \
    if (!(cond)) {Print("\nException\n");               \
      Print(fmt, ##__VA_ARGS__);                        \
      DebugAssert(vm);                                  \
      throw STR(cond);                                  \
    }}while(0)

#define AssertVT(vm, cond, vt, fmt, ...) do{            \
    if (!(cond)) {Print("\nException\n");               \
      vm->printvalue0(vt);Print("\n");                  \
      Print(fmt, ##__VA_ARGS__);                        \
      DebugAssert(vm);                                  \
      throw STR(cond);                                  \
    }}while(0)

#define AssertVT2(vm, cond, vt, vt2, fmt, ...) do{      \
    if (!(cond)) {Print("\nException\n");               \
      vm->printvalue0(vt);Print("\n");                  \
      vm->printvalue0(vt2);Print("\n");                 \
      Print(fmt, ##__VA_ARGS__);                        \
      DebugAssert(vm);                                  \
      throw STR(cond);                                  \
    }}while(0)

#define AssertArgPair(vm, p, op) AssertArg(vm, ispair(p), op, p, " not a pair")

#define AssertArg(vm, cond, op, vt, fmt, ...) do{   \
    if (!(cond)) {Print("\nException in %s: ", op); \
      vm->printvalue0(vt);                          \
      Print(fmt, ##__VA_ARGS__);                    \
      DebugAssert(vm);                              \
      throw STR(cond);                              \
    }}while(0)

#define Error(vm, fmt, ...) do{                   \
    Print(fmt, ##__VA_ARGS__);                    \
    DebugAssert(vm);                              \
    throw "Error ";                               \
  }while(0)

#define ErrorVT(vm, vt, fmt, ...) do{             \
    Print("\nError:\n");                          \
    vm->printvalue0(vt);                          \
    Print("\n");                                  \
    Print(fmt, ##__VA_ARGS__);                    \
    DebugAssert(vm);                              \
    throw "Error ";                               \
  }while(0)

#define Serrorvt(vm, vt, fmt, ...) do {           \
    Print(fmt, ##__VA_ARGS__);                    \
    vm->printvalue0(vt, true);                    \
    DebugAssertStop(*((int*)0) = 0);              \
 } while(0)

#define Serror(fmt, ...) do {                     \
 Print(fmt, ##__VA_ARGS__);                       \
 DebugAssertStop(*((int*)0) = 0);                 \
 } while(0)

class VM;

struct Lbuffer {
  static const int INIT_LEN;

  VM* vm;

  char* buf;
  int size;
  int count;

  Lbuffer(VM* v);
  ~Lbuffer();
  void close();

  void put(char* str) {
    put(str, strlen(str));
  }
  void put(const char* str) {
    put(str, strlen(str));
  }
  void put(char* str, int n) {
    for (int i=0; i<n; i++)
      put(str[i]);
  }
  void put(const char* str, int n) {
    for (int i=0; i<n; i++)
      put(str[i]);
  }

  void put(char c);
  void reset() { count = 0; }
  void reverse() { reverse(0); }
  void reverse(int n) {
    for (int i = n, j = count - 1; i < j; i++, j--) {
      char c = buf[i];
      buf[i] = buf[j];
      buf[j] = c;
    }
  }
};

typedef void * (*ScmAlloc) (void *ptr, size_t nsize);

struct ValueT;
typedef ValueT (*CProc0)(VM* vm);
typedef ValueT (*CProc1)(VM* vm, ValueT*);
typedef ValueT (*CProc2)(VM* vm, ValueT*, ValueT*);
typedef ValueT (*CProc3)(VM* vm, ValueT*, ValueT*, ValueT*);
typedef ValueT (*CProc4)(VM* vm, ValueT*, ValueT*, ValueT*, ValueT*);
typedef ValueT (*CProc5)(VM* vm, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*);
typedef ValueT (*CProc6)(VM* vm, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*);
typedef ValueT (*CProc7)(VM* vm, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*);
typedef ValueT (*CProc8)(VM* vm, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*);
typedef ValueT (*CProc9)(VM* vm, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*);
typedef ValueT (*CProc10)(VM* vm, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*, ValueT*);

union CProc {
  CProc0 cp0;
  CProc1 cp1;
  CProc2 cp2;
  CProc3 cp3;
  CProc4 cp4;
  CProc5 cp5;
  CProc6 cp6;
  CProc7 cp7;
  CProc8 cp8;
  CProc9 cp9;
  CProc10 cp10;
};

struct RefObject;
typedef RefObject* RefPtr;

struct PairObj;
typedef PairObj* PairPtr;

struct StrObj;
typedef StrObj* SymPtr;
typedef StrObj* StrPtr;

struct HygieneSymObj;
typedef HygieneSymObj* HygieneSymPtr;

struct LambdaObj;
typedef LambdaObj* LambdaPtr;

struct SyntaxObj;
typedef SyntaxObj* SyntaxPtr;

struct ClosureObj;
typedef ClosureObj* ClosurePtr;

struct ContinuationObj;
typedef ContinuationObj* ContinuationPtr;

class ArrayObj;
class SCompiler;

struct NativeProcObj;

struct CallFrame;

//#define checkexp(c, e) (Util::assert((c), STR(c), "\nEXE FAIL\n%s", STR(e)), (e))
#define checkexp(c, e) (e)

/* Type Ref */
#define settyperef(VT, tp, e) ((VT)->v.p = (e), (VT)->t = tp)
#define istype(VT, tp) ((VT)->t == tp)
#define vttype(VT) ((VT)->t)
#define typevt(VT) ((VT)->t)
#define settype(VT, tp) ((VT)->t = tp)
#define refp(VT) ((VT)->v.p)
#define toref(VT, PTR) ((PTR) refp(VT))

/* Ref */
#define setref(VT, e) settyperef(VT, VT_REF, e)
#define isref(VT) ((VT)->t >= VT_REF)

/* LambdaObj */
#define lambdaref(VT) toref(VT, LambdaObj*)
#define setlambda(VT, e) settyperef(VT, VT_REF_LAMBDA, e)

/* CallFrame */
#define callframeref(VT) toref(VT, CallFrame*)

/* ContinuationObj */
#define iscontinuation(VT) istype(VT, VT_REF_CONTINUATION)
#define setcontinuation(VT, e) settyperef(VT, VT_REF_CONTINUATION, e)
#define continuationref(VT) toref(VT, ContinuationPtr)

/* ClosureObj */
#define isclosure(VT) istype(VT, VT_REF_CLOSURE)
#define setclosure(VT, e) settyperef(VT, VT_REF_CLOSURE, e)
#define closureref(VT) toref(VT, ClosurePtr)

/* NativeProc */
#define isnativeproc(VT) istype(VT, VT_REF_NATIVE)
#define setnativeproc(VT, f) settyperef(VT, VT_REF_NATIVE, f)
#define nativeprocref(VT) toref(VT, NativeProcObj*)

/* InputPort OutputPort */
#define isiport(VT) istype(VT, VT_REF_IPORT)
#define isoport(VT) istype(VT, VT_REF_OPORT)
#define setiport(VT, e) settyperef(VT, VT_REF_IPORT, e)
#define setoport(VT, e) settyperef(VT, VT_REF_OPORT, e)
#define iportref(VT) toref(VT, InputPortObj*)
#define oportref(VT) toref(VT, OutputPortObj*)
#define oportstrref(VT) toref(VT, OutputPortStrObj*)

/* SymObj Or StrObj */
#define ishygienesym(VT) istype(VT, VT_REF_HYGIENE_SYM)
#define sethygienesym(VT, e) settyperef(VT, VT_REF_HYGIENE_SYM, e)
#define hygienesymref(VT) toref(VT, HygieneSymPtr)

#define issym(VT) istype(VT, VT_REF_SYM)
#define symref(VT) toref(VT, SymPtr)
#define setsym(VT, e) settyperef(VT, VT_REF_SYM, e)

#define vtstr(VT) strref(VT)->str
#define vtstrlen(VT) strref(VT)->len
#define strref(VT) toref(VT, StrPtr)
#define isstr(VT) istype(VT, VT_REF_STR)
#define setstr(VT, e) settyperef(VT, VT_REF_STR, e)

#define Ssstr(VT) ((VT)->str)
#define Sshash(VT) ((VT)->hash)
#define Sslen(VT) ((VT)->len)

/* Pair */
#define PAIR_FOR(p, list) \
  for(ValueT* (p) = (list); !isnull(p); (p) = Scdr(p))

#define ANNOTATE_PAIR_FOR(p, list) \
  for(ValueT* (p) = annotatevt(list); !isnull(p); (p) = Scdr(p))

#define ispair(VT) istype(VT, VT_REF_PAIR)
#define pairref(VT) toref(VT, PairPtr)
#define Spairref(VT) checkexp(ispair(VT), pairref(VT))
#define setpair(VT, e) settyperef(VT, VT_REF_PAIR, e)

#define Scdr(VT) (&(pairref(VT)->scdr))
#define Scar(VT) (&(pairref(VT)->scar))
#define Scadr(VT) Scar(Scdr(VT))
#define Scaddr(VT) Scar(Scddr(VT))
#define Scddr(VT) Scdr(Scdr(VT))

/* Array */
#define isarray(VT) istype(VT, VT_REF_ARRAY)
#define arrayref(VT) toref(VT, ArrayObj*)
#define setarray(VT, e) settyperef(VT, VT_REF_ARRAY, e)

/* SyntaxObj */
#define ismacro(VT) istype(VT, VT_REF_MACRO)
#define setmacro(VT, e) settyperef(VT, VT_REF_MACRO, e)

#define issyntax(VT) istype(VT, VT_REF_SYNTAX)
#define setsyntax(VT, e) settyperef(VT, VT_REF_SYNTAX, e)
#define syntaxref(VT) toref(VT, SyntaxObj*)

#define issyntaxrules(VT) istype(VT, VT_REF_SYNTAX_RULES)
#define setsyntaxrules(VT, e) settyperef(VT, VT_REF_SYNTAX_RULES, e)
#define syntaxrules(VT) toref(VT, SyntaxRules*)

/* Num */
#define isnumber(VT)                                                  \
  (isnumi(VT) || isnumratio(VT) || isnumreal(VT) || isnumcomplex(VT) || isnumbig(VT))

struct NumBigObj;
/* Num Big */
#define isnumbig(VT) istype(VT, VT_REF_NUM_BIG)
#define numbigref(VT) toref(VT, NumBigObj*)
#define setnumbig(VT, e) settyperef(VT, VT_REF_NUM_BIG, e)

/* Num I */
#define setnumi(VT, e) ((VT)->v.num.i = (e),(VT)->t = VT_NUM_INTEGER)
#define numi(VT) (VT)->v.num.i
#define isnumi(VT) istype(VT, VT_NUM_INTEGER)

/* Num Real */
#define setnumreal(VT, e) ((VT)->v.num.r = (e),(VT)->t = VT_NUM_REAL)
#define numreal(VT) (VT)->v.num.r
#define isnumreal(VT) istype(VT, VT_NUM_REAL)

/* Num Ratio */
#define numratioref(VT) toref(VT, NumRatioObj*)
#define setnumratio(VT, e) settyperef(VT, VT_REF_NUM_RATIO, e)
#define isnumratio(VT) istype(VT, VT_REF_NUM_RATIO)
#define numrationu(VT) numratioref(VT)->numerator
#define numratiode(VT) numratioref(VT)->denominator

/* Num Complex */
#define numcomplexref(VT) toref(VT, NumComplexObj*)
#define setnumcomplex(VT, e) settyperef(VT, VT_REF_NUM_COMPLEX, e)
#define isnumcomplex(VT) istype(VT, VT_REF_NUM_COMPLEX)
#define numcomplexreal(VT) numcomplexref(VT)->d[0]
#define numcompleximag(VT) numcomplexref(VT)->d[1]

/* Null */
#define isnull(VT) istype(VT, VT_NULL)
#define setnull(VT) (VT)->reset()

/* Eof */
#define iseof(VT) istype(VT, VT_EOF)

/* True or False */
#define isfalse(VT) istype(VT, VT_FALSE)
#define istrue(VT) istype(VT, VT_TRUE)
#define isboolean(VT) isfalse(VT) || istrue(VT)

/* Char */
#define ischar(VT) istype(VT, VT_CHAR)
#define vtchar(VT) (VT)->v.c
#define setchar(VT, e) ((VT)->v.c = (e),(VT)->t = VT_CHAR)

/* undefined */
#define isundefined(VT) istype(VT, VT_UNDEFINED)
#define setundefined(VT) ((VT)->t = VT_UNDEFINED)

#define isvoid(VT) istype(VT, VT_VOID)

#define frombool(STAT) ((STAT) ? Strueref : Sfalseref)

/* AnnotationObj */
#define setannotate(VT, e) settyperef(VT, VT_REF_ANNOTATION, e)
#define annotateref(VT) toref(VT, AnnotationObj*)
#define annotatevt(VT) (&(annotateref(VT)->vt))
#define annotateline(VT) (annotateref(VT)->line)

/* Const */
#define Strueref (&ValueT::trueref)
#define Sfalseref (&ValueT::falseref)
#define Snullref (&ValueT::nullref)
#define Seofref (&ValueT::eofref)

#define Sundefined (&ValueT::undefined)
#define Svoidref (&ValueT::voidref)

#define Snext (&ValueT::linknext)
#define Sreturn (&ValueT::linkreturn)

#define ishykeyword(VT, VD) (ishygienesym(VT) && hygienesymref(VT)->sym == symref(VD))
#define iskeyword(VT, VD) ((issym(VT) && symref(VT) == symref(VD)) || ishykeyword(VT, VD))

#define iskwdefine(vm, VT) (iskeyword(VT, &vm->definevt))
#define iskwset(vm, VT) (iskeyword(VT, &vm->setvt))
#define iskwbegin(vm, VT) (iskeyword(VT, &vm->beginvt))
#define iskwif(vm, VT) (iskeyword(VT, &vm->ifvt))
#define iskwlambda(vm, VT) (iskeyword(VT, &vm->lambdavt))
#define iskwsyntaxrules(vm, VT) (iskeyword(VT, &vm->syntaxrvt))
#define iskwsyntaxerr(vm, VT) (iskeyword(VT, &vm->syntaxerrvt))
#define iskwdefsyntax(vm, VT) (iskeyword(VT, &vm->defsyntaxvt))
#define iskwellipsis(vm, VT) (iskeyword(VT, &vm->ellipsisvt))

#define iskwquote(vm, VT) (iskeyword(VT, &vm->quotevt))
#define iskwqquote(vm, VT) (iskeyword(VT, &vm->qquotevt))
#define iskwuquote(vm, VT) (iskeyword(VT, &vm->uquotevt))
#define iskwuquotes(vm, VT) (iskeyword(VT, &vm->uquotesvt))

#define iskwform(VT, kw) (ispair(VT) && iskeyword(Scar(VT), (kw)) && ispair(Scdr(VT)) && isnull(Scddr(VT)))
#define isformquote(vm, VT) (iskwform(VT, &vm->quotevt))
#define isformuquote(vm, VT) (iskwform(VT, &vm->uquotevt))
#define isformuquotes(vm, VT) (iskwform(VT, &vm->uquotesvt))
#define isformqquote(vm, VT) (iskwform(VT, &vm->qquotevt))

enum ValueTEnum {
  VT_UNDEFINED = 0, // internal use
  VT_VOID, // the unspecified value
  VT_NULL, // nil or null
  VT_EOF,
  VT_NUM_INTEGER,
  VT_NUM_REAL,
  VT_FALSE,
  VT_TRUE,
  VT_CHAR,
  VT_REF, /* all types below must be gcref type */
  VT_REF_NUM_RATIO,
  VT_REF_NUM_COMPLEX,
  VT_REF_NUM_BIG,
  VT_REF_STR,
  VT_REF_SYM,
  VT_REF_HYGIENE_SYM,
  VT_REF_PAIR,
  VT_REF_ARRAY,
  VT_REF_NATIVE,
  VT_REF_IPORT,
  VT_REF_OPORT,
  VT_REF_LAMBDA,
  VT_REF_CLOSURE,
  VT_REF_CONTINUATION,
  VT_REF_MACRO,
  VT_REF_SYNTAX,
  VT_REF_SYNTAX_RULES,
  VT_REF_ANNOTATION,
  VT_REF_STACKSEG,
  VT_REF_FRAMESEG,
};

union BasicNum {
  scm_int i; // integer
  double r; // real
};

union ValueU {
  RefPtr p;
  BasicNum num;
  char c;
};

struct ValueT {
  static ValueT eofref;

  static ValueT nullref;
  static ValueT undefined;
  static ValueT voidref;

  static ValueT trueref;
  static ValueT falseref;

  static ValueT linknext;
  static ValueT linkreturn;

  ValueU v;

  byte t;

  ValueT() { reset(); }
  ValueT(int type):ValueT() { t = type; }

  ValueT(const ValueT& rhs):ValueT() { t = rhs.t; v = rhs.v; }
  ValueT(ValueT* rhs):ValueT() { copy(rhs); }

  const ValueT& operator = (const ValueT& rhs) {
    t = rhs.t; v = rhs.v;
    return *this;
  }
  void copy(ValueT* rhs) { t = rhs->t; v = rhs->v; }
  const ValueT& operator = (ValueT* rhs) { copy(rhs); return *this; }

  void reset() { t = VT_NULL; v.p = NULL; }
private:
  ValueT(bool);
};

#define CONST     0
#define GROUP0BIT 1 // object is put in group0
#define GROUP1BIT 2 // object is put in group1
#define FULLGCBIT 3 // set 1 when fullgc
#define IMMUTABLE 4

#define bitmask(a) (1 << (a))

#define constmask bitmask(CONST)
#define bit0mask bitmask(GROUP0BIT)
#define bit1mask bitmask(GROUP1BIT)
#define fullgcmask bitmask(FULLGCBIT)
#define immutmask bitmask(IMMUTABLE)

#define GCBITS (bit0mask | bit1mask | fullgcmask)

#define setgcmask(bitmask) do {                 \
    bitmask[0] = bit0mask;                      \
    bitmask[1] = bit1mask;} while(0)


#define setfullgc(bitmask) do {                     \
    bitmask[0] = fullgcmask | bit0mask;             \
    bitmask[1] = fullgcmask | bit1mask; } while(0)

#define Check(a) GC(vm)->trace(a)

#define Visit1(a) virtual void visit(VM* vm) {Check(a);}
#define Visit2(a, b) virtual void visit(VM* vm) {Check(a);Check(b);}
#define Visit3(a, b, c) virtual void visit(VM* vm) {Check(a);Check(b);Check(3);}
#define GetSize(T) virtual int getsize() { return sizeof(T); }

struct RefObject {
  RefObject() { marked = 0; gcnxt = NULL; }
  virtual ~RefObject() {}

  bool isimmutable() { return marked & immutmask; }
  void setimmutable() { marked |= immutmask; }

  bool isconst() { return marked & constmask; }
  void markconst() { marked |= constmask; }

  bool isgcmark(byte bits) { return bits & marked; }
  void gcmark(byte bit) { marked = ((marked & ~GCBITS) | bit); }

  RefPtr link(RefPtr ref) { gcnxt = ref; return this; }

  virtual void visit(VM* vm) {}
  virtual void finz(VM* vm);

  virtual int getsize() = 0;

  RefPtr gcnxt;
  byte marked;
};

#define VEC_REVERSE_FOR(i, vec) for(int i = (vec)->n-1; i >=0; i--)

#define VEC_FOR(i, vec) for(int i = 0; i < (vec)->n; i++)

#define vec_finz(T, vm, arr) do {                     \
    if ((arr)->ptr)                                   \
      vm->free((arr)->ptr, (arr)->size * sizeof(T));  \
    (arr)->init();                                    \
  } while(0)
#define vec_shrink(T, vm, arr) do {                   \
    if ((arr)->size > (arr)->n) {                     \
      (vm)->shrinkvec(arr);                           \
    }                                                 \
  } while(0)

/* Array, Vector */
template<typename T>
struct VecT {
  VecT() { init(); }

  void init() {ptr = NULL; n = 0; size = 0;}

  void set(int i, T t) { ptr[i] = t; }
  void set(int i, T* t) { ptr[i] = *t; }
  T get(int i) { return ptr[i]; }
  T* getptr(int i) { return &ptr[i]; }
  const T operator [] (int i) { return get(i); }

  T* ptr;
  int n;
  int size;
};

#define vec_fill1(T) (ptr)[i] = T
#define vec_fill2(T) new (&(ptr)[i]) T

#define vec_init(T, vm, arr, count, fill) do{    \
    int size = (arr)->size = count;              \
    T* ptr = (arr)->ptr =                        \
      (T*)vm->alloc((arr)->size*sizeof(T));      \
    for(int i=0;i<size;i++)                      \
      fill;                                      \
  }while(0)

#define vec_ensure(T, vm, arr, fill) do{           \
    if ((arr)->n >= (arr)->size) {                 \
      (vm)->growvec(arr);                          \
      T* ptr = (arr)->ptr;                         \
      int old = (arr)->size;                       \
      for(int i=old;i<(arr)->size;i++)             \
        fill;                                      \
    }                                              \
  }while(0)

#define MINVEC_SIZE 4

#define vec_add1(T, vm, ks, v)                 \
  vec_ensure(T, (vm), &(ks), vec_fill1(0));    \
  (ks).set((ks).n++, (v));

#define vec_add2(T, vm, ks, v)                    \
  vec_ensure(T, (vm), &(ks), vec_fill2(T ()));    \
  (ks).set((ks).n++, (v));

class IntArray {
public:
  IntArray(VM* v):vm(v){}
  ~IntArray();
  int get(int i) { return vec.get(i); }
  void suffixset(int i, int v);
  void set(int i, int v) { vec.set(i, v); }
  VecT<int> vec;
  VM* vm;
};

class ArrayObj : public RefObject {
public:
  static ArrayObj empty;
public:
  ArrayObj() {}
  ArrayObj(VM* vm, int c);
  void set(int i, ValueT* ele) { array.set(i, *ele); }
  ValueT* get(int i) { return array.getptr(i); }
  void shrink(VM* vm);
  int add(VM* vm, ValueT* ele);
  virtual void visit(VM* vm);
  virtual void finz(VM* vm);

  GetSize(ArrayObj)

  VecT<ValueT> array;
};

class SCM {
public:
  static uint hash(const char* s, int l, int seed);
  static void assert(bool flag, const char* info, const char* fmt, ...);
  static bool strcaseeql(const char* str1, int len1, const char* str2);
public:
  static PairPtr list(VM* vm, ValueT* a);
  static int length(VM* vm, ValueT* a, const char* prompt);
  static int length(VM* vm, ValueT* a) { return length(vm, a, NULL); }
  static int length(ValueT* a);
  static int thlength(ValueT* a);
  static bool listp(ValueT* a);
  static ValueT* listlast(ValueT* a);
  static ValueT* listlastlen(ValueT* a, int* len);
  static bool eqp(ValueT* p1, ValueT* p2);
  static bool eqvp(ValueT* p1, ValueT* p2);
  static bool equalp(ValueT* p1, ValueT* p2);

  static ValueT* lastappend(VM* vm, ValueT* h, ValueT* t);
  static ValueT* append(VM* vm, ValueT* h, ValueT* t);
  static PairPtr cons(VM* vm, ValueT* h, ValueT* t);

  static void tokwform(VM* vm, ValueT* kw, ValueT* v, int line, bool);
  static void toAnnotation(VM* vm, ValueT* v, int line);

  static PairPtr list(VM* vm, ValueT* a, ValueT* b);
  static PairPtr list(VM* vm, ValueT* a, ValueT* b, ValueT* c);
  static PairPtr list(VM* vm, ValueT* a, ValueT* b, ValueT* c, ValueT* d);

  static scm_uint div128by64(scm_uint, scm_uint, scm_uint, scm_uint*);
  static NumBigObj* numbigdiv(VM*, NumBigObj*, NumBigObj*);
  static NumBigObj* numbigmod(VM*, NumBigObj*, NumBigObj*);
  static void numbig2str(VM* vm, NumBigObj* num, Lbuffer* buf);
  static void copystripanno(VM* vm, ValueT* out, ValueT* a);
  static ValueT* copylist(VM* vm, ValueT* out, ValueT* a);
  static void vector2cons(VM* vm, ValueT* out, ArrayObj* arr);
  static void list2vector(VM* vm, ValueT* lst, ValueT* out);
  static void reverselist(VM* vm, ValueT* lst);
};

struct NumRatioObj : public RefObject {
  NumRatioObj(scm_int n, scm_int d): numerator(n), denominator(d) {}
  GetSize(NumRatioObj)

  scm_int numerator;
  scm_int denominator;
};

struct NumComplexObj : public RefObject {
  NumComplexObj(double r, double i) { d[0] = r;d[1]=i; }
  GetSize(NumComplexObj)

  double d[2];//real+imag
};

struct NumBigDivRes {
    NumBigObj* quot;
    NumBigObj* rem;
};

#define Sbigzero (&NumBigObj::zero)

struct NumBigObj : public RefObject {
  NumBigObj(int l, char s): len(l),sign(s) {
    for (int i = 0; i < len; i++) data[i] = 0;
  }
  static int totalsize(int l) {
    return offsetof(NumBigObj, data) + sizeof(scm_uint) * l;
  }
  int bitlength();
  NumBigObj* normalize() {
    while (len > 1 && data[len - 1] == 0) len--;
    if (len == 1 && data[0] == 0) sign = 1;
    return this;
  }
  bool iszero() {
    for (int i = 0; i < len; i++) if (data[i] != 0) return false;
    return true;
  }
  int cmpabs(NumBigObj* other) {
    if (len != other->len) return len > other->len ? 1 : -1;
    for (int i = len - 1; i >= 0; i--) {
      if (data[i] != other->data[i]) return data[i] > other->data[i] ? 1 : -1;
    }
    return 0;
  }
  int cmp(NumBigObj* other) {
    bool az = iszero(), bz = other->iszero();
    if (az && bz) return 0;
    if (az) return other->sign > 0 ? -1 : 1;
    if (bz) return sign > 0 ? 1 : -1;
    if (sign != other->sign) return sign > other->sign ? 1 : -1;
    int m = cmpabs(other);
    return other->sign > 0 ? m : -m;
  }
  void tostr(VM* vm, Lbuffer* buf);
  static NumBigObj zero;
  NumBigObj* addabs(VM* vm, NumBigObj* other);
  NumBigObj* subabs(VM* vm, NumBigObj* other);
  NumBigObj* mulabs(VM* vm, NumBigObj* other);
  NumBigObj* add(VM* vm, NumBigObj* other);
  NumBigObj* sub(VM* vm, NumBigObj* other);
  NumBigObj* mul(VM* vm, NumBigObj* other);
  NumBigObj* copy(VM* vm);
  virtual int getsize() { return totalsize(len); }

  char sign;
  scm_int len;
  scm_uint data[1];
};

struct StrObj : public RefObject {
  StrObj(const char* s, int n, int h): len(n), hash(h) {
    memcpy(str, s, n);
    str[n] = '\0';
  }
  StrObj(char c, int n, int h): len(n), hash(h) {
    memset(str, c, n);
    str[n] = '\0';
  }
  StrObj(int n, int h): len(n), hash(h) {
    str[n] = '\0';
  }

  bool equalp(StrObj* p) {
    if (len != p->len) return false;
    if (len <= MAXSHORTLEN && this == p)
      return true;
    return 0 == memcmp(str, p->str, len);
  }

  static int totalsize(int n) {
    return (offsetof(StrObj, str) + sizeof(char) * ((n) + 1));
  }

  virtual int getsize() { return totalsize(len); }

  int len;
  int hash;

  char str[1];
};

struct PairObj : public RefObject {
  PairObj(ValueT* h, ValueT* t): scar(*h), scdr(*t) {}
  PairObj() {}

  ValueT* car() { return &scar; }
  ValueT* cdr() { return &scdr; }

  void car(ValueT* obj) { scar = obj; }
  void cdr(ValueT* obj) { scdr = obj; }

  virtual void visit(VM* vm);
  virtual void finz(VM* vm);

  GetSize(PairObj)

  ValueT scar;
  ValueT scdr;
};

#define CONCAT(A, B) A##B
#define VARNAME(NAME) CONCAT(NAME, _stk_)
#define VARNAME2(NAME) CONCAT(NAME, _gc_)
#define VARNAME3(NAME) CONCAT(NAME, __LINE__)

#define __s_gcvar(vm, NAME, y)                                    \
  ValueT VARNAME(NAME); ValueT* NAME = &VARNAME(NAME);            \
  StkVar VARNAME(y)(NAME, Stk(vm)->sv); StkVar* y = &VARNAME(y);  \
  Stk(vm)->sv = y;

#define Sgcvar1(vm, NAME)                                               \
  __s_gcvar(vm, NAME, __stack_var_gc_reserve1);                         \
  StkGCVar VARNAME2(__stack_var_gc_reserve1)(vm, __stack_var_gc_reserve1);

#define Sgcvar2(vm, a, b) Sgcvar1(vm,a);__s_gcvar(vm,b,__stack_var_gc_reserve2);
#define Sgcvar3(vm, a, b, c) Sgcvar2(vm,a,b);__s_gcvar(vm,c,__stack_var_gc_reserve3);
#define Sgcvar4(vm, a, b, c, d) Sgcvar3(vm,a,b,c);__s_gcvar(vm,d,__stack_var_gc_reserve4);
#define Sgcvar5(vm, a, b, c, d, e) Sgcvar4(vm,a,b,c,d);__s_gcvar(vm,e,__stack_var_gc_reserve5);
#define Sgcvar6(vm, a, b, c, d, e, f) Sgcvar5(vm,a,b,c,d,e);__s_gcvar(vm,f,__stack_var_gc_reserve6);

struct StkVar {
  ValueT* var;
  StkVar* next;
  StkVar():var(NULL),next(NULL) {}
  StkVar(ValueT* v, StkVar* n):
    var(v),next(n) {}
};

struct StkGCVar {
  StkVar* sv;
  VM* vm;

  StkGCVar(VM* v, StkVar* v1):
    vm(v), sv(v1) {}
  ~StkGCVar();
};

#define SEGMENT_SLOTS_SIZE 128
struct OuterVal;
class StackSegment : public RefObject {
public:
  StackSegment() { outers = NULL; frozen = 0; }
  ValueT slots[SEGMENT_SLOTS_SIZE];

  ValueT* first() { return &slots[0]; }
  ValueT* end() { return &slots[SEGMENT_SLOTS_SIZE]; }
  OuterVal* findouterval(VM*, ValueT* level);
  void closeouterval(VM* vm, CallFrame* frm, ValueT* level);

  GetSize(StackSegment)

  int frozen;
  OuterVal* outers;
};

struct CallFrame : public RefObject {
  CallFrame():
    pc(-1),prev(NULL),seg(NULL),start(NULL),base(NULL),top(NULL) {}

  void setpc(int p) { pc = p; }
  int getpc() { return pc; }

  GetSize(CallFrame)
  virtual void visit(VM* vm);
  int pc;
  StackSegment* seg;
  CallFrame* prev;
  ValueT* start;
  ValueT* base;
  ValueT* top; // not included
};

class Stack {
public:
  Stack(VM* v);

  void fullmark();
  void stepmark();

  void setvoid(ValueT* s, ValueT* e);

  CallFrame* rtnfrm(CallFrame*);
  CallFrame* newfrm(CallFrame*, ValueT* base, int argnum, int arity);
  bool isbasefrm(CallFrame* frm) { return frm == &basefrm; }

  VM* vm;
  StkVar* sv;
  CallFrame* curfrm;
  CallFrame basefrm;
  StackSegment baseseg;
};

class ReserveStack {
public:
  ReserveStack(VM* vm);
  ~ReserveStack();
protected:
  VM* vm;
  CallFrame* curfrm;
};

class RefObjGroup {
public:
  void swapobjset() { othIdx = curIdx; curIdx ^= 1; }

  void fullsweep();
  bool stepsweep();

  byte othbit() { return bitmask[othIdx]; }
  byte curbit() { return bitmask[curIdx]; }

  void addrefcur(RefPtr ref) { addref(ref, curIdx); }
  RefPtr recobj(RefPtr ref);
  void unsetfullgcmask() { setgcmask(bitmask); }
  void setfullgcmask() { setfullgc(bitmask); }

protected:
  void addref(RefPtr ref, int idx) {
    ref->link(gclst[idx]);
    gclst[idx] = ref;
  }

  friend class VM;
  RefObjGroup(VM* v) {
    vm = v;

    gclst[0] = NULL;
    gclst[1] = NULL;

    setgcmask(bitmask);

    curIdx = 0;
    othIdx = 1;
  }
protected:
  VM* vm;

  static const int SIZE = 2;

  RefPtr gclst[SIZE];
  byte bitmask[SIZE];

  byte curIdx;
  byte othIdx;
};

class CacheGroup {
public:
  void addref(PairPtr ref);

  PairPtr getonepair();
  void fullsweep();
protected:
  friend class VM;
  CacheGroup(VM* v): vm(v) { pairs = NULL; }
protected:
  VM* vm;
  PairPtr pairs;
};

enum GCState {
  GCSNone,
  GCSMarkStk,
  GCSMarkReg,
  GCSMarkFrame,
  GCSMarkGray,
  GCSSweepObjGroup,
  GCSSweepInterns,
  GCSSweepEnd,
  GCSEnd,
};

class SGC {
public:
  typedef void (SGC::*TouchPtr)(RefPtr ptr);
protected:
  TouchPtr ftouchptr;

  void addgraylst(RefPtr ptr);
  void touchchild(RefPtr ptr) { ptr->visit(vm); }
public:
  void stepfullgc();

  void singlestep();

  void checkBarrier(RefPtr ptr);
  void trace(ValueT v) { trace(&v); }
  void trace(ValueT* v) {
    if (isref(v))
      trace(refp(v));
  }
  void trace(RefPtr ptr);

  void startstep();

  int getstate() { return state; }
  void fullgc();

  void condgc() { if (threshold > 0 && debtbytes > threshold) fullgc(); }

  void toDel(RefPtr ref);

  void toDelOrMark(RefPtr ref);
public:
  void debt(long size) { debtbytes = size; }
  long debt() { return debtbytes; }

  void setthreshold() { threshold = 2 * debtbytes; }
  long threshold;
  long debtbytes;
protected:
  bool markgray();

  void sweepgray();
  void clearDel();

  friend class VM;
  SGC(VM* v):threshold(0) {
    vm = v;
    state = GCSNone;
    toDelLst = NULL;
    graylstLast = graylst = NULL;
    ftouchptr = NULL;
    debtbytes = 0;
  }
protected:
  VM* vm;

  int state;

  PairPtr graylst;
  PairPtr graylstLast;

  RefPtr toDelLst;
};

class Interns {
public:
  SymPtr intern(const char* str) { return intern(str, strlen(str)); }
  SymPtr intern(const char* str, int n);

  SymPtr internsym(const char* str, int n);
  SymPtr internsym(const char* str) { return internsym(str, strlen(str)); }

  void checkResize();

  void fullsweep();
  bool stepsweep();

  void startstep() { bidx = 0; }

protected:
  void rehash(int oldl, int newl);

  friend class VM;
  Interns(VM* v):vm(v),buf(v),bidx(0),bucketlist(0),blistlen(0),bcount(0) {}

  ~Interns();
  void init();

  Lbuffer buf;

protected:
  VM* vm;

  const static int INIT_LEN;

  int bidx;

  SymPtr* bucketlist;
  int blistlen;
  int bcount;
};

#define twoto(x)	(1<<(x))

struct TSlot {
  TSlot() { tv = NULL; next = NULL; }
  void settv(PairPtr p) { tv = p; }

  PairPtr tv;
  TSlot* next;
};

class GSymTable {
public:
  static const int MINLTSIZE = 4;
  void setslot(SymPtr sym, ValueT* val);
  PairPtr getslot(SymPtr sym);
  void getval(SymPtr sym, ValueT* v);
  void newkey(SymPtr sym, ValueT* val);
  void newkeyorupdate(SymPtr sym, ValueT* val);
protected:
  GSymTable(VM* v):vm(v) {}

  void init();
  TSlot* getpos(SymPtr sym);
  TSlot* getfreepos();
  void rehash();
  void insert(SymPtr sym, PairPtr tv);
protected:
  friend class VM;
  VM* vm;

  byte lsizenode;
  TSlot* slots;
  TSlot* lastfree;
};

#define GC(vm) (vm)->getgc()
#define Stk(vm) (vm)->getstk()
#define Cache(vm) (vm)->getcachegroup()
#define ObjGroup(vm) (vm)->getobjgroup()
#define Intern(vm) (vm)->getintern()
#define GEnv(vm) (vm)->getgenv()

#define Snew0(vm, TYPE) new (vm->alloc(sizeof(TYPE))) TYPE ()
#define Snew1(vm, TYPE, a) new (vm->alloc(sizeof(TYPE))) TYPE (a)
#define Snew2(vm, TYPE, a, b) new (vm->alloc(sizeof(TYPE))) TYPE (a, b)
#define Snew3(vm, TYPE, a, b, c) new (vm->alloc(sizeof(TYPE))) TYPE (a, b, c)
#define Snew4(vm, TYPE, a, b, c, d) new (vm->alloc(sizeof(TYPE))) TYPE (a, b, c, d)

#define Sr0(vm, TYPE) (TYPE*)ObjGroup(vm)->recobj(Snew0(vm, TYPE))
#define Sr1(vm, TYPE, a) (TYPE*)ObjGroup(vm)->recobj(Snew1(vm, TYPE, a))
#define Sr2(vm, TYPE, a, b) (TYPE*)ObjGroup(vm)->recobj(Snew2(vm, TYPE, a, b))
#define Sr3(vm, TYPE, a, b, c) (TYPE*)ObjGroup(vm)->recobj(Snew3(vm, TYPE, a, b, c))
#define Sr4(vm, TYPE, a, b, c, d) (TYPE*)ObjGroup(vm)->recobj(Snew4(vm, TYPE, a, b, c, d))

#define STUB_REG1(X) {#X, scm_stub_##X}

#define regcfunc(vm, FUNCS) do{                             \
 for (const RegCProc *l = FUNCS; l->name != NULL; l++)      \
   if (l->cf.cp0)                                           \
     vm->regNative(l->name, l->cf, l->argnum, l->argrest);  \
   else                                                     \
     vm->regComplex(l->name, l->complexid);                 \
 }while(0)

#define REGCPROC(CTYPE, n, cp)                                      \
  RegCProc(const char* na, CTYPE f, bool rest):                     \
    name(na),complexid(-1),argnum(n),argrest(rest) { cf. cp = f; }  \
  RegCProc(const char* na, CTYPE f):                                \
    name(na),complexid(-1),argnum(n),argrest(false) { cf. cp = f; }

struct RegCProc {
  const char* name;

  int complexid;

  CProc cf;
  int argnum;
  bool argrest;

  RegCProc(const char* na, int i):
    name(na),complexid(i),argnum(0),argrest(false) { cf.cp0 = NULL; }
  RegCProc(const char* na, CProc0 f):
    name(na),complexid(-1),argnum(0),argrest(false) { cf.cp0 = f; }

  REGCPROC(CProc1, 1, cp1)
  REGCPROC(CProc2, 2, cp2)
  REGCPROC(CProc3, 3, cp3)
  REGCPROC(CProc4, 4, cp4)
  REGCPROC(CProc5, 5, cp5)
  REGCPROC(CProc6, 6, cp6)
  REGCPROC(CProc7, 7, cp7)
  REGCPROC(CProc8, 8, cp8)
  REGCPROC(CProc9, 9, cp9)
  REGCPROC(CProc10, 10, cp10)
};

enum NATIVE_COMPLEX_PROC {
  NATIVE_COMPLEX_APPLY,
  NATIVE_COMPLEX_CALLCC,
  NATIVE_COMPLEX_CALL_WITH_IN_FILE,
  NATIVE_COMPLEX_CALL_WITH_OUT_FILE,
  NATIVE_COMPLEX_CALL_WITH_OUT_STR,
  NATIVE_COMPLEX_EVAL,
  NATIVE_COMPLEX_MAX,
};

struct OuterVal;

enum EKConst {
  K_NULL,
  K_UNDEFINED,
  K_UNQUOTE,
  K_UNQUOTES,
  K_QQUOTE,
  K_ConstMax,
};

class Lexer;
struct InputPortObj;
struct OutputPortObj;

class VM {
public:
  ValueT* kconst[K_ConstMax];

  ValueT quotevt;
  ValueT qquotevt;
  ValueT uquotevt;
  ValueT uquotesvt;

  ValueT definevt;
  ValueT setvt;
  ValueT beginvt;
  ValueT ifvt;
  ValueT lambdavt;
  ValueT syntaxrvt;
  ValueT syntaxerrvt;
  ValueT defsyntaxvt;
  ValueT ellipsisvt;

  ValueT ac0;
  InputPortObj* iport;
  OutputPortObj* oport;
public:
  VM(ScmAlloc a);
  VM():VM(salloc) {}

  void regComplex(const char* name, int id);
  void regNative(const char* name, CProc f, int n, bool rest);
public:
  void getuniquesym(SymPtr sym, ValueT* out);
  int uniquen;

public:
  void stepmarkframes();
  void regfullmark();
  void checkgc();

public:
  void printvalue(ValueT* val);
  void printvalue0(OutputPortObj*, ValueT* val, bool stripanno);
  void printvalue0(OutputPortObj* o, ValueT* val) { printvalue0(o, val, false); }
  void printvalue0(ValueT* val) { printvalue0(oport, val); }
  void printvalue0(ValueT* val, bool strip) { printvalue0(oport, val, strip); }
  void printframe();

  void execute(CallFrame* frm);
  void printccode0(FILE *f, LambdaPtr lambda, int pc);
  void printccode(FILE *f, LambdaPtr lambda);
  void printccode(LambdaObj* lambda) { printccode(stderr, lambda); }
public:
  void dorepl();
  void loadfile(const char* fname);
  bool dolex(Lexer* lex, StrPtr source);

public:
  LambdaPtr gettoplambda();
  const char* gettopsource();
protected:
  ScmAlloc frealloc;
public:
  StrObj* newstr(const char* str, int len, int h);
  StrObj* newstr(char c, int len, int h);
  StrObj* newstr(int len, int h);
  NumBigObj* newnumbig(int, char);
public:
  ClosurePtr newclosure(int n);
public:
  void* alloc(size_t size);
  void* realloc(void* ptr, size_t osize, size_t nsize);

  template<class T>
  void shrinkvec(VecT<T> *arr) {
    arr->ptr = (T*)shrinkvec(arr->ptr, &arr->size, arr->n, sizeof(T));
  }

  template<class T>
  void growvec(VecT<T> *arr) {
    arr->ptr = (T*)growvec(arr->ptr, &arr->size, arr->n + 1, sizeof(T), INT_MAX, "");
  }

  void* shrinkvec(void* ptr, int *psize, int fn, int esize);
  void* growvec(void *ptr, int *psize, int ne, int esize, int limit, const char* what);
  void free(void* ptr, size_t size) {
    frealloc(ptr, 0);
    long debt = getgc()->debt();
    getgc()->debt(debt - size);
    debt = getgc()->debt();

    DebugMem(Print("free mem(%u) -> %p \n", size, ptr));
  }

  PairPtr getonepairnor();

  StrObj* ninstrintern(const char* str, int len);
  StrObj* strintern(const char* str, int len);
  StrObj* strintern(const char* str);
  StrObj* strintern(int len);
  StrObj* strintern(int len, char c);

public:
  void makeseed();
  int seed;
protected:
  void init();
  void initprimitive();

public:
  Stack* getstk() { return &stk; }
  CacheGroup* getcachegroup() { return &cacheGroup; }
  SGC* getgc() { return &gc; }
  RefObjGroup* getobjgroup() { return &objGroup; }
  Interns* getintern() { return &intern; }
  GSymTable* getgenv() { return &genv; }

protected:
  Stack stk;
  RefObjGroup objGroup;
  CacheGroup cacheGroup;
  SGC gc;
  Interns intern;
  GSymTable genv;
};

struct OuterVar {
  SymPtr name;
  short idx;
  bool islocal;

  void visit(VM* vm) {
    Check(name);
  }
  OuterVar():name(NULL),idx(-1),islocal(false) {}
};

class LambdaVarsObj : public RefObject {
public:
  LambdaVarsObj() {}

  SymPtr reflocal(int n) { return local.get(n); }
  OuterVar* refovar(int n) { return ovar.getptr(n); }

  bool isvalidloc(int n) { return n >= 0 && n < local.n; }
  bool isvalidouter(int n) { return n >= 0 && n < ovar.n; }

  int addlocal(VM* vm, SymPtr sym);
  int looklocal(SymPtr sym);

  int addovar(VM* vm, SymPtr sym, int idx, bool islocal);
  int lookovar(SymPtr sym);

  void addsyntax(VM* vm, SyntaxObj*);
  int looksyntax(SymPtr sym, SyntaxPtr* sp);

  virtual void visit(VM* vm);
  void shrink(VM* vm);
  virtual void finz(VM* vm);
  GetSize(LambdaVarsObj)

  VecT<SymPtr> local;
  VecT<OuterVar> ovar;
  VecT<SyntaxObj*> syntax;
};

struct AbsLine {
  int pc;
  int line;
  AbsLine():pc(0), line(0) {}
};

#define ABSLINE	(-0x80)
#define MLINEDIFF	0x80

struct LambdaObj : public RefObject {
  LambdaObj():lines(NULL),sizelines(0),source(NULL),vars(NULL),argnum(0),
              argrest(false), top(0), defline(0) {}

  int getcodestart() { return code.n-1; }
  void saveline(VM* vm, int pc, int line, int preline);
  int pushcode(VM* vm, Instruction i);
  VecT<Instruction>* fetchcode() { return &code; }
  int fetchi(int pc) { return code.get(pc); }
  int addk(VM* vm, ValueT* v);
  ValueT* getk(int i) { return ks.getptr(i); }
  int addl(VM* vm, LambdaPtr l);
  LambdaPtr getl(int i) { return ksl.get(i); }
  virtual void visit(VM* vm);
  void shrink(VM* vm);
  virtual void finz(VM* vm);
  GetSize(LambdaObj)

  int top;
  StrPtr source;
  int defline;
  byte* lines;
  int sizelines;
  VecT<AbsLine> abslines;
  int argnum;
  bool argrest;
  LambdaVarsObj* vars;
  VecT<Instruction> code;
  VecT<ValueT> ks;
  VecT<LambdaPtr> ksl;
};

struct HygieneSymObj : public RefObject {
  HygieneSymObj(SymPtr s, SymPtr c): sym(s), hysym(c) {}

  Visit2(sym, hysym)

  GetSize(HygieneSymObj)

  SymPtr sym;
  SymPtr hysym;
};

struct NativeProcObj : public RefObject {
  NativeProcObj(SymPtr s, CProc f, int n, bool rest):
    var(s), cf(f), complexid(-1), argnum(n), argrest(rest) {}

  NativeProcObj(SymPtr s, int i):
    var(s), complexid(i), argnum(0), argrest(false) { cf.cp0 = NULL; }

  bool iscomplex() { return cf.cp0 == NULL; }

  Visit1(var)
  GetSize(NativeProcObj)

  SymPtr var;
  CProc cf;
  int argnum;
  bool argrest;

  int complexid;
};

struct InputPortObj : public RefObject {
  InputPortObj(): fname(NULL), file(NULL), size(0), n(0) {}

  void close() {
    if (file) {
      fclose(file);
      file = NULL;
      size = 0;
    }
  }
  virtual void finz(VM* vm);
  Visit1(fname)
  GetSize(InputPortObj)

  void read(VM* vm, ValueT* vt);
  int peekchar() { return n < size ? buff[n] : fillbuff(0); }
  int readchar() { return n < size ? buff[n++] : fillbuff(1); }
  int nextc() { return readchar(); }
  void unreadchar(int c) { n--; }
  int fillbuff(int initn) {
    if (!file) return -1;
    size = fread(buff, 1, sizeof(buff), file);
    if (size > 0)
      return (n=initn, buff[0]);
    else
      return -1;
  }

  char buff[32];
  int n;
  int size;

  StrPtr fname;
  FILE* file;
};

struct OutputPortObj : public RefObject {
  virtual void close() = 0;
  virtual int write(VM* vm, ValueT* vt) = 0;
  virtual int writestr(const char* str) { return writestr(str, strlen(str)); }
  virtual int writestr(const char* str, int len) = 0;
  virtual int writechar(char c) = 0;
  void finz(VM* vm) {
    close();
    RefObject::finz(vm);
  }
  virtual int flush() {}
};

struct OutputPortStrObj : public OutputPortObj {
  OutputPortStrObj(VM* vm): strbuf(vm), closed(false) {}

  virtual void close() {
    if (!closed) {
      closed = true;
      strbuf.close();
    }
  }
  virtual int write(VM* vm, ValueT* vt);
  virtual int writestr(const char* str, int len) { strbuf.put(str, len); }
  virtual int writechar(char c) { strbuf.put(c); }

  GetSize(OutputPortStrObj)

  bool closed;
  Lbuffer strbuf;
};

struct OutputPortFileObj : public OutputPortObj {
  OutputPortFileObj(): fname(NULL), file(NULL) {}

  virtual void close() {
    if (file && fname) {
      fclose(file);
      file = NULL;
    }
  }
  virtual int write(VM* vm, ValueT* vt);
  virtual int writestr(const char* str, int len) {
    if (!file) return -1;
    fwrite(str, 1, len, file);
    fflush(file);
    return 1;
  }
  virtual int writechar(char c) {
    if (!file) return -1;
    putc(c, file);
    fflush(file);
    return c;
  }
  virtual int flush() { if(file) fflush(file); }
  Visit1(fname)
  GetSize(OutputPortFileObj)

  StrPtr fname;
  FILE* file;
};

struct AnnotationObj : public RefObject {
  AnnotationObj() : line(-1) {}

  Visit1(vt)
  GetSize(AnnotationObj)

  ValueT vt;
  int line;
};

typedef void (*UnWindFunc)(VM*, CallFrame*, ValueT*);

struct OuterVal : public RefObject {
  OuterVal() {
    valp = NULL;
    next = NULL;
    unwind = NULL;
  }
  GetSize(OuterVal)
  void close(VM*, CallFrame*);
  ValueT* valp;
  ValueT val;
  OuterVal* next;
  UnWindFunc unwind;
};

struct ClosureObj : public RefObject {
  ClosureObj(int no): n(no), lambda(NULL) {
    for (int i = 0; i < n ; i++)
      outers[i] = NULL;
  }

  static int totalsize(int n) {
    return (offsetof(ClosureObj, outers) + sizeof(OuterVal*) * n);
  }
  virtual int getsize() { return totalsize(n); }
  virtual void visit(VM* vm);
  void initouters(VM*, StackSegment* seg, ValueT* base, OuterVal** encouter);

  LambdaPtr lambda;
  short n;
  OuterVal* outers[1];
};

struct ContinuationObj : public RefObject {
  ContinuationObj(CallFrame* s, ValueT* base);
  virtual void finz(VM* vm);
  Visit2(frm, base)
  GetSize(ContinuationObj)
  CallFrame* frm;
  ValueT* base;
};

class Reader {
public:
  virtual ~Reader() {}
  virtual int nextc() = 0;
  virtual void unread(int) = 0;
};

class ReaderPort : public Reader {
public:
  ReaderPort(InputPortObj* p):port(p) {}
  virtual int nextc() { return port->readchar(); }
  virtual void unread(int c) { port->unreadchar(c); }
  InputPortObj* port;
};

class ReaderF : public Reader {
public:
  ReaderF(const char* filename):n(0) {
    file = fopen(filename, "r");
    if (file == NULL) {
      Print("error read file %s", filename);
      throw "ReadError: failed to read file";
    }

    Debug(Print("read file from %s\n", filename));

    size = fread(buff, 1, sizeof(buff), file);
  }

  ~ReaderF() { if (file) { fclose(file); file = NULL; } }

  virtual int nextc() { return n < size ? buff[n++] : fillbuff(1); }
  virtual void unread(int c) { n--; }
  int fillbuff(int initn) {
    size = fread(buff, 1, sizeof(buff), file);
    if (size > 0)
      return (n=initn, buff[0]);
    else
      return -1;
  }

  FILE* file;
  char buff[32];
  int n;
  int size;
};

class ReaderS : public Reader {
public:
  ReaderS(const char* p): buff(p), n(0) {
    size = strlen(p);
  }

  virtual int nextc() { return n < size ? buff[n++] : -1; }
  virtual void unread(int c) { n--; }

  const char* buff;
  int n;
  int size;
};

class ReaderI : public Reader {
public:
  ReaderI(VM* v): lexer(NULL),buff(v),n(0) {}

  void prompt(char c) {
    if (c)
      putc(c, stdout);
    putc(' ', stdout);
    fflush(stdout);
  }

  virtual int nextc() { return n < buff.count ? buff.buf[n++] : fillbuff(1); }
  virtual void unread(int c) { n--; }

  int fillbuff(int);
  Lexer* lexer;
  Lbuffer buff;
  int n;
};

#define READ_END -1

enum TokenType {
	TOKEN_UNKNOWN,
	TOKEN_END,
	TOKEN_NUM,
  TOKEN_CHAR,
	TOKEN_STRING,
	TOKEN_DOT,
  TOKEN_VECTOR,
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_SQUARE_PAREN,
  TOKEN_RIGHT_SQUARE_PAREN,
	TOKEN_SYMBOL,
  TOKEN_BOOL,
	TOKEN_QUOTE,
	TOKEN_UNQUOTE,
	TOKEN_UNQUOTE_SPLICING,
	TOKEN_QUASIQUOTE,
  TOKEN_MAX,
};

class Lexer {
public:
  Lexer(VM* v, Reader* r):anno(true),
                          vm(v), reader(r), buff(v), aheadToken(TOKEN_UNKNOWN),linenum(1) {
    nextc();
  }

  bool anno;
  int aheadToken;
  void readOne(ValueT* v);
  void unread() { if (curchar != READ_END) reader->unread(curchar); }
protected:
  void nextc() { curchar = reader->nextc(); }
  void plusline();
  int dLex();
protected:
  int readNum(char init);
  int readNum();
  int readString();
  int readChar();
  int readBool();
  int readSymbol(char init);
  int readSymbol();

  void readValueT(ValueT* v);
  void readListT(ValueT* v);
  void readListT0(ValueT* v);
  void readVector(ValueT* v);

  void skipBlankComment();

protected:
  VM* vm;

  bool readT;
  char readC;

  Lbuffer buff;
  Reader* reader;

  int curchar;
  int startline;
  int linenum;
};

};
