#include "vm.h"
#include "scmcode.h"
#include "scmcompiler.h"
#include "scmmath.h"

using namespace Scheme;

#define compileassert(vm, cond, expr, fmt, ...) do {      \
 if (!(cond)) compileerror(vm, expr, fmt, ##__VA_ARGS__); \
 } while(0)

#define compileerror(vm, expr, fmt, ...) do {                    \
 Print(fmt, ##__VA_ARGS__);                                      \
 Print(" %s:%d: ", vm->gettopsource(), annotateref(expr)->line); \
 Print("\nexpr:");                                               \
 vm->printvalue0(stderr, expr, true);                            \
 DebugAssertStop(*((int*)0) = 0);                                \
 Print("\n");                                                    \
 throw "compile error";                                          \
 } while(0)

#define Serrorannotate(vm, vt, fmt, ...) do {   \
 Serror(fmt, ##__VA_ARGS__);                    \
 vm->printvalue0(stderr, vt, true);             \
 } while(0)

static void checkhygienesym(ValueT* hsym, ValueT* out)
{
  if (ishygienesym(hsym))
    setsym(out, hygienesymref(hsym)->sym);
}

static void checkhygienesymre(ValueT* hsym, ValueT* out)
{
  if (ishygienesym(hsym))
    setsym(out, hygienesymref(hsym)->hysym);
}

static const char* whatqq1 = "compileqquote: internal error";
static const char* whatdefine = "define: bad syntax";
static const char* whatlambda = "lambda: bad syntax";
static const char* whatsyntaxr = "syntax-rules: bad syntax";

static ValueT* quietsplitannotate(ValueT* expr, int n, ...)
{
  ValueT* pair = expr;
  va_list ap;
  va_start(ap, n);
  for (pair = expr; n-- > 0; pair = Scdr(pair))
  {
    if (isnull(pair))
    {
      pair = NULL;
      break;
    }
    pair = annotatevt(pair);
    if (!ispair(pair))
    {
      pair = NULL;
      break;
    }
    ValueT* vt = va_arg(ap, ValueT*);
    *vt = Scar(pair);
  }
  va_end(ap);
  return pair;
}

static bool isformannotate(ValueT* vt, ValueT* formkey)
{
  ValueT dummy1, dummy2;
  ValueT* vt0 = quietsplitannotate(vt, 2, &dummy1, &dummy2);
  if (vt0 == NULL)
    return false;
  if (!isnull(vt0))
    return false;
  return iskeyword(annotatevt(&dummy1), formkey);
}

static int annotatelistlen(VM* vm, const char* what, ValueT* expr)
{
  if (isnull(expr))
    return 0;
  ValueT* expr0 = annotatevt(expr);
  compileassert(vm, ispair(expr0), expr, "%s", what);
  return 1+annotatelistlen(vm, what, Scdr(expr0));
}

static bool annotatelistp(ValueT* expr)
{
  if (isnull(expr))
    return true;
  else
  {
    ValueT* expr0 = annotatevt(expr);
    return ispair(expr0) && annotatelistp(Scdr(expr0));
  }
}

static ValueT* splitannotatelist(VM* vm, ValueT* expr, const char* what, int n, ...)
{
  ValueT* pair = expr;
  va_list ap;
  va_start(ap, n);
  for (pair = expr; n-- > 0; pair = Scdr(pair))
  {
    compileassert(vm, !isnull(pair), expr, "%s", what);
    pair = annotatevt(pair);
    compileassert(vm, ispair(pair), expr, "%s", what);
    ValueT* vt = va_arg(ap, ValueT*);
    *vt = Scar(pair);
  }
  va_end(ap);
  return pair;
}

static void vectorannotate2cons(VM* vm, ValueT* out, ArrayObj* arr)
{
  *out = Snullref;
  int count = arr->array.n;
  if (count > 0)
  {
    *out = arr->get(count-1);
    for (int i = count-2; i>=0; i--)
    {
      ValueT* item = arr->get(i);
      setpair(out, SCM::cons(vm, item, out));
      SCM::toAnnotation(vm, out, annotateline(item));
    }
  }
}

static void stripannotate(ValueT* annos)
{
  if (isnull(annos))
    return;
  ValueT* annovt = annotatevt(annos);
  *annos = annovt;
  if (ispair(annovt))
  {
    stripannotate(Scar(annovt));
    stripannotate(Scdr(annovt));
  }
  else if (isarray(annovt))
  {
    ArrayObj* arr = arrayref(annovt);
    VecT<ValueT>* array = &arr->array;
    VEC_FOR(i, array)
      stripannotate(array->getptr(i));
  }
}

static void stripannotateliterals(VM* vm, ValueT* literals)
{
  if (isnull(literals))
    return;
  static const char* what = "syntax-rules: bad syntax, not a sym in literals";
  ValueT* literals0 = annotatevt(literals);
  if (ispair(literals0))
  {
    ValueT* symvt = Scar(literals0);
    ValueT* symvt0 = annotatevt(symvt);
    compileassert(vm, issym(symvt0), symvt, "%s", what);
    *symvt = symvt0;
    stripannotateliterals(vm, Scdr(literals0));
  }
  else
    compileassert(vm, isnull(literals0), literals, "%s", what);
  *literals = literals0;
}

static void addlambdaparam(VM* vm, LambdaPtr lambda, ValueT* expr)
{
  ValueT* psym = annotatevt(expr);
  checkhygienesymre(psym, psym);
  compileassert(vm, issym(psym), expr, "%s, param not a symbol", whatlambda);
  LambdaVarsObj* vars = lambda->vars;
  SymPtr psymp = symref(psym);
  compileassert(vm, vars->looklocal(psymp) < 0, expr, "%s, multiple %s", whatlambda, Ssstr(psymp));
  lambda->argnum++;
  vars->addlocal(vm, psymp);
}

static void addlambdaparamseq(VM* vm, LambdaPtr lambda, ValueT *expr)
{
  if (isnull(expr))
    return;
  else if (ispair(annotatevt(expr)))
  {
    ValueT vt;
    ValueT* expr0 = splitannotatelist(vm, expr, whatlambda, 1, &vt);
    addlambdaparam(vm, lambda, &vt);
    addlambdaparamseq(vm, lambda, expr0);
  }
  else
  {
    addlambdaparam(vm, lambda, expr);
    lambda->argrest = true;
  }
}

static void initlambdaparam(VM* vm, LambdaPtr lambda, ValueT* expr)
{
  if (isnull(expr) || isnull(annotatevt(expr)))
    return;
  addlambdaparamseq(vm, lambda, expr);
}

