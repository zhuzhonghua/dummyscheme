#include "dummyscheme.h"

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
