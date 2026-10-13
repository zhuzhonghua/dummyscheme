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

  const int n = numi(p);
  AssertArg(vm, n >= SCM_CHAR_MIN && n <= SCM_CHAR_MAX, "integer->char", p,
            "not in char range [%d, %d]", SCM_CHAR_MIN, SCM_CHAR_MAX);
  ValueT out;
  setchar(&out, (uchar)n);
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

/* ---- format (SRFI-28 basic + SRFI-159 numeric subset, R7RS-large) ----
   (format fmt . args)  /  (string-format fmt . args)
   ~a display | ~s write | ~d decimal | ~x hex | ~o octal | ~b binary
   ~c char | ~f fixed | ~e exp | ~g general | ~% newline | ~~ tilde
   numeric conversions accept integers; ~f/~e/~g also accept reals. */

static void padnum(OutputPortStrObj* out, const char* buf, int width, char pad)
{
  int len = (int)strlen(buf);
  for (int i = len; i < width; i++)
    out->writechar(pad);
  out->writestr(buf, len);
}

static ValueT scm_stub_format(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "format";
  AssertArg(vm, isstr(p), METHOD, p, " not a string");
  StrObj* fmt = strref(p);
  OutputPortStrObj* out = Sr1(vm, OutputPortStrObj, vm);

  for (int i = 0; i < fmt->len; i++)
  {
    char c = fmt->str[i];
    if (c != '~')
    {
      out->writechar(c);
      continue;
    }
    if (++i >= fmt->len)
      Error(vm, "%s: dangling ~", METHOD);
    /* optional params: [0][width][,precision]  (SRFI-159 subset) */
    char pad = ' ';
    int width = 0;
    int prec = -1;
    if (fmt->str[i] == '0')
    {
      pad = '0';
      if (++i >= fmt->len)
        Error(vm, "%s: dangling ~", METHOD);
    }
    while (i < fmt->len && isdigit((unsigned char)fmt->str[i]))
    {
      width = width * 10 + (fmt->str[i] - '0');
      i++;
    }
    if (i >= fmt->len)
      Error(vm, "%s: dangling ~", METHOD);
    if (fmt->str[i] == ',')
    {
      prec = 0;
      if (++i >= fmt->len)
        Error(vm, "%s: dangling ~", METHOD);
      while (i < fmt->len && isdigit((unsigned char)fmt->str[i]))
      {
        prec = prec * 10 + (fmt->str[i] - '0');
        i++;
      }
      if (i >= fmt->len)
        Error(vm, "%s: dangling ~", METHOD);
    }
    char d = fmt->str[i];
    char buf[128] = {0};
    switch (d)
    {
    case '~': out->writechar('~'); break;
    case '%': out->writechar('\n'); break;
    case '&': out->writechar('\n'); break;
    case 'a':
    case 's':
      if (isnull(args)) Error(vm, "%s: too few arguments", METHOD);
      {
        ValueT* v = Scar(args);
        args = Scdr(args);
        if (d == 'a' && isstr(v))
          out->writestr(Ssstr(strref(v)), Sslen(strref(v)));
        else if (d == 'a' && ischar(v))
          out->writechar(vtchar(v));
        else
          out->write(vm, v);
      }
      break;
    case 'd':
    case 'x':
    case 'o':
      if (isnull(args)) Error(vm, "%s: too few arguments", METHOD);
      {
        ValueT* v = Scar(args);
        args = Scdr(args);
        AssertArg(vm, isnumi(v), METHOD, v, " ~d/~x/~o: not an integer");
        const char* fmtp = d == 'd' ? "%lld" : (d == 'x' ? "%llx" : "%llo");
        snprintf(buf, sizeof(buf), fmtp, (long long)(scm_uint)numi(v));
        padnum(out, buf, width, pad);
      }
      break;
    case 'b':
      if (isnull(args)) Error(vm, "%s: too few arguments", METHOD);
      {
        ValueT* v = Scar(args);
        args = Scdr(args);
        AssertArg(vm, isnumi(v), METHOD, v, " ~b: not an integer");
        scm_uint u = (scm_uint)numi(v);
        int pos = 0;
        bool any = false;
        for (int bit = 63; bit >= 0; bit--)
        {
          if ((u >> bit) & 1) { buf[pos++] = '1'; any = true; }
          else if (any) buf[pos++] = '0';
        }
        if (!any) buf[pos++] = '0';
        buf[pos] = 0;
        padnum(out, buf, width, pad);
      }
      break;
    case 'c':
      if (isnull(args)) Error(vm, "%s: too few arguments", METHOD);
      {
        ValueT* v = Scar(args);
        args = Scdr(args);
        AssertArg(vm, ischar(v), METHOD, v, " ~c: not a char");
        out->writechar(vtchar(v));
      }
      break;
    case 'f':
    case 'e':
    case 'g':
      if (isnull(args)) Error(vm, "%s: too few arguments", METHOD);
      {
        ValueT* v = Scar(args);
        args = Scdr(args);
        double f;
        if (isnumreal(v))
          f = numreal(v);
        else if (isnumi(v))
          f = (double)numi(v);
        else
          AssertArg(vm, false, METHOD, v, " ~f/~e/~g: not a number");
        const char* fmtp = d == 'f' ? "%f" : (d == 'e' ? "%e" : "%g");
        if (prec >= 0)
        {
          char pf[16] = {0};
          snprintf(pf, sizeof(pf), "%%.%df", prec);
          fmtp = pf;
        }
        snprintf(buf, sizeof(buf), fmtp, f);
        padnum(out, buf, width, pad);
      }
      break;
    default:
      Error(vm, "%s: unknown format directive ~%c", METHOD, d);
    }
  }
  if (!isnull(args))
    Error(vm, "%s: too many arguments", METHOD);

  ValueT res;
  setstr(&res, G(vm)->strintern(out->strbuf.buf, out->strbuf.count));
  out->close();
  return res;
}

