#pragma once

#include <string>
#include <map>

#include "dummyscheme.h"

namespace DummyScheme {
/*
	runtime environment
 */
class DummyEnv : public DummyRefCount {
public:
	DummyEnv(DummyEnvPtr outer);
	~DummyEnv();
public:
	DummyValuePtr get(const String& symbol);
	DummyEnvPtr find(const String& symbol);
	DummyValuePtr set(const String& symbol, DummyValuePtr value);
protected:
	typedef std::map<String, DummyValuePtr> SymbolMap;
	SymbolMap symbols;
	DummyEnvPtr outer;
};

/*
	read time environment, share same symbol value and int value
	dummped after read
 */
class DummyShareEnv : public DummyRefCount {
public:
	DummyValuePtr get(const String& symbol);
	DummyValuePtr get(int val);

	void add(const String& symbol, DummyValuePtr ptr);
	void add(int val, DummyValuePtr ptr);
protected:
	typedef std::map<String, DummyValuePtr> MapSymbolValue;
	typedef std::map<int, DummyValuePtr> MapIntValue;
	
	MapSymbolValue shareSymbols;
	MapIntValue shareIntegers;
};

}
