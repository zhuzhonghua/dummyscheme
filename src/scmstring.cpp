#include "vm.h"
#include "scmstring.h"

namespace Scheme {

static ValueT scm_stub_make_string(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "make-string";
  AssertVT(vm, isnumi(p), p, "%s:not a number", METHOD);
  int n = numi(p);
  StrObj* ptr = NULL;
  if (isnull(args))
    ptr = vm->strintern(n);
  else
  {
    AssertVT(vm, ischar(Scar(args)), args, "%s: not a char", METHOD);
    AssertVT(vm, isnull(Scdr(args)), args, "%s: to many arguments", METHOD);
    ptr = vm->strintern(n, vtchar(Scar(args)));
  }
  ValueT out;
  setstr(&out, ptr);
  return out;
}

static ValueT scm_stub_string(VM* vm, ValueT* args)
{
  int len = SCM::length(args);
  StrObj* ptr = vm->strintern(len);
  int i = 0;
  PAIR_FOR(p, args)
  {
    ValueT* pvt = Scar(p);
    AssertArg(vm, ischar(pvt), "string", args, "not a char");
    ptr->str[i++] = vtchar(pvt);
  }

  ValueT out;
  setstr(&out, ptr);
  return out;
}

static ValueT scm_stub_string_2list(VM* vm, ValueT* p)
{
  AssertArg(vm, isstr(p), "string->list", p, "not a string");

  StrObj* pstr = strref(p);
  Sgcvar1(vm, out);

  for (int n = Sslen(pstr) - 1; n >= 0; n--)
  {
    ValueT svc;
    setchar(&svc, pstr->str[n]);
    setpair(out, SCM::cons(vm, &svc, out));
  }
  return out;
}

static ValueT scm_stub_list_2string(VM* vm, ValueT* p)
{
  int len = SCM::thlength(p);
  AssertArg(vm, len >= 0, "list->string", p, "not a proper list");
  Sgcvar1(vm, out);
  StrObj* pstr = vm->strintern(len);
  setstr(out, pstr);
  int i = 0;
  PAIR_FOR(pp, p)
  {
    ValueT* ppvt = Scar(pp);
    AssertArg(vm, ischar(ppvt), "list->string", p, "not a char");
    pstr->str[i++] = vtchar(ppvt);
  }
  return out;
}

static ValueT scm_stub_string_fill(VM* vm, ValueT* p1, ValueT* p2)
{
  AssertArg(vm, isstr(p1), "string-fill!", p1, "not a string");
  AssertArg(vm, ischar(p2), "string-fill!", p2, "not a string");

  StrObj* ptr = strref(p1);
  AssertArg(vm, !ptr->isimmutable(), "string-fill!", p1, "an immutable string");
  int len = Sslen(ptr);
  for (int i = 0; i < len; i++)
    ptr->str[i] = vtchar(p2);

  return Svoidref;
}

static ValueT scm_stub_string_copy(VM* vm, ValueT* p)
{
  AssertArg(vm, isstr(p), "string-copy", p, "not a string");
  StrObj* ptr = strref(p);
  int len = Sslen(ptr);
  StrObj* pstr = vm->strintern(ptr->str, len);
  ValueT out;
  setstr(&out, pstr);
  return out;
}

static ValueT scm_stub_string_append(VM* vm, ValueT* args)
{
  int tlen = 0;
  PAIR_FOR(p, args)
  {
    ValueT* pvt = Scar(p);
    AssertArg(vm, isstr(pvt), "string-append", pvt, "not a string");
    StrObj* pvtstr = strref(pvt);
    tlen += Sslen(pvtstr);
  }
  StrObj* pstr = vm->strintern(tlen);
  tlen = 0;
  PAIR_FOR(p, args)
  {
    ValueT* pvt = Scar(p);
    StrObj* pvtstr = strref(pvt);
    int len = Sslen(pvtstr);
    memcpy(&pstr->str[tlen], pvtstr->str, len);
    tlen += len;
  }

  ValueT out;
  setstr(&out, pstr);
  return out;
}

static ValueT scm_stub_substring(VM* vm, ValueT* p, ValueT* start, ValueT* end)
{
  AssertArg(vm, isstr(p), "substring", p, "not a string");
  AssertArg(vm, isnumi(start), "substring", start, "not an integer");
  AssertArg(vm, isnumi(end), "substring", end, "not an integer");

  StrObj* pstr = strref(p);
  int istart = numi(start);
  int iend = numi(end);

  Assert(vm, 0 <= istart && istart <= iend && iend <= Sslen(pstr),
         "substring: out of range start:%d end:%d", istart, iend);

  int len = iend - istart;
  StrObj* ptr = vm->strintern(pstr->str+istart, len);

  ValueT out;
  setstr(&out, ptr);
  return out;
}

static ValueT scm_stub_string_len(VM* vm, ValueT* p)
{
  AssertArg(vm, isstr(p), "string-length", p, "not a string");

  ValueT out;
  setnumi(&out, Sslen(strref(p)));
  return out;
}

static ValueT scm_stub_string_ref(VM* vm, ValueT* p, ValueT* k)
{
  AssertArg(vm, isstr(p), "string-ref", p, "not a string");
  AssertArg(vm, isnumi(k), "string-ref", k, "not an integer");

  StrObj* pstr = strref(p);
  AssertVT2(vm, numi(k) < Sslen(pstr), p, k, "string-ref: out of range");

  ValueT out;
  setchar(&out, pstr->str[numi(k)]);
  return out;
}

static ValueT scm_stub_string_set(VM* vm, ValueT* p, ValueT* k, ValueT* ch)
{
  AssertArg(vm, isstr(p), "string-set!", p, "not a string");
  AssertArg(vm, isnumi(k), "string-set!", k, "not an integer");
  AssertArg(vm, ischar(ch), "string-set!", ch, "not a char");

  StrObj* pstr = strref(p);
  AssertVT(vm, !pstr->isimmutable(), p, "string-set!: immutable");
  AssertVT2(vm, numi(k) < Sslen(pstr), p, k, "string-set!: out of range");

  pstr->str[numi(k)] = vtchar(ch);

  return Svoidref;
}

#define STUB_STRING_CMP(NAME, NAME_S, OP) static                      \
 ValueT scm_stub_string_##NAME(VM* vm, ValueT* p, ValueT* args) {     \
 AssertArg(vm, isstr(p), #NAME_S, p, "not a string");                 \
 StrObj* pstr = strref(p);                                            \
 PAIR_FOR(pa, args) {                                                 \
   ValueT* pavt = Scar(pa);                                         \
   AssertArg(vm, isstr(pavt), #NAME_S, pavt, "not a string");         \
   StrObj* pastr = strref(pavt);                                      \
   int len = Sslen(pstr) < Sslen(pastr) ? Sslen(pstr) : Sslen(pastr); \
   int diff = strncmp(pstr->str, pastr->str, len);                   \
   if (!diff)                                                         \
     diff = Sslen(pstr) - Sslen(pastr);                               \
   if (!(diff OP 0))                                                  \
     return Sfalseref;                                                \
 }                                                                    \
 return Strueref;                                                     \
}

STUB_STRING_CMP(less, string<?, <)
STUB_STRING_CMP(bigger, string>?, >)
STUB_STRING_CMP(lesseq, string<=?, <=)
STUB_STRING_CMP(biggereq, string>=?, >=)

#define STUB_STRING_CI_CMP(NAME, NAME_S, OP) static                   \
 ValueT scm_stub_string_ci_##NAME(VM* vm, ValueT* p, ValueT* args) {  \
 AssertArg(vm, isstr(p), #NAME_S, p, "not a string");                 \
 StrObj* pstr = strref(p);                                            \
 PAIR_FOR(pa, args) {                                                 \
   ValueT* pavt = Scar(pa);                                         \
   AssertArg(vm, isstr(pavt), #NAME_S, pavt, "not a string");         \
   StrObj* pastr = strref(pavt);                                      \
   int len = Sslen(pstr) < Sslen(pastr) ? Sslen(pstr) : Sslen(pastr); \
   int diff = 0;                                                      \
   for (int i = 0; i < len; i++) {                                    \
     diff = tolower(pstr->str[i]) - tolower(pastr->str[i]);           \
     if (!diff) break;                                                \
   }                                                                  \
   if (!diff)                                                         \
     diff = Sslen(pstr) - Sslen(pastr);                               \
   if (!(diff OP 0))                                                  \
     return Sfalseref;                                                \
 }                                                                    \
 return Strueref;                                                     \
}

STUB_STRING_CI_CMP(less, string-ci<?, <)
STUB_STRING_CI_CMP(bigger, string-ci>?, >)
STUB_STRING_CI_CMP(lesseq, string-ci<=?, <=)
STUB_STRING_CI_CMP(biggereq, string-ci>=?, >=)

static ValueT scm_stub_string_eq(VM* vm, ValueT* p, ValueT* args)
{
  AssertArg(vm, isstr(p), "string=?", p, "not a string");

  StrObj* pstr = strref(p);
  PAIR_FOR(pa, args)
  {
    ValueT* pavt = Scar(pa);
    AssertArg(vm, isstr(pavt), "string=?", pavt, "not a string");
    StrObj* pastr = strref(pavt);
    if (!pstr->equalp(pastr)) return Sfalseref;
  }
  return Strueref;
}

static ValueT scm_stub_string_ci_eq(VM* vm, ValueT* p, ValueT* args)
{
  AssertArg(vm, isstr(p), "string-ci=?", p, "not a string");

  StrObj* pstr = strref(p);
  PAIR_FOR(pa, args)
  {
    ValueT* pavt = Scar(pa);
    AssertArg(vm, isstr(pavt), "string-ci=?", pavt, "not a string");
    StrObj* pastr = strref(pavt);
    if (Sslen(pstr) != Sslen(pastr))
      return Sfalseref;

    int len = Sslen(pstr);
    for (int i = 0; i < len; i++)
      if (toupper(pstr->str[i]) != toupper(pastr->str[i]))
        return Sfalseref;
  }

  return Strueref;
}

static ValueT scm_stub_char_upcase(VM* vm, ValueT* p)
{
  AssertArg(vm, ischar(p), "char-upcase", p, "not a char");

  ValueT out;
  setchar(&out, toupper(vtchar(p)));
  return out;
}

static ValueT scm_stub_char_downcase(VM* vm, ValueT* p)
{
  AssertArg(vm, ischar(p), "char-downcase", p, "not a char");

  ValueT out;
  setchar(&out, tolower(vtchar(p)));
  return out;
}

static ValueT scm_stub_char_toint(VM* vm, ValueT* p)
{
  AssertArg(vm, ischar(p), "char->integer", p, "not a char");

  ValueT out;
  setnumi(&out, vtchar(p));
  return out;
}

static ValueT scm_stub_int_tochar(VM* vm, ValueT* p)
{
  AssertArg(vm, isnumi(p), "integer->char", p, "not an integer");

  ValueT out;
  setchar(&out, numi(p));
  return out;
}

#define STUB_CHAR_CMP(NAME, NAME_S, OP) static                    \
 ValueT scm_stub_char_##NAME(VM* vm, ValueT* p, ValueT* args){    \
 AssertArg(vm, ischar(p), #NAME_S, p, "not a char");              \
 PAIR_FOR(p1, args){                                              \
   AssertArg(vm, ischar(Scar(p1)), #NAME_S, p1, "not a char");  \
   if (!(vtchar(p) OP vtchar(Scar(p1)))) return Sfalseref;      \
 }                                                                \
 return Strueref;                                                 \
}

STUB_CHAR_CMP(eq, char=?, ==)
STUB_CHAR_CMP(less, char<?, <)
STUB_CHAR_CMP(bigger, char>?, >)
STUB_CHAR_CMP(lesseq, char<=?, <=)
STUB_CHAR_CMP(biggereq, char>=?, >=)

#define STUB_CHAR_CI_CMP(NAME, NAME_S, OP) static                   \
  ValueT scm_stub_char_ci_##NAME(VM* vm, ValueT* p, ValueT* args){  \
    AssertArg(vm, ischar(p), #NAME_S, p, "not a char");             \
    char cp = tolower(vtchar(p));                                   \
    PAIR_FOR(p1, args){                                             \
      AssertArg(vm, ischar(Scar(p1)), #NAME_S, p1, "not a char"); \
      if (!(cp OP tolower(vtchar(Scar(p1))))) return Sfalseref;   \
    }                                                               \
    return Strueref;                                                \
  }

STUB_CHAR_CI_CMP(eq, char-ci=?, ==)
STUB_CHAR_CI_CMP(less, char-ci<?, <)
STUB_CHAR_CI_CMP(bigger, char-ci>?, >)
STUB_CHAR_CI_CMP(lesseq, char-ci<=?, <=)
STUB_CHAR_CI_CMP(biggereq, char-ci>=?, >=)

static ValueT scm_stub_char_alphabeticq(VM* vm, ValueT* p)
{
  AssertArg(vm, ischar(p), "char-alphabetic?", p, "not a char");

  char cu = toupper(vtchar(p));
  return frombool(65 <= cu && cu <= 90);
}

static ValueT scm_stub_char_numericq(VM* vm, ValueT* p)
{
  AssertArg(vm, ischar(p), "char-numeric?", p, "not a char");

  char cu = vtchar(p);
  return frombool(48 <= cu && cu <= 57);
}

static ValueT scm_stub_char_upperq(VM* vm, ValueT* p)
{
  AssertArg(vm, ischar(p), "char-upper-case?", p, "not a char");

  char cu = vtchar(p);
  return frombool(65 <= cu && cu <= 90);
}

static ValueT scm_stub_char_lowerq(VM* vm, ValueT* p)
{
  AssertArg(vm, ischar(p), "char-lower-case?", p, "not a char");

  char cu = vtchar(p);
  return frombool(97 <= cu && cu <= 122);
}

static ValueT scm_stub_char_whitespaceq(VM* vm, ValueT* p)
{
  AssertArg(vm, ischar(p), "char-whitespace?", p, "not a char");

  char cu = vtchar(p);
  switch(vtchar(p)) {
  case ' ':
  case '\t':
  case '\n':
  case '\r':
    return Strueref;
  }

  return Sfalseref;
}

void SCMStr::init(VM* vm)
{
  const RegCProc strlib[] = {
    {"make-string", scm_stub_make_string, true},
    {"string", scm_stub_string, true},
    {"substring", scm_stub_substring},
    {"string-append", scm_stub_string_append, true},
    {"string-copy", scm_stub_string_copy},
    {"string-fill!", scm_stub_string_fill},
    {"string-length", scm_stub_string_len},
    {"string-ref", scm_stub_string_ref},
    {"string-set!", scm_stub_string_set},
    {"string->list", scm_stub_string_2list},
    {"list->string", scm_stub_list_2string},

    {"string=?", scm_stub_string_eq, true},
    {"string<?", scm_stub_string_less, true},
    {"string>?", scm_stub_string_bigger, true},
    {"string<=?", scm_stub_string_lesseq, true},
    {"string>=?", scm_stub_string_biggereq, true},

    {"string-ci=?", scm_stub_string_ci_eq, true},
    {"string-ci<?", scm_stub_string_ci_less, true},
    {"string-ci>?", scm_stub_string_ci_bigger, true},
    {"string-ci<=?", scm_stub_string_ci_lesseq, true},
    {"string-ci>=?", scm_stub_string_ci_biggereq, true},

    {"char-upcase", scm_stub_char_upcase},
    {"char-downcase", scm_stub_char_downcase},

    {"char->integer", scm_stub_char_toint},
    {"integer->char", scm_stub_int_tochar},

    {"char=?", scm_stub_char_eq, true},
    {"char<?", scm_stub_char_less, true},
    {"char>?", scm_stub_char_bigger, true},
    {"char<=?", scm_stub_char_lesseq, true},
    {"char>=?", scm_stub_char_biggereq, true},

    {"char-ci=?", scm_stub_char_ci_eq, true},
    {"char-ci<?", scm_stub_char_ci_less, true},
    {"char-ci>?", scm_stub_char_ci_bigger, true},
    {"char-ci<=?", scm_stub_char_ci_lesseq, true},
    {"char-ci>=?", scm_stub_char_ci_biggereq, true},

    {"char-alphabetic?", scm_stub_char_alphabeticq},
    {"char-numeric?", scm_stub_char_numericq},
    {"char-whitespace?", scm_stub_char_whitespaceq},
    {"char-upper-case?", scm_stub_char_upperq},
    {"char-lower-case?", scm_stub_char_lowerq},

    {NULL, -1}
  };
  regcfunc(vm, strlib);
}

};