/* ---- string-format: Lua string.format (printf-style), strobj-backed ----
   %[flags][width][.precision]conv, conv in c d i u o x X e E f g G s %;
   flags/width/precision are copied verbatim into the C sub-format (no '*'),
   mirroring Lua's getformat + addlenmod (lstrlib.c str_format). */

static ValueT* fmt_nextarg(VM* vm, ValueT*& args, const char* METHOD)
{
  if (isnull(args))
    Error(vm, "%s: no value for format directive", METHOD);
  ValueT* v = Scar(args);
  args = Scdr(args);
  return v;
}

static scm_int fmt_intval(VM* vm, ValueT* v, const char* METHOD)
{
  AssertArg(vm, isnumi(v), METHOD, v, " not an integer");
  return numi(v);
}

static double fmt_numval(VM* vm, ValueT* v, const char* METHOD)
{
  if (isnumreal(v))
    return numreal(v);
  if (isnumi(v))
    return (double)numi(v);
  AssertArg(vm, false, METHOD, v, " not a number");
  return 0;
}

/* single snprintf call on the fully-formed sub-format; error on truncation */
template<typename T>
static void fmt_emit(VM* vm, Lbuffer& out, const char* METHOD, const char* subfmt, T value)
{
  char buf[512];
  int r = snprintf(buf, sizeof(buf), subfmt, value);
  if (r < 0 || r >= (int)sizeof(buf))
    Error(vm, "%s: format result too large", METHOD);
  out.put(buf, r);
}

/* Lua addlenmod: splice the 64-bit length modifier before the specifier */
static void addlenmod(char* subfmt, int* sl, char spec)
{
  subfmt[(*sl)++] = 'l';
  subfmt[(*sl)++] = 'l';
  subfmt[(*sl)++] = spec;
  subfmt[*sl] = 0;
}

