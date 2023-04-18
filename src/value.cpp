#include "scheme.h"

using namespace Dummy;

NullValue* NullValue::create()
{
  NullValue* ret = new NullValue();
  RefGC::newRef(ret);
  return ret;
}

NumValue* NumValue::create(int n)
{
  NumValue* ret = new NumValue(n);
  RefGC::newRef(ret);
  return ret;
}

BoolValue* BoolValue::create(bool b)
{
  BoolValue* ret = new BoolValue(b);
  RefGC::newRef(ret);
  return ret;
}

StringValue* StringValue::create(const String &val)
{
  StringValue* ret = new StringValue(val);
  RefGC::newRef(ret);
  return ret;
}

SymbolValue* SymbolValue::create(const String &val)
{
  SymbolValue* ret = new SymbolValue(val);
  RefGC::newRef(ret);
  return ret;
}

LexSymbolValue::LexSymbolValue(VarValue value, int addr)
{
  variable = value;
  lexAddr = addr;
}

String LexSymbolValue::toString()
{
  return variable->toString();
}

VarValue LexSymbolValue::create(VarValue variable, VarValue env)
{
  int lexAddr = env->getSymLexAddr(variable);
  Assert(lexAddr >= 0, "unbound variable: %s", ValueCStr(variable));

  // primitive optimization
  if (!env->findEnv(variable, lexAddr+1, false))
  {
    return Scheme::getPrimProc(variable);
  }

  LexSymbolValue* ret = new LexSymbolValue(variable, lexAddr);
  RefGC::newRef(ret);
  return ret;
}

VarValue LexSymbolValue::eval(VarValue env)
{
  return env->getEnvSym(variable, lexAddr);
}

PairValue* PairValue::create(VarValue h, VarValue t)
{
  PairValue* ret = new PairValue(h, t);
  RefGC::newRef(ret);
  return ret;
}

String PairValue::toString()
{
    StringStream out;
    out << "'(" << head->toString();
    for (VarValue tmp = tail; !Snullp(tmp); tmp = Scdr(tmp))
    {
      if (Spairp(tmp))
      {
        out << " " << Scar(tmp)->toString();
        continue;
      }
      else
      {
        out << " " << tmp->toString();
        break;
      }
    }
    out << ")";

    return out.str();
}

/*
  (set! a 2)
 */
AssignmentValue* AssignmentValue::create(VarValue exp, VarValue env)
{
  Assert(Snullp(Scdddr(exp)), "too many args to set! %s", ValueCStr(exp));

  VarValue variable = Scadr(exp);

  int lexAddr = env->getSymLexAddr(variable);
  Assert(lexAddr >= 0, "unbound variable: SET! %s", ValueCStr(variable));

  VarValue value = Scheme::analyze(Scaddr(exp), env);

  AssignmentValue* ret = new AssignmentValue(variable, value, lexAddr);
  RefGC::newRef(ret);
  return ret;
}

VarValue AssignmentValue::eval(VarValue env)
{
  VarValue resEnv = env->findEnv(var, lexAddr, true);

  resEnv->setEnvSym(var, vproc->eval(env));
  return var;
}

VarValue DefineValue::def_variable(VarValue val)
{
  VarValue var = Scadr(val);
  if (Ssymbolp(var)) return var;

  return Scar(var);
}

VarValue DefineValue::def_value(VarValue val)
{
  if (Ssymbolp(Scadr(val)))
    return Scaddr(val);
  else
    return Smake_lambda(Scdadr(val), Scddr(val));
}

DefineValue* DefineValue::create(VarValue exp, VarValue env)
{
  VarValue variable = def_variable(exp);
  int lexAddr = env->getSymLexAddr(variable);
  //
  if (lexAddr < 0) lexAddr = 0;

  VarValue vproc = Scheme::analyze(def_value(exp), env);

  DefineValue* ret = new DefineValue(variable, vproc, lexAddr);
  RefGC::newRef(ret);
  return ret;
}

VarValue DefineValue::eval(VarValue env)
{
  VarValue resEnv = env->findEnv(var, lexAddr, true);
  if (resEnv)
    resEnv->setEnvSym(var, vproc->eval(env));
  else
    env->setEnvSym(var, vproc->eval(env));
  return var;
}

IfValue* IfValue::create(VarValue p, VarValue c, VarValue a)
{
  IfValue* ret = new IfValue(p, c, a);
  RefGC::newRef(ret);
  return ret;
}

IfValue* IfValue::create(VarValue exp, VarValue env)
{
  VarValue p = Scheme::analyze(predicate(exp), env);
  VarValue c = Scheme::analyze(consequent(exp), env);
  VarValue a = Scheme::analyze(alternative(exp), env);

  return create(p, c, a);
}

VarValue IfValue::eval(VarValue env)
{
  if (Struep(pproc->eval(env)))
    return cproc->eval(env);
  else
    return aproc->eval(env);
}

VarValue IfValue::predicate(VarValue exp)
{
  return Scadr(exp);
}

VarValue IfValue::consequent(VarValue exp)
{
  return Scaddr(exp);
}

