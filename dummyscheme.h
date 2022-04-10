#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <map>

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
}

