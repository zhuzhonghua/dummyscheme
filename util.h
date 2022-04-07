#pragma once

#include <cstdarg>
#include <cstdio>

namespace DummyScheme {
	
#define Error(fmt, ...) errorThrow(fmt" File: %s Line: %d, Function: %s", ##__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__)

extern void errorThrow(const char *fmt, ...);
 
}
