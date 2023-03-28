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

ProcMap Scheme::primProcs;
SymbolMap Scheme::constSyms;

void Scheme::init()
{
  initIntern();
  initPrimProc();
}

void Scheme::initIntern()
{
  Null = intern("null");
  True = intern("true");
  False = intern("false");

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
  RegPrimProc procs[] = {
    {"+", {1, Splus}},
    {"cons", {2, Scons}},
    {"list", {1, Slist}},
    {"car", {1, Scar}},
    {"cdr", {1, Scdr}},
    {"cadr", {1, Scadr}},
    {"cddr", {1, Scddr}},
    {"caadr", {1, Scaadr}},
    {"caddr", {1, Scaddr}},
    {"cdadr", {1, Scdadr}},
    {"cdddr", {1, Scdddr}},
    {"cadddr", {1, Scadddr}},
  };
  regPrimProcs(procs, sizeof(procs)/sizeof(RegPrimProc));
}

VarValue Scheme::plus(VarValue arg)
{
  int num = 0;
  for (; !nullp(arg); arg = arg->cdr())
  {
    num += arg->car()->getNum();
  }

  return new NumValue(num);
}

VarValue Scheme::cons(VarValue arg1, VarValue arg2)
{
  return new PairValue(arg1, arg2);
}

VarValue Scheme::list(VarValue arg)
{
  return arg;
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
  primProcs.insert(std::make_pair(intern(name), proc));
}

VarValue Scheme::intern(const String& symbol)
{
  SymbolMapItr itr = constSyms.find(symbol);
  if (itr != constSyms.end())
    return itr->second;

  VarValue value(new SymbolValue(symbol));
  constSyms[symbol] = value;

  return value;
}

VarValue Scheme::list2(VarValue a, VarValue b)
{
  return new PairValue(a, Scons(b, Null));
}

VarValue Scheme::list3(VarValue a, VarValue b, VarValue c)
{
  return new PairValue(a, list2(b, c));
}

VarValue Scheme::list4(VarValue a, VarValue b, VarValue c, VarValue d)
{
  return new PairValue(a, list3(b, c, d));
}

VarValue Scheme::listn(int num, ...)
{
  VarValue res = Null;
  VarValue parent;

  va_list ap;
  va_start(ap, num);
  for (int i = 0; i < num; ++i)
  {
    VarValue one = va_arg(ap, VarValue);
    VarValue tmp = new PairValue(one, Null);

    if (nullp(res))
    {
      res = tmp;
    }
    if (parent)
    {
      parent->cdr(tmp);
    }
    parent = tmp;
  }
  va_end(ap);

  return res;
}

bool Scheme::nullp(VarValue exp)
{
  return exp == Null;
}

bool Scheme::notp(VarValue exp)
{
  return !falsep(exp);
}

bool Scheme::truep(VarValue exp)
{
  return exp != False;
}

bool Scheme::falsep(VarValue exp)
{
  return exp == False;
}

bool Scheme::booleanp(VarValue exp)
{
  return truep(exp) || falsep(exp);
}

bool Scheme::numberp(VarValue val)
{
  return dynamic_cast<NumValue*>(val.ptr()) != NULL ? true : false;
}

bool Scheme::pairp(VarValue val)
{
  return dynamic_cast<PairValue*>(val.ptr()) != NULL ? true : false;
}

bool Scheme::stringp(VarValue val)
{
  return dynamic_cast<StringValue*>(val.ptr()) != NULL ? true : false;
}

bool Scheme::symbolp(VarValue val)
{
  return dynamic_cast<SymbolValue*>(val.ptr()) != NULL ? true : false;
}

bool Scheme::assignmentp(VarValue val)
{
  return tagged_list_p(val, SetMark);
}

VarValue Scheme::assignment_variable(VarValue val)
{
  return cadr(val);
}

VarValue Scheme::assignment_value(VarValue val)
{
  return caddr(val);
}

bool Scheme::self_evaluating_p(VarValue val)
{
  return numberp(val) || stringp(val);
}

bool Scheme::tagged_list_p(VarValue val, VarValue tag)
{
  if (!pairp(val))
    return false;

  return tag == val->car();
}

bool Scheme::eqp(VarValue e1, VarValue e2)
{
  return e1 == e2;
}

bool Scheme::quotedp(VarValue val)
{
  return tagged_list_p(val, Quote);
}

VarValue Scheme::text_of_quotation(VarValue val)
{
  return cadr(val);
}

VarValue Scheme::analyze(VarValue exp)
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
    return exp;
  }
  if (assignmentp(exp))
  {
    new AssignmentValue(assignment_variable(exp),
                        analyze(assignment_value(exp)));
  }
  if (definep(exp))
  {
    new DefineValue(define_variable(exp),
                    analyze(define_value(exp)));
  }
  if (ifp(exp))
  {
    VarValue pproc = analyze(if_predicate(exp));
    VarValue cproc = analyze(if_consequent(exp));
    VarValue aproc = analyze(if_alternative(exp));

    return make_if(pproc, cproc, aproc);
  }
  if (lambdap(exp))
  {
    VarValue vars = lambda_parameters(exp);
    VarValue bproc = analyze_sequence(lambda_body(exp));
    return new ProcedureValue(vars, bproc);
  }
  if (beginp(exp))
  {
    return analyze_sequence(begin_actions(exp));
  }
  if (condp(exp))
  {
    return analyze(cond_to_if(exp));
  }
  if (applicationp(exp))
  {

  }

  return NULL;
}

