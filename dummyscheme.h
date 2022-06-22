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
typedef std::vector<DummyValuePtr>::iterator DummyValueListItr;

#define DUMMY_VALUE_LIST_FOR(list) for (DummyValueListItr itr = list.begin(); itr != list.end(); ++itr)

class DummyEnv;
typedef DummyRefCountPtr<DummyEnv>  DummyEnvPtr;

class DummyShareEnv;
typedef DummyRefCountPtr<DummyShareEnv> DummyShareEnvPtr;

typedef std::vector<std::string> BindList;
typedef std::string String;
typedef std::stringstream StringStream;

DummyValuePtr opTypeValue(int type, const DummyValuePtr &value);
DummyValuePtr opTypeValue(int type, const DummyValueList& list);
DummyValuePtr opTypeValue(int type, DummyValueListItr begin, DummyValueListItr end);
DummyValuePtr listValue(const DummyValueList& list);
DummyValuePtr listValue(DummyValueListItr begin, DummyValueListItr end);
DummyValuePtr strValue(const std::string &str);
DummyValuePtr numValue(int num);
DummyValuePtr symbolValue(const std::string &symbol);
DummyValuePtr lambdaValue(const BindList& binds, const DummyValueList& list);
DummyValuePtr lambdaValue(const BindList& binds, DummyValueListItr begin, DummyValueListItr end);
DummyValuePtr macroValue(const BindList& binds, const DummyValuePtr item);
DummyValuePtr macroValue(const BindList& binds, DummyValueListItr begin, DummyValueListItr end);
}

