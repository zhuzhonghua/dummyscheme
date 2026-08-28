#include "vm.h"
#include "scmbasic.h"

namespace Scheme {

static ValueT scm_stub_cdr(VM* vm, ValueT* p)
{
  AssertArgPair(vm, p, "cdr");
  return Scdr(p);
}

static ValueT scm_stub_set_cdr(VM* vm, ValueT* p, ValueT* pd)
{
  AssertArgPair(vm, p, "set-cdr!");
  pairref(p)->cdr(pd);
  return Svoidref;
}

static ValueT scm_stub_car(VM* vm, ValueT* p)
{
  AssertArgPair(vm, p, "car");
  return Scar(p);
}

static ValueT scm_stub_set_car(VM* vm, ValueT* p, ValueT* pd)
{
  AssertArgPair(vm, p, "set-car!");
  pairref(p)->car(pd);
  return Svoidref;
}


#define CARCDR2(NAME, A, B)                           \
static ValueT scm_stub_ ## NAME (VM* vm, ValueT* p) { \
  AssertArgPair(vm, p, #NAME);                            \
  p = B (p);                                          \
  AssertArgPair(vm, p, #NAME);                            \
  return A (p);                                       \
}

CARCDR2(caar, Scar, Scar)
CARCDR2(cadr, Scar, Scdr)
CARCDR2(cdar, Scdr, Scar)
CARCDR2(cddr, Scdr, Scdr)

static ValueT scm_stub_cons(VM* vm, ValueT* p1, ValueT* p2)
{
  ValueT ret;
  setpair(&ret, SCM::cons(vm, p1, p2));
  return ret;
}

static ValueT scm_stub_list(VM* vm, ValueT* p)
{
  return p;
}

static void copylistlast(VM* vm, ValueT* out, ValueT* val, ValueT* rest, const char* METHOD)
{
  PAIR_FOR(valp, val)
  {
    AssertVT(vm, ispair(valp), val, "%s: not a proper list", val);
    setpair(out, SCM::cons(vm, Scar(valp), Snullref));
    out = Scdr(out);
  }
  *out = rest;
}

static void copylistlast(VM* vm, ValueT* out, ValueT* val, const char* METHOD)
{
  if (isnull(Scdr(val)))
  {
    *out = Scar(val);
    return;
  }
  Sgcvar1(vm, out2);
  copylistlast(vm, out2, Scdr(val), METHOD);
  copylistlast(vm, out, Scar(val), out2, METHOD);
}

static ValueT scm_stub_append(VM* vm, ValueT* p)
{
  const static char* METHOD = "append";
  if (isnull(p)) return Snullref;
  Sgcvar1(vm, out);
  copylistlast(vm, out, p, METHOD);
  return out;
}

static ValueT scm_stub_reverse(VM* vm, ValueT* p)
{
  Sgcvar1(vm, rlist);
  PAIR_FOR(pp, p)
  {
    AssertArg(vm, ispair(pp), "reverse", p, "not a list");
    setpair(rlist, SCM::cons(vm, Scar(pp), rlist));
  }
  return rlist;
}

static ValueT scm_stub_memq(VM* vm, ValueT* p1, ValueT* p2)
{
  PAIR_FOR(p2p, p2)
  {
    AssertVT(vm, ispair(p2p), p2, "memq: not a list");

    if (SCM::eqp(p1, Scar(p2p)))
      return p2p;
  }

  return Sfalseref;
}

static ValueT scm_stub_memv(VM* vm, ValueT* p1, ValueT* p2)
{
  PAIR_FOR(p2p, p2)
  {
    AssertVT(vm, ispair(p2p), p2, "memv: not a list");

    if (SCM::equalp(p1, Scar(p2p)))
      return p2p;
  }

  return Sfalseref;
}

static ValueT scm_stub_equalp(VM* vm, ValueT* p1, ValueT* p2)
{
  return frombool(SCM::equalp(p1, p2));
}

static ValueT scm_stub_eqp(VM* vm, ValueT* p1, ValueT* p2)
{
  return frombool(SCM::eqp(p1, p2));
}

static ValueT scm_stub_eqvp(VM* vm, ValueT* p1, ValueT* p2)
{
  return frombool(SCM::eqvp(p1, p2));
}

static ValueT scm_stub_assoc(VM* vm, ValueT* p1, ValueT* p2)
{
  PAIR_FOR(p, p2)
  {
    AssertVT(vm, ispair(p), p, "assoc: not a pair");
    ValueT* aa = Scar(p);
    AssertVT(vm, ispair(aa), aa, "assoc: not a pair");

    if (SCM::equalp(p1, Scar(aa)))
      return aa;
  }

  return Sfalseref;
}

static ValueT scm_stub_assq(VM* vm, ValueT* p1, ValueT* p2)
{
  PAIR_FOR(p, p2)
  {
    AssertVT(vm, ispair(p), p, "assq: not a pair");
    ValueT* aa = Scar(p);
    AssertVT(vm, ispair(aa), aa, "assq: not a pair");

    if (SCM::eqp(p1, Scar(aa)))
      return aa;
  }

  return Sfalseref;
}

static ValueT scm_stub_assv(VM* vm, ValueT* p1, ValueT* p2)
{
  PAIR_FOR(p, p2)
  {
    AssertVT(vm, ispair(p), p, "assv: not a pair");
    ValueT* aa = Scar(p);
    AssertVT(vm, ispair(aa), aa, "assv: not a pair");

    if (SCM::eqp(p1, Scar(aa)))
      return aa;
  }

  return Sfalseref;
}

static ValueT scm_stub_notp(VM* vm, ValueT* p)
{
  return frombool(isfalse(p));
}

static ValueT scm_stub_booleanp(VM* vm, ValueT* p)
{
  return frombool(istrue(p)||isfalse(p));
}

static ValueT scm_stub_charp(VM* vm, ValueT* p)
{
  return frombool(ischar(p));
}

static ValueT scm_stub_symbolp(VM* vm, ValueT* p)
{
  return frombool(issym(p));
}

static ValueT scm_stub_stringp(VM* vm, ValueT* p)
{
  return frombool(isstr(p));
}

static ValueT scm_stub_vectorp(VM* vm, ValueT* p)
{
  return frombool(isarray(p));
}

static ValueT scm_stub_listp(VM* vm, ValueT* p)
{
  return frombool(SCM::listp(p));
}

static ValueT scm_stub_pairp(VM* vm, ValueT* p)
{
  return frombool(ispair(p));
}

static ValueT scm_stub_nullp(VM* vm, ValueT* p)
{
  return frombool(isnull(p));
}

static ValueT scm_stub_procedurep(VM* vm, ValueT* p)
{
  return frombool(isclosure(p)||isnativeproc(p)||iscontinuation(p));
}

static ValueT scm_stub_length(VM* vm, ValueT* p)
{
  int len = SCM::thlength(p);

  ValueT out;
  setnumi(&out, len);
  return out;
}

static ValueT scm_stub_list_ref(VM* vm, ValueT* p1, ValueT* p2)
{
  AssertArg(vm, isnumi(p2), "list-ref", p2, "not a number");
  int k = numi(p2);
  AssertArg(vm, k >= 0, "list-ref", p2, "number must be non-negative");

  ValueT* p = p1;
  while (k-- > 0)
  {
    AssertArg(vm, ispair(p1), "list-ref", p, "not a pair");
    p1 = Scdr(p1);
  }

  AssertArg(vm, ispair(p1), "list-ref", p, "not a pair");
  return Scar(p1);
}

static ValueT scm_stub_list_tail(VM* vm, ValueT* p1, ValueT* p2)
{
  AssertArg(vm, isnumi(p2), "list-tail", p2, "not a number");
  int k = numi(p2);
  AssertArg(vm, k >= 0, "list-tail", p2, "number must be non-negative");

  ValueT* p = p1;
  while (k-- > 0)
  {
    AssertArg(vm, ispair(p1), "list-tail", p, "not a pair");
    p1 = Scdr(p1);
  }

  return p1;
}

static ValueT scm_stub_symbol2string(VM* vm, ValueT* sym)
{
  AssertArg(vm, issym(sym), "symbol->string", sym, "not a symbol");
  ValueT out;
  setstr(&out, symref(sym));
  strref(&out)->setimmutable();
  return out;
}

static ValueT scm_stub_string2symbol(VM* vm, ValueT* p)
{
  AssertArg(vm, isstr(p), "string->symbol", p, "not a string");
  StrPtr sp = strref(p);
  ValueT out;
  setsym(&out, Intern(vm)->intern(sp->str, sp->len));
  return out;
}

static ValueT scm_stub_vector2list(VM* vm, ValueT* vec)
{
  AssertArg(vm, isarray(vec), "vector->list", vec, "not a vector");

  Sgcvar1(vm, out);
  SCM::vector2cons(vm, out, arrayref(vec));

  return out;
}

static ValueT scm_stub_list2vector(VM* vm, ValueT* lst)
{
  AssertArg(vm, SCM::listp(lst), "list->vector", lst, "not a list");

  Sgcvar1(vm, out);
  SCM::list2vector(vm, lst, out);

  return out;
}

void SCMBasic::init(VM* vm)
{
  const RegCProc basics[] = {
    {"car", scm_stub_car},
    {"set-car!", scm_stub_set_car},

    {"cdr", scm_stub_cdr},
    {"set-cdr!", scm_stub_set_cdr},

    STUB_REG1(caar),
    STUB_REG1(cadr),
    STUB_REG1(cddr),
    STUB_REG1(cdar),

    {"cons", scm_stub_cons},
    {"list", scm_stub_list, true},
    {"append", scm_stub_append, true},
    {"reverse", scm_stub_reverse},
    {"length", scm_stub_length},

    {"list-tail", scm_stub_list_tail},
    {"list-ref", scm_stub_list_ref},

    {"memq", scm_stub_memq},
    {"memv", scm_stub_memv},
    {"member", scm_stub_memv},

    {"equal?", scm_stub_equalp},
    {"eq?", scm_stub_eqp},
    {"eqv?", scm_stub_eqvp},
    {"assv", scm_stub_assv},
    {"assq", scm_stub_assq},
    {"assoc", scm_stub_assoc},

    {"not", scm_stub_notp},

    {"procedure?", scm_stub_procedurep},
    {"boolean?", scm_stub_booleanp},
    {"null?", scm_stub_nullp},
    {"pair?", scm_stub_pairp},
    {"list?", scm_stub_listp},
    {"string?", scm_stub_stringp},
    {"vector?", scm_stub_vectorp},
    {"char?", scm_stub_charp},
    {"symbol?", scm_stub_symbolp},

    {"symbol->string", scm_stub_symbol2string},
    {"string->symbol", scm_stub_string2symbol},
    {"vector->list", scm_stub_vector2list},
    {"list->vector", scm_stub_list2vector},

    {NULL, -1}
  };
  regcfunc(vm, basics);
}

};
