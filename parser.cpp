#include "parser.h"
#include "value.h"
#include "env.h"
#include "core.h"

using namespace DummyScheme;


/*
	create optype or normal list type
*/
DummyValuePtr DummyParser::Compile(DummyValueList& list)
{
	DummyValuePtr front = list.front();
	if (front->isSymbol())
	{
		// specified constructor
		String symbol = front->getSymbol();
		
		if (0 == symbol.compare("lambda"))
			return CompileLambda(list);
		
		if (0 == symbol.compare("apply"))
			return CompileApply(list);
		
		MapOpTypeItr opTypeItr = DummyCore::opToType.find(symbol);
		if (opTypeItr != DummyCore::opToType.end())
		{
			int type = opTypeItr->second;
			MapOpCompileItr opCompileItr = DummyCore::opCompile.find(type);
			
			// attention here is the full list
			if (opCompileItr != DummyCore::opCompile.end())
				return opTypeValue(type, (opCompileItr->second)(list));

			MapOpNumItr opNumItr = DummyCore::opNum.find(symbol);
			if (opNumItr != DummyCore::opNum.end() && opNumItr->second > 0)
				AssertDummyValueList(list.size() >= opNumItr->second + 1, list, "need more parameters");
			
			// attention: here is the begin + 1
			return opTypeValue(type, list.begin()+1, list.end());
		}
		// (let ((c 2)) c)
		//	Error("unexpected type %d", type);	
	}
	else if (front->isLambda()) // ((lambda (a) (+ a 2)) 2)
		return opTypeValue(DUMMY_TYPE_APPLY, list);

	return listValue(list);
}

/*
	(lambda (p1 p2) (+ p1 p2))
*/
DummyValuePtr DummyParser::CompileLambda(DummyValueList& list)
{
	AssertDummyValueList(list.size() >= 3, list, "parameter >= 3");
	
	DummyValueListItr itr= list.begin();	
	DummyValuePtr front = *itr;

	DummyValuePtr binds = *++itr;
	DummyValueList bindList = binds->getList();
	DummyValueListItr bindItr = bindList.begin();
	BindList symbols;
	for (; bindItr != bindList.end(); ++bindItr)
	{
		symbols.push_back((*bindItr)->getSymbol());
	}
	// rest is the body
	return lambdaValue(symbols, ++itr, list.end());	
}


/*
	(apply (lambda (a) (+ a 2)) 3)
	(apply f 2 3)
	
	TODO:
	(apply + 2 3)
	(define a +)
	(apply a 2 3)
*/
DummyValuePtr DummyParser::CompileApply(DummyValueList& list)
{
	DummyValueListItr itr = list.begin();
	DummyValuePtr front = *itr;
	if (!front->isLambda())
	{
		AssertDummyValueList(list.size() >= 2, list, "parameter >= 2");
		++itr;
	}
	// first maybe the lambda

	return opTypeValue(DUMMY_TYPE_APPLY, itr, list.end());
}
