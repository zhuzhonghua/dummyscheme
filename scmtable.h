#pragma once

#include "vm.h"

namespace Scheme {

class SCMTable {
public:
  static void init(VM* vm);
  static ValueT maketable(VM* vm);
  static ValueT ref(VM* vm, ValueT* ht, ValueT* key);
  static void set(VM* vm, ValueT* ht, ValueT* key, ValueT* val);
};

};