static ValueT scm_stub_string_format(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "string-format";
  AssertArg(vm, isstr(p), METHOD, p, " not a string");
  StrObj* fmt = strref(p);
  Lbuffer out(vm);
  int i = 0;
  while (i < fmt->len)
  {
    char c = fmt->str[i];
    if (c != '%')
    {
      out.put(c);
      i++;
      continue;
    }
    i++;
    if (i < fmt->len && fmt->str[i] == '%')
    {
      out.put('%');
      i++;
      continue;
    }
    /* getformat: copy flags/width/precision verbatim ('0' is a flag);
       also record numeric width/precision for %s */
    char subfmt[32];
    int sl = 0;
    subfmt[sl++] = '%';
    int wnum = 0, pnum = -1;  /* numeric width / precision (0 = none) */
    bool left = false;
    while (i < fmt->len && strchr("-+ #0123456789.", fmt->str[i]))
    {
      char fc = fmt->str[i];
      subfmt[sl++] = fc;
      if (fc == '-')
        left = true;
      else if (fc == '.')
        pnum = 0;
      else if (fc >= '0' && fc <= '9')
      {
        if (pnum >= 0)
          pnum = pnum * 10 + (fc - '0');
        else
          wnum = wnum * 10 + (fc - '0');
      }
      i++;
    }
    if (i >= fmt->len || sl >= (int)sizeof(subfmt) - 3)
      Error(vm, "%s: invalid format (too long)", METHOD);
    char conv = fmt->str[i++];
    switch (conv)
    {
    case 'd': case 'i': case 'u': case 'o': case 'x': case 'X':
    {
      ValueT* v = fmt_nextarg(vm, args, METHOD);
      scm_int iv = fmt_intval(vm, v, METHOD);
      char mc = conv == 'd' || conv == 'i' ? 'd' : conv == 'u' ? 'u'
                 : conv == 'o' ? 'o' : conv == 'X' ? 'X' : 'x';
      addlenmod(subfmt, &sl, mc);
      if (mc == 'd')
        fmt_emit(vm, out, METHOD, subfmt, iv);
      else
        fmt_emit(vm, out, METHOD, subfmt, (scm_uint)iv);
      break;
    }
    case 'c':
    {
      ValueT* v = fmt_nextarg(vm, args, METHOD);
      int ch = (int)fmt_intval(vm, v, METHOD);
      AssertArg(vm, ch >= 0 && ch <= 255, METHOD, v, " %c: out of range 0..255");
      out.put((char)ch);
      break;
    }
    case 's':
    {
      ValueT* v = fmt_nextarg(vm, args, METHOD);
      AssertArg(vm, isstr(v), METHOD, v, " %s: not a string");
      StrObj* sv = strref(v);
      int slen = sv->len;
      if (pnum >= 0 && pnum < slen)
        slen = pnum;  /* precision: truncate */
      if (wnum > slen)
      {
        int pad = wnum - slen;
        if (left)
        {
          out.put(sv->str, slen);
          while (pad-- > 0)
            out.put(' ');
        }
        else
        {
          while (pad-- > 0)
            out.put(' ');
          out.put(sv->str, slen);
        }
        break;
      }
      out.put(sv->str, slen);
      break;
    }
    case 'f': case 'e': case 'E': case 'g': case 'G':
      subfmt[sl++] = conv;
      subfmt[sl] = 0;
      {
        ValueT* v = fmt_nextarg(vm, args, METHOD);
        fmt_emit(vm, out, METHOD, subfmt, fmt_numval(vm, v, METHOD));
      }
      break;
    default:
      Error(vm, "%s: invalid conversion '%c'", METHOD, conv);
    }
  }
  if (!isnull(args))
    Error(vm, "%s: too many arguments", METHOD);
  ValueT res;
  setstr(&res, G(vm)->strintern(out.buf, out.count));
  return res;  /* ~Lbuffer() frees the buffer */
}

