# dummyscheme

A portable, embeddable Scheme implementation based on a register-oriented bytecode vm

# Thoughts

I have many years of work experiences in game programming, especially backend.
Lua is so pupular in the game industry, and also in many other areas.
I like SICP very much, and have read the article [The Roots Of Lisp](https://paulgraham.com/rootsoflisp.html) and the book <Hackers and Painters>.
Lisp is highly praised.

I want to have a scheme implementation that's like lua, a vm register and bytecode-based with line level debug info

It could be easily embed to many host programs, only need to copy some .h/.cpp files to compile together, and write stubs

## Features

- Bytecode compiler with register-based instruction set
- tail-call optimization
- First-class continuations (`call/cc`)
multi-short, unlimited, use the method copy-on-write of the callframes.
delimited continuation will be supported in the near future too
- Flatten box value(inspired by Lua)
no chain, box the stack value if captured by inner lambda
- Stack segment technique for continuation capture and chain walking, inspired by Chez Scheme
- Two-halves incremental generational GC (partially implemented, not used currently)
- `syntax-rules` hygienic macros
custom ellipsis is also supported
- Number tower
bignum is supported too
- Line-level debug info with source location tracking
- Lua-like embeddability and portability: platform-independent, easy to integrate into C/C++ host applications
(windows, linux, mac, ios, android, any platform where there is a c99/c++98 compiliance compiler)
- Full tests passed with r4rstest (pass SCHEME_STD_R4RS=1) and r5rs-tests
- dynamic-wind support
netsted with call/cc, jump to(out) dynamic-wind's thunk/body
allow to jump out dynamic-wind's before and after, but jumping into is not forbidden
- values+call-with-values

## Extra

- no transcript-on / transcript-off
- no eval but has vm->evalstr
- hash-table (inspired by lua)
(make-hash-table)
(hash-table-ref table key)
(hash-table-set! table key val)
(hash-table-for-each (lambda (key val) xxx) table)

## How To Use

-1. to test scheme code, please refer to r4rstest.scm, r5rs-tests.scm, aftertest.scm in the project

0. copy *.cpp/h to your project except main.cpp
1. include header file
```
#include "vm.h"
using namespace Scheme;
```
2. init vm

use default malloc/free
```
VM vm;
```
or use custom malloc/free
```
static void * myalloc(void *ptr, size_t nsize) {
  if (nsize == 0) {
    myfree(ptr);
    return NULL;
  }
  else
    return myrealloc(ptr, nsize);
}
VM vm(myalloc);
```
3. load file
```
vm.loadfile("filename.scm");
```
4. write stubs
```
// (lambda (a b) xxx)
static ValueT scm_stub_my_add(VM* vm, ValueT* a, ValueT* b)
{
  return scm_make_int(vm, scm_get_int(vm, a) + scm_get_int(vm, b));
}
// (lambda rest xxx)
static ValueT scm_stub_my_sum(VM* vm, ValueT* rest)
{
  scm_int t = 0;
  ValueT* p = rest;
  while (!scm_is_null(vm, p))
  {
    t += scm_get_int(vm, scm_car(vm, p));
    p = scm_cdr(vm, p);
  }
  return scm_make_int(vm, t);
}
const RegCProc myext[] = {
  RegCProc("my-add", scm_stub_my_add), // (lambda (a) xxx)
  RegCProc("my-sum", scm_stub_my_sum, true), // (lambda (a . rest) xxx)
  RegCProc(NULL, -1)
};
regcfunc(vm, myext);

```
5. call stubs from scheme
```
(my-add 2 3)
(my-sum 1 2 3 4)
```
6. eval scheme from c/c++
```
ValueT val = vm->evalstr("(+ 1 2)")
int iv = scm_get_int(vm, &val);
// iv = 3
```
7. call lambda from c/c++
```
ValueT* argv[] = {xxx};
ValueT val = vm->call("f", argv, n)
```

## Compliance

- R5RS with no extra setting(default)
- R4RS with the macro SCHEME_STD_R4RS(mainly about the SYMBOL upcase conversion)

## Future

Optimization and features are endless

## License

MIT
