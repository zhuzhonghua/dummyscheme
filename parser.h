#pragma once

#include "dummyscheme.h"

namespace DummyScheme{

class DummyParser{
public:
	static DummyValuePtr Compile(DummyValueList& list);
	static DummyValuePtr CompileLambda(DummyValueList& list);
	static DummyValuePtr CompileApply(DummyValueList& list);

};
}
