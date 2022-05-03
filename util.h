#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

namespace DummyScheme {
	
//#define STR(s) #s
#define FILE_LINE_FUNCTION stringPrintf("[%s:%d:%s]\n", __FILE__, __LINE__, __FUNCTION__)

//#define Error(...) errorThrow(FILE_LINE_FUNCTION, __VA_ARGS__)
#define Error(...) throw stringPrintf(__VA_ARGS__) + "\nerror happended at" + FILE_LINE_FUNCTION;

#define Assert(condition, ...) do { if (!(condition)) { Error(#condition "" __VA_ARGS__); } } while (0)

#define AssertDummyValue(condition, fmt, ptr) \
	do { if (!(condition)) { \
			Error(fmt" type=%d, value=%s", ptr->getType(), DummyValueCStr(ptr)); \
		} } while (0)

#define AssertDummyValueList(condition, fmt, list) \
	do { if (!(condition)) { \
			Error(fmt" value=%s", DummyValueCStr(DummyValuePtr(new DummyValue(list)))); \
		} } while (0)

//#define isSymEqual(first, second) \
//	AssertDummyValue(second->isSymbol(), "compare must be a symbol", second); \
//	return 0 == first.compare(second->getSymbol());

#define DummyValueCStr(ptr) ptr->toString().c_str()
#define DummyValueSymbolCStr(ptr) ptr->getSymbol().c_str()

extern std::string stringPrintf(const char* fmt, ...);
//extern void errorThrow(const char* fileLineFunc, const char *fmt, ...);
//extern void Print(const char *fmt, ...);
#define Print(...) printf(stringPrintf(__VA_ARGS__).c_str());
extern bool isEqual(const std::string& first, const std::string& second);
extern bool isFloatEqual(double a, double b);
}
