#include "util.h"

namespace Dummy {

String Util::stringPrintf(const char* fmt, ...)
{
	int size = 128;
	std::string str;
	va_list ap;
	int times = 5;
	while (true && times-- > 0)
	{
		str.resize(size);
		va_start(ap, fmt);
		int n = vsnprintf((char *)str.data(), size, fmt, ap);
		va_end(ap);

		// n might be bigger than size
		if (n > -1 && n < size)
		{
			str.resize(n);
			return str;
		}

		size *= 2;
	}
  if (times < 0)
    throw "internal error at printf";

	return str;
}

bool Util::isFloatEqual(double a, double b)
{
	return std::abs(a - b) <= DBL_EPSILON;
}

};
