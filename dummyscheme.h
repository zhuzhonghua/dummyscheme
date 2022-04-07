#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include "refcount.h"
#include "util.h"

namespace DummyScheme{

class DummyValue;
typedef DummyRefCountPtr<DummyValue>  DummyValuePtr;
typedef std::vector<DummyValuePtr> DummyValueList;

class DummyEnv;
typedef DummyRefCountPtr<DummyEnv>  DummyEnvPtr;

// operator function
typedef DummyValuePtr (*OpFunc)(DummyValuePtr, DummyEnvPtr);
typedef std::map<std::string, OpFunc> OpMap;

extern OpFunc getOpFunc(const std::string& symbol);
extern void addOpFunc(const std::string& symbol, OpFunc func);

extern void init();
}
