#include "scheme.h"

namespace Dummy {

VarValue Scheme::Nil;
VarValue Scheme::Null;
VarValue Scheme::True;
VarValue Scheme::False;

VarValue Scheme::Quote;
VarValue Scheme::UnQuote;
VarValue Scheme::UnQuoteSplicing;
VarValue Scheme::QuasiQuote;

VarValue Scheme::SetMark;
VarValue Scheme::Define;
VarValue Scheme::Lambda;
VarValue Scheme::If;
VarValue Scheme::Procedure;
VarValue Scheme::Begin;
VarValue Scheme::Cond;
VarValue Scheme::Else;

VarValue Scheme::globalEnv;

Scheme::SymbolMap Scheme::constSyms;

void Scheme::init()
{
  initIntern();
  initPrimProc();
}

void Scheme::initIntern()
{
  Null = intern("null", NullValue::create());
//  Nil = intern("nil", Null);
  Nil = Null;
  True = intern("true", BoolValue::create(true));
  False = intern("false", BoolValue::create(false));

  Quote = intern("quote");
  UnQuote = intern("unquote");
  UnQuoteSplicing = intern("unquote-splicing");
  QuasiQuote = intern("quasiquote");

  SetMark = intern("set!");
  Define = intern("define");
  Lambda = intern("lambda");
  If = intern("if");
  Procedure = intern("procedure");
  Cond = intern("cond");
  Else = intern("else");
}

void Scheme::initPrimProc()
{
  globalEnv = EnvValue::create(NULL);

  RegPrimProc procs[] = {
    {"+", Splus},
    {"cons", Scons},
    {"length", Slength},
    {"list", Slist},
    {"car", Scar},
    {"cdr", Scdr},
    {"set-car!", set_car},
    {"set-cdr!", set_cdr},
    {"cadr", Scadr},
    {"cddr", Scddr},
    {"caadr", Scaadr},
    {"caddr", Scaddr},
    {"cdadr", Scdadr},
    {"cdddr", Scdddr},
    {"cadddr", Scadddr},
  };
  regPrimProcs(procs, sizeof(procs)/sizeof(RegPrimProc));
}

VarValue Scheme::length(VarValue arg)
{
  int num = 0;
  for (; !Snullp(arg); num++, arg = arg->cdr());

  return NumValue::create(num);
}

VarValue Scheme::plus(VarValue arg)
{
  int num = 0;
  for (; !Snullp(arg); arg = arg->cdr())
  {
    num += arg->car()->getNum();
  }

  return NumValue::create(num);
}

VarValue Scheme::cons(VarValue arg1, VarValue arg2)
{
  return PairValue::create(arg1, arg2);
}

VarValue Scheme::list(VarValue arg)
{
  return arg;
}

VarValue Scheme::set_car(VarValue val, VarValue a)
{
  val->car(a);
  return val;
}

VarValue Scheme::set_cdr(VarValue val, VarValue d)
{
  val->cdr(d);
  return val;
}

VarValue Scheme::car(VarValue val)
{
  return val->car();
}

VarValue Scheme::cdr(VarValue val)
{
  return val->cdr();
}

VarValue Scheme::cadr(VarValue val)
{
  // (car (cdr))
  return val->cdr()->car();
}

VarValue Scheme::cddr(VarValue val)
{
  return val->cdr()->cdr();
}

VarValue Scheme::caadr(VarValue val)
{
  return cadr(val)->car();
}

VarValue Scheme::caddr(VarValue val)
{
  return cddr(val)->car();
}

VarValue Scheme::cdadr(VarValue val)
{
  return cadr(val)->cdr();
}

VarValue Scheme::cdddr(VarValue val)
{
  return cddr(val)->cdr();
}

VarValue Scheme::cadddr(VarValue val)
{
  return cadr(cddr(val));
}

void Scheme::regPrimProcs(RegPrimProc* procs, int num)
{
  for (int i = 0; i < num; i++)
  {
    regPrimProc(procs[i].name, procs[i].proc);
  }
}

void Scheme::regPrimProc(const String& name, PrimProc proc)
{
  globalEnv->setEnvSym(intern(name), PrimProcValue::create(proc));
}

VarValue Scheme::getPrimProc(VarValue sym)
{
  return globalEnv->getEnvSym(sym);
}

VarValue Scheme::intern(const String& symbol, VarValue value)
{
  constSyms[symbol] = value;
  return value;
}

VarValue Scheme::intern(const String& symbol)
{
  SymbolMapItr itr = constSyms.find(symbol);
  if (itr != constSyms.end())
    return itr->second;

  return intern(symbol, SymbolValue::create(symbol));
}

VarValue Scheme::list2(VarValue a, VarValue b)
{
  return cons(a, cons(b, Null));
}

VarValue Scheme::list3(VarValue a, VarValue b, VarValue c)
{
  return cons(a, list2(b, c));
}

VarValue Scheme::list4(VarValue a, VarValue b, VarValue c, VarValue d)
{
  return PairValue::create(a, list3(b, c, d));
}

VarValue Scheme::listn(int num, ...)
{
  VarValue res = Snull;

  va_list ap;
  va_start(ap, num);
  for (int i = 0; i < num; ++i)
  {
    res = Scons(va_arg(ap, VarValue), res);
  }
  va_end(ap);
  return Sreverse(res);
}

VarValue Scheme::nullp(VarValue exp)
{
//  return exp == Null || exp.ptr() == NULL ? True : False;
  return exp == Null ? True : False;
}

VarValue Scheme::notp(VarValue exp)
{
  return falsep(exp) ? True : False;
}

VarValue Scheme::truep(VarValue exp)
{
  return exp != False ? True : False;
}

VarValue Scheme::falsep(VarValue exp)
{
  return exp == False ? True : False;
}

VarValue Scheme::booleanp(VarValue exp)
{
  return exp == True || exp == False ? True : False;
}

VarValue Scheme::numberp(VarValue val)
{
  return dynamic_cast<NumValue*>(val.ptr()) != NULL ? True : False;
}

VarValue Scheme::pairp(VarValue val)
{
  return dynamic_cast<PairValue*>(val.ptr()) != NULL ? True : False;
}

VarValue Scheme::stringp(VarValue val)
{
  return dynamic_cast<StringValue*>(val.ptr()) != NULL ? True : False;
}

VarValue Scheme::symbolp(VarValue val)
{
  return dynamic_cast<SymbolValue*>(val.ptr()) != NULL ? True : False;
}

VarValue Scheme::assignmentp(VarValue val)
{
  return tagged_list_p(val, SetMark);
}

VarValue Scheme::self_evaluating_p(VarValue val)
{
  return numberp(val) || stringp(val) || booleanp(val) || nullp(val) ? True : False;
}

VarValue Scheme::tagged_list_p(VarValue val, VarValue tag)
{
  if (!pairp(val))
    return False;

  return tag == val->car() ? True : False;
}

VarValue Scheme::eqp(VarValue e1, VarValue e2)
{
  return e1 == e2 ? True : False;
}

VarValue Scheme::quotedp(VarValue val)
{
  return tagged_list_p(val, Quote);
}

VarValue Scheme::text_of_quotation(VarValue val)
{
  return cadr(val);
}

VarValue Scheme::analyze(VarValue exp, VarValue env)
{
  if (self_evaluating_p(exp))
  {
    return exp;
  }
  if (quotedp(exp))
  {
    return text_of_quotation(exp);
  }
  // variable?
  if (symbolp(exp))
  {
    return LexSymbolValue::create(exp, env);
  }
  if (assignmentp(exp))
  {
    return AssignmentValue::create(exp, env);
  }
  if (definep(exp))
  {
    return DefineValue::create(exp, env);
  }
  if (ifp(exp))
  {
    return IfValue::create(exp, env);
  }
  if (lambdap(exp))
  {
    return ProcedureValue::create(exp, env);
  }
  if (beginp(exp))
  {
    return analyze_sequence(begin_actions(exp), env);
  }
  if (condp(exp))
  {
    return analyze(cond_to_if(exp), env);
  }
  if (applicationp(exp))
  {
    return ApplicationValue::create(exp, env);
  }

  Error("unknown expression type: ANALYZE %s", ValueCStr(exp));

  return Snull;
}

VarValue Scheme::definep(VarValue val)
{
  return tagged_list_p(val, Define);
}

VarValue Scheme::make_lambda(VarValue parameters, VarValue body)
{
  return cons(Lambda, cons(parameters, body));
}

VarValue Scheme::ifp(VarValue exp)
{
  return tagged_list_p(exp, If);
}

VarValue Scheme::lambdap(VarValue exp)
{
  return tagged_list_p(exp, Lambda);
}

VarValue Scheme::make_procedure(VarValue parameters, VarValue body, VarValue env)
{
  return list4(Procedure, parameters, body, env);
}

VarValue Scheme::compound_procedure_p(VarValue exp)
{
  return tagged_list_p(exp, Procedure);
}

VarValue Scheme::procedure_parameters(VarValue exp)
{
  return cadr(exp);
}

VarValue Scheme::procedure_body(VarValue exp)
{
  return caddr(exp);
}

VarValue Scheme::procedure_env(VarValue exp)
{
  return cadddr(exp);
}

VarValue Scheme::sequence_to_exp(VarValue seq)
{
  if (Snullp(seq))
    return seq;
  else if (last_exp_p(seq))
    return first_exp(seq);
  else
    return make_begin(seq);
}

VarValue Scheme::last_exp_p(VarValue seq)
{
  return Snullp(seq->cdr()) ? True : False;
}

VarValue Scheme::first_exp(VarValue seq)
{
  return seq->car();
}

VarValue Scheme::rest_exp(VarValue seq)
{
  return seq->cdr();
}

VarValue Scheme::make_begin(VarValue seq)
{
  return cons(Begin, seq);
}

VarValue Scheme::beginp(VarValue exp)
{
  return tagged_list_p(exp, Begin);
}

VarValue Scheme::begin_actions(VarValue exp)
{
  return exp->cdr();
}

VarValue Scheme::condp(VarValue exp)
{
  return tagged_list_p(exp, Cond);
}

VarValue Scheme::cond_clauses(VarValue exp)
{
  return exp->cdr();
}

VarValue Scheme::cond_predicate(VarValue clause)
{
  return clause->car();
}

VarValue Scheme::cond_else_clause_p(VarValue clause)
{
  return eqp(clause->car(), Else);
}

VarValue Scheme::cond_actions(VarValue clause)
{
  return clause->cdr();
}

VarValue Scheme::cond_to_if(VarValue exp)
{
  return expand_clauses(cond_clauses(exp));
}

VarValue Scheme::expand_clauses(VarValue clauses)
{
  if (Snullp(clauses))
  {
    return quote(False);
  }
  else
  {
    VarValue first = clauses->car();
    VarValue rest = clauses->cdr();

    if (cond_else_clause_p(first))
    {
      if (Snullp(rest))
      {
        return sequence_to_exp(cond_actions(first));
      }
      else
      {
        Error("ELSE clause isn't last: COND->IF", ValueCStr(clauses));
        return NULL;
      }
    }
    else
    {
      return IfValue::create(cond_predicate(first),
                             sequence_to_exp(cond_actions(first)),
                             expand_clauses(rest));
    }
  }
}

VarValue Scheme::no_operands_p(VarValue ops)
{
  return Snullp(ops);
}

VarValue Scheme::first_operand(VarValue ops)
{
  return Scar(ops);
}

VarValue Scheme::rest_operands(VarValue ops)
{
  return Scdr(ops);
}

VarValue Scheme::applicationp(VarValue exp)
{
  return pairp(exp);
}

bool Scheme::primitivep(VarValue exp)
{
  return dynamic_cast<PrimProcValue*>(exp.ptr()) != NULL;
}

VarValue Scheme::execute_prim(VarValue proc, VarValue args)
{
  PrimProc prim = proc->getPrimProc();
  switch(prim.argNum){
  case 0:
    return prim.eval();
  case 1:
    return prim.eval(args);
  case 2:
    return prim.eval(Scar(args), Scadr(args));
  case 3:
    return prim.eval(Scar(args), Scadr(args), Scaddr(args));
  case 4:
    return prim.eval(Scar(args), Scadr(args), Scaddr(args), Scadddr(args));
  default:
    Error("unkonwn argnum ", prim.argNum, ValueCStr(proc));
    return Snull;
  }
}

VarValue Scheme::extend_env(VarValue vars, VarValue vals, VarValue outer)
{
  VarValue env(EnvValue::create(outer));

  while (!Snullp(vars) && !Snullp(vals))
  {
    VarValue p1 = vars->car();
    VarValue a1 = vals->car();
    env->setEnvSym(p1, a1);

    vars = vars->cdr();
    vals = vals->cdr();
  }

  if (!Snullp(vars))
  {
    Error("too many arguments supplied", ValueCStr(vars), ValueCStr(vals));
  }
  else if (!Snullp(vals))
  {
    Error("too few arguments supplied", ValueCStr(vars), ValueCStr(vals));
  }

  return env;
}

VarValue Scheme::reverse(VarValue ls)
{
  VarValue before = ls;
  VarValue after = cdr(ls);
  set_cdr(before, Null);

  VarValue tmp;
  for (; pairp(after); before = after, after = tmp)
  {
    tmp = cdr(after);
    set_cdr(after, before);
  }

  return before;
}

#define MAP_CALL_PROC(EXPS, PROC, ...)                      \
do{                                                         \
  VarValue res = Snull;                                     \
  for (VarValue arg = EXPS; !Snullp(arg); arg = Scdr(arg)){ \
    if (Spairp(arg))                                        \
      res = Scons( PROC (Scar(arg), __VA_ARGS__), res);     \
    else{                                                   \
      res = Scons( PROC (arg, __VA_ARGS__), res);           \
      break;                                                \
    }                                                       \
  }                                                         \
  return Sreverse(res);                                     \
 }while(0)

VarValue Scheme::analyze_sequence(VarValue exps, VarValue env)
{
  MAP_CALL_PROC(exps, Scheme::analyze, env);
  return Snull;
}

VarValue Scheme::map_proc(VarValue exps, PrimProc2 proc, VarValue v2)
{
  MAP_CALL_PROC(exps, proc, v2);
  return Snull;
}

VarValue Scheme::map_proc(VarValue exps, PrimProc1 proc)
{
  MAP_CALL_PROC(exps, proc);
  return Snull;
}

VarValue Scheme::map_proc(VarValue exps, PrimProc3 proc, VarValue v2, VarValue v3)
{
  MAP_CALL_PROC(exps, proc, v2, v3);
  return Snull;
}

VarValue Scheme::map_proc(VarValue exps, PrimProc4 proc, VarValue v2, VarValue v3, VarValue v4)
{
  MAP_CALL_PROC(exps, proc, v2, v3, v4);
  return Snull;
}

VarValue Scheme::eval(VarValue exp, VarValue env)
{
  return exp->eval(env);
}

}