static bool isannowithbegin(VM* vm, ValueT* expr)
{
  ValueT* expr0 = annotatevt(expr);
  if (ispair(expr0))
  {
    ValueT* type = Scar(expr0);
    ValueT* type0 = annotatevt(type);
    if (iskwbegin(vm, type0))
      return true;
  }
  return false;
}

void SCompiler::compilesym(int target, ValueT* expr, ValueT* link)
{
  int line = annotateline(expr);
  ValueT* sym = annotatevt(expr);
  compilelink(link, line);
  if (lambda->vars)
  {
    SymPtr symp = getsym(sym);
    int local = -1, ovar = -1;
    compilerefsym(&local, &ovar, symp);
    if (local >= 0)
    {
      setsym(sym, symp);
      putcode(code_varreflocal(target, local), line);
    }
    else if (ovar >= 0)
    {
      setsym(sym, symp);
      putcode(code_varrefovar(target, ovar), line);
    }
    else
    {
      checkhygienesym(sym, sym);
      int k = lambda->addk(vm, sym);
      putcode(code_varrefglobal(target, k), line);
    }
  }
  else
  {
    compileassert(vm, issym(sym), expr, "internal error, not a normal symbol");
    int k = lambda->addk(vm, sym);
    putcode(code_varrefglobal(target, k), line);
  }
}

void SCompiler::compiledefpre(ValueT* expr)
{
  ValueT _dummy_, vt1;
  splitannotatelist(vm, expr, whatdefine, 2, &_dummy_, &vt1);
  ValueT* defcar = annotatevt(&vt1);
  checkhygienesymre(defcar, defcar);
  SymPtr sym = NULL;
  if (issym(defcar))
    sym = symref(defcar);
  else
  {
    compileassert(vm, ispair(defcar), &vt1, "%s", whatdefine);
    ValueT* vt2 = Scar(defcar);
    defcar = annotatevt(vt2);
    checkhygienesymre(defcar, defcar);
    compileassert(vm, issym(defcar), &vt1, "%s", whatdefine);
    sym = symref(defcar);
  }
  compileassert(vm, !finddef(sym), &vt1, "define: multiple binding %s", Ssstr(sym));
  int offset = lambda->vars->looklocal(sym);
  if (offset < 0)
    lambda->vars->addlocal(vm, sym);
  Debug(Print("adddef %s to compiler %p\n", Ssstr(sym), this));
  adddef(sym);
}

int SCompiler::compiledefsym0(int target, ValueT* symvt)
{
  ValueT* symvt0 = annotatevt(symvt);
  LambdaVarsObj* vars = lambda->vars;
  if (vars)
  {
    SymPtr sym = symref(symvt0);
    int offset = vars->looklocal(sym);
    compileassert(vm, offset>=0, symvt, "internal error, no binding for %s", Ssstr(sym));
    target = offset+1;
  }
  else
  {
    int line = annotateline(symvt);
    int k = lambda->addk(vm, symvt0);
    putcode(code_defglobal(k, target), line);
  }
  return target;
}

void SCompiler::compiledefsym(int target, ValueT* symvt, ValueT* expr, ValueT* link)
{
  target = compiledefsym0(target, symvt);
  ValueT vt1;
  ValueT* expr0 = splitannotatelist(vm, expr, whatdefine, 1, &vt1);
  compileassert(vm, isnull(expr0), expr0, "%s", whatdefine);
  compile(target, &vt1, Snext, false);
}

void SCompiler::compilelambda0(int target, int line, ValueT* param, ValueT* body)
{
  Sgcvar2(vm, newlambdavt, name2hy);
  LambdaPtr newlambda = NULL;
  setref(newlambdavt, newlambda = Sr0(vm, LambdaObj));
  int k = lambda->addl(vm, newlambda);
  putcode(code_lambda(target, k), line);
  newlambda->vars = Sr0(vm, LambdaVarsObj);
  newlambda->source = lambda->source;
  newlambda->defline = line;
  SCompiler compiler(vm, newlambda, this, name2hy);
  compiler.prevline = newlambda->defline;
  initlambdaparam(vm, newlambda, param);
  compiler.compileseqpre(body);
  compiler.compileseq(newlambda->vars->local.n+1, body, Sreturn, 1);

#ifdef DebugCCode
  Print("<--Compiled Code-->\n");
  vm->printccode(newlambda);
  Print("<--Compiled End-->\n");
#endif
}

void SCompiler::compiledeflambda(int target, ValueT* vt, ValueT* body, ValueT* link)
{
  ValueT* vt0 = annotatevt(vt);
  ValueT* vt1 = Scar(vt0);
  ValueT* param = Scdr(vt0);
  ValueT* defcar = annotatevt(vt1);
  checkhygienesymre(defcar, defcar);
  compileassert(vm, issym(defcar), vt, "%s", whatdefine);
  target = compiledefsym0(target, vt1);
  int line = isnull(param) ? annotateline(vt) : annotateline(param);
  compilelambda0(target, line, param, body);
}

void SCompiler::compiledef(int target, ValueT* expr, ValueT* link)
{
  int line = annotateline(expr);
  compilelink(link, line);
  ValueT _dummy_, vt1;
  ValueT* expr0 = splitannotatelist(vm, expr, whatdefine, 2, &_dummy_, &vt1);
  ValueT* defcar = annotatevt(&vt1);
  checkhygienesymre(defcar, defcar);
  if (issym(defcar))
    compiledefsym(target, &vt1, expr0, link);
  else
  {
    compileassert(vm, ispair(defcar), &vt1, "%s", whatdefine);
    compiledeflambda(target, &vt1, expr0, link);
  }
}

void SCompiler::compilerefsym(int* local, int* ovar, SymPtr sym)
{
  LambdaVarsObj* vars = lambda->vars;
  if (vars)
  {
    *local = vars->looklocal(sym);
    if (*local < 0)
    {
      *ovar = vars->lookovar(sym);
      if (*ovar < 0)
      {
        if (enclose)
          enclose->compilerefsym(local, ovar, sym);
        if (*local >= 0)
        {
          *ovar = vars->addovar(vm, sym, *local, true);
          *local = -1;
        }
        else if (*ovar >= 0)
          *ovar = vars->addovar(vm, sym, *ovar, false);
      }
    }
  }
}

