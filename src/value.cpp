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

VarValue SymbolValue::eval(VarValue env)
{
  return env->findEnv(this);
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
AssignmentValue* AssignmentValue::create(VarValue exp)
{
  if (!Snullp(Scdddr(exp)))
    Error("too many args to set! %s", ValueCStr(exp));

  VarValue variable = Scadr(exp);
  VarValue value = Scaddr(exp);

  AssignmentValue* ret = new AssignmentValue(variable, value);
  RefGC::newRef(ret);
  return ret;
}

VarValue AssignmentValue::eval(VarValue env)
{
  VarValue resEnv = env->findEnv(var);
  if (!(resEnv))
    throw "unbound variable: SET! "+var;

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

DefineValue* DefineValue::create(VarValue exp)
{
  DefineValue* ret = new DefineValue(def_variable(exp), Scheme::analyze(def_value(exp)));
  RefGC::newRef(ret);
  return ret;
}

VarValue DefineValue::eval(VarValue env)
{
  env->setEnvSym(var, vproc->eval(env));
  return var;
}

IfValue* IfValue::create(VarValue p, VarValue c, VarValue a)
{
  IfValue* ret = new IfValue(p, c, a);
  RefGC::newRef(ret);
  return ret;
}

IfValue* IfValue::create(VarValue exp)
{
  VarValue p = Scheme::analyze(predicate(exp));
  VarValue c = Scheme::analyze(consequent(exp));
  VarValue a = Scheme::analyze(alternative(exp));

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
    if (!(Snullp(Scdr(tmp))))
      throw "bad if syntax";
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

ProcedureValue* ProcedureValue::create(VarValue exp)
{
  VarValue vars = parameters(exp);
  VarValue bproc = Scheme::analyze_sequence(body(exp));

  ProcedureValue* ret = new ProcedureValue(vars, bproc);
  RefGC::newRef(ret);
  return ret;
}

VarValue ProcedureValue::eval(VarValue env)
{
  return Scheme::make_procedure(vars, bproc, env);
}

VarValue ApplicationValue::operatr(VarValue exp)
{
  return Scar(exp);
}

VarValue ApplicationValue::operands(VarValue exp)
{
  return Scdr(exp);
}

ApplicationValue* ApplicationValue::create(VarValue exp)
{
  VarValue fproc = Scheme::analyze(operatr(exp));
  VarValue aprocs = Scheme::map_proc(operands(exp), Scheme::analyze);

  ApplicationValue* ret = new ApplicationValue(fproc, aprocs);
  RefGC::newRef(ret);
  return ret;
}

VarValue ApplicationValue::eval(VarValue env)
{
  VarValue fRes = fproc;
  if (!Scheme::isPrimProc(fproc))
  {
    fRes = fproc->eval(env);
  }

  return Scheme::execute_application(fRes,
                                     Scheme::map_proc(aprocs, Scheme::eval, env));
}

EnvValue* EnvValue::create(VarValue o)
{
  EnvValue* ret = new EnvValue(o);
  RefGC::newRef(ret);
  return ret;
}

void EnvValue::trace()
{
  for (VarMapItr itr = vars.begin(); itr != vars.end(); itr++)
  {
    VarValue key = itr->first;
    VarValue value = itr->second;
    RefGC::trace(key);
    RefGC::trace(value);
  }

  RefGC::trace(outer);
}

VarValue EnvValue::getEnvSym(VarValue symbol)
{
	for (EnvValue* env = this; env; env = dynamic_cast<EnvValue*>(env->outer.ptr()))
  {
		VarMapItr it = env->vars.find(symbol);
		if (it != env->vars.end())
    {
			return it->second;
		}
	}

	Error("didn't find value of symobl %s", ValueCStr(symbol));

	return NULL;
}

VarValue EnvValue::findEnv(VarValue symbol)
{
	for (EnvValue* env = this; env; env = dynamic_cast<EnvValue*>(env->outer.ptr()))
  {
		if (env->vars.find(symbol) != env->vars.end())
    {
			return env;
		}
	}

	Error("didn't find env of symobl %s", ValueCStr(symbol));

	return NULL;
}

VarValue EnvValue::setEnvSym(VarValue symbol, VarValue value)
{
	vars[symbol] = value;
	return value;
}
