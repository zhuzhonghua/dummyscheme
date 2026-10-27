# dummyscheme

[中文](README_cn.md) · [日本語](README_ja.md)

A portable, embeddable Scheme implementation based on a register-oriented bytecode vm

[Try Online](https://zhuzhonghua.github.io/dummyscheme/repl.html)

To test scheme code, please refer to r4rstest.scm, r5rs-tests.scm, aftertest.scm in the project.
aftertest.scm (449 checks) additionally covers pcall/xpcall/error, coroutine multi-values, and the language-layer additions below.

# Thoughts

I have many years of work experiences in game programming, especially backend.
Lua is so pupular in the game industry, and also in many other areas.
I like SICP very much, and have read the article [The Roots Of Lisp](https://paulgraham.com/rootsoflisp.html) and the book "Hackers and Painters".
Lisp is highly praised.

I want to have a scheme implementation that's like lua, a vm register and bytecode-based with line level debug info

It could be easily embed to many host programs, only need to copy some .h/.cpp files to compile together, and write stubs

## Features

- Bytecode compiler with register-based instruction set
- tail-call optimization
- First-class continuations (`call/cc`), multi-short, unlimited, use the method copy-on-write of the callframes, delimited continuation will be supported in the near future too
- Flatten box value(inspired by Lua), no chain, box the stack value if captured by inner lambda
- Stack segment technique for continuation capture and chain walking, inspired by Chez Scheme
- Two-halves incremental generational GC (partially implemented, not used currently)
- `syntax-rules` hygienic macros, custom ellipsis is also supported
- Number tower, bignum is supported too
- Line-level debug info with source location tracking
- Lua-like Scheme-level debug API (debug library):
```
debug-getinfo <level-or-proc>       → alist (what/source/short_src/currentline/
                                        linedefined/lastlinedefined/nups/nparams/
                                        isvararg/istailcall/name/namewhat/
                                        func/activelines)
debug-getlocal <level> <index>      → name + value (multi-value) or #f
debug-setlocal <level> <index> <v>  → name or #f
debug-getoutervalue <proc> <idx>    → name + value (1-based outer index) or #f
debug-setoutervalue <proc> <idx> <v>→ name or #f
debug-outervalueid <proc> <idx>     → opaque id; shared box = same id
debug-outervaluejoin <p1> <i1> <p2> <i2>  → #t; write-through after join
debug-sethook <spec> <count> <proc> → (set hook; spec = 'call/'return/'line/
                                        'count; count = trigger every N insns)
debug-gethook                       → (spec count proc) or (#f ())
debug-getregistry                   → shared hash-table (survives GC)
debug-traceback [<msg>]             → string, multi-line
```
Frame-stepping uses the same mask bits as Lua (`call`, `return`, `line`,
`count`) and a single hook closure — no C-level breakpoint table;
breakpoints are built on top of `debug-sethook` in Scheme itself.
- Lua-like embeddability and portability: platform-independent, easy to integrate into C/C++ host applications
- Full tests passed with r4rstest (pass SCHEME_STD_R4RS=1) and r5rs-tests
- dynamic-wind support, netsted with call/cc, jump to(out) dynamic-wind's thunk/body, allow to jump out dynamic-wind's before and after, but jumping into is not forbidden
- values+call-with-values

## Extra

- no transcript-on / transcript-off
- no eval but has vm->evalstr
- hash-table (inspired by lua)
```
(make-hash-table)
(hash-table-ref table key)
(hash-table-set! table key val)
(hash-table-for-each (lambda (key val) xxx) table)
```
- coroutine
```
`coroutine-create` / `coroutine-resume` / `coroutine-yield` / `coroutine-status` / `coroutine-running`
```
Each coroutine owns a private VM (independent frame chain + dynamic-wind chain + StkVar);
resume/yield/finish switch the VM's current execution context (one coroutine per VM,).
`yield`/`resume` support multiple values: resume may pass
multiple values into the coroutine (received by yield as multiple values), yield may
produce multiple values (returned by resume), consumed via `call-with-values`.

- error / pcall / xpcall (inspired by Lua)
```
`(error msg irritant...)`     -- raise a VM error; message is text: strings as-is,
                                 other objects printed with write semantics, joined by spaces
`(pcall thunk)`               -- protected call of a 0-arg closure; returns (ok . result)
                                 or (false . err-string); errors never escape
`(xpcall thunk handler)`      -- protected call: on success returns thunk's value; on failure
                                 unwinds (running dynamic-wind after thunks), passes the error
                                 string to handler and returns its value; errors inside handler
                                 propagate
```

- Language-layer additions: R7RS (bytevector/bitwise/random/format) + Lua (string-format/string-pack)
```
bytevector (R7RS-small builtin, binary data):
`bytevector?` / `make-bytevector` / `bytevector` / `bytevector-length` /
`bytevector-u8-ref` / `bytevector-u8-set!` / `bytevector=?` / `bytevector-copy` /
`bytevector-copy!` / `bytevector-append` / `utf8->string` / `string->utf8`

bitwise (SRFI-151 / R7RS-large (scheme bitwise), 64-bit integers):
`bitwise-not` / `bitwise-and` / `bitwise-ior` / `bitwise-xor` (and/ior/xor variadic)
`bitwise-arithmetic-shift` and alias `ash` (arithmetic; right shift keeps sign)
`bitwise-arithmetic-shift-left` / `bitwise-arithmetic-shift-right`
`bit-count` / `bit-length` / `bit-set?` / `bit-field` / `first-set-bit`
SRFI-60 aliases: lognot / logand / logior / logxor / logcount / integer-length

random (SRFI-27 names):
`(random-integer n)` -- integer in [0, n); `(random-real)` -- float in [0, 1);
`(random-seed n)` -- sets the seed; same seed reproduces the same sequence

format (R7RS/SRFI-28 syntax + SRFI-159 numeric params):
`~a` display  `~s` write  `~d` decimal  `~x` hex  `~o` octal  `~b` binary
`~c` char  `~f` fixed  `~e` exp  `~g` general  `~%` newline  `~~` tilde
numeric directives accept params: `~08x` (zero-padded width), `~,2f` (precision)

string-format (Lua string.format, printf-style):
`%s %d %i %u %o %x %X %c %f %e %E %g %G %%` with flags `- + # 0`,
width/precision (`%08x %03d %.2f %8.2f %-35s`), no `*` (Lua rejects it too)

string-pack / string-unpack / string-packsize (Lua syntax, strobj-backed, binary safe):
endian `<` little / `>` big / `=` native; ints `b B h H l L j J T i[n] I[n]`;
floats `f` 4-byte float / `d` `n` 8-byte double (Lua 'f'/'d'/'n'); `s[n]` length-prefixed string (prefix always big-endian);
`z` NUL-terminated string; `c[n]` fixed-length string; `x` pad byte; `Xop` align
`(string-pack fmt . args)`          -- binary string
`(string-unpack fmt s [pos])`       -- multiple values (vals... nextpos); pos may be negative
`(string-packsize fmt)`             -- fixed size (s/z/c error)
`(string-char . bytes)`             -- Lua string.char
`(string-byte s [i [j]])`           -- Lua string.byte (multiple byte values)
`(type x)`                          -- Lua type; returns type symbol (number/string/boolean/
                                        char/symbol/pair/null/vector/hash-table/bytevector/
                                        closure/values/void)
indices/positions are 0-based (negative counts from the tail)
```

## How To Use

0. copy *.cpp/h to your project except main.cpp
1. include header file
```
#include "vm.h"
using namespace Scheme;
```
2. create / destroy a VM

```
VM* vm = scm_create_vm();   // default allocator (malloc/realloc), ud = NULL
// ... work ...
scm_destroy_vm(vm);         // frees all runtime objects and the VM itself
```
custom allocator — realloc semantics plus an opaque ud context (e.g. for
per-VM memory accounting or quota enforcement)
```
static void * myalloc(void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud;
  if (nsize == 0) {
    myfree(ptr);
    return NULL;
  }
  return myrealloc(ptr, nsize);  // osize = old size, useful for accounting
}
VM* vm = scm_create_vm(myalloc, NULL);
```
3. load file / eval code
```
vm->loadfile("filename.scm");
ValueT val = vm->evalstr("(+ 1 2)");
int iv = scm_get_int(vm, &val);   // iv = 3
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
6. per-VM global registry (survives GC, so C bindings can keep values alive)
```
scm_registry_set(vm, "config", scm_make_string(vm, "v1"));
ValueT got = scm_registry_get(vm, "config");   // missing key -> null
scm_registry_unset(vm, "config");
```
7. value -> string with write semantics (pointer valid only until the next
   scm_tostring call)
```
int len;
const char* s = scm_tostring(vm, &val, &len);  // e.g. "42" / "\"hi\""
```
8. type name (static string, never GC'd)
```
scm_typename(vm, &val);   // "number" / "string" / "pair" / "hash-table" / ...
```
9. call scheme lambdas from c/c++
```
ValueT* argv[] = {xxx};
ValueT val = vm->call("f", argv, n);       // unguarded call
bool ok = vm->pcall(&fn, argv, n, &out);   // protected call: false + error on failure
```

## Compliance

- R5RS with no extra setting(default)
- R4RS with the macro SCHEME_STD_R4RS(mainly about the SYMBOL upcase conversion)

## Future

Optimization and features are endless

## License

MIT
