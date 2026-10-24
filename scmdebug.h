#pragma once

#include <cstddef>

namespace Scheme {

class VM;
struct CallFrame;
struct ValueT;

/* Hook engine: fires call/ret/line/count events from the interpreter
   loop; C hook (Debug::sethook), mask 0 turns the engine off. */

enum ScmHookEvent {
  SCM_HOOKCALL = 0,
  SCM_HOOKRET,
  SCM_HOOKLINE,
  SCM_HOOKCOUNT,
  SCM_HOOKTAILCALL,
};

enum ScmHookMask {
  SCM_MASKCALL  = 1 << 0,
  SCM_MASKRET   = 1 << 1,
  SCM_MASKLINE  = 1 << 2,
  SCM_MASKCOUNT = 1 << 3,
};

/* C-side hook.  It may run Scheme code; the engine guards reentrancy. */
typedef void (*ScmHookFn)(VM* vm, int event, int line, CallFrame* frm);

/* Per-VM hook state; members of VM, never GC traced (plain values). */
struct DebugState {
  DebugState():mask(0), count(0), basecount(0), allowhook(true) {}

  int mask;       /* enabled events; 0 = engine off */
  int count;      /* instructions left before the next count event */
  int basecount;  /* count event reload value */
  bool allowhook; /* false while a hook is running */

  /* Event handler; may run Scheme code (guarded against reentrancy). */
  ScmHookFn hook;
};

/* Frame line information used by the debugger. */
struct FrameInfo {
  FrameInfo():source(NULL), currentline(-1) {}

  const char* source;
  int currentline;
};

class Debug {
public:
  /* Line information of a live frame; 'frm' is guaranteed non-NULL with
     base set by the caller.  Returns false if the frame is not a Scheme
     closure. */
  static bool frameinfo(VM* vm, CallFrame* frm, FrameInfo* info);

  /* Print every frame of the chain, one line each (error reporting). */
  static void printframe(VM* vm);

  /* Print one frame of the chain as a single traceback line. */
  static void printframe0(VM* vm, CallFrame* frm);

  /* ── hook engine (C hook only) ── */

  /* Install a C hook.  count seeds the count event and is only meaningful
     with SCM_MASKCOUNT. */
  static void sethook(VM* vm, ScmHookFn hook, int mask, int count);

  /* Turn the engine off and drop the hook. */
  static void clearhook(VM* vm);

  /* Interpreter-loop entry point, called from execute0 each instruction.
     Fires count and line events. */
  static void traceexec(VM* vm, CallFrame* frm, int pc);

  /* Fire one raw event (call/return/tail call events from execute0). */
  static void dochook(VM* vm, int event, int line, CallFrame* frm);
};

}