void SCompiler::compileset(int target, ValueT* expr, ValueT* link)
{
  int line = annotateline(expr);
  static const char* what = "set!: bad syntax";
  compilelink(link, line);
  ValueT _dummy_, symvt, symval;
  ValueT* expr0 = splitannotatelist(vm, expr, what, 3, &_dummy_, &symvt, &symval);
  compileassert(vm, isnull(expr0), expr0, "%s", what);
  ValueT* sym = annotatevt(&symvt);
  compileassert(vm, ishygienesym(sym) || issym(sym), &symvt, "%s", what);
  if (lambda->vars)
  {
    SymPtr symp = getsym(sym);
    int local = -1, ovar = -1;
    compilerefsym(&local, &ovar, symp);
    if (local >= 0)
    {
      target += 1;
      line = annotateline(&symvt);
      putcode(code_setlocal(local, target), line);
    }
    else if (ovar >= 0)
    {
      target += 1;
      line = annotateline(&symvt);
      putcode(code_setovar(ovar, target), line);
    }
    else
    {
      checkhygienesym(sym, sym);
      target += 1;
      line = annotateline(&symvt);
      int k = lambda->addk(vm, sym);
      putcode(code_setglobal(k, target), line);
    }
  }
  else
  {
    checkhygienesym(sym, sym);
    target += 1;
    int k = lambda->addk(vm, sym);
    line = annotateline(&symvt);
    putcode(code_setglobal(k, target), line);
  }
  compile(target, &symval, Snext, false);
}

void SCompiler::compileseqpre0(ValueT* vt)
{
  static const char* what = "begin: bad syntax, last not an expression";
  ValueT* vt0 = annotatevt(vt);
  if (ispair(vt0))
  {
    ValueT* type = Scar(vt0);
    type = annotatevt(type);
    if (iskwdefine(vm, type))
      compiledefpre(vt);
    else if (iskwdefsyntax(vm, type))
      compiledefsyntax(vt);
    else if (iskwbegin(vm, type))
    {
      if (!isnull(Scdr(vt0)))
        compileseqpre(Scdr(vt0));
    }
  }
}

void SCompiler::compileseqpre(ValueT* expr)
{
  static const char* what = "begin: bad syntax, not a list";
  ValueT* expr0 = annotatevt(expr);
  compileassert(vm, ispair(expr0), expr, "%s", what);
  if (isnull(Scdr(expr0)))
    compileseqpre0(Scar(expr0));
  else
  {
    compileseqpre0(Scar(expr0));
    compileseqpre(Scdr(expr0));
  }
}

void SCompiler::compileseq(int target, ValueT* expr, ValueT* link, bool defok)
{
  static const char* what = "begin: bad syntax, not a proper list";
  ValueT vt;
  ValueT* expr0 = splitannotatelist(vm, expr, what, 1, &vt);
  if (isnull(expr0))
    compile(target, &vt, link, defok);
  else
  {
    compileseq(target, expr0, link, defok);
    compile(target, &vt, Snext, defok);
  }
}

void SCompiler::compileif(int target, ValueT* expr, ValueT* link)
{
  static const char* what = "if: bad syntax";
  AnnotationObj annovoid;
  ValueT _dummy_, test, conseq, alter;
  ValueT* expr0 = splitannotatelist(vm, expr, what, 3, &_dummy_, &test, &conseq);
  if (isnull(expr0))
  {
    setannotate(&alter, &annovoid);
    annovoid.vt = Svoidref;
    annovoid.line = annotateline(&conseq);
  }
  else
  {
    expr0 = splitannotatelist(vm, expr0, what, 1, &alter);
    compileassert(vm, isnull(expr0), expr0, "%s", what);
  }
  ValueT afterifjump;
  setnumi(&afterifjump, lambda->getcodestart());
  compile(target, &alter, link, false);
  int branchfalsejmp = lambda->getcodestart();
  ValueT* conseqlink = link == Snext ? &afterifjump : link;
  compile(target, &conseq, conseqlink, false);
  int line = annotateline(&test);
  putcode(code_iffalsejmp(branchfalsejmp, target), line);
  compile(target, &test, Snext, false);
}

void SCompiler::compilelambda(int target, ValueT* expr, ValueT* link)
{
#ifdef DebugCCode
  Print("<--To Compile Lambda-->\n");
  vm->printvalue0(stderr, expr, true);
  Print("\n");
#endif

  int line = annotateline(expr);
  compilelink(link, line);
  static const char* what = "lambda: bad syntax";
  ValueT _dummy_, param;
  ValueT* expr0 = splitannotatelist(vm, expr, what, 2, &_dummy_,  &param);
  compileassert(vm, !isnull(expr0), &param, "%s", what);
  compilelambda0(target, annotateline(&param), &param, expr0);
}

void SCompiler::compilesyntaxrules(ValueT* expr, ValueT* out)
{
  static const char* what = "syntax-rules: bad syntax";
  ValueT _dummy_, literals, vt;
  ValueT* expr0 = splitannotatelist(vm, expr, what, 2, &_dummy_, &literals);
  SyntaxRules* syntaxr = Sr0(vm, SyntaxRules);
  setsyntaxrules(out, syntaxr);
  ValueT* literals0 = annotatevt(&literals);
  if (ispair(literals0))
  {
    syntaxr->literals = literals;
    stripannotateliterals(vm, &syntaxr->literals);
  }
  else
    compileassert(vm, isnull(literals0), &literals, "%s, not null or list in literals", what);
  compileassert(vm, !isnull(expr0), &literals, "%s, not null or list in literals", what);
  ValueT* expr1 = annotatevt(expr0);
  if (ispair(expr1))
  {
    do {
      expr0 = splitannotatelist(vm, expr0, what, 1, &vt);
      ValueT* vt0 = annotatevt(&vt);
      compileassert(vm, ispair(vt0), &vt, "%s", what);
      PatnTmpl* pt = syntaxr->newsrule(vm);
      pt->init(vm, &syntaxr->literals, &vt);
    } while (!isnull(expr0));
  }
  else
    compileassert(vm, isnull(expr1), expr0, "%s", what);
  syntaxr->shrink(vm);
}

