#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cfloat>
#include <cmath>

namespace Dummy {

typedef std::string String;
typedef std::vector<String> BindList;
typedef std::stringstream StringStream;

class Util {
public:
  static String stringPrintf(const char* fmt, ...);
  static bool isFloatEqual(double a, double b);
};

#define StringPrint Util::stringPrintf
#define STR(s) #s
#define Print(...) printf(StringPrint(__VA_ARGS__).c_str());

#define FILE_LINE_FUNCTION StringPrint("%s:%d:%s \t", __FILE__, __LINE__, __FUNCTION__)
#define Error(...) throw FILE_LINE_FUNCTION + StringPrint(__VA_ARGS__) + "\nerror happended\n";
#define Assert(condition, ...) do { if (!(condition)) { Error(#condition "" __VA_ARGS__); } } while (0)

};
