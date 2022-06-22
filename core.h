#pragma once

#include "dummyscheme.h"

namespace DummyScheme{

typedef DummyValuePtr (*DummyOpEval)(DummyValuePtr value, DummyEnvPtr env);
typedef DummyValueList (*DummyOpCompile)(DummyValueList& list);

class DummyTypeOpHelper{
public:
	DummyTypeOpHelper(int type, const String op, int num);
	DummyTypeOpHelper(int type, const String op);
	DummyTypeOpHelper(int type, const String op, DummyOpCompile opCompile);
	DummyTypeOpHelper(int type, const String op, DummyOpEval eval);
};

typedef std::map<int, DummyOpCompile> MapOpCompile;
typedef std::map<int, DummyOpCompile>::iterator MapOpCompileItr;

typedef std::map<String, int> MapOpType;
typedef std::map<String, int>::iterator MapOpTypeItr;

typedef std::map<String, int> MapOpNum;
typedef std::map<String, int>::iterator MapOpNumItr;

class DummyCore{
public:
	static DummyValuePtr Eval(DummyValuePtr ast, DummyEnvPtr env);
	static DummyValuePtr Eval(DummyValueList list, DummyEnvPtr env);
	static DummyValuePtr Eval(DummyValueListItr begin, DummyValueListItr end, DummyEnvPtr env);
	static DummyValuePtr EvalOpType(DummyValuePtr ast, DummyEnvPtr env);
public:
	static String GetTypeStr(int type);
private:
	friend class DummyTypeOpHelper;
	friend class DummyParser;
	
	static MapOpCompile opCompile;
	static MapOpNum opNum;
	static DummyOpEval opEval[];
	
	static MapOpType opToType;
	static String typeToOp[];
};
};
