#pragma once

#include <string>
#include <map>

#include "dummyscheme.h"

class DummyEnv : public DummyRefCount {
public:
	DummyEnv(DummyEnvPtr outer);
	~DummyEnv();
public:
	DummyValuePtr get(const std::string& symbol);
	DummyEnvPtr find(const std::string& symbol);
	DummyValuePtr set(const std::string& symbol, DummyValuePtr value);
protected:
	typedef std::map<std::string, DummyValuePtr> SymbolMap;
	SymbolMap symbols;
	DummyEnvPtr outer;
};
