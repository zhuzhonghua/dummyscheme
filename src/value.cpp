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

AssignmentValue* AssignmentValue::create(VarValue v, VarValue vp)
{
  AssignmentValue* ret = new AssignmentValue(v, vp);
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

String AssignmentValue::toString()
{
  StringStream out;
	out << "(set! " << var << " " << vproc->toString() << ")";

	return out.str();
}

void AssignmentValue::trace()
{
  RefGC::trace(var);
  RefGC::trace(vproc);
}

DefineValue* DefineValue::create(VarValue v, VarValue vp)
{
  DefineValue* ret = new DefineValue(v, vp);
  RefGC::newRef(ret);
  return ret;
}

VarValue DefineValue::eval(VarValue env)
{
  env->setEnvSym(var, vproc->eval(env));
  return var;
}

String DefineValue::toString()
{
  StringStream out;
	out << "(define " << var << " " << vproc->toString() << ")";

	return out.str();
}

IfValue* IfValue::create(VarValue p, VarValue c, VarValue a)
{
  IfValue* ret = new IfValue(p, c, a);
  RefGC::newRef(ret);
  return ret;
}

VarValue IfValue::eval(VarValue env)
{
  if (Struep(pproc->eval(env)))
    return cproc->eval(env);
  else
    return aproc->eval(env);
}

String IfValue::toString()
{
  StringStream out;
	out << "(if " << pproc->toString() << " "
      << cproc->toString() << " "
      << aproc->toString() << ")";

	return out.str();
}

void IfValue::trace()
{
  RefGC::trace(pproc);
  RefGC::trace(cproc);
  RefGC::trace(aproc);
}

ProcedureValue* ProcedureValue::create(VarValue v, VarValue b)
{
  ProcedureValue* ret = new ProcedureValue(v, b);
  RefGC::newRef(ret);
  return ret;
}

VarValue ProcedureValue::eval(VarValue env)
{
  return Scheme::make_procedure(vars, bproc, env);
}

String ProcedureValue::toString()
{
  StringStream out;
	out << "#<procedure>";

	return out.str();
}

void ProcedureValue::trace()
{
  RefGC::trace(vars);
  RefGC::trace(bproc);
}

ApplicationValue* ApplicationValue::create(VarValue f, VarValue a)
{
  ApplicationValue* ret = new ApplicationValue(f, a);
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

  VarValue res = Snull;
  VarValue parent;
  for (VarValue arg = aprocs; !Snullp(arg); arg = Scdr(arg))
  {
    set_cons_parent(Scar(arg)->eval(env), res, parent);
  }

  return Scheme::execute_application(fRes, res);
}

String ApplicationValue::toString()
{
  StringStream out;
	out << "#<application>:" << fproc->toString();

	return out.str();
}

void ApplicationValue::trace()
{
  RefGC::trace(fproc);
  RefGC::trace(aprocs);
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
