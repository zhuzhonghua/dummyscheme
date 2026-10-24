#include "scmdebug.h"
#include "vm.h"

namespace Scheme {

/*
** Fill line information of the closure running in 'frm'.
*/
bool Debug::frameinfo(VM* vm, CallFrame* frm, FrameInfo* info)
{
  (void)vm;
  if (!isclosure(frm->base))
    return false;
  LambdaPtr lambda = closureref(frm->base)->lambda;

  info->source = lambda->source != NULL ? lambda->source->str : "<unknown source>";
  info->currentline = lambda->getline(frm->getpc());
  return true;
}

void Debug::printframe(VM* vm)
{
  fprintf(stderr, "stack traceback:\n");
  for (CallFrame* frm = vm->curfrm; frm != NULL; frm = frm->prev)
    printframe0(vm, frm);
}

void Debug::printframe0(VM* vm, CallFrame* frm)
{
  FrameInfo info;
  if (!frameinfo(vm, frm, &info))
  {
    fprintf(stderr, "\t<native>\n");
    return;
  }

  if (info.currentline > 0)
    fprintf(stderr, "\t%s:%d\n", info.source, info.currentline);
  else
    fprintf(stderr, "\t%s\n", info.source);

  if (frm->tailcall)
    fprintf(stderr, "\t(...tail calls...)\n");
}

/*
** ── hook engine (C hook only) ──
*/

/*
** Scope guard: keep a rooted snapshot of the interrupted computation's
** ac0 and re-enable hooks even when the hook raises; the error keeps
** propagating to the enclosing pcall like any other VM error.
*/
struct HookGuard {
  VM* vm;
  StkVar savedac;

  HookGuard(VM* v):vm(v), savedac(v)
  {
    vm->dbg.allowhook = false;
    savedac.val = G(v)->ac0;
  }
  ~HookGuard()
  {
    vm->debug.allowhook = true;
    G(vm)->ac0 = savedac.val;
  }
};

/*
** Install a C hook.  count seeds the count event and is only meaningful
** with SCM_MASKCOUNT.
*/
void Debug::sethook(VM* vm, ScmHookFn hook, int mask, int basecount)
{
  DebugState* dbg = &vm->debug;
  dbg->hook = hook;
  dbg->mask = mask;
  dbg->basecount = (mask & SCM_MASKCOUNT) ? basecount : 0;
  dbg->count = dbg->basecount;

  /* Seed the line history from the current trace point: after turning
     the hook on, no entry-style line event fires for instructions
     already executed (and already on the current line). */
  if (mask & SCM_MASKLINE)
    for (CallFrame* frm = vm->curfrm; frm != NULL; frm = frm->prev)
      frm->oldpc = frm->getpc();
}

/*
** Turn the engine off and drop the hook.
*/
void Debug::clearhook(VM* vm)
{
  DebugState* dbg = &vm->debug;
  dbg->hook = NULL;
  dbg->mask = 0;
  dbg->basecount = 0;
  dbg->count = 0;
}

/*
** Fire one hook event.  Quiet if no hook is installed, if the event is
** not enabled, or if we are already inside a hook (reentrancy guard).
** A hook that raises propagates the error like any other VM error; the
** enclosing pcall catches it and the engine stays installed.
*/
void Debug::dochook(VM* vm, int event, int line, CallFrame* frm)
{
  DebugState* dbg = &vm->debug;
  if (!dbg->allowhook)
    return;
  if (!dbg->hook)
    return;
  if (!(dbg->mask & (1 << event)))
    return;
  HookGuard g(vm);
  dbg->hook(vm, event, line, frm);
}

/*
** Interpreter-loop hook point, called once per instruction while count
** or line events are enabled.  Bytes run from high pc to low pc, so a
** pc greater than the frame's previous traced pc is a backward jump
** (loop back edge) and fires a line event even when the pc stays on
** one source line.  oldpc == -1 means the frame just started (first
** entry or resume after a suspension): always report.
*/
void Debug::traceexec(VM* vm, CallFrame* frm, int pc)
{
  DebugState* dbg = &vm->debug;

  if ((dbg->mask & SCM_MASKCOUNT) && dbg->basecount > 0)
  {
    dbg->count--;   /* only decremented between count events */
    if (dbg->count <= 0)
    {
      dbg->count = dbg->basecount;
      dochook(vm, SCM_HOOKCOUNT, -1, frm);
    }
  }

  if (!(dbg->mask & SCM_MASKLINE))
    return;

  /* pc is validated at the call site (>= 0, inside the code table). */
  LambdaPtr lambda = closureref(frm->base)->lambda;
  int line = lambda->getline(pc);
  int oldpc = frm->oldpc;
  frm->oldpc = pc;
  if (oldpc < 0 || pc > oldpc || line != lambda->getline(oldpc))
    dochook(vm, SCM_HOOKLINE, line, frm);
}

}
