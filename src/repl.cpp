#include "scheme.h"

using namespace Dummy;

int read(String &input)
{
	if(getline(std::cin, input))
		return input.size();
	else
		return 0;
}

void eval(String &input, VarValue env)
{
  Reader reader(input);
  VarValue exp;
  while (exp = reader.readOne())
  {
    exp = Scheme::analyze(exp);
    std::cout << ValueCStr(exp->eval(env)) << std::endl;
  }
}

int main()
{
  std::cout << "dummyscheme v0.0.1" << std::endl;

  Scheme::init();
  VarValue env(new EnvValue());

	while(true) {
		try {
      std::cout << "scheme> ";
			String input;
			if(!read(input))
				break;
			eval(input, env);
		}	catch(String &exception) {
      std::cout << exception << std::endl;
		}
	}

  std::cout << "pause";
  String input;
  getline(std::cin, input);

  return 0;
}