void SCompiler::compiledefsyntax(ValueT* expr)
{
  static const char* what = "define-syntax: bad syntax";
  ValueT _dummy_, vt1, vt2;
  ValueT* expr0 = splitannotatelist(vm, expr, what, 3, &_dummy_, &vt1, &vt2);
  compileassert(vm, isnull(expr0), expr0, "%s", what);
  ValueT* symvt = annotatevt(&vt1);
  compileassert(vm, issym(symvt), &vt1, "%s", what);
  SymPtr name = symref(symvt);
  LambdaVarsObj* vars = lambda->vars;
  if (vars)
  {
    compileassert(vm, vars->looklocal(name) < 0, expr, "%s, multiple bindings %s", what, Ssstr(name));
    compileassert(vm, vars->looksyntax(name, NULL) < 0, expr, "%s, multiple bindings %s", what, Ssstr(name));
  }
  Sgcvar2(vm, syntaxvt, out);
  splitannotatelist(vm, &vt2, what, 1, &vt1);
  ValueT* kwvt = annotatevt(&vt1);
  compileassert(vm, iskwsyntaxrules(vm, kwvt), expr, "%s, only support syntax-rules", what);
  compilesyntaxrules(&vt2, out);
  SyntaxObj* syntaxo = NULL;
  setsyntax(syntaxvt, syntaxo = Sr2(vm, SyntaxObj, name, syntaxrules(out)));
  if (vars)
    vars->addsyntax(vm, syntaxo);
  else
    GEnv(vm)->newkeyorupdate(name, syntaxvt);
}

void SCompiler::compileappargs(int target, ValueT* expr)
{
  if (isnull(expr))
    return;
  ValueT* expr0 = annotatevt(expr);
  compileappargs(target + 1, Scdr(expr0));
  compile(target, Scar(expr0), Snext, false);
}

void SCompiler::compileapp(int target, ValueT* expr, ValueT* link)
{
  int line = annotateline(expr);
  ValueT* expr0 = annotatevt(expr);
  ValueT* type = Scar(expr0);
  ValueT* type0 = annotatevt(type);
  ValueT typeout;
  SyntaxPtr syntax = getsyntax(type0, &typeout);
  if (syntax)
  {
    if (ishygienesym(type0))
      *type0 = typeout;
    SyntaxRules* proc = syntax->proc;
    Sgcvar1(vm, out);
    proc->expand(this, out, expr);
    if (enclose && isannowithbegin(vm, out))
      compileseqpre(out);
    compile(target, out, link, true);
    return;
  }
  ValueT* args = Scdr(expr0);
  int len = annotatelistlen(vm, "appcall: bad syntax", args);
  if (link == Sreturn)
    putcode(code_tailcallapp(target, len), line);
  else
  {
    compilelink(link, line);
    putcode(code_callapp(target, len), line);
  }
  compileappargs(target+1, args);
  compile(target, type, Snext, false);
}

void SCompiler::compileqquote(int target, ValueT* expr, int depth)
{
  int line = annotateline(expr);
  ValueT* expr0 = annotatevt(expr);
  if (ispair(expr0))
  {
    ValueT dummy, vt;
    if (isformannotate(expr, &vm->uquotevt))
    {
      splitannotatelist(vm, expr, whatqq1, 2, &dummy, &vt);
      if (depth == 0)
        compile(target, &vt, Snext, false);
      else
      {
        putcode(code_listk(target, K_UNQUOTE), line);
        compileqquote(target, &vt, depth-1);
      }
    }
    else if (isformannotate(expr, &vm->qquotevt))
    {
      splitannotatelist(vm, expr, whatqq1, 2, &dummy, &vt);
      putcode(code_listk(target, K_QQUOTE), line);
      compileqquote(target, &vt, depth+1);
    }
    else
    {
      ValueT* head = Scar(expr0);
      ValueT* tail = Scdr(expr0);
      if (isformannotate(head, &vm->uquotesvt))
      {
        splitannotatelist(vm, head, whatqq1, 2, &dummy, &vt);
        if (depth == 0)
        {
          line = annotateline(head);
          if (isnull(tail))
            compile(target, &vt, Snext, false);
          else
          {
            putcode(code_append(target), line);
            compileqquote(target+1, tail, 0);
            compile(target, &vt, Snext, false);
          }
        }
        else
        {
          line = annotateline(head);
          putcode(code_cons(target), line);
          compileqquote(target+1, tail, depth);
          putcode(code_listk(target, K_UNQUOTES), line);
          compileqquote(target, &vt, depth-1);
        }
      }
      else
        compileqquotecons(target, head, tail, depth);
    }
  }
  else if (isarray(expr0))
    compileqquotearray(target, expr, depth);
  else
  {
    int k = lambda->addk(vm, expr0);
    putcode(code_assign(target, k), line);
  }
}

void SCompiler::compileqquotearray(int target, ValueT* expr, int depth)
{
  ValueT* item = NULL, dummy, vt;
  ValueT* expr0 = annotatevt(expr);
  int line = annotateline(expr);
  ArrayObj* ao = arrayref(expr0);
  VecT<ValueT>* array = &ao->array;
  putcode(code_list2vec(target), line);
  if (array->n > 0)
  {
    for(int i = 0; i < array->n-1; i++)
    {
      item = array->getptr(i);
      if (isformannotate(item, &vm->uquotesvt))
      {
        splitannotatelist(vm, item, whatqq1, 2, &dummy, &vt);
        if (depth == 0)
        {
          line = annotateline(item);
          putcode(code_append2(target, target+1, target), line);
          compile(target+1, &vt, Snext, false);
        }
        else
        {
          line = annotateline(item);
          putcode(code_cons2(target, target+1, target), line);
          putcode(code_listk(target+1, K_UNQUOTES), line);
          compileqquote(target+1, &vt, depth-1);
        }
      }
      else
      {
        line = annotateline(item);
        putcode(code_cons2(target, target+1, target), line);
        compileqquote(target+1, item, depth);
      }
    }
    item = array->getptr(array->n-1);
    if (isformannotate(item, &vm->uquotesvt))
    {
      splitannotatelist(vm, item, whatqq1, 2, &dummy, &vt);
      if (depth == 0)
        compile(target, &vt, Snext, false);
      else
      {
        putcode(code_listk(target, K_UNQUOTES), line);
        compileqquote(target, &vt, depth-1);
      }
    }
    else
    {
      line = annotateline(item);
      putcode(code_list(target), line);
      compileqquote(target, item, depth);
    }
  }
  else
  {
    int k = lambda->addk(vm, Snullref);
    putcode(code_assign(target, k), line);
  }
}

void SCompiler::compileqquotecons(int target, ValueT* expra, ValueT* exprd, int depth)
{
  int line = annotateline(expra);
  ValueT* expr0 = annotatevt(expra);
  if (isnull(exprd))
  {
    putcode(code_list(target), line);
    compileqquote(target, expra, depth);
  }
  else
  {
    putcode(code_cons(target), line);
    compileqquote(target+1, exprd, depth);
    compileqquote(target, expra, depth);
  }
}

