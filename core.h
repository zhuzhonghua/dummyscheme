#pragma once

#include "dummyscheme.h"

namespace DummyScheme{

typedef DummyValuePtr (*DummyOpEval)(DummyValuePtr value, DummyEnvPtr env);

typedef DummyValuePtr (*DummyOpCreate)(DummyValueList& list);

class DummyBuiltInOp{
public:
	DummyBuiltInOp(DummyType type, DummyOpEval op);
	DummyBuiltInOp(const std::string &typeStr, DummyOpCreate op);
public:
	static DummyOpEval builtInOps[];
	static std::map<std::string, DummyOpCreate> builtInOpsCreate;
};

class DummyCore{
public:
	static DummyValuePtr Eval(DummyValuePtr ast, DummyEnvPtr env);	
private:
	static void ConstructLetEnv(DummyValueList varList, DummyEnvPtr letEnv);
public:
	static bool isEqual(const std::string& first, const DummyValuePtr& second);

	static DummyValuePtr OpEvalQuasiQuote(DummyValuePtr value, DummyEnvPtr env);
};
};
