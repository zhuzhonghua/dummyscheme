#pragma once

#include "dummyscheme.h"

namespace DummyScheme{

typedef DummyValuePtr (*DummyOpEval)(DummyValuePtr value, DummyEnvPtr env);
typedef DummyValuePtr (*DummyOpCompile)(DummyValueList& list);

class DummyBuiltInHelper{
public:
	DummyBuiltInHelper(int type, const String& op, int num, DummyOpEval opEval);
	DummyBuiltInHelper(int type, const String& op, int num);
	DummyBuiltInHelper(int type, const String& op);
	DummyBuiltInHelper(const String& op, DummyOpCompile opCompile);
};
	
typedef std::map<String, DummyOpCompile> MapOpCompile;
typedef std::map<String, int> MapOpType;
typedef std::map<String, int> MapOpNum;

class DummyCore{
public:
	static DummyValuePtr Eval(DummyValuePtr ast, DummyEnvPtr env);
	static DummyValuePtr EvalOpType(DummyValuePtr ast, DummyEnvPtr env);
	static DummyValuePtr Compile(DummyValueList& list);
public:
	static String GetTypeStr(int type);
private:
	friend class DummyBuiltInHelper;
	
	static DummyOpEval builtInOpEval[];
	static MapOpCompile builtInOpCompile;
	static MapOpType builtInOpToType;
	static MapOpNum builtInOpNum;
	static String builtInTypeToOp[];
};
};
