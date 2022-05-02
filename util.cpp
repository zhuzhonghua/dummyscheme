#include "util.h"
#include "value.h"
#include <cfloat>
#include <cmath>

namespace DummyScheme {

// Adapted from: http://stackoverflow.com/questions/2342162
std::string stringPrintf(const char* fmt, ...) {
	int size = strlen(fmt); // make a guess
	std::string str;
	va_list ap;
	while (1) {
		str.resize(size);
		va_start(ap, fmt);
		int n = vsnprintf((char *)str.data(), size, fmt, ap);
		va_end(ap);
		if (n > -1 && n < size) {  // Everything worked
			str.resize(n);
			return str;
		}
		if (n > -1)  // Needed size returned
			size = n + 1;   // For null char
		else
			size *= 2;      // Guess at a larger size (OS specific)
	}
	return str;
}

//void errorThrow(const char* fileLineFunc, const char *fmt, ...)
//{
//	std::string error("\nerror happpended at ");
//	error.append(fileLineFunc);
//	error.append(stringPrintf(fmt, __VA_ARGS__));
//
//	throw error;
//}
//
//void Print(const char *fmt, ...)
//{
//	printf(stringPrintf(fmt, __VA_ARGS__));
//}
//
bool isEqual(const std::string& first, const std::string& second)
{
	return first.size() == second.size() && first.size() > 0 && 0 == first.compare(second);
}

bool isFloatEqual(double a, double b)
{
	return std::abs(a - b) <= DBL_EPSILON;
}
}
