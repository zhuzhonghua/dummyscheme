// =====================================================================
// scmapi.cpp — implementation of scmapi.h
//
// =====================================================================

#include "vm.h"
#include <string>

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

int scm_proper_list_len(VM* vm, ValueT* list)
{
  (void)vm;
  int n = 0;
  ValueT* p = list;
  for (;;)
  {
    if (isnull(p))
    {
      return n;
    }
    if (!ispair(p))
    {
      return -1;
    }
    p = Scdr(p);
    n++;
  }
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

int scm_vector_len(VM* vm, ValueT* vec)
{
  if (!isarray(vec))
  {
    typeerr(vm, vec, "vector");
    return 0;
  }
  return arrayref(vec)->arrayn();
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

ValueT scm_table_get(VM* vm, ValueT* tbl, ValueT* key)
{
  if (!ishashtable(tbl))
  {
    typeerr(vm, tbl, "hash-table");
    return *Snullref;
  }
  HashTableObj* t = hashtableref(tbl);
  ValueT* slot = t->get(key);
  if (!slot || isundefined(slot))
  {
    return *Snullref;
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

// ---------- VM entry ----------

VM* scm_create_vm(ScmAlloc a, void* ud)
{
  int gsize = sizeof(GlobalVM);
  void* mem = a(ud, NULL, 0, gsize);
  if (!mem)
    return NULL;
  GlobalVM* g = NULL;
  TRY {
    g = new (mem) GlobalVM(a, ud);
  }
  CATCH(err) {
    a(ud, mem, gsize, 0);  /* release the raw block before propagating */
    throw;
  }
  return g->mainstk();
}

void scm_destroy_vm(VM* vm)
{
  GlobalVM* g = vm->g;
  ScmAlloc a = g->getfrealloc();
  void* ud = g->getallocud();
  g->getcachegroup()->fullsweep();   /* release cached pair pool (same path as GC) */
  g->getobjgroup()->releaseall();    /* free all runtime objects, like Lua's freeall */
  g->~GlobalVM();                    /* run member destructors (intern table, genv, ...) */
  a(ud, g, sizeof(GlobalVM), 0);     /* free the GlobalVM itself via its allocator */
}

// ---------- host library registration & eval ----------

void scm_register_lib(VM* vm, const RegCProc* funcs)
{
  regcfunc(vm, funcs);
}

ValueT scm_eval_string(VM* vm, const char* s, int len)
{
  /* evalstr() needs a NUL-terminated source; len may come from a wire
     buffer without a terminator, so copy locally. */
  char* src = (char*)malloc(len + 1);
  if (!src)
    throw "OutOfMemory";
  memcpy(src, s, len);
  src[len] = '\0';
  ValueT res;
  try {
    res = vm->evalstr(src);
  } catch (...) {
    free(src);
    throw;
  }
  free(src);
  return res;
}

ValueT scm_compile(VM* vm, const char* s, int len)
{
  /* one (begin ...) form; the newline before ")" keeps a trailing
     "; comment" from eating the closing paren */
  std::string src(s, len);
  std::string wrapped = "(begin ";
  wrapped += src;
  wrapped += "\n)";
  return vm->compile(wrapped.c_str(), src.c_str());
}

bool scm_apply(VM* vm, ValueT* fn, ValueT** argv, int argc, ValueT* out)
{
  return G(vm)->pcall(fn, argv, argc, out);
}

// ---------- global registry ----------

void scm_registry_set(VM* vm, const char* name, ValueT v)
{
  Sgcvar1(vm, key);
  *key = scm_make_string(vm, name, (int)strlen(name));
  hashtableref(&G(vm)->registry)->set(vm, key, &v);
}

ValueT scm_registry_get(VM* vm, const char* name)
{
  Sgcvar1(vm, key);
  *key = scm_make_string(vm, name, (int)strlen(name));
  ValueT* v = hashtableref(&G(vm)->registry)->get(key);
  return v ? *v : *Snullref;  /* null when absent, like Lua registry (nil) */
}

void scm_registry_unset(VM* vm, const char* name)
{
  Sgcvar1(vm, key);
  *key = scm_make_string(vm, name, (int)strlen(name));
  hashtableref(&G(vm)->registry)->set(vm, key, Snullref);  /* stores null, like Lua pushing nil to clear */
}

// ---------- global variables ----------

void scm_set_global(VM* vm, const char* name, ValueT v)
{
  Sgcvar2(vm, symv, val);
  *val = v;
  SymPtr sym = Intern(vm)->internsym(name, (int)strlen(name));
  setsym(symv, sym);                    /* keep the sym alive across any GC inside set */
  GEnv(vm)->set(vm, symv, val);
}

ValueT scm_get_global(VM* vm, const char* name)
{
  Sgcvar1(vm, symv);
  SymPtr sym = Intern(vm)->internsym(name, (int)strlen(name));
  setsym(symv, sym);
  ValueT* slot = GEnv(vm)->get(symv);
  ValueT v;
  if (slot) v = *slot; else setundefined(&v);
  return isundefined(&v) ? *Snullref : v;  /* null when not defined, like the registry API */
}

// ---------- option args / type names ----------

scm_int scm_opt_int(VM* vm, ValueT* p, scm_int def)
{
  if (!p || isnull(p))
    return def;
  if (!isnumi(p))
  {
    typeerr(vm, p, "integer (or nil)");
    return def;
  }
  return numi(p);
}

bool scm_is_none_or_nil(VM* vm, ValueT* p)
{
  (void)vm;
  return p == NULL || isnull(p);
}

const char* scm_typename(VM* vm, ValueT* p)
{
  (void)vm;
  if (isnumi(p) || isnumreal(p) || isnumbig(p)) return "number";
  if (isstr(p)) return "string";
  if (isboolean(p)) return "boolean";
  if (ischar(p)) return "char";
  if (issym(p)) return "symbol";
  if (ispair(p)) return "pair";
  if (isnull(p)) return "null";
  if (isarray(p)) return "vector";
  if (ishashtable(p)) return "hash-table";
  if (isbytevector(p)) return "bytevector";
  if (isclosure(p)) return "closure";
  if (isvaluesobj(p)) return "values";
  if (isvoid(p)) return "void";
  return "unknown";
}

// ---------- lightuserdata (tagged raw pointer; no payload, no GC) ----------

ValueT scm_make_lightuserdata(VM* vm, void* p)
{
  (void)vm;
  ValueT out;
  setlightuserdata(&out, p);
  return out;
}

bool scm_is_lightuserdata(VM* vm, ValueT* p)
{
  (void)vm;
  return islightuserdata(p);
}

void* scm_to_lightuserdata(VM* vm, ValueT* p)
{
  if (!p || !islightuserdata(p))
  {
    typeerr(vm, p, "lightuserdata");
    return NULL;
  }
  return lightuserdataptr(p);
}

// ---------- value -> string ----------

const char* scm_tostring(VM* vm, ValueT* p, int* len)
{
  /* protect the temp port from GC while writing (API boundary; GC may run at any point) */
  Sgcvar1(vm, outv);
  setoport(outv, Sr1(vm, OutputPortStrObj, vm));
  OutputPortStrObj* out = oportstrref(outv);
  out->write(vm, p);

  Sgcvar1(vm, sv);
  *sv = scm_make_string(vm, out->strbuf.buf, out->strbuf.count);
  /* keep the result alive across GC, like Lua's stack-top for lua_tolstring:
     the returned pointer stays valid until the next scm_tostring call overwrites it */
  scm_registry_set(vm, "~tostring", *sv);
  if (len)
    *len = strref(sv)->len;
  return strref(sv)->str;
}

} // namespace Scheme