void SCompiler::compilepair(int target, ValueT* expr, ValueT* link, bool defok)
{
  ValueT dummy;
  ValueT* expr0 = splitannotatelist(vm, expr, "internal error", 1, &dummy);
  ValueT* type = annotatevt(&dummy);
  if (iskwdefine(vm, type))
  {
    compileassert(vm, defok, expr, "define: not allowed");
    compiledef(target, expr, link);
  }
  else if (iskwset(vm, type))
    compileset(target, expr, link);
  else if (iskwbegin(vm, type))
  {
    if (!isnull(expr0))
      compileseq(target, expr0, link, defok);
  }
  else if (iskwif(vm, type))
    compileif(target, expr, link);
  else if (iskwlambda(vm, type))
    compilelambda(target, expr, link);
  else if (iskwquote(vm, type))
  {
    static const char* whatq = "quote: bad sytnax";
    expr0 = splitannotatelist(vm, expr0, whatq, 1, &dummy);
    compileassert(vm, isnull(expr0), expr0, "%s", whatq);
    int line = annotateline(&dummy);
    Sgcvar1(vm, dummy0);
    SCM::copystripanno(vm, dummy0, &dummy);
    //ValueT* dummy0 = annotatevt(&dummy);
    compilelink(link, line);
    int k = lambda->addk(vm, dummy0);
    putcode(code_assign(target, k), line);
  }
  else if (iskwqquote(vm, type))
  {
    static const char* whatqq = "quasiquote: bad sytnax";
    expr0 = splitannotatelist(vm, expr0, whatqq, 1, &dummy);
    compileassert(vm, isnull(expr0), expr0, "%s", whatqq);
    compileassert(vm, !isformannotate(&dummy, &vm->uquotesvt), &dummy, "%s, unquote-splicing not in a list", whatqq);
    int line = annotateline(&dummy);
    compilelink(link, line);
    compileqquote(target, &dummy, 0);
  }
  else if (iskwuquote(vm, type) || iskwuquotes(vm, type))
    compileerror(vm, expr, "must be in an quasiquote expression");
  else if (iskwsyntaxrules(vm, type))
  {
    int line = annotateline(expr);
    compilelink(link, line);
    Sgcvar1(vm, out);
    compilesyntaxrules(expr, out);
    int k = lambda->addk(vm, out);
    putcode(code_assign(target, k), line);
  }
  else if (iskwsyntaxerr(vm, type))
    Serrorannotate(vm, expr, "syntax-error: bad syntax");
  else if (iskwdefsyntax(vm, type))
  {
    if (!enclose)
      compiledefsyntax(expr);
    //else;
  }
  else
    compileapp(target, expr, link);
}

void SCompiler::putcode(Instruction i,  int line)
{
  int pc = lambda->pushcode(vm, i);
  lambda->saveline(vm, pc, line, prevline);
  prevline = line;
}

void SCompiler::compilelink(ValueT* link, int line)
{
  if (link == Snext) {} // do nothing
  else if (link == Sreturn)
    putcode(code_ret(), line);
  else if (isnumi(link))
    putcode(code_jmplabel(numi(link)), line);
  else
    Serrorvt(vm, link, "unknown link");
}

void SCompiler::compile(int target, ValueT* expr, ValueT* link, bool defok)
{
  if (target > lambda->top)
    lambda->top = target;
  ValueT* expr0 = annotatevt(expr);
  if (ispair(expr0))
    compilepair(target, expr, link, defok);
  else if (issym(expr0) || ishygienesym(expr0))
    compilesym(target, expr, link);
  else
  {
    int line = annotateline(expr);
    compilelink(link, line);
    Sgcvar1(vm, stripexpr);
    SCM::copystripanno(vm, stripexpr, expr);
    int k = lambda->addk(vm, stripexpr);
    putcode(code_assign(target, k), line);
  }
}

SCompiler::~SCompiler()
{
  lambda->shrink(vm);
}

SymPtr SCompiler::getsym(ValueT* symvt)
{
  if (ishygienesym(symvt))
    return hygienesymref(symvt)->hysym;
  else
    return symref(symvt);
}

void SCompiler::adddef(SymPtr sym)
{
  ValueT symvt;
  setsym(&symvt, sym);
  setpair(&defs, SCM::cons(vm, &symvt, &defs));
}

bool SCompiler::finddef(SymPtr sym)
{
  PAIR_FOR(p, &defs)
    if (symref(Scar(p)) == sym)
      return true;
  return false;
}

void SCompiler::sym2hygiene(ValueT* name, ValueT* out)
{
  PAIR_FOR(p, name2hy)
  {
    ValueT* pcar = Scar(p);
    if (hygienesymref(pcar)->sym == symref(name))
    {
      *out = pcar;
      return;
    }
  }
  Sgcvar1(vm, hyname);
  vm->getuniquesym(symref(name), hyname);
  sethygienesym(out, Sr2(vm, HygieneSymObj, symref(name), symref(hyname)));
  setpair(name2hy, SCM::cons(vm, out, name2hy));
}

SyntaxPtr SCompiler::getsyntax0(SymPtr symtype)
{
  SCompiler* enclosing = enclose;
  SyntaxPtr syntax = NULL;
  if (lambda->vars)
    lambda->vars->looksyntax(symtype, &syntax);
  for (;!syntax && enclosing && enclosing->lambda->vars; enclosing = enclosing->enclose)
    enclosing->lambda->vars->looksyntax(symtype, &syntax);
  if (!syntax)
  {
    ValueT syntaxvt;
    GEnv(vm)->getval(symtype, &syntaxvt);
    if (issyntax(&syntaxvt))
      syntax = syntaxref(&syntaxvt);
  }
  return syntax;
}

SyntaxPtr SCompiler::getsyntax(ValueT* type, ValueT* out)
{
  if (issym(type))
    return getsyntax0(symref(type));
  else if (ishygienesym(type))
  {
    HygieneSymPtr hsym = hygienesymref(type);
    SyntaxPtr syntax = getsyntax0(hsym->hysym);
    if (syntax)
    {
      setsym(out, hsym->hysym);
      return syntax;
    }
    syntax = getsyntax0(hsym->sym);
    if (syntax)
      setsym(out, hsym->sym);
    return syntax;
  }
  return NULL;
}