/* ---- string.pack family (Lua-compatible syntax, strobj-backed) ----
   (string-pack fmt . args) -> binary string
   (string-unpack fmt s [pos]) -> values ... nextpos  (pos 0-based, may be negative)
   (string-packsize fmt) -> fixed size (errors on variable-length options)
   options: < > = endian | b B h H l L j J T i[n] I[n] ints | f d n floats
            s[n] len-prefixed str (big-endian prefix) | z NUL-terminated
            c[n] fixed-length str | x pad byte | Xop alignment
   (string-char . ints) -> string of bytes | (string-byte s [i [j]]) -> bytes
   (string-byte indices are 0-based, may be negative) */

static void packint(VM* vm, Lbuffer* out, scm_int val, int n, bool sgn, int endian, const char* METHOD)
{
  scm_uint u;
  if (sgn)
  {
    if (n == 8)
      u = (scm_uint)val;
    else
    {
      scm_int lo = -(((scm_int)1) << (8 * n - 1));
      scm_int hi = (((scm_int)1) << (8 * n - 1)) - 1;
      AssertArg(vm, val >= lo && val <= hi, METHOD, NULL, " integer overflow");
      u = (scm_uint)val & (((((scm_uint)1) << (8 * n)) - 1));
    }
  }
  else
  {
    AssertArg(vm, val >= 0, METHOD, NULL, " negative value for unsigned option");
    if (n == 8)
      u = (scm_uint)val;
    else
    {
      AssertArg(vm, (scm_uint)val < (((scm_uint)1) << (8 * n)), METHOD, NULL, " integer overflow");
      u = (scm_uint)val;
    }
  }
  if (endian)
    for (int k = n - 1; k >= 0; k--)
      out->put((char)(u >> (8 * k)));
  else
    for (int k = 0; k < n; k++)
      out->put((char)(u >> (8 * k)));
}

static scm_uint loaduint(const char* s, int n, int endian)
{
  scm_uint v = 0;
  if (endian)
    for (int k = 0; k < n; k++)
      v = (v << 8) | (unsigned char)s[k];
  else
    for (int k = n - 1; k >= 0; k--)
      v = (v << 8) | (unsigned char)s[k];
  return v;
}

static scm_int signext(scm_uint u, int n)
{
  if (n == 8)
    return (scm_int)u;
  scm_uint sign = ((scm_uint)1) << (8 * n - 1);
  if (u & sign)
    return (scm_int)(u | ~((((scm_uint)1) << (8 * n)) - 1));
  return (scm_int)u;
}

static void packdouble(Lbuffer* out, double f, int n, int endian)
{
  unsigned char tmp[8] = {0};
  if (n == 4)
  {
    float g = (float)f;  /* 'f' option: 4-byte float (Lua packs float) */
    memcpy(tmp, &g, 4);
  }
  else
    memcpy(tmp, &f, n);
  if (endian)
    for (int k = 0; k < n / 2; k++)
    {
      char t = tmp[k];
      tmp[k] = tmp[n - 1 - k];
      tmp[n - 1 - k] = t;
    }
  out->put((const char*)tmp, n);
}

static double loaddouble(const char* s, int n, int endian)
{
  unsigned char tmp[8] = {0};
  memcpy(tmp, s, n);
  if (endian)
    for (int k = 0; k < n / 2; k++)
    {
      char t = tmp[k];
      tmp[k] = tmp[n - 1 - k];
      tmp[n - 1 - k] = t;
    }
  if (n == 4)
  {
    float f;
    memcpy(&f, tmp, 4);
    return f;
  }
  double d;
  memcpy(&d, tmp, 8);
  return d;
}

static ValueT* pack_nextarg(VM* vm, ValueT** args, const char* METHOD)
{
  if (isnull(*args))
    Error(vm, "%s: too few arguments", METHOD);
  ValueT* v = Scar(*args);
  *args = Scdr(*args);
  return v;
}

static int parse_intwidth(const char* s, int* ip, int len, int deflt)
{
  int n = 0;
  while (*ip < len && isdigit((unsigned char)s[*ip]))
  {
    n = n * 10 + (s[*ip] - '0');
    (*ip)++;
  }
  return n == 0 ? deflt : n;
}

