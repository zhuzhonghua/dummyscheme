#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

namespace DummyScheme {
	
#define StringPrint stringPrintf

#define STR(s) #s

#define FILE_LINE_FUNCTION StringPrint("%s:%d:%s \t", __FILE__, __LINE__, __FUNCTION__)

#define Error(...) throw FILE_LINE_FUNCTION + StringPrint(__VA_ARGS__) + "\nerror happended\n";

#define Assert(condition, ...) do { if (!(condition)) { Error(#condition "" __VA_ARGS__); } } while (0)

#define AssertDummyValue(condition, ptr, ...)				                                             \
	do { if (!(condition)) {                                                                       \
			std::string fmt = StringPrint(__VA_ARGS__);                         											 \
			Error(#condition "%s type=%d value=%s", fmt.c_str(), ptr->getType(), DummyValueCStr(ptr)); \
		} } while (0)

#define AssertDummyValueList(condition, list, ...)	                                     \
	do {																							                                     \
	  if (!(condition)) {			                 																						 \
			std::string fmt = StringPrint(__VA_ARGS__);											                   \
			Error(#condition "%s listvalue=%s", fmt.c_str(), DummyValueCStr(listValue(list))); \
		}																																		                 \
	} while (0)

#define DummyValueCStr(ptr) ptr->toString().c_str()
#define DummyValueSymbolCStr(ptr) ptr->getSymbol().c_str()

extern std::string stringPrintf(const char* fmt, ...);
#define Print(...) printf(StringPrint(__VA_ARGS__).c_str());
extern bool isFloatEqual(double a, double b);
}