int MatchObj::matchcount()
{
  return matches.array.n;
}

void MatchObj::reuse()
{
  matches.array.n = 0;
}

void MatchObj::merge(MatchObj* sub, int idx)
{
  if (sub->matches.array.n == 0)
    return;
  ArrayObj* submatches = &sub->matches;
  int subcount = submatches->array.n;
  for (int i = 0; i < subcount; i++)
  {
    PairPtr pair = pairref(submatches->get(i));
    PairPtr sympair = getsymmatch(symref(pair->car()));
    if (sympair)
    {
      ArrayObj* matchpair = arrayref(sympair->cdr());
      for (int i = matchcount(); i < idx; i++)
        matchpair->add(vm, Sundefined);

      matchpair->add(vm, pair->cdr());
    }
    else
    {
      ArrayObj* matchpair = NULL;
      Sgcvar1(vm, arrm);
      setarray(arrm, matchpair = Sr0(vm, ArrayObj));
      for (int i = 0; i < idx; i++)
        matchpair->add(vm, Sundefined);
      matchpair->add(vm, pair->cdr());
      addmatch(pair->car(), arrm);
    }
  }
}

void MatchObj::addmatch(ValueT* sym, ValueT* expr)
{
  Sgcvar1(vm, se);
  setpair(se, SCM::cons(vm, sym, expr));
  matches.add(vm, se);
}

PairPtr MatchObj::getsymmatch(SymPtr sym)
{
  if (matches.array.n == 0)
    return NULL;
  int count = matches.array.n;
  for (int i = 0; i < count; i++)
  {
    PairPtr p = pairref(matches.get(i));
    if (sym == symref(p->car()))
      return p;
  }
  return NULL;
}

void PatnTmpl::init(VM* vm, ValueT* literals, ValueT* expr)
{
  ValueT* expr0 = splitannotatelist(vm, expr, whatsyntaxr, 2, &patn, &tmpl);
  compileassert(vm, isnull(expr0), expr0, "%s", whatsyntaxr);
  initpatn(vm, literals, &patn);
  inittmpl(vm, literals, &tmpl, NULL, 0);
  stripannotate(&patn);
  stripannotate(&tmpl);
}

void PatnTmpl::checktmplsym(VM* vm, ValueT* literals, ValueT* expr, ValueT* usedpvd, int depth)
{
  ValueT* sym = annotatevt(expr);
  SymPtr symtmpl = symref(sym);
  if (isliteral(literals, symtmpl))
    return;
  PatnVarDepth* pvd = ispatnvar(symtmpl);
  if (pvd)
  {
    compileassert(vm, pvd->depth <= depth, expr,
                  "%s, mismatch var pattern %s pvd->depth=%d depth=%d",
                  whatsyntaxr, Ssstr(pvd->sym), pvd->depth, depth);
    if (usedpvd)
      setpair(usedpvd, SCM::cons(vm, sym, usedpvd));
  }
}

void PatnTmpl::inittmpl(VM* vm, ValueT* literals, ValueT* expr, ValueT* usedpvd, int depth)
{
  ValueT* expr0 = annotatevt(expr);
  if (issym(expr0))
    checktmplsym(vm, literals, expr, usedpvd, depth);
  else if(ispair(expr0))
  {
    ValueT* paira = Scar(expr0), * paird = Scdr(expr0);
    ValueT* paira0 = annotatevt(paira);
    compileassert(vm, !iskwellipsis(vm, paira0), paira, "%s, ellipsis is first", whatsyntaxr);
    if (isnull(paird))
      inittmpl(vm, literals, paira, usedpvd, depth);
    else
    {
      ValueT* paird0 = annotatevt(paird);
      compileassert(vm, !iskwellipsis(vm, paird0), paird, "%s, ellipsis is in a improper list", whatsyntaxr);
      // (a ... b)
      if (ispair(paird0) && iskwellipsis(vm, annotatevt(Scar(paird0))))
      {
        Sgcvar1(vm, expectpvd);
        ValueT* usedpvd0 = usedpvd ? usedpvd : expectpvd;
        inittmpl(vm, literals, paira, usedpvd0, depth + 1);
        compileassert(vm, !isnull(usedpvd0), paira, "%s, no pattern variable before ...", whatsyntaxr);
        paird = Scdr(paird0);
        if (!isnull(paird))
          inittmpl(vm, literals, paird, usedpvd, depth);
      }
      else
      {
        inittmpl(vm, literals, paira, usedpvd, depth);
        inittmpl(vm, literals, paird, usedpvd, depth);
      }
    }
  }
}

void PatnTmpl::initpatn(VM* vm, ValueT* literals, ValueT* expr)
{
  ValueT key;
  splitannotatelist(vm, expr, whatsyntaxr, 1, &key);
  ValueT* key0 = annotatevt(&key);
  compileassert(vm, issym(key0), &key, "%s, not a symbol", whatsyntaxr);
  compileassert(vm, !isliteral(literals, symref(key0)), &key, "%s, cannot be in literals", whatsyntaxr);
  initpatn(vm, literals, expr, 0);
}

void PatnTmpl::initpatn(VM* vm, ValueT* literals, ValueT* expr, int depth)
{
  ValueT* expr0 = annotatevt(expr);
  if (issym(expr0))
  {
    SymPtr sym = symref(expr0);
    if (isliteral(literals, sym))
      return;
    compileassert(vm, ispatnvar(sym) == NULL, expr, "%s, multi var %s", whatsyntaxr, Ssstr(sym));
    addvar(vm, sym, depth);
  }
  else if (ispair(expr0))
  {
    ValueT* paira = Scar(expr0), * paird = Scdr(expr0);
    ValueT* paira0 = annotatevt(paira);
    compileassert(vm, !iskwellipsis(vm, paira0), paira, "%s, ellipsis is the car", whatsyntaxr);
    if (isnull(paird))
      initpatn(vm, literals, paira, depth);
    else
    {
      ValueT* paird0 = annotatevt(paird);
      compileassert(vm, !iskwellipsis(vm, paird0), paird, "%s, ellipsis is in a improper list", whatsyntaxr);
      if (ispair(paird0) && iskwellipsis(vm, annotatevt(Scar(paird0))))
      {
        compileassert(vm, isnull(Scdr(paird0)), paird, "%s, don't support items after ellipsis", whatsyntaxr);
        initpatn(vm, literals, paira, depth + 1);
      }
      else
      {
        initpatn(vm, literals, paira, depth);
        initpatn(vm, literals, paird, depth);
      }
    }
  }
}

