#include "scmdebug.h"
#include "vm.h"

namespace Scheme {

/*
** Fill line information of the closure running in 'frm'.
*/
bool SCMDebug::frameinfo(VM* vm, CallFrame* frm, FrameInfo* info)
{
  (void)vm;
  if (!isclosure(frm->base))
    return false;
  LambdaPtr lambda = closureref(frm->base)->lambda;

  info->source = lambda->source != NULL ? lambda->source->str : "<unknown source>";
  info->currentline = lambda->getline(frm->getpc());
  return true;
}

void SCMDebug::printframe(VM* vm)
{
  fprintf(stderr, "stack traceback:\n");
  for (CallFrame* frm = vm->curfrm; frm != NULL; frm = frm->prev)
    printframe0(vm, frm);
}

void SCMDebug::printframe0(VM* vm, CallFrame* frm)
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
** ── hook engine ──
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
    vm->debug.allowhook = false;
    savedac.val = G(v)->ac0;
  }
  ~HookGuard()
  {
    vm->debug.allowhook = true;
    G(vm)->ac0 = savedac.val;
  }
};

/* Mask spec table: the events debug-sethook accepts, name -> mask bit.
   Entry order matches ScmHookEvent up to SCM_HOOKCOUNT; the tail-call
   event is reported under SCM_MASKCALL and is not a spec symbol. */
struct HookSpec {
  const char* name;
  int bit;
};

static const HookSpec hookspecs[] = {
  { "call", SCM_MASKCALL }, { "ret", SCM_MASKRET },
  { "line", SCM_MASKLINE }, { "count", SCM_MASKCOUNT },
};

static const int hookspeccount = (int)(sizeof(hookspecs) / sizeof(hookspecs[0]));

/* Event name reported to hooks, indexed by ScmHookEvent. */
static const char* hookeventname(int event)
{
  return event == SCM_HOOKTAILCALL ? "tail-call" : hookspecs[event].name;
}

/* Interned event symbols, filled by SCMDebug::init; indexed by ScmHookEvent.
   The symbols are markconst'ed so the static table needs no GC tracing. */
static ValueT hookeventsymbols[SCM_HOOKEVENTCOUNT];

/* Fire the Scheme hook: (hookfn event line), line is -1 for call/ret/count.
   The hook runs on its own root frame chain: ReserveStack detaches the
   interrupted computation's frames (invokecall's root frame would otherwise
   leave curfrm dangling when it returns), so curfrm and the dynamic-wind
   chain are untouched for the interrupted code.  Errors propagate through
   HookGuard like C-hook errors. */
static void schemehook(VM* vm, int event, int line)
{
  ReserveStack _reservestk_(vm);
  ValueT ln;
  setnumi(&ln, line);
  ValueT* argv[2] = { &hookeventsymbols[event], &ln };
  SCM::invokecall(vm, &vm->hookfn, argv, 2);
}

/*
** Seed the line history from the current trace point: after turning the
** hook on, no entry-style line event fires for instructions already
** executed (and already on the current line).
*/
static void seedlinehistory(VM* vm, int mask)
{
  if (mask & SCM_MASKLINE)
    for (CallFrame* frm = vm->curfrm; frm != NULL; frm = frm->prev)
      frm->oldpc = frm->getpc();
}

/*
** Install a C hook.  count seeds the count event and is only meaningful
** with SCM_MASKCOUNT.  Drops any Scheme hook: the hook installed last is
** the only active one.
*/
void SCMDebug::sethook(VM* vm, ScmHookFn hook, int mask, int basecount)
{
  DebugState* dbg = &vm->debug;
  dbg->hook = hook;
  setnull(&vm->hookfn);
  dbg->mask = mask;
  dbg->basecount = basecount;  /* only consumed when SCM_MASKCOUNT is set */
  dbg->count = dbg->basecount;
  seedlinehistory(vm, mask);
}

/*
** Activate the Scheme hook stored in VM::hookfn (set by debug-sethook).
** Drops any C hook: the hook installed last is the only active one.
*/
void SCMDebug::setschemehook(VM* vm, int mask, int basecount)
{
  DebugState* dbg = &vm->debug;
  dbg->hook = NULL;
  dbg->mask = mask;
  dbg->basecount = basecount;  /* only consumed when SCM_MASKCOUNT is set */
  dbg->count = dbg->basecount;
  seedlinehistory(vm, mask);
}

/*
** Turn the engine off and drop both hooks.
*/
void SCMDebug::clearhook(VM* vm)
{
  DebugState* dbg = &vm->debug;
  dbg->hook = NULL;
  setnull(&vm->hookfn);
  dbg->mask = 0;
  dbg->basecount = 0;
  dbg->count = 0;
}

/*
** Turn a hook spec (symbol or list of symbols) into a mask bit set.
*/
static int hookeventbit(VM* vm, ValueT* sym, const char* method)
{
  const char* name = Ssstr(symref(sym));
  for (int i = 0; i < hookspeccount; i++)
    if (strcmp(name, hookspecs[i].name) == 0)
      return hookspecs[i].bit;
  AssertArg(vm, false, method, sym, "unknown hook event");
  return 0;
}

int SCMDebug::schemehookspec(VM* vm, ValueT* spec)
{
  static const char* METHOD = "debug-sethook";
  if (issym(spec))
    return hookeventbit(vm, spec, METHOD);
  AssertVT(vm, SCM::listp(spec), spec,
           "%s: hook spec must be a symbol or a list of symbols", METHOD);
  int mask = 0;
  PAIR_FOR(p, spec)
  {
    AssertArg(vm, issym(Scar(p)), METHOD, Scar(p), "hook event must be a symbol");
    mask |= hookeventbit(vm, Scar(p), METHOD);
  }
  return mask;
}

