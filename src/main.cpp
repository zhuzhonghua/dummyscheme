#include "dummyscheme.h"

namespace Dummy{
extern void runTest();
};

using namespace Dummy;


int main()
{
  Scheme::init();

  runTest();

  std::cout << "pause";
  String input;
  getline(std::cin, input);

  return 0;
}
