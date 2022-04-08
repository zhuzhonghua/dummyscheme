#pragma once

#include <cstdarg>
#include <cstdio>

namespace DummyScheme {
	
#define Error(fmt, ...) errorThrow(fmt" File: %s Line: %d, Function: %s", ##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__)

#define Assert(condition, fmt, ...) do { if (!(condition)) { Error(fmt, __VA_ARGS__); } } while (0)

extern void errorThrow(const char *fmt, ...);
extern void Print(const char *fmt, ...);
}