bool PatnTmpl::trymatchrepeat2(ArrayObj* arr, int idx, ValueT* tomatch, int depth, MatchState* state)
{
  VM* vm = state->lstate->vm;
  int i = 0;
  state->idxarr->suffixset(depth, i);
  MatchObj* matches = state->matches;
  Sgcvar1(vm, matchvt);
  MatchObj submatch(vm, matchvt);
  state->matches = &submatch;
  for (int j = idx; j < arr->array.n; j++)
  {
    submatch.reuse();
    if (!trymatch(arr->array.getptr(j), tomatch, depth + 1, state))
      return false;
    matches->merge(&submatch, i);
    state->idxarr->suffixset(depth, ++i);
  }
  state->matches = matches;
  return true;
}

bool PatnTmpl::trymatchrepeat1(ValueT* expr, ValueT* tomatch, int depth, MatchState* state)
{
  VM* vm = state->lstate->vm;
  ValueT* expr0 = annotatevt(expr);
  if (isnull(expr0))
    return true;
  if (!ispair(expr0))
    return false;
  int i = 0;
  state->idxarr->suffixset(depth, i);
  MatchObj* matches = state->matches;
  Sgcvar1(vm, matchvt);
  MatchObj submatch(vm, matchvt);
  state->matches = &submatch;
  while(true)
  {
    submatch.reuse();
    if (!trymatch(Scar(expr0), tomatch, depth + 1, state))
      return false;
    matches->merge(&submatch, i);
    state->idxarr->suffixset(depth, ++i);
    expr0 = Scdr(expr0);
    if (isnull(expr0))
      break;
    else
    {
      expr0 = annotatevt(expr0);
      if (!ispair(expr0)) return false;
    }
  }
  state->matches = matches;
  return true;
}

bool PatnTmpl::trymatchpair(ValueT* expr, ValueT* ptncar, ValueT* ptncdr, int depth, MatchState* state)
{
  VM* vm = state->lstate->vm;
  if (ispair(ptncdr) && iskwellipsis(vm, Scar(ptncdr)))
  {
    if (!isnull(Scdr(ptncdr)))
      Serrorvt(vm, ptncdr, "internal error, ellipsis not the last");
    if (isnull(expr)) return true;
    return trymatchrepeat1(expr, ptncar, depth, state);
  }
  else
  {
    if (isnull(expr)) return false;
    ValueT* expr0 = annotatevt(expr);
    if (!ispair(expr0)) return false;
    if (!trymatch(Scar(expr0), ptncar, depth, state))
      return false;
    if (!trymatch(Scdr(expr0), ptncdr, depth, state))
      return false;
    return true;
  }
}

bool PatnTmpl::trymatch(ValueT* expr, ValueT* ptn, int depth, MatchState* state)
{
  VM* vm = state->lstate->vm;
  if (issym(ptn))
  {
    if (isnull(expr)) return false;
    SymPtr ptnsym = symref(ptn);
    if (isliteral(state->literals, ptnsym))
    {
      ValueT* expr0 = annotatevt(expr);
      return issym(expr0) && ptnsym == symref(expr0);
    }
    state->matches->addmatch(ptn, expr);
    return true;
  }
  else if (ispair(ptn))
    return trymatchpair(expr, Scar(ptn), Scdr(ptn), depth, state);
  else if (isarray(ptn))
  {
    ValueT* expr0 = annotatevt(expr);
    if (!isarray(expr0)) return false;
    ArrayObj* arr = arrayref(expr0);
    ArrayObj* ptnarr = arrayref(ptn);
    for (int i = 0; i < arr->array.n; i++)
    {
      if (i+1 < ptnarr->array.n)
      {
        ValueT* nextvt = ptnarr->array.getptr(i+1);
        if (iskwellipsis(vm, nextvt))
        {
          if (i+1 != ptnarr->array.n-1)
            Serrorvt(vm, ptn, "internal error, ellipsis not the last");
          return trymatchrepeat2(arr, i, ptnarr->array.getptr(i), depth, state);
        }
      }
      else if (!trymatch(arr->array.getptr(i), ptnarr->array.getptr(i), depth, state))
        return false;
    }
    return true;
  }
  else
  {
    if (isnull(expr)) return isnull(ptn);
    ValueT* expr0 = annotatevt(expr);
    return SCM::equalp(ptn, expr0);
  }
}

void PatnTmpl::expandpair(ValueT* out, ValueT* atpl, ValueT* dtpl, int depth, MatchState* state)
{
  VM* vm = state->lstate->vm;
  if (ispair(dtpl) && iskwellipsis(vm, Scar(dtpl)))
  {
    Sgcvar3(vm, elloutvt, repeat, next);
    ArrayObj* ellout = NULL;
    setarray(elloutvt, ellout = Sr0(vm, ArrayObj));
    int i = 0;
    state->idxarr->suffixset(depth, i);
    for (;;)
    {
      setundefined(repeat);
      expand(repeat, atpl, depth + 1, state);
      if (isundefined(repeat))
        break;
      ellout->add(vm, repeat);
      state->idxarr->suffixset(depth, ++i);
    }
    if (!isnull(Scdr(dtpl)))
      expand(next, Scdr(dtpl), depth, state);
    ellout->add(vm, next);
    vectorannotate2cons(vm, out, ellout);
  }
  else
  {
    Sgcvar2(vm, aout, dout);
    expand(aout, atpl, depth, state);
    if (!isnull(dtpl))
      expand(dout, dtpl, depth, state);
    if (isundefined(aout) || isundefined(dout))
      setundefined(out);
    else
    {
      if (isnull(aout))
        SCM::toAnnotation(vm, aout, annotateline(dout));
      setpair(out, SCM::cons(vm, aout, dout));
      SCM::toAnnotation(vm, out, annotateline(aout));
    }
  }
}

