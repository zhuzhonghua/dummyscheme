#pragma once

#include "dummyscheme.h"

namespace DummyScheme{

class DummyCore{
public:
	static bool isEqual(const std::string& first, const DummyValuePtr& second);

	static DummyValuePtr OpConstructPlus(DummyValueList& list);
	static DummyValuePtr OpConstructMinus(DummyValueList& list);
	static DummyValuePtr OpConstructMul(DummyValueList& list);
	static DummyValuePtr OpConstructDivide(DummyValueList& list);
	static DummyValuePtr OpConstructDefine(DummyValueList& list);
	static DummyValuePtr OpConstructLet(DummyValueList& list);
	static DummyValuePtr OpConstructBegin(DummyValueList& list);
	static DummyValuePtr OpConstructIf(DummyValueList& list);
	static DummyValuePtr OpConstructWhen(DummyValueList& list);
	static DummyValuePtr OpConstructUnless(DummyValueList& list);
	static DummyValuePtr OpConstructLambda(DummyValueList& list);
	static DummyValuePtr OpConstructApply(DummyValueList& list);

	static DummyValuePtr OpEvalPlus(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalMinus(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalMul(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalDivide(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalDefine(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalLet(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalBegin(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalIf(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalWhen(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalUnless(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalApply(DummyValuePtr value, DummyEnvPtr env);
};
};