static ValueT scm_stub_string_pack(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "string-pack";
  AssertArg(vm, isstr(p), METHOD, p, " not a string");
  StrObj* fmt = strref(p);
  Lbuffer out(vm);
  int endian = 0;  /* native = little on arm64 */
  int i = 0;
  while (i < fmt->len)
  {
    char c = fmt->str[i];
    i++;
    switch (c)
    {
    case '<': endian = 0; break;
    case '>': endian = 1; break;
    case '=': endian = 0; break;
    case 'x': out.put((char)0); break;
    case 'X':
    {
      int op = parse_intwidth(fmt->str, &i, fmt->len, 0);
      if (op > 1)
      {
        int rem = out.count % op;
        if (rem)
          for (int k = 0; k < op - rem; k++)
            out.put((char)0);
      }
      break;
    }
    case 'b': case 'B': case 'h': case 'H': case 'l': case 'L': case 'j': case 'J': case 'T':
    {
      int n = (c == 'b' || c == 'B') ? 1 : ((c == 'h' || c == 'H') ? 2 : ((c == 'l' || c == 'L') ? 4 : 8));
      bool sgn = (c == 'b' || c == 'h' || c == 'l' || c == 'j');
      ValueT* v = pack_nextarg(vm, &args, METHOD);
      AssertArg(vm, isnumi(v), METHOD, v, " not an integer");
      packint(vm, &out, numi(v), n, sgn, endian, METHOD);
      break;
    }
    case 'i': case 'I':
    {
      int n = parse_intwidth(fmt->str, &i, fmt->len, 8);
      bool sgn = (c == 'i');
      ValueT* v = pack_nextarg(vm, &args, METHOD);
      AssertArg(vm, isnumi(v), METHOD, v, " not an integer");
      packint(vm, &out, numi(v), n, sgn, endian, METHOD);
      break;
    }
    case 'f': case 'd': case 'n':
    {
      int n = (c == 'f') ? 4 : 8;
      ValueT* v = pack_nextarg(vm, &args, METHOD);
      double f;
      if (isnumreal(v))
        f = numreal(v);
      else if (isnumi(v))
        f = (double)numi(v);
      else
        AssertArg(vm, false, METHOD, v, " not a number");
      packdouble(&out, f, n, endian);
      break;
    }
    case 's':
    {
      int n = parse_intwidth(fmt->str, &i, fmt->len, 8);
      ValueT* v = pack_nextarg(vm, &args, METHOD);
      AssertArg(vm, isstr(v), METHOD, v, " not a string");
      StrObj* ss = strref(v);
      scm_uint len = (scm_uint)Sslen(ss);
      for (int k = n - 1; k >= 0; k--)
        out.put((char)(len >> (8 * k)));
      out.put(Ssstr(ss), Sslen(ss));
      break;
    }
    case 'z':
    {
      ValueT* v = pack_nextarg(vm, &args, METHOD);
      AssertArg(vm, isstr(v), METHOD, v, " not a string");
      StrObj* ss = strref(v);
      out.put(Ssstr(ss), Sslen(ss));
      out.put((char)0);
      break;
    }
    case 'c':
    {
      int n = parse_intwidth(fmt->str, &i, fmt->len, 0);
      if (n == 0)
        Error(vm, "%s: c option needs a size", METHOD);
      ValueT* v = pack_nextarg(vm, &args, METHOD);
      AssertArg(vm, isstr(v), METHOD, v, " not a string");
      StrObj* ss = strref(v);
      AssertArg(vm, Sslen(ss) >= n, METHOD, v, " string too short for c option");
      out.put(Ssstr(ss), n);
      break;
    }
    default:
      Error(vm, "%s: invalid format option '%c'", METHOD, c);
    }
  }
  if (!isnull(args))
    Error(vm, "%s: too many arguments", METHOD);
  ValueT res;
  setstr(&res, G(vm)->strintern(out.buf, out.count));
  return res;  /* ~Lbuffer() frees the buffer */
}

