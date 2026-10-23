#pragma once

#include "vm.h"

namespace Scheme {

/* Line information of a frame, used by the debugger. */
struct FrameInfo {
  FrameInfo():source(NULL), currentline(-1) {}

  const char* source;
  int currentline;
};

class Debug {
public:
  /* Fill 'info' from a live frame; 'frm' is guaranteed non-NULL with base
     set by the caller.  Returns false if the frame is not a Scheme closure. */
  static bool frameinfo(VM* vm, CallFrame* frm, FrameInfo* info);

  /* Print every frame in the chain, one line each (error reporting). */
  static void printframe(VM* vm);

  /* Print one frame of the chain as a single traceback line. */
  static void printframe0(VM* vm, CallFrame* frm);
};

}
