#include "value.h"

using namespace Dummy;

NumValue::NumValue(int num)
{
  this->num = num;
}

String NumValue::toString()
{
  StringStream out;
  out << num;
	return out.str();
}

StringValue::StringValue(const String &val)
{
  str = val;
}

String StringValue::toString()
{
  return "\"" + str + "\"";
}

SymbolValue::SymbolValue(const String &value)
  :symbol(value)
{
}

String SymbolValue::toString()
{
  return symbol;
}

VarValue SymbolValue::eval(VarValue env)
{
  return env->findEnv(this);
}

PairValue::PairValue(VarValue head, VarValue tail)
{
  this->head = head;
  this->tail = tail;
}

String PairValue::toString()
{
  StringStream out;
	out << "(" << head->toString() << " " << tail->toString() << ")";

	return out.str();
}

void PairValue::trace()
{
  RefGC::trace(head);
  RefGC::trace(tail);
}

AssignmentValue::AssignmentValue(VarValue v, VarValue vp)
{
  var = v;
  vproc = vp;
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

IfValue::IfValue(VarValue p, VarValue c, VarValue a)
{
  pproc = p;
  cproc = c;
  aproc = a;
}

VarValue IfValue::eval(VarValue env)
{
  Scheme* scm = Scheme::inst();
  if (scm->truep(pproc->eval(env)))
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

ProcedureValue::ProcedureValue(VarValue v, VarValue b)
{
  vars = v;
  bproc = b;
}

VarValue ProcedureValue::eval(VarValue env)
{
  Scheme* scm = Scheme::inst();

  return scm->make_procedure(vars, bproc, env);
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

ApplicationValue::ApplicationValue(VarValue f, VarValue a)
{
  fproc = f;
  aprocs = a;
}

VarValue ApplicationValue::eval(VarValue env)
{
  fproc->eval(env);

  Scheme* scm = Scheme::inst();
  VarValue tmp = aprocs;
  while (!scm->nullp(tmp))
  {
    tmp->car()->eval(env);
    tmp = tmp->cdr();
  }
  return scm->True;
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

EnvValue::EnvValue(VarValue out)
{
  outer = out;
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