void SCMDebug::gethook(VM* vm, ValueT* proc, int* mask, int* basecount)
{
  if (isnull(&vm->hookfn))
    *proc = *Sfalseref;
  else
    *proc = vm->hookfn;
  *mask = vm->debug.mask;
  *basecount = vm->debug.basecount;
}

/*
** Fire one hook event.  Quiet if the event is not enabled or if we are
** already inside a hook (reentrancy guard).  A hook that raises
** propagates the error like any other VM error; the enclosing pcall
** catches it and the engine stays installed.
*/
void SCMDebug::dochook(VM* vm, int event, int line, CallFrame* frm)
{
  DebugState* dbg = &vm->debug;
  if (!dbg->allowhook)
    return;
  /* A tail call is a call-flavored event: one mask bit covers both. */
  int bit = event == SCM_HOOKTAILCALL ? SCM_MASKCALL : 1 << event;
  if (!(dbg->mask & bit))
    return;
  if (dbg->hook)
  {
    HookGuard g(vm);
    dbg->hook(vm, event, line, frm);
  }
  else if (!isnull(&vm->hookfn))
  {
    HookGuard g(vm);
    schemehook(vm, event, line);
  }
}

/*
** Interpreter-loop hook point, called once per instruction while count
** or line events are enabled.  Bytes run from high pc to low pc, so a
** pc greater than the frame's previous traced pc is a backward jump
** (loop back edge) and fires a line event even when the pc stays on
** one source line.  oldpc == -1 means the frame just started (first
** entry or resume after a suspension): always report.
*/
void SCMDebug::traceexec(VM* vm, CallFrame* frm, int pc)
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

/*
** ── Scheme-side primitives ──
*/

/* The hook is called as (hook event line): accept exactly two named
   parameters, or a rest parameter covering them. */
static void checkhookarity(VM* vm, ValueT* proc, const char* method)
{
  int argnum;
  bool argrest;
  if (isclosure(proc))
  {
    LambdaObj* lambda = closureref(proc)->lambda;
    argnum = lambda->argnum;
    argrest = lambda->argrest;
  }
  else
  {
    NativeProcObj* nproc = nativeprocref(proc);
    argnum = nproc->argnum;
    argrest = nproc->argrest;
  }
  Assert(vm, argrest ? argnum <= 2 : argnum == 2,
         "%s: hook must accept 2 arguments (event, line)", method);
}

/*
** (debug-sethook spec count proc)
** spec is 'call / 'ret / 'line / 'count or a list of them; count seeds the
** count event (positive when 'count is in the mask).  proc is the hook
** closure called as (proc event line), event one of the spec symbols plus
** 'tail-call and line -1 for call/ret/count.  proc #f turns the engine
** off and drops both hooks.
*/
static ValueT scm_stub_debug_sethook(VM* vm, ValueT* spec, ValueT* count, ValueT* proc)
{
  static const char* METHOD = "debug-sethook";
  AssertArg(vm, isnumi(count), METHOD, count, "not a number");
  int n = numi(count);
  AssertArg(vm, n >= 0, METHOD, count, "count must not be negative");

  int mask = SCMDebug::schemehookspec(vm, spec);
  if (isfalse(proc) || isnull(proc))
  {
    SCMDebug::clearhook(vm);
    return_void();
  }
  AssertArg(vm, isclosure(proc) || isnativeproc(proc), METHOD, proc, "not a procedure");
  checkhookarity(vm, proc, METHOD);
  Assert(vm, !(mask & SCM_MASKCOUNT) || n > 0,
         "%s: the count event needs a positive count", METHOD);
  vm->hookfn = *proc;
  SCMDebug::setschemehook(vm, mask, n);
  return_void();
}

/*
** (debug-gethook)
** Three values: the hook procedure or #f (a C hook has no Scheme
** representation), the mask spec -- the event symbol when exactly one
** event is enabled, the list of symbols otherwise, '() when the engine
** is off -- and the count seed.
*/
static ValueT scm_stub_debug_gethook(VM* vm)
{
  Sgcvar3(vm, proc, spec, res);
  int mask = 0;
  int basecount = 0;
  SCMDebug::gethook(vm, proc, &mask, &basecount);

  /* Build the spec as a list (consing in reverse keeps table order), then
     unwrap the single-event case to the bare symbol -- the two forms
     debug-sethook accepts; '() when the engine is off. */
  setnull(spec);
  for (int i = hookspeccount - 1; i >= 0; i--)
    if (mask & hookspecs[i].bit)
      setpair(spec, SCM::cons(vm, &hookeventsymbols[i], spec));
  if (!isnull(spec) && isnull(Scdr(spec)))
    *spec = Scar(spec);

  ValueT count;
  setnumi(&count, basecount);
  setpair(res, SCM::cons(vm, &count, Snullref));
  setpair(res, SCM::cons(vm, spec, res));
  setpair(res, SCM::cons(vm, proc, res));
  return SCM::packvalues(vm, res);
}

void SCMDebug::init(VM* vm)
{
  /* Intern the event symbols once and mark them const: they are kept in
     a static table outside GC tracing, so they must survive every sweep
     on their own (same treatment as the reserved keywords). */
  for (int i = 0; i < SCM_HOOKEVENTCOUNT; i++)
  {
    SymPtr sym = Intern(vm)->internsym(hookeventname(i));
    sym->markconst();
    setsym(&hookeventsymbols[i], sym);
  }

  const RegCProc procs[] = {
    RegCProc("debug-sethook", scm_stub_debug_sethook),
    RegCProc("debug-gethook", scm_stub_debug_gethook),
    RegCProc(NULL, -1)
  };
  regcfunc(vm, procs);
}

}
