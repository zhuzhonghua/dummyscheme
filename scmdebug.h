#pragma once

#include <cstddef>

namespace Scheme {

class VM;
struct CallFrame;
struct ValueT;
struct Lbuffer;

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
  DebugState():mask(0), count(0), basecount(0), allowhook(true), hook(NULL),
               pausedfrm(NULL) {}

  int mask;       /* enabled events; 0 = engine off */
  int count;      /* instructions left before the next count event */
  int basecount;  /* count event reload value */
  bool allowhook; /* false while a hook is running */

  /* Event handler; the hook installed last is the only active one:
     exactly one of the two fields is set. */
  ScmHookFn hook;

  /* Frame chain under inspection while a hook runs (the interrupted
     computation); NULL outside hooks.  Not GC traced: the chain stays
     alive through ReserveStack (Scheme hook) or the live curfrm chain
     (C hook) for as long as this points into it. */
  CallFrame* pausedfrm;
};

/* Max length of short_src: the chunk name copied and truncated to this
   size. */
#define SCM_IDSIZE 60

/* Frame information, the 'S' / 'l' / 't' / 'u' part of the frame API.
   Function names are not tracked: name / namewhat stay NULL. */
typedef struct scm_Debug {
  int event;             /* hook event while inside a hook, else -1 */
  const char* name;      /* NULL */
  const char* namewhat;  /* NULL */
  const char* what;      /* "scheme" / "native" / "main" */
  const char* source;    /* chunk name; evalstr uses the source text */
  int srclen;
  int currentline;       /* -1 when unknown */
  int linedefined;
  int lastlinedefined;   /* not tracked: always -1 */
  int nups;              /* outer variable count, environment excluded */
  int nparams;           /* named parameters, rest parameter excluded */
  int isvararg;
  int istailcall;
  char short_src[SCM_IDSIZE];
  CallFrame* i_frm;      /* the frame the info was filled from */
} scm_Debug;

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

  /* ── inspected stack ── */

  /* Fill 'ar' (zeroed first) from the frame running 'level' calls deep,
     0 = innermost.  Walks the hook-paused chain when a hook is running,
     the live chain otherwise.  Returns false when the level is out of
     range, in which case 'ar' is untouched. */
  static bool getstack(VM* vm, int level, scm_Debug* ar);

  /* Number of frames on the inspected chain. */
  static int stackdepth(VM* vm);

  /* Format the inspected chain into 'buf': a header line and one line
     per frame (source:line), same layout as printframe. */
  static void traceback(VM* vm, Lbuffer* buf);

  /* ── interactive console ── */

  /* Run the inspection console on the current inspected chain until the
     user continues (or exits).  Called by the (debug) primitive; while a
     hook is paused level 0 is the interrupted frame, otherwise it is the
     primitive's caller. */
  static void console(VM* vm);

  /* ── locals and outers ── */

  /* Locals of the frame: base[1 + i], parameters first (the rest
     parameter, when present, is the last one). */
  static int localcount(CallFrame* frm);

  /* Outers of the frame's closure, environment excluded: index 0 is the
     first outer.  Outer values always live in boxes. */
  static int outercount(CallFrame* frm);

  /* Name of local/outer 'n' as a symbol value. */
  static void localname(CallFrame* frm, int n, ValueT* out);
  static void outername(CallFrame* frm, int n, ValueT* out);

  /* Read/write local 'n'.  Slots holding a box (captured or assigned
     variables) deref through it; *out is the void value while the slot
     is not yet initialized. */
  static void getlocal(CallFrame* frm, int n, ValueT* out);
  static void setlocal(CallFrame* frm, int n, ValueT* val);

  /* Read/write outer 'n'. */
  static void getouter(CallFrame* frm, int n, ValueT* out);
  static void setouter(CallFrame* frm, int n, ValueT* val);

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