static ValueT scm_stub_string_unpack(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "string-unpack";
  AssertArg(vm, isstr(p), METHOD, p, " not a string");
  StrObj* fmt = strref(p);
  AssertArg(vm, !isnull(args) && isstr(Scar(args)), METHOD, args, " not a string");
  StrObj* s = strref(Scar(args));
  int pos = 0;
  ValueT* rest = Scdr(args);
  if (!isnull(rest))
  {
    AssertArg(vm, isnumi(Scar(rest)), METHOD, Scar(rest), " not an integer");
    pos = numi(Scar(rest));
  }
  if (pos < 0)
    pos = s->len + pos;  /* negative: from the tail */
  AssertArg(vm, pos >= 0 && pos <= s->len, METHOD, NULL, " position out of bounds");

  ArrayObj* vals = Sr0(vm, ArrayObj);
  int endian = 0;
  int fi = 0;        /* format index */
  int off = pos;     /* data offset, 0-based */
  while (fi < fmt->len)
  {
    char c = fmt->str[fi];
    fi++;
    switch (c)
    {
    case '<': endian = 0; break;
    case '>': endian = 1; break;
    case '=': endian = 0; break;
    case 'x': off++; break;
    case 'X':
    {
      int op = parse_intwidth(fmt->str, &fi, fmt->len, 0);
      if (op > 1)
      {
        int rem = off % op;
        if (rem)
          off += op - rem;
      }
      break;
    }
    case 'b': case 'B': case 'h': case 'H': case 'l': case 'L': case 'j': case 'J': case 'T':
    {
      int n = (c == 'b' || c == 'B') ? 1 : ((c == 'h' || c == 'H') ? 2 : ((c == 'l' || c == 'L') ? 4 : 8));
      bool sgn = (c == 'b' || c == 'h' || c == 'l' || c == 'j');
      AssertArg(vm, off + n <= s->len, METHOD, NULL, " data string too short");
      scm_uint u = loaduint(s->str + off, n, endian);
      off += n;
      ValueT out;
      if (sgn)
        setnumi(&out, signext(u, n));
      else
        setnumi(&out, (scm_int)u);
      vals->add(vm, &out);
      break;
    }
    case 'i': case 'I':
    {
      int n = parse_intwidth(fmt->str, &fi, fmt->len, 8);
      bool sgn = (c == 'i');
      AssertArg(vm, off + n <= s->len, METHOD, NULL, " data string too short");
      scm_uint u = loaduint(s->str + off, n, endian);
      off += n;
      ValueT out;
      if (sgn)
        setnumi(&out, signext(u, n));
      else
        setnumi(&out, (scm_int)u);
      vals->add(vm, &out);
      break;
    }
    case 'f': case 'd': case 'n':
    {
      int n = (c == 'f') ? 4 : 8;
      AssertArg(vm, off + n <= s->len, METHOD, NULL, " data string too short");
      double f = loaddouble(s->str + off, n, endian);
      off += n;
      ValueT out;
      setnumreal(&out, f);
      vals->add(vm, &out);
      break;
    }
    case 's':
    {
      int n = parse_intwidth(fmt->str, &fi, fmt->len, 8);
      AssertArg(vm, off + n <= s->len, METHOD, NULL, " data string too short");
      scm_uint len = loaduint(s->str + off, n, 1);  /* prefix always big-endian */
      off += n;
      AssertArg(vm, off + (int)len <= s->len, METHOD, NULL, " data string too short");
      ValueT out;
      setstr(&out, G(vm)->strintern(s->str + off, (int)len));
      off += (int)len;
      vals->add(vm, &out);
      break;
    }
    case 'z':
    {
      int j = off;
      while (j < s->len && s->str[j] != 0)
        j++;
      AssertArg(vm, j < s->len, METHOD, NULL, " missing NUL terminator");
      ValueT out;
      setstr(&out, G(vm)->strintern(s->str + off, j - off));
      off = j + 1;
      vals->add(vm, &out);
      break;
    }
    case 'c':
    {
      int n = parse_intwidth(fmt->str, &fi, fmt->len, 0);
      if (n == 0)
      {
        n = s->len - off;
        AssertArg(vm, n >= 0, METHOD, NULL, " data string too short");
      }
      else
        AssertArg(vm, off + n <= s->len, METHOD, NULL, " data string too short");
      ValueT out;
      setstr(&out, G(vm)->strintern(s->str + off, n));
      off += n;
      vals->add(vm, &out);
      break;
    }
    default:
      Error(vm, "%s: invalid format option '%c'", METHOD, c);
    }
  }
  ValueT out;
  setnumi(&out, off);  /* next position, 0-based */
  vals->add(vm, &out);

  ArrayObj* cells = Sr0(vm, ArrayObj);
  ValueT* lst = Snullref;
  for (int k = vals->arrayn() - 1; k >= 0; k--)
  {
    ValueT cellv;
    setpair(&cellv, SCM::cons(vm, vals->get(k), lst));
    cells->add(vm, &cellv);
    lst = cells->get(cells->arrayn() - 1);
  }
  return SCM::packvalues(vm, lst);
}

