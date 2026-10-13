#include "vm.h"
#include "scmvector.h"

namespace Scheme {

static ValueT scm_stub_make_vector(VM* vm, ValueT* p, ValueT* args)
{
  AssertArg(vm, isnumi(p), "make-vector", p, "not a number");

  int n = numi(p);

  ValueT out;
  if (isnull(args))
  {
    ArrayObj* arr = NULL;
    setarray(&out, arr = Sr2(vm, ArrayObj, vm, n));

    for (int i = 0; i < n; i++)
      arr->set(i, Sundefined);
  }
  else
  {
    AssertArg(vm, isnull(Scdr(args)), "make-vector", args, "too many arguments");
    ValueT* fill = Scar(args);

    ArrayObj* arr = NULL;
    setarray(&out, arr = Sr2(vm, ArrayObj, vm, n));

    for (int i = 0; i < n; i++)
      arr->set(i, fill);
  }

  return out;
}

static ValueT scm_stub_vector(VM* vm, ValueT* args)
{
  ValueT out;
  if (isnull(args))
    setarray(&out, Sr0(vm, ArrayObj));

  else
  {
    int count = 0;
    PAIR_FOR(p, args)
      count++;

    ArrayObj* arr = NULL;
    setarray(&out, arr = Sr2(vm, ArrayObj, vm, count));

    int i = 0;
    PAIR_FOR(p, args)
      arr->set(i++, Scar(p));
  }

  return out;
}

static ValueT scm_stub_vector_ref(VM* vm, ValueT* vec, ValueT* idx)
{
  AssertArg(vm, isarray(vec), "vector-ref", vec, "not a vector");
  AssertArg(vm, isnumi(idx), "vector-ref", idx, "not a number");

  ArrayObj* arr = arrayref(vec);
  int i = numi(idx);
  Assert(vm, i < arr->array.n,
         "vector-ref: index %d is beyond bounds of array: %d",
         i,
         arr->array.n);

  return arr->get(i);
}

static ValueT scm_stub_vector_length(VM* vm, ValueT* vec)
{
  AssertArg(vm, isarray(vec), "vector-length", vec, "not a vector");

  ArrayObj* arr = arrayref(vec);
  ValueT out;
  setnumi(&out, arr->array.n);

  return out;
}

static ValueT scm_stub_vector_set(VM* vm, ValueT* vec, ValueT* idx, ValueT* obj)
{
  AssertArg(vm, isarray(vec), "vector-set!", vec, " not a vector");
  AssertArg(vm, isnumi(idx), "vector-set!", idx, " not a number");
  ArrayObj* arr = arrayref(vec);
  AssertArg(vm, !arr->isimmutable(), "vector-set!", vec, " immutable");
  int i = numi(idx);
  Assert(vm, i < arr->array.n,
         "vector-set!: index %d is beyond bounds of array: %d",
         i,
         arr->array.n);
  arr->set(i, obj);
  return Svoidref;
}


static ValueT scm_stub_vector_fill(VM* vm, ValueT* vec, ValueT* obj)
{
  AssertArg(vm, isarray(vec), "vector-set!", vec, "not a vector");

  ArrayObj* arr = arrayref(vec);
  for (int i = 0; i < arr->array.n; i++)
    arr->set(i, obj);

  return Svoidref;
}

/* ---- bytevector ---- */

static int byteval(VM* vm, ValueT* v, const char* op)
{
  AssertArg(vm, isnumi(v), op, v, " not an integer");
  int n = numi(v);
  AssertArg(vm, n >= 0 && n <= 255, op, v, " byte value out of range 0..255");
  return n;
}

static ValueT scm_stub_bytevectorp(VM* vm, ValueT* p)
{
  return frombool(isbytevector(p));
}

static ValueT scm_stub_make_bytevector(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "make-bytevector";
  AssertArg(vm, isnumi(p), METHOD, p, " not an integer");
  int n = numi(p);
  AssertArg(vm, n >= 0, METHOD, p, " negative length");
  int fill = 0;
  if (!isnull(args))
  {
    AssertArg(vm, isnull(Scdr(args)), METHOD, args, " too many arguments");
    fill = byteval(vm, Scar(args), METHOD);
  }
  StrObj* bv = G(vm)->newbytevector(n);
  for (int i = 0; i < n; i++)
    bv->str[i] = (byte)fill;
  ValueT out;
  setbytevector(&out, bv);
  return out;
}

static ValueT scm_stub_bytevector(VM* vm, ValueT* args)
{
  static const char* METHOD = "bytevector";
  int count = 0;
  PAIR_FOR(p, args)
  {
    byteval(vm, Scar(p), METHOD);
    count++;
  }
  StrObj* bv = G(vm)->newbytevector(count);
  int i = 0;
  PAIR_FOR(p, args)
    bv->str[i++] = (byte)numi(Scar(p));
  ValueT out;
  setbytevector(&out, bv);
  return out;
}

static ValueT scm_stub_bytevector_length(VM* vm, ValueT* p)
{
  AssertArg(vm, isbytevector(p), "bytevector-length", p, " not a bytevector");
  ValueT out;
  setnumi(&out, bytevectorref(p)->len);
  return out;
}

static ValueT scm_stub_bytevector_u8_ref(VM* vm, ValueT* p, ValueT* k)
{
  static const char* METHOD = "bytevector-u8-ref";
  AssertArg(vm, isbytevector(p), METHOD, p, " not a bytevector");
  AssertArg(vm, isnumi(k), METHOD, k, " not an integer");
  StrObj* bv = bytevectorref(p);
  int i = numi(k);
  AssertArg(vm, i >= 0 && i < bv->len, METHOD, k, " index out of range");
  ValueT out;
  setnumi(&out, bv->str[i]);
  return out;
}

static ValueT scm_stub_bytevector_u8_set(VM* vm, ValueT* p, ValueT* k, ValueT* v)
{
  static const char* METHOD = "bytevector-u8-set!";
  AssertArg(vm, isbytevector(p), METHOD, p, " not a bytevector");
  AssertArg(vm, isnumi(k), METHOD, k, " not an integer");
  StrObj* bv = bytevectorref(p);
  int i = numi(k);
  AssertArg(vm, i >= 0 && i < bv->len, METHOD, k, " index out of range");
  bv->str[i] = (byte)byteval(vm, v, METHOD);
  return Svoidref;
}

static ValueT scm_stub_bytevector_eq(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "bytevector=?";
  AssertArg(vm, isbytevector(p), METHOD, p, " not a bytevector");
  StrObj* prev = bytevectorref(p);
  PAIR_FOR(a, args)
  {
    ValueT* v = Scar(a);
    AssertArg(vm, isbytevector(v), METHOD, v, " not a bytevector");
    if (!bytevectorref(v)->equalp(prev))
      return *Sfalseref;
    prev = bytevectorref(v);
  }
  return *Strueref;
}

static int bytevector_indexarg(VM* vm, ValueT* v, int len, int deflt, const char* op)
{
  if (isnull(v)) return deflt;
  ValueT* v0 = Scar(v);
  AssertArg(vm, isnumi(v0), op, v0, " not an integer");
  int i = numi(v0);
  AssertArg(vm, i >= 0 && i <= len, op, v0, " index out of range");
  return i;
}

static ValueT scm_stub_bytevector_copy(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "bytevector-copy";
  AssertArg(vm, isbytevector(p), METHOD, p, " not a bytevector");
  StrObj* bv = bytevectorref(p);
  int start = bytevector_indexarg(vm, args, bv->len, 0, METHOD);
  ValueT* endv = isnull(args) ? args : Scdr(args);
  int end = bytevector_indexarg(vm, endv, bv->len, bv->len, METHOD);
  AssertArg(vm, start <= end, METHOD, p, " start > end");
  StrObj* nbv = G(vm)->newbytevector(end - start);
  memcpy(nbv->str, bv->str + start, end - start);
  ValueT out;
  setbytevector(&out, nbv);
  return out;
}

static ValueT scm_stub_bytevector_copy_to(VM* vm, ValueT* to, ValueT* at, ValueT* from, ValueT* args)
{
  static const char* METHOD = "bytevector-copy!";
  AssertArg(vm, isbytevector(to), METHOD, to, " not a bytevector");
  AssertArg(vm, isnumi(at), METHOD, at, " not an integer");
  AssertArg(vm, isbytevector(from), METHOD, from, " not a bytevector");
  StrObj* dst = bytevectorref(to);
  StrObj* src = bytevectorref(from);
  int start = bytevector_indexarg(vm, args, src->len, 0, METHOD);
  ValueT* endv = isnull(args) ? args : Scdr(args);
  int end = bytevector_indexarg(vm, endv, src->len, src->len, METHOD);
  AssertArg(vm, start <= end, METHOD, from, " start > end");
  int atv = numi(at);
  AssertArg(vm, atv >= 0 && atv + (end - start) <= dst->len, METHOD, at, " target range out of bounds");
  memmove(dst->str + atv, src->str + start, end - start);
  return Svoidref;
}

static ValueT scm_stub_bytevector_append(VM* vm, ValueT* args)
{
  static const char* METHOD = "bytevector-append";
  int total = 0;
  PAIR_FOR(p, args)
  {
    ValueT* v = Scar(p);
    AssertArg(vm, isbytevector(v), METHOD, v, " not a bytevector");
    total += bytevectorref(v)->len;
  }
  StrObj* bv = G(vm)->newbytevector(total);
  int off = 0;
  PAIR_FOR(p, args)
  {
    StrObj* src = bytevectorref(Scar(p));
    memcpy(bv->str + off, src->str, src->len);
    off += src->len;
  }
  ValueT out;
  setbytevector(&out, bv);
  return out;
}

static ValueT scm_stub_utf8_to_string(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "utf8->string";
  AssertArg(vm, isbytevector(p), METHOD, p, " not a bytevector");
  StrObj* bv = bytevectorref(p);
  int start = bytevector_indexarg(vm, args, bv->len, 0, METHOD);
  ValueT* endv = isnull(args) ? args : Scdr(args);
  int end = bytevector_indexarg(vm, endv, bv->len, bv->len, METHOD);
  AssertArg(vm, start <= end, METHOD, p, " start > end");
  ValueT out;
  setstr(&out, G(vm)->strintern((const char*)bv->str + start, end - start));
  return out;
}

static ValueT scm_stub_string_to_utf8(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "string->utf8";
  AssertArg(vm, isstr(p), METHOD, p, " not a string");
  StrObj* s = strref(p);
  int start = bytevector_indexarg(vm, args, s->len, 0, METHOD);
  ValueT* endv = isnull(args) ? args : Scdr(args);
  int end = bytevector_indexarg(vm, endv, s->len, s->len, METHOD);
  AssertArg(vm, start <= end, METHOD, p, " start > end");
  StrObj* bv = G(vm)->newbytevector(end - start);
  memcpy(bv->str, s->str + start, end - start);
  ValueT out;
  setbytevector(&out, bv);
  return out;
}

void SCMVector::init(VM* vm)
{
  const RegCProc vec[] = {
    RegCProc("vector", scm_stub_vector, true),
    RegCProc("make-vector", scm_stub_make_vector, true),
    RegCProc("vector-set!", scm_stub_vector_set),
    RegCProc("vector-fill!", scm_stub_vector_fill),
    RegCProc("vector-ref", scm_stub_vector_ref),
    RegCProc("vector-length", scm_stub_vector_length),
    RegCProc("bytevector?", scm_stub_bytevectorp),
    RegCProc("make-bytevector", scm_stub_make_bytevector, true),
    RegCProc("bytevector", scm_stub_bytevector, true),
    RegCProc("bytevector-length", scm_stub_bytevector_length),
    RegCProc("bytevector-u8-ref", scm_stub_bytevector_u8_ref),
    RegCProc("bytevector-u8-set!", scm_stub_bytevector_u8_set),
    RegCProc("bytevector=?", scm_stub_bytevector_eq, true),
    RegCProc("bytevector-copy", scm_stub_bytevector_copy, true),
    RegCProc("bytevector-copy!", scm_stub_bytevector_copy_to, true),
    RegCProc("bytevector-append", scm_stub_bytevector_append, true),
    RegCProc("utf8->string", scm_stub_utf8_to_string, true),
    RegCProc("string->utf8", scm_stub_string_to_utf8, true),
    RegCProc(NULL, -1),
  };
  regcfunc(vm, vec);
}

};