void PatnTmpl::expandarray(ValueT* out, ValueT* tpl, int i, int depth, MatchState* state)
{
  VM* vm = state->lstate->vm;
  ArrayObj* arro = arrayref(tpl), * arro2 = arrayref(out);
  int n = arro->array.n;
  if (i >= n)
    return;
  bool repeat = false;
  if (i + 1 < n)
  {
    ValueT* rep = arro->get(i+1);
    if (iskwellipsis(vm, rep))
      repeat = true;
  }
  if (repeat)
  {
    int j = 0;
    state->idxarr->suffixset(depth, j);
    for (;;)
    {
      int n2 = arro2->add(vm, Snullref);
      ValueT* vt2 = arro2->get(n2);
      setundefined(vt2);
      expand(vt2, arro->get(i), depth + 1, state);
      if (isundefined(vt2))
      {
        arro2->array.n -= 1;
        break;
      }
      state->idxarr->suffixset(depth, ++i);
    }
    expandarray(out, tpl, i+2, depth, state);
  }
  else
  {
    int n2 = arro2->add(vm, Snullref);
    ValueT* vt2 = arro2->get(n2);
    expand(vt2, arro->get(i), depth, state);
    compileassert(vm, !isundefined(vt2), state->expr, "bad syntax");
    expandarray(out, tpl, i+1, depth, state);
  }
}

void PatnTmpl::expand(ValueT* out, ValueT* tpl, int depth, MatchState* state)
{
  VM* vm = state->lstate->vm;
  if (ispair(tpl))
    expandpair(out, Scar(tpl), Scdr(tpl), depth, state);
  else if (isarray(tpl))
  {
    setarray(out, Sr0(vm, ArrayObj));
    expandarray(out, tpl, 0, depth, state);
    arrayref(out)->shrink(vm);
    SCM::toAnnotation(vm, out, annotateline(state->expr));
  }
  else if (issym(tpl))
  {
    SymPtr symtpl = symref(tpl);
    if (isliteral(state->literals, symtpl))
    {
      *out = tpl;
      SCM::toAnnotation(vm, out, annotateline(state->expr));
    }
    else
    {
      PatnVarDepth* pvd = ispatnvar(symtpl);
      if (pvd)
      {
        compileassert(vm, pvd->depth <= depth, state->expr,
                      "bad syntax: depth not equal %s", Ssstr(pvd->sym));
        PairPtr symvalpair = state->matches->getsymmatch(pvd->sym);
        if (symvalpair)
        {
          ValueT* val = symvalpair->cdr();
          if (pvd->depth > 0)
          {
            int indexi = 0;
          loop:
            if (isundefined(val))
              setundefined(out);
            else
            {
              ArrayObj* arr = arrayref(val);
              int arri = state->idxarr->get(indexi++);
              if (arri < arr->array.n)
              {
                val = arr->get(arri);
                if (indexi  == pvd->depth)
                  *out = val;
                else
                  goto loop;
              }
              else
                setundefined(out);
            }
          }
          else
            *out = val;
        }
        else
          compileassert(vm, pvd->depth > 0, state->expr, "bad syntax: empty match %s", Ssstr(pvd->sym));
      }
      else
      {
        state->lstate->sym2hygiene(tpl, out);
        SCM::toAnnotation(vm, out, annotateline(state->expr));
      }
    }
  }
  else
  {
    *out = tpl;
    SCM::toAnnotation(vm, out, annotateline(state->expr));
  }
}

PatnVarDepth* PatnTmpl::ispatnvar(SymPtr sym)
{
  VEC_FOR(i, &allvars)
    if (sym == allvars.get(i).sym)
      return allvars.getptr(i);
  return NULL;
}

void PatnTmpl::addvar(VM* vm, SymPtr sym, int d)
{
  vec_ensure(PatnVarDepth, vm, &allvars, vec_fill2(PatnVarDepth()));
  PatnVarDepth* pd = allvars.getptr(allvars.n++);
  pd->sym = sym;
  pd->depth = d;
}

PatnTmpl* SyntaxRules::newsrule(VM* vm)
{
  vec_ensure(PatnTmpl, vm, &rules, vec_fill2(PatnTmpl()));
  return rules.getptr(rules.n++);
}

void SyntaxRules::finz(VM* vm)
{
  vec_finz(PatnTmpl, vm, &rules);
}

void SyntaxRules::shrink(VM* vm)
{
  vec_shrink(PatnTmpl, vm, &rules);
  VEC_FOR(i, &rules)
    rules.getptr(i)->shrink(vm);
}

void SyntaxRules::addlits(VM* vm, SymPtr lit, ValueT* expr)
{
  PAIR_FOR(p, &literals)
    AssertVT(vm, lit != symref(Scar(p)), expr, "multiple literals %s", Ssstr(lit));
  ValueT symlit;
  setsym(&symlit, lit);
  setpair(&literals, SCM::cons(vm, &symlit, &literals));
}

PatnTmpl* SyntaxRules::expand(SCompiler* lstate, ValueT* out, ValueT* expr)
{
  VM* vm = lstate->vm;
#ifdef DebugSRule
  Print("\nTransforming=> ");
  vm->printvalue0(stderr, expr, true);
#endif

  ValueT* expr0 = annotatevt(expr);
  Sgcvar1(vm, matchvt);
  MatchObj match(vm, matchvt);
  IntArray arr(vm);
  MatchState mstate;
  mstate.expr = expr;
  mstate.lstate = lstate;
  mstate.literals = &literals;
  mstate.idxarr = &arr;
  mstate.matches = &match;
  VEC_FOR(i, &rules)
  {
    PatnTmpl* pt = rules.getptr(i);
    match.reuse();
    match.addmatch(Scar(&pt->patn), Scar(expr0));
    if (pt->trymatch(Scdr(expr0), Scdr(&pt->patn), 0, &mstate))
    {
#ifdef DebugSRule
      Print("\n===Match %d=========", i);
      Print("\n\nPattern=> ");vm->printvalue0(stderr, &pt->patn, true);
      Print("\nTemplate=> ");vm->printvalue0(stderr, &pt->tmpl, true);
      Print("\n\nMatches\n");
      ArrayObj* matches = &match.matches;
      int count = matches->array.n;
      for (int i = 0; i < count; i++)
      {
        ValueT* p = matches->get(i);
        ValueT* pcar = Scar(p), * pcdr Scdr(p);
        PatnVarDepth* pvd = pt->ispatnvar(symref(pcar));
        if (pvd && pvd->depth > 1)
            compileassert(vm, isarray(pcdr), pcdr,
                          "pattern |%s| depth %d matches error",
                          Ssstr(symref(pcar)), pvd->depth);

        Print("%s=>", Ssstr(symref(pcar)));
        vm->printvalue0(stderr, pcdr, true);
        Print("\n");
      }
#endif

      pt->expand(out, &pt->tmpl, 0, &mstate);

#ifdef DebugSRule
      Print("\nAfter=> ");
      vm->printvalue0(stderr, out, true);
      Print("\n=================\n");
#endif

      return pt;
    }
  }
  compileerror(vm, expr, "match failed");
  return NULL;
}
