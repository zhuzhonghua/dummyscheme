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
typedef std::vector<DummyValuePtr>::iterator DummyValueItr;

class DummyEnv;
typedef DummyRefCountPtr<DummyEnv>  DummyEnvPtr;

class DummyShareEnv;
typedef DummyRefCountPtr<DummyShareEnv> DummyShareEnvPtr;

typedef std::vector<std::string> BindList;
typedef std::string String;
typedef std::stringstream StringStream;

DummyValuePtr opTypeValue(int type, const DummyValuePtr &value);
DummyValuePtr opTypeValue(int type, const DummyValueList& list);
DummyValuePtr opTypeValue(int type, DummyValueItr begin, DummyValueItr end);
DummyValuePtr listValue(const DummyValueList& list);
DummyValuePtr listValue(DummyValueItr begin, DummyValueItr end);
DummyValuePtr strValue(const std::string &str);
DummyValuePtr numValue(int num);
DummyValuePtr symbolValue(const std::string &symbol);
DummyValuePtr lambdaValue(const BindList& binds, const DummyValueList& list);
DummyValuePtr lambdaValue(const BindList& binds, DummyValueList::iterator begin, DummyValueList::iterator end);
DummyValuePtr macroValue(const BindList& binds, const DummyValuePtr item);
DummyValuePtr macroValue(const BindList& binds, DummyValueItr begin, DummyValueItr end);
}

