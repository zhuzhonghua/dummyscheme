#pragma once

#include "dummyscheme.h"

namespace DummyScheme{

class DummyCore{
public:
	static DummyValuePtr Eval(DummyValuePtr ast, DummyEnvPtr env);	
private:
	static void ConstructLetEnv(DummyValueList varList, DummyEnvPtr letEnv);
public:
	static bool isEqual(const std::string& first, const DummyValuePtr& second);

	static DummyValuePtr OpConstructTypeList(const char* typeStr, DummyType type, DummyValueList list, int paraLenMin);

	static DummyValuePtr OpConstructDefine(DummyValueList& list);
	static DummyValuePtr OpConstructLet(DummyValueList& list);
	static DummyValuePtr OpConstructLambda(DummyValueList& list);
	static DummyValuePtr OpConstructApply(DummyValueList& list);

	static DummyValuePtr OpEvalPlus(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalMinus(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalMul(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalDivide(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalDefine(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalApply(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalDisplay(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalList(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalListMark(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalNullMark(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalEqualMark(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalEqual(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalLess(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalLessEqual(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalBig(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalBigEqual(DummyValuePtr value, DummyEnvPtr env);
	static DummyValuePtr OpEvalLength(DummyValuePtr value, DummyEnvPtr env);
};
};
