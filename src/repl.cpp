#include "scheme.h"

using namespace Dummy;

int read(String &input)
{
	if(getline(std::cin, input))
		return input.size();
	else
		return 0;
}

void eval(String &input)
{
  Reader reader(input);
  VarValue exp;
  VarValue env(Scheme::globalEnv);

  while ((exp = reader.readOne()).ptr())
  {
    exp = Scheme::analyze(exp, env);
    std::cout << ValueCStr(exp->eval(env)) << std::endl;
  }
}

int main()
{
  std::cout << "dummyscheme v0.0.1" << std::endl;

  Scheme::init();

	while(true) {
		try {
      std::cout << "scheme> ";
			String input;
			if(!read(input))
				break;
			eval(input);
		}	catch(String &exception) {
      std::cout << exception << std::endl;
		}
	}

  std::cout << "pause";
  String input;
  getline(std::cin, input);

  return 0;
}