static ValueT scm_stub_string_packsize(VM* vm, ValueT* p)
{
  static const char* METHOD = "string-packsize";
  AssertArg(vm, isstr(p), METHOD, p, " not a string");
  StrObj* fmt = strref(p);
  int size = 0;
  int i = 0;
  while (i < fmt->len)
  {
    char c = fmt->str[i];
    i++;
    switch (c)
    {
    case '<': case '>': case '=': break;
    case 'x': size += 1; break;
    case 'X':
    {
      int op = parse_intwidth(fmt->str, &i, fmt->len, 0);
      if (op > 1)
      {
        int rem = size % op;
        if (rem)
          size += op - rem;
      }
      break;
    }
    case 'b': case 'B': size += 1; break;
    case 'h': case 'H': size += 2; break;
    case 'l': case 'L': size += 4; break;
    case 'j': case 'J': case 'T': size += 8; break;
    case 'i': case 'I': size += parse_intwidth(fmt->str, &i, fmt->len, 8); break;
    case 'f': size += 4; break;
    case 'd': case 'n': size += 8; break;
    case 's': case 'z': case 'c':
      Error(vm, "%s: variable-length format", METHOD);
      break;
    default:
      Error(vm, "%s: invalid format option '%c'", METHOD, c);
    }
  }
  ValueT out;
  setnumi(&out, size);
  return out;
}

/* Lua string.char / string.byte (bytes as integers, 0-based indexing) */

static ValueT scm_stub_string_char(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "string-char";
  Lbuffer out(vm);
  ValueT* cur = p;
  for (;;)
  {
    AssertArg(vm, isnumi(cur), METHOD, cur, " not an integer");
    int b = numi(cur);
    AssertArg(vm, b >= 0 && b <= 255, METHOD, cur, " byte out of range");
    out.put((char)b);
    if (isnull(args))
      break;
    cur = Scar(args);
    args = Scdr(args);
  }
  ValueT res;
  setstr(&res, G(vm)->strintern(out.buf, out.count));
  return res;
}

static ValueT scm_stub_string_byte(VM* vm, ValueT* p, ValueT* args)
{
  static const char* METHOD = "string-byte";
  AssertArg(vm, isstr(p), METHOD, p, " not a string");
  StrObj* s = strref(p);
  int i, j;
  if (isnull(args))
    i = j = 0;
  else
  {
    ValueT* ivt = Scar(args);
    AssertArg(vm, isnumi(ivt), METHOD, ivt, " not an integer");
    args = Scdr(args);
    i = numi(ivt);
    if (isnull(args))
      j = i;
    else
    {
      ValueT* jvt = Scar(args);
      AssertArg(vm, isnumi(jvt), METHOD, jvt, " not an integer");
      j = numi(jvt);
      Assert(vm, isnull(Scdr(args)), METHOD, " too many arguments");
    }
  }
  if (i < 0)
    i = s->len + i;
  if (j < 0)
    j = s->len + j;
  Assert(vm, 0 <= i && i < s->len && i <= j, "%s: illegal index", METHOD);
  if (j >= s->len)
    j = s->len - 1;   /* clamp to last byte */
  Sgcvar1(vm, lst);
  for (int k = j; k >= i; k--)
  {
    ValueT bv;
    setnumi(&bv, (unsigned char)s->str[k]);
    setpair(lst, SCM::cons(vm, &bv, lst));
  }
  return SCM::packvalues(vm, lst);
}

