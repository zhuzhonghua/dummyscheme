#include "util.h"
#include "value.h"
#include <cfloat>
#include <cmath>

namespace DummyScheme {

void errorThrow(const char *fmt, ...)
{
	//char buffer[512] = {0};
	va_list args;
	va_start(args, fmt);	
	//vsnprintf(buffer, sizeof(buffer), fmt, args);
	//perror(buffer);
	vprintf(fmt, args);
	va_end(args);

	// don't throw buffer
	throw "\nerror happended";
}

void Print(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);	
	//vsnprintf(buffer, sizeof(buffer), fmt, args);
	//perror(buffer);
	vprintf(fmt, args);
	va_end(args);	
}

bool isEqual(const std::string& first, const std::string& second)
{
	return first.size() == second.size() && first.size() > 0 && 0 == first.compare(second);
}

bool isFloatEqual(double a, double b)
{
	return std::abs(a - b) <= DBL_EPSILON;
}
}