VarValue IfValue::alternative(VarValue exp)
{
  VarValue tmp = Scdddr(exp);
  if (!(Snullp(tmp)))
  {
    Assert(Snullp(Scdr(tmp)), "bad IF syntax");
    return Scar(tmp);
  }
  else
    return Sfalse;
}

VarValue ProcedureValue::parameters(VarValue exp)
{
  return Scadr(exp);
}

VarValue ProcedureValue::body(VarValue exp)
{
  return Scddr(exp);
}

ProcedureValue* ProcedureValue::create(VarValue exp, VarValue env)
{
  VarValue vars = parameters(exp);
  env = EnvValue::create(env, vars);

  VarValue bproc = Scheme::analyze_sequence(body(exp), env);

  ProcedureValue* ret = new ProcedureValue(vars, bproc);
  RefGC::newRef(ret);
  return ret;
}

VarValue ProcedureValue::eval(VarValue env)
{
  return Scheme::make_procedure(vars, bproc, env);
}

ApplicationValue* ApplicationValue::create(VarValue exp, VarValue env)
{
  VarValue operatr = Scar(exp);
  VarValue operands = Scdr(exp);

  VarValue fproc = Scheme::analyze(operatr, env);
  VarValue aprocs = Scheme::map_proc(operands, Scheme::analyze, env);
  // pre check
  if (Sprimitivep(fproc))
  {
    PrimProc pp = fproc->getPrimProc();
    int len = Slength(aprocs)->getNum();

    Assert(fproc->getPrimProc().argNum == Slength(aprocs)->getNum(),
           "argnum provided not equal op=%s arg=%s",
           ValueCStr(fproc), ValueCStr(aprocs));
  }

  ApplicationValue* ret = new ApplicationValue(fproc, aprocs);
  RefGC::newRef(ret);
  return ret;
}

VarValue ApplicationValue::eval(VarValue env)
{
  VarValue fRes = fproc;
  if (!Sprimitivep(fRes))
  {
    fRes = fRes->eval(env);
  }

  VarValue args = Scheme::map_proc(aprocs, Scheme::eval, env);
  if (Sprimitivep(fRes))
  {
    return Scheme::execute_prim(fRes, args);
  }
  else if (Scheme::compound_procedure_p(fproc))
  {
    VarValue body = Scheme::procedure_body(fproc);
    VarValue parameters = Scheme::procedure_parameters(fproc);
    VarValue childEnv = Scheme::extend_env(parameters,
                                           args,
                                           Scheme::procedure_env(fproc));
    return body->eval(childEnv);
  }
  else
  {
    Error("Unknown procedure type: execute-application %s", ValueCStr(fproc));
    return NULL;
  }
}

EnvValue::EnvValue(VarValue o)
  :outer(o)
{
}

EnvValue::EnvValue(VarValue o, VarValue variables)
  :EnvValue(o)
{
  if (!variables)
    return;

  for (; !Snullp(variables); variables = Scdr(variables))
  {
    if (Spairp(variables))
      EnvValue::setEnvSym(Scar(variables), Snull);
    else
    {
      EnvValue::setEnvSym(variables, Snull);
      break;
    }
  }
}


EnvValue* EnvValue::create(VarValue o)
{
  return create(0, Snull);
}

EnvValue* EnvValue::create(VarValue o, VarValue variables)
{
  EnvValue* ret = new EnvValue(o, variables);
  RefGC::newRef(ret);
  return ret;
}

VarValue EnvValue::getEnvSym(VarValue symbol, int lexAddr)
{
  EnvValue* env = findEnvLex(symbol, lexAddr);
  return env->getEnvSym(symbol);
}

VarValue EnvValue::getEnvSym(VarValue symbol)
{
  return vars.find(symbol)->second;
}

EnvValue* EnvValue::findEnvLex(VarValue symbol, int& lexAddr)
{
  if (lexAddr < 0)
    return NULL;

  // TODO: check null
  EnvValue* env = this;
  while (lexAddr-- > 0) {
    if (!env)
      return NULL;

    env = dynamic_cast<EnvValue*>(env->outer.ptr());
  }

  return env;
}

VarValue EnvValue::findEnv(VarValue symbol, int lexAddr, bool f)
{
  EnvValue* env = findEnvLex(symbol, lexAddr);
  if (!env && f)
    Error("internal error in finding lexcial symbol address");
  return env;
}

VarValue EnvValue::setEnvSym(VarValue symbol, VarValue value)
{
	vars[symbol] = value;
	return value;
}

int EnvValue::getSymLexAddr(VarValue symbol)
{
  int n = 0;
  EnvValue* env = this;
  for (; env; n++, env = dynamic_cast<EnvValue*>(env->outer.ptr()))
  {
    if (env->vars.find(symbol) != env->vars.end())
      return n;
  }

  if (env)
    return n;
  else
  {
    return -1;
  }
}

PrimProcValue* PrimProcValue::create(const PrimProc& prim)
{
  PrimProcValue* ret = new PrimProcValue(prim);
  RefGC::newRef(ret);
  return ret;
}
