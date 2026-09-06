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

void SCMVector::init(VM* vm)
{
  const RegCProc vec[] = {
    {"vector", scm_stub_vector, true},
    {"make-vector", scm_stub_make_vector, true},
    {"vector-set!", scm_stub_vector_set},
    {"vector-fill!", scm_stub_vector_fill},
    {"vector-ref", scm_stub_vector_ref},
    {"vector-length", scm_stub_vector_length},
    {NULL, -1},
  };
  regcfunc(vm, vec);
}

};
