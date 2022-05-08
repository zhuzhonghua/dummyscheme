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

typedef std::vector<std::string> BindList;

DummyValuePtr opTypeValue(const char* const typeStr, int type, const DummyValueList& list);
DummyValuePtr opTypeValue(const char* const typeStr, int type, DummyValueList::iterator begin, DummyValueList::iterator end);
DummyValuePtr listValue(const DummyValueList& list);
DummyValuePtr listValue(DummyValueList::iterator begin, DummyValueList::iterator end);
DummyValuePtr strValue(const std::string &str);
DummyValuePtr numValue(int num);
DummyValuePtr symbolValue(const std::string &symbol);
DummyValuePtr lambdaValue(const BindList& binds, const DummyValueList& list);
DummyValuePtr lambdaValue(const BindList& binds, DummyValueList::iterator begin, DummyValueList::iterator end);
}

