#include "dummyscheme.h"

namespace Dummy{
extern void runTest();
};

using namespace Dummy;


int main()
{
  runTest();

  std::cout << "pause";
  String input;
  getline(std::cin, input);

  return 0;
}
