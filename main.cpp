#include "vm.h"

using namespace Scheme;

int main(int argc, char **argv)
{
  VM vm;
  TRY {
  for (int i = 1; i < argc; i++)
    vm.loadfile(argv[i]);
  }
  CATCH(err) {
    Print("\ncaught: %s\n", err);
    return 1;
  }

  //vm.dorepl();

  //getchar();
  return 0;
}
