#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cstdio>

#include "refcount.h"
#include "util.h"
#include "type.h"

namespace DummyScheme{

class DummyValue;
typedef DummyRefCountPtr<DummyValue>  DummyValuePtr;
typedef std::vector<DummyValuePtr> DummyValueList;

class DummyEnv;
typedef DummyRefCountPtr<DummyEnv>  DummyEnvPtr;

DummyValuePtr opTypeValue(const char* const typeStr, DummyType type, DummyValueList& list);
DummyValuePtr opTypeValue(const char* const typeStr, DummyType type, DummyValueList::iterator begin, DummyValueList::iterator end);
DummyValuePtr listValue(DummyValueList& list);
DummyValuePtr listValue(DummyValueList::iterator begin, DummyValueList::end);
DummyValuePtr strValue(const std::string &str);
DummyValuePtr numValue(int num);
DummyValuePtr symbolValue(const std::string &symbol);
DummyValuePtr lambdaValue(BindList& binds, DummyValueList& list);
DummyValuePtr lambdaValue(BindList& binds, DummyValueList::iterator begin, DummyValueList::iterator end);
}