bool Scheme::definep(VarValue val)
{
  return tagged_list_p(val, Define);
}

VarValue Scheme::define_variable(VarValue val)
{
  VarValue var = cadr(val);
  if (symbolp(var)) return var;

  return var->car();
}

VarValue Scheme::define_value(VarValue val)
{
  if (symbolp(cadr(val)))
    return caddr(val);
  else
    return make_lambda(cdadr(val), cddr(val));
}

VarValue Scheme::make_lambda(VarValue parameters, VarValue body)
{
  return cons(Lambda, cons(parameters, body));
}

VarValue Scheme::make_if(VarValue predicate, VarValue consequent, VarValue alternative)
{
  return new IfValue(predicate, consequent, alternative);
}

bool Scheme::ifp(VarValue exp)
{
  return tagged_list_p(exp, If);
}

VarValue Scheme::if_predicate(VarValue exp)
{
  return cadr(exp);
}

VarValue Scheme::if_consequent(VarValue exp)
{
  return caddr(exp);
}

VarValue Scheme::if_alternative(VarValue exp)
{
  VarValue tmp = cdddr(exp);
  if (!(nullp(tmp)))
  {
    if (!(nullp(tmp->cdr())))
      throw "bad if syntax";
    return tmp->car();
  }
  else
    return False;
}

bool Scheme::lambdap(VarValue exp)
{
  return tagged_list_p(exp, Lambda);
}

VarValue Scheme::lambda_parameters(VarValue exp)
{
  return cadr(exp);
}

VarValue Scheme::lambda_body(VarValue exp)
{
  return cddr(exp);
}

VarValue Scheme::analyze_sequence(VarValue exps)
{
  VarValue res = Null;
  VarValue parent;

  for (; !nullp(exps); exps = exps->cdr())
  {
    VarValue tmp(new PairValue(analyze(exps->car()), Null));
    if (nullp(res))
    {
      res = tmp;
    }
    if (parent)
    {
      parent->cdr(tmp);
    }
    parent = tmp;
  }

  return res;
}

VarValue Scheme::make_procedure(VarValue parameters, VarValue body, VarValue env)
{
  return list4(Procedure, parameters, body, env);
}

bool Scheme::compound_procedure_p(VarValue exp)
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
  if (nullp(seq))
    return seq;
  else if (last_exp_p(seq))
    return first_exp(seq);
  else
    return make_begin(seq);
}

bool Scheme::last_exp_p(VarValue seq)
{
  return nullp(seq->cdr());
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

bool Scheme::beginp(VarValue exp)
{
  return tagged_list_p(exp, Begin);
}

VarValue Scheme::begin_actions(VarValue exp)
{
  return exp->cdr();
}

bool Scheme::condp(VarValue exp)
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

bool Scheme::cond_else_clause_p(VarValue clause)
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
  if (nullp(clauses))
  {
    return quote(False);
  }
  else
  {
    VarValue first = clauses->car();
    VarValue rest = clauses->cdr();

    if (cond_else_clause_p(first))
    {
      if (nullp(rest))
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
      return make_if(cond_predicate(first),
                     sequence_to_exp(cond_actions(first)),
                     expand_clauses(rest));
    }
  }
}

VarValue Scheme::execute_application(VarValue proc, VarValue args)
{
  ProcMapItr itr = primProcs.find(proc);
  if (itr != primProcs.end())
  {
    PrimProc prim = itr->second;
    switch(prim.argNum){
    case 0:
      return prim.proc.proc0();
    case 1:
      return prim.proc.proc1(args);
    case 2:
      return prim.proc.proc2(Scar(args), Scadr(args));
    case 3:
      return prim.proc.proc3(Scar(args), Scadr(args), Scaddr(args));
    case 4:
      return prim.proc.proc4(Scar(args), Scadr(args), Scaddr(args), Scadddr(args));
    default:
      Error("unkonwn argnum ", prim.argNum, ValueCStr(proc));
    }
  }
  else if (compound_procedure_p(proc))
  {
    VarValue body = procedure_body(proc);
    return body->eval(extend_env(procedure_parameters(proc),
                                 args,
                                 procedure_env(proc)));
  }
  else
  {
    Error("Unknown procedure type: execute-application", ValueCStr(proc));
    return NULL;
  }
}

VarValue Scheme::extend_env(VarValue vars, VarValue vals, VarValue outer)
{
  VarValue env(new EnvValue(outer));

  while (!nullp(vars) && !nullp(vals))
  {
    VarValue p1 = vars->car();
    VarValue a1 = vals->car();
    env->setEnvSym(p1, a1);

    vars = vars->cdr();
    vals = vals->cdr();
  }

  if (!nullp(vars))
  {
    Error("too many arguments supplied", ValueCStr(vars), ValueCStr(vals));
  }
  else if (!nullp(vals))
  {
    Error("too few arguments supplied", ValueCStr(vars), ValueCStr(vals));
  }

  return env;
}

}