void SCMStr::init(VM* vm)
{
  const RegCProc strlib[] = {
    RegCProc("format", scm_stub_format, true),
    RegCProc("string-format", scm_stub_string_format, true),
    RegCProc("string-pack", scm_stub_string_pack, true),
    RegCProc("string-unpack", scm_stub_string_unpack, true),
    RegCProc("string-packsize", scm_stub_string_packsize),
    RegCProc("string-char", scm_stub_string_char, true),
    RegCProc("string-byte", scm_stub_string_byte, true),
    RegCProc("make-string", scm_stub_make_string, true),
    RegCProc("string", scm_stub_string, true),
    RegCProc("substring", scm_stub_substring),
    RegCProc("string-append", scm_stub_string_append, true),
    RegCProc("string-copy", scm_stub_string_copy),
    RegCProc("string-fill!", scm_stub_string_fill),
    RegCProc("string-length", scm_stub_string_len),
    RegCProc("string-ref", scm_stub_string_ref),
    RegCProc("string-set!", scm_stub_string_set),
    RegCProc("string->list", scm_stub_string_2list),
    RegCProc("list->string", scm_stub_list_2string),

    RegCProc("string=?", scm_stub_string_eq, true),
    RegCProc("string<?", scm_stub_string_less, true),
    RegCProc("string>?", scm_stub_string_bigger, true),
    RegCProc("string<=?", scm_stub_string_lesseq, true),
    RegCProc("string>=?", scm_stub_string_biggereq, true),

    RegCProc("string-ci=?", scm_stub_string_ci_eq, true),
    RegCProc("string-ci<?", scm_stub_string_ci_less, true),
    RegCProc("string-ci>?", scm_stub_string_ci_bigger, true),
    RegCProc("string-ci<=?", scm_stub_string_ci_lesseq, true),
    RegCProc("string-ci>=?", scm_stub_string_ci_biggereq, true),

    RegCProc("char-upcase", scm_stub_char_upcase),
    RegCProc("char-downcase", scm_stub_char_downcase),

    RegCProc("char->integer", scm_stub_char_toint),
    RegCProc("integer->char", scm_stub_int_tochar),

    RegCProc("char=?", scm_stub_char_eq, true),
    RegCProc("char<?", scm_stub_char_less, true),
    RegCProc("char>?", scm_stub_char_bigger, true),
    RegCProc("char<=?", scm_stub_char_lesseq, true),
    RegCProc("char>=?", scm_stub_char_biggereq, true),

    RegCProc("char-ci=?", scm_stub_char_ci_eq, true),
    RegCProc("char-ci<?", scm_stub_char_ci_less, true),
    RegCProc("char-ci>?", scm_stub_char_ci_bigger, true),
    RegCProc("char-ci<=?", scm_stub_char_ci_lesseq, true),
    RegCProc("char-ci>=?", scm_stub_char_ci_biggereq, true),

    RegCProc("char-alphabetic?", scm_stub_char_alphabeticq),
    RegCProc("char-numeric?", scm_stub_char_numericq),
    RegCProc("char-whitespace?", scm_stub_char_whitespaceq),
    RegCProc("char-upper-case?", scm_stub_char_upperq),
    RegCProc("char-lower-case?", scm_stub_char_lowerq),

    RegCProc(NULL, -1)
  };
  regcfunc(vm, strlib);
}

};
