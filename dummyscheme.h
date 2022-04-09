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

extern bool isEqual(const std::string& first, const DummyValuePtr& second);

extern DummyValuePtr OpConstructPlus(DummyValueList& list);
extern DummyValuePtr OpConstructMinus(DummyValueList& list);
extern DummyValuePtr OpConstructMul(DummyValueList& list);
extern DummyValuePtr OpConstructDivide(DummyValueList& list);
extern DummyValuePtr OpConstructDefine(DummyValueList& list);
extern DummyValuePtr OpConstructLet(DummyValueList& list);
extern DummyValuePtr OpConstructBegin(DummyValueList& list);
extern DummyValuePtr OpConstructIf(DummyValueList& list);
extern DummyValuePtr OpConstructWhen(DummyValueList& list);
extern DummyValuePtr OpConstructUnless(DummyValueList& list);
extern DummyValuePtr OpConstructLambda(DummyValueList& list);
extern DummyValuePtr OpConstructApply(DummyValueList& list);

extern DummyValuePtr OpEvalPlus(DummyValuePtr value, DummyEnvPtr env);
extern DummyValuePtr OpEvalMinus(DummyValuePtr value, DummyEnvPtr env);
extern DummyValuePtr OpEvalMul(DummyValuePtr value, DummyEnvPtr env);
extern DummyValuePtr OpEvalDivide(DummyValuePtr value, DummyEnvPtr env);
extern DummyValuePtr OpEvalDefine(DummyValuePtr value, DummyEnvPtr env);
extern DummyValuePtr OpEvalLet(DummyValuePtr value, DummyEnvPtr env);
extern DummyValuePtr OpEvalBegin(DummyValuePtr value, DummyEnvPtr env);
extern DummyValuePtr OpEvalIf(DummyValuePtr value, DummyEnvPtr env);
extern DummyValuePtr OpEvalWhen(DummyValuePtr value, DummyEnvPtr env);
extern DummyValuePtr OpEvalUnless(DummyValuePtr value, DummyEnvPtr env);
extern DummyValuePtr OpEvalApply(DummyValuePtr value, DummyEnvPtr env);
}

