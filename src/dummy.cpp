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

Scheme::ProcMap Scheme::primProcs;
Scheme::SymbolMap Scheme::constSyms;

void Scheme::init()
{
  initIntern();
  initPrimProc();
}

void Scheme::initIntern()
{
  Null = intern("null", NullValue::create());
  Nil = intern("nil", Null);
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
  VarValue res = Null;
  VarValue parent;

  va_list ap;
  va_start(ap, num);
  for (int i = 0; i < num; ++i)
  {
    set_cons_parent(va_arg(ap, VarValue), res, parent);
  }
  va_end(ap);

  return res;
}

VarValue Scheme::nullp(VarValue exp)
{
  return exp ? False : True;
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
  return truep(exp) || falsep(exp) ? True : False;
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

VarValue Scheme::assignment_variable(VarValue val)
{
  return cadr(val);
}

VarValue Scheme::assignment_value(VarValue val)
{
  return caddr(val);
}

VarValue Scheme::self_evaluating_p(VarValue val)
{
  return numberp(val) || stringp(val) ? True : False;
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
    return AssignmentValue::create(assignment_variable(exp),
                                   analyze(assignment_value(exp)));
  }
  if (definep(exp))
  {
    return DefineValue::create(define_variable(exp),
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
    return ProcedureValue::create(vars, bproc);
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
    return analyze_application(exp);
  }

  return NULL;
}

VarValue Scheme::definep(VarValue val)
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
  return IfValue::create(predicate, consequent, alternative);
}

VarValue Scheme::ifp(VarValue exp)
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
  if (!(Snullp(tmp)))
  {
    if (!(Snullp(Scdr(tmp))))
      throw "bad if syntax";
    return tmp->car();
  }
  else
    return False;
}

VarValue Scheme::lambdap(VarValue exp)
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
  VarValue res;
  VarValue parent;
  for (; !Snullp(exps); exps = Scdr(exps))
  {
    set_cons_parent(analyze(Scar(exps)), res, parent);
  }

  return res;
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
      return make_if(cond_predicate(first),
                     sequence_to_exp(cond_actions(first)),
                     expand_clauses(rest));
    }
  }
}

VarValue Scheme::analyze_application(VarValue exp)
{
  VarValue fproc = analyze(operatr(exp));
  VarValue aprocs;
  VarValue parent;
  for (VarValue arg = operands(exp); !Snullp(arg); arg = Scdr(arg))
  {
    set_cons_parent(analyze(Scar(arg)), aprocs, parent);
  }

  return ApplicationValue::create(fproc, aprocs);
}

VarValue Scheme::operatr(VarValue exp)
{
  return Scar(exp);
}

VarValue Scheme::operands(VarValue exp)
{
  return Scdr(exp);
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

bool Scheme::isPrimProc(VarValue exp)
{
  return primProcs.find(exp) != primProcs.end();
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

}
