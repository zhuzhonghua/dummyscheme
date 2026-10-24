#pragma once

#include <cstddef>

namespace Scheme {

class VM;
struct CallFrame;
struct ValueT;

/* Hook engine: fires call/ret/line/count/tail-call events from the
   interpreter loop.  The active hook is either a C function
   (SCMDebug::sethook) or a Scheme closure (debug-sethook, kept in
   VM::hookfn); the one installed last wins.  Mask 0 turns the engine
   off. */

enum ScmHookEvent {
  SCM_HOOKCALL = 0,
  SCM_HOOKRET,
  SCM_HOOKLINE,
  SCM_HOOKCOUNT,
  SCM_HOOKTAILCALL,
  SCM_HOOKEVENTCOUNT  /* number of hook events reported to hooks */
};

enum ScmHookMask {
  SCM_MASKCALL  = 1 << 0,
  SCM_MASKRET   = 1 << 1,
  SCM_MASKLINE  = 1 << 2,
  SCM_MASKCOUNT = 1 << 3,
};

/* C-side hook.  It may run Scheme code; the engine guards reentrancy. */
typedef void (*ScmHookFn)(VM* vm, int event, int line, CallFrame* frm);

/* Per-VM hook state; plain values, never GC traced.  The Scheme-side
   hook closure lives in VM::hookfn and is traced there. */
struct DebugState {
  DebugState():mask(0), count(0), basecount(0), allowhook(true), hook(NULL) {}

  int mask;       /* enabled events; 0 = engine off */
  int count;      /* instructions left before the next count event */
  int basecount;  /* count event reload value */
  bool allowhook; /* false while a hook is running */

  /* Event handler; the hook installed last is the only active one:
     exactly one of the two fields is set. */
  ScmHookFn hook;
};

/* Frame line information used by the debugger. */
struct FrameInfo {
  FrameInfo():source(NULL), currentline(-1) {}

  const char* source;
  int currentline;
};

class SCMDebug {
public:
  /* Line information of a live frame; 'frm' is guaranteed non-NULL with
     base set by the caller.  Returns false if the frame is not a Scheme
     closure. */
  static bool frameinfo(VM* vm, CallFrame* frm, FrameInfo* info);

  /* Print every frame of the chain, one line each (error reporting). */
  static void printframe(VM* vm);

  /* Print one frame of the chain as a single traceback line. */
  static void printframe0(VM* vm, CallFrame* frm);

  /* ── hook engine ── */

  /* Register the Scheme-side debug primitives (debug-sethook/gethook). */
  static void init(VM* vm);

  /* Install a C hook.  count seeds the count event and is only meaningful
     with SCM_MASKCOUNT.  Drops any Scheme hook: the hook installed last
     is the only active one. */
  static void sethook(VM* vm, ScmHookFn hook, int mask, int count);

  /* Activate the Scheme hook already stored in VM::hookfn (set by the
     debug-sethook primitive).  Drops any C hook. */
  static void setschemehook(VM* vm, int mask, int count);

  /* Turn a hook spec -- a symbol or a list of the symbols 'call / 'ret /
     'line / 'count -- into a ScmHookMask bit set; raises on anything
     else.  'tail-call is reported to hooks under SCM_MASKCALL but is not
     a spec symbol. */
  static int schemehookspec(VM* vm, ValueT* spec);

  /* Current hook state: *proc is the Scheme hook closure or #f (a C hook
     has no Scheme representation); *mask / *basecount the engine state. */
  static void gethook(VM* vm, ValueT* proc, int* mask, int* basecount);

  /* Turn the engine off and drop the hook. */
  static void clearhook(VM* vm);

  /* Interpreter-loop entry point, called from execute0 each instruction.
     Fires count and line events. */
  static void traceexec(VM* vm, CallFrame* frm, int pc);

  /* Fire one raw event (call/return/tail call events from execute0). */
  static void dochook(VM* vm, int event, int line, CallFrame* frm);
};

}
