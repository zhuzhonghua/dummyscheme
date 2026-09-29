// =====================================================================
// scmapi.cpp — implementation of scmapi.h
//
// =====================================================================

#include "vm.h"
#include "scmapi.h"

namespace Scheme {

// ---------- internals ----------

// Type mismatch error: expected <type>, got <value>; always throws.
static void typeerr(VM* vm, ValueT* p, const char* expect)
{
  Print("expected %s", expect);
  if (p)
  {
    Print(", got ");
    vm->printvalue0(p);
  }
  Print("\n");
  throw "ScmError";
}

// ---------- type predicates ----------

bool scm_is_int(VM* vm, ValueT* p)
{
  (void)vm;
  return isnumi(p);
}

bool scm_is_float(VM* vm, ValueT* p)
{
  (void)vm;
  return isnumreal(p);
}

bool scm_is_string(VM* vm, ValueT* p)
{
  (void)vm;
  return isstr(p);
}

bool scm_is_bool(VM* vm, ValueT* p)
{
  (void)vm;
  return istrue(p) || isfalse(p);
}

bool scm_is_true(VM* vm, ValueT* p)
{
  (void)vm;
  return istrue(p);
}

bool scm_is_false(VM* vm, ValueT* p)
{
  (void)vm;
  return isfalse(p);
}

bool scm_is_char(VM* vm, ValueT* p)
{
  (void)vm;
  return ischar(p);
}

bool scm_is_pair(VM* vm, ValueT* p)
{
  (void)vm;
  return ispair(p);
}

bool scm_is_list(VM* vm, ValueT* p)
{
  (void)vm;
  return isnull(p) || ispair(p);
}

bool scm_is_null(VM* vm, ValueT* p)
{
  (void)vm;
  return isnull(p);
}

bool scm_is_vector(VM* vm, ValueT* p)
{
  (void)vm;
  return isarray(p);
}

bool scm_is_table(VM* vm, ValueT* p)
{
  (void)vm;
  return ishashtable(p);
}

// ---------- getters (strict) ----------

scm_int scm_get_int(VM* vm, ValueT* p)
{
  if (!p || !isnumi(p))
  {
    typeerr(vm, p, "integer");
    return 0;
  }
  return numi(p);
}

scm_float scm_get_float(VM* vm, ValueT* p)
{
  if (!p || !isnumreal(p))
  {
    typeerr(vm, p, "float");
    return 0.0;
  }
  return numreal(p);
}

const char* scm_get_string(VM* vm, ValueT* p, int* len)
{
  if (!p || !isstr(p))
  {
    typeerr(vm, p, "string");
    return NULL;
  }
  StrObj* s = strref(p);
  if (len)
    *len = (int)s->len;
  return s->str;
}

bool scm_get_bool(VM* vm, ValueT* p)
{
  if (!p || !isboolean(p))
  {
    typeerr(vm, p, "boolean");
    return false;
  }
  return istrue(p);
}

scm_char scm_get_char(VM* vm, ValueT* p)
{
  if (!p || !ischar(p))
  {
    typeerr(vm, p, "char");
    return 0;
  }
  return (scm_char)vtchar(p);
}

// ---------- list access ----------

ValueT* scm_car(VM* vm, ValueT* p)
{
  if (!p || !ispair(p))
  {
    typeerr(vm, p, "pair");
    return NULL;
  }
  return Scar(p);
}

ValueT* scm_cdr(VM* vm, ValueT* p)
{
  if (!p || !ispair(p))
  {
    typeerr(vm, p, "pair");
    return NULL;
  }
  return Scdr(p);
}

ValueT* scm_caar(VM* vm, ValueT* p) { return scm_car(vm, scm_car(vm, p)); }
ValueT* scm_cadr(VM* vm, ValueT* p) { return scm_car(vm, scm_cdr(vm, p)); }
ValueT* scm_cdar(VM* vm, ValueT* p) { return scm_cdr(vm, scm_car(vm, p)); }
ValueT* scm_cddr(VM* vm, ValueT* p) { return scm_cdr(vm, scm_cdr(vm, p)); }

ValueT* scm_caaar(VM* vm, ValueT* p) { return scm_car(vm, scm_caar(vm, p)); }
ValueT* scm_caadr(VM* vm, ValueT* p) { return scm_car(vm, scm_cadr(vm, p)); }
ValueT* scm_cadar(VM* vm, ValueT* p) { return scm_car(vm, scm_cdar(vm, p)); }
ValueT* scm_caddr(VM* vm, ValueT* p) { return scm_car(vm, scm_cddr(vm, p)); }
ValueT* scm_cdaar(VM* vm, ValueT* p) { return scm_cdr(vm, scm_caar(vm, p)); }
ValueT* scm_cdadr(VM* vm, ValueT* p) { return scm_cdr(vm, scm_cadr(vm, p)); }
ValueT* scm_cddar(VM* vm, ValueT* p) { return scm_cdr(vm, scm_cdar(vm, p)); }
ValueT* scm_cdddr(VM* vm, ValueT* p) { return scm_cdr(vm, scm_cddr(vm, p)); }

ValueT* scm_caaaar(VM* vm, ValueT* p) { return scm_car(vm, scm_caaar(vm, p)); }
ValueT* scm_caaadr(VM* vm, ValueT* p) { return scm_car(vm, scm_caadr(vm, p)); }
ValueT* scm_caadar(VM* vm, ValueT* p) { return scm_car(vm, scm_cadar(vm, p)); }
ValueT* scm_caaddr(VM* vm, ValueT* p) { return scm_car(vm, scm_caddr(vm, p)); }
ValueT* scm_cadaar(VM* vm, ValueT* p) { return scm_car(vm, scm_cdaar(vm, p)); }
ValueT* scm_cadadr(VM* vm, ValueT* p) { return scm_car(vm, scm_cdadr(vm, p)); }
ValueT* scm_caddar(VM* vm, ValueT* p) { return scm_car(vm, scm_cddar(vm, p)); }
ValueT* scm_cadddr(VM* vm, ValueT* p) { return scm_car(vm, scm_cdddr(vm, p)); }
ValueT* scm_cdaaar(VM* vm, ValueT* p) { return scm_cdr(vm, scm_caaar(vm, p)); }
ValueT* scm_cdaadr(VM* vm, ValueT* p) { return scm_cdr(vm, scm_caadr(vm, p)); }
ValueT* scm_cdadar(VM* vm, ValueT* p) { return scm_cdr(vm, scm_cadar(vm, p)); }
ValueT* scm_cdaddr(VM* vm, ValueT* p) { return scm_cdr(vm, scm_caddr(vm, p)); }
ValueT* scm_cddaar(VM* vm, ValueT* p) { return scm_cdr(vm, scm_cdaar(vm, p)); }
ValueT* scm_cddadr(VM* vm, ValueT* p) { return scm_cdr(vm, scm_cdadr(vm, p)); }
ValueT* scm_cdddar(VM* vm, ValueT* p) { return scm_cdr(vm, scm_cddar(vm, p)); }
ValueT* scm_cddddr(VM* vm, ValueT* p) { return scm_cdr(vm, scm_cdddr(vm, p)); }

int scm_list_len(VM* vm, ValueT* list)
{
  (void)vm;
  int n = 0;
  ValueT* p = list;
  while (!isnull(p))
  {
    if (!ispair(p))
    {
      Print("expected a proper list, got ");
      if (p) vm->printvalue0(p);
      Print("\n");
      throw "ScmError";
    }
    n++;
    p = Scdr(p);
  }
  return n;
}

ValueT* scm_list_ref(VM* vm, ValueT* list, int i)
{
  (void)vm;
  ValueT* p = list;
  while (i-- > 0)
  {
    if (!ispair(p))
    {
      Print("list index out of range\n");
      throw "ScmError";
    }
    p = Scdr(p);
  }
  if (isnull(p))
  {
    Print("list index out of range\n");
    throw "ScmError";
  }
  return Scar(p);
}

// ---------- constructors ----------

ValueT scm_make_int(VM* vm, scm_int x)
{
  ValueT v;
  setnumi(&v, x);
  return v;
}

ValueT scm_make_float(VM* vm, scm_float x)
{
  ValueT v;
  setnumreal(&v, x);
  return v;
}

ValueT scm_make_bool(VM* vm, bool b)
{
  return b ? *Strueref : *Sfalseref;
}

ValueT scm_make_char(VM* vm, scm_char c)
{
  ValueT v;
  setchar(&v, (uchar)c);
  return v;
}

ValueT scm_make_string(VM* vm, const char* s)
{
  return scm_make_string(vm, s, (int)strlen(s));
}

ValueT scm_make_string(VM* vm, const char* s, int len)
{
  ValueT v;
  setstr(&v, vm->strintern(s, len));
  return v;
}

ValueT scm_make_vector(VM* vm, int n)
{
  ValueT v;
  setarray(&v, Sr2(vm, ArrayObj, vm, n));
  return v;
}

ValueT scm_make_list(VM* vm, ValueT* items, int n)
{
  Sgcvar1(vm, out);
  for (int i = n - 1; i >= 0; i--)
    *out = scm_make_pair(vm, &items[i], out);
  return *out;
}

ValueT scm_make_pair(VM* vm, ValueT* a, ValueT* b)
{
  ValueT v;
  setpair(&v, SCM::cons(vm, a, b));
  return v;
}

ValueT scm_make_table(VM* vm)
{
  ValueT v;
  sethashtable(&v, Sr1(vm, HashTableObj, vm));
  return v;
}

void scm_vector_set(VM* vm, ValueT* vec, int i, ValueT v)
{
  if (!isarray(vec))
  {
    typeerr(vm, vec, "vector");
    return;
  }
  ArrayObj* arr = arrayref(vec);
  if (i < 0 || i >= arr->arrayn())
  {
    Print("vector index %d out of range\n", i);
    throw "ScmError";
  }
  arr->set(i, &v);
}

ValueT scm_vector_ref(VM* vm, ValueT* vec, int i)
{
  if (!isarray(vec))
  {
    typeerr(vm, vec, "vector");
    return *Svoidref;
  }
  ArrayObj* arr = arrayref(vec);
  if (i < 0 || i >= arr->arrayn())
  {
    Print("vector index %d out of range\n", i);
    throw "ScmError";
  }
  return *arr->get(i);
}

void scm_table_set(VM* vm, ValueT* tbl, ValueT* key, ValueT val)
{
  if (!ishashtable(tbl))
  {
    typeerr(vm, tbl, "hash-table");
    return;
  }
  hashtableref(tbl)->set(vm, key, &val);
}

ValueT scm_table_ref(VM* vm, ValueT* tbl, ValueT* key)
{
  if (!ishashtable(tbl))
  {
    typeerr(vm, tbl, "hash-table");
    return *Svoidref;
  }
  HashTableObj* t = hashtableref(tbl);
  ValueT* slot = t->get(key);
  if (!slot || isundefined(slot))
  {
    Print("key not found\n");
    throw "ScmError";
  }
  return *slot;
}

// ---------- error ----------

void scm_error(VM* vm, const char* fmt, ...)
{
  (void)vm;
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  Print("%s\n", buf);
  throw "ScmError";
}

} // namespace Scheme
