#pragma once

#include <string>
#include <sstream>
#include <cstdarg>
#include <vector>
#include <map>

#include "refcount.h"

#define Error(fmt, ...) errorThrow(fmt" File: %s Line: %d, Function: %s", ##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__)

extern void errorThrow(const char *fmt, ...);

class DummyValue;
typedef DummyRefCountPtr<DummyValue>  DummyValuePtr;
typedef std::vector<DummyValuePtr> DummyValueList;

class DummyEnv;
typedef DummyRefCountPtr<DummyEnv>  DummyEnvPtr;

// operator function
typedef DummyValuePtr (*OpFunc)(DummyValuePtr, DummyEnvPtr);
typedef std::map<std::string, OpFunc> OpMap;

