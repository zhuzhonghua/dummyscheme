#include "vm.h"

using namespace Scheme;

int main(int argc, char **argv)
{
  VM vm;
  for (int i = 1; i < argc; i++)
    vm.loadfile(argv[i]);

  //vm.dorepl();

  //getchar();
  return 0;
}
