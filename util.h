#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>

namespace DummyScheme {
	
#define Error(...) errorThrow("%s\n %s:%d Function: %s", ##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__)

#define Assert(condition, ...) do { if (!(condition)) { Error(__VA_ARGS__); } } while (0)

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

extern void errorThrow(const char *fmt, ...);
extern void Print(const char *fmt, ...);
extern bool isEqual(const std::string& first, const std::string& second);
extern bool isFloatEqual(double a, double b);
}
