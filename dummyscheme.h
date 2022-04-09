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

typedef std::vector<std::string> BindList;

extern void init();
extern bool isEqual(const std::string& first, const DummyValuePtr& second);
extern DummyValuePtr createDummyValue(DummyValueList& list);
extern DummyValuePtr valueEval(DummyValuePtr value, DummyEnvPtr env);
extern std::string valueToString(DummyValuePtr value);
}

