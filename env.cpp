#include "env.h"
#include "dummyscheme.h"
#include "value.h"

DummyEnv::DummyEnv(DummyEnvPtr out)
	:outer(out)
{
	
}

DummyEnv::~DummyEnv()
{
	
}

DummyValuePtr DummyEnv::get(const std::string& symbol)
{
	for (DummyEnvPtr env = this; env; env = env->outer) {
		SymbolMap::iterator it = env->symbols.find(symbol);
		if (it != env->symbols.end()) {
			return it->second;
		}
	}

	Error("didn't find value of symobl %s", symbol.c_str());
	
	return DummyValuePtr();
}

DummyEnvPtr DummyEnv::find(const std::string& symbol)
{
	for (DummyEnvPtr env = this; env; env = env->outer) {
		if (env->symbols.find(symbol) != env->symbols.end()) {
			return env;
		}
	}

	Error("didn't find env of symobl %s", symbol.c_str());
	
	return DummyEnvPtr();
}

DummyValuePtr DummyEnv::set(const std::string& symbol, DummyValuePtr value)
{
	symbols[symbol] = value;
	return value;
}
