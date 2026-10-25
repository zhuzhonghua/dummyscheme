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
** Scope guard for one hook event: keeps a rooted snapshot of the
** interrupted computation's ac0, points pausedfrm at the event's frame
** chain for the inspection APIs, and re-enables hooks even when the hook
** raises; the error keeps propagating to the enclosing pcall like any
** other VM error.
*/
struct HookGuard {
  VM* vm;
  StkVar savedac;
  CallFrame* savedfrm;

  HookGuard(VM* v, CallFrame* frm):vm(v), savedac(v)
  {
    vm->debug.allowhook = false;
    savedac.val = G(v)->ac0;
    savedfrm = vm->debug.pausedfrm;
    vm->debug.pausedfrm = frm;
  }
  ~HookGuard()
  {
    vm->debug.pausedfrm = savedfrm;
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
** already inside a hook (reentrancy guard).  While the hook runs,
** pausedfrm points at the event's frame chain for the inspection APIs.
** A hook that raises propagates the error like any other VM error; the
** enclosing pcall catches it and the engine stays installed.
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
    HookGuard g(vm, frm);
    dbg->hook(vm, event, line, frm);
  }
  else if (!isnull(&vm->hookfn))
  {
    HookGuard g(vm, frm);
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
** ── inspected stack ──
*/

/* Chain the inspection APIs look at: the interrupted computation while a
   hook runs, the live chain otherwise. */
static CallFrame* inspectedchain(VM* vm)
{
  return vm->debug.pausedfrm != NULL ? vm->debug.pausedfrm : vm->curfrm;
}

/*
** Fill 'ar' (zeroed) from one frame.  Top-level lambdas carry no variable
** table and are the chunk bodies, so they report as "main".
*/
static void fillinfo(VM* vm, CallFrame* frm, scm_Debug* ar)
{
  (void)vm;
  ClosurePtr clo = closureref(frm->base);
  LambdaPtr lambda = clo->lambda;

  ar->event = -1;
  ar->name = NULL;
  ar->namewhat = NULL;
  ar->what = lambda->vars == NULL ? "main" : "scheme";
  ar->source = lambda->source != NULL ? lambda->source->str : "=?";
  ar->srclen = (int)strlen(ar->source);
  ar->currentline = lambda->getline(frm->getpc());
  ar->linedefined = lambda->defline;
  ar->lastlinedefined = -1;
  ar->nups = clo->n - 1;  /* outers[0] is the environment */
  ar->nparams = lambda->vars != NULL
                ? lambda->vars->local.n - (lambda->argrest ? 1 : 0) : 0;
  ar->isvararg = lambda->argrest ? 1 : 0;
  ar->istailcall = frm->tailcall ? 1 : 0;
  ar->i_frm = frm;
  /* short_src: the chunk name, copied and truncated to SCM_IDSIZE */
  snprintf(ar->short_src, sizeof(ar->short_src), "%s", ar->source);
}

int SCMDebug::stackdepth(VM* vm)
{
  int depth = 0;
  for (CallFrame* frm = inspectedchain(vm); frm != NULL; frm = frm->prev)
    depth++;
  return depth;
}

bool SCMDebug::getstack(VM* vm, int level, scm_Debug* ar)
{
  if (level < 0)
    return false;
  CallFrame* frm = inspectedchain(vm);
  for (; level > 0 && frm != NULL; frm = frm->prev)
    level--;
  if (level > 0 || frm == NULL)
    return false;
  memset(ar, 0, sizeof(*ar));
  fillinfo(vm, frm, ar);
  return true;
}

/*
** ── Scheme-side primitives ──
*/

/*
** Put (key . val) onto the front of an alist.  'key' is a pre-interned
** markconst symbol (framekeys).  The pointed-to value must stay reachable
** on its own while the conses allocate: immediates, interned symbols,
** booleans and strings owned by live lambdas qualify.
*/
static void alistput(VM* vm, ValueT* alist, ValueT* key, ValueT* val)
{
  Sgcvar1(vm, kv);
  setpair(kv, SCM::cons(vm, key, val));
  setpair(alist, SCM::cons(vm, kv, alist));
}

/*
** Interned names used by the frame description alist -- the alist keys
** plus the two what-values -- filled and markconst'ed by SCMDebug::init
** (same treatment as the event symbols: the static table needs no GC
** tracing and alistput does no interning).
*/
enum FrameName {
  FN_DEPTH, FN_SOURCE, FN_WHAT, FN_LINE, FN_DEFLINE, FN_TAIL, FN_PARAMS,
  FN_OUTERS, FN_MAIN, FN_SCHEME,
  FRAMENAMECOUNT
};

static const char* const framenamestrs[FRAMENAMECOUNT] = {
  "depth", "source", "what", "line", "defline", "tail", "params", "outers",
  "main", "scheme",
};

static ValueT framename[FRAMENAMECOUNT];

/*
** Build the description alist of the frame running 'level' calls deep:
** (depth source what line defline tail params outers); #f when the level
** is out of range.  source shares the lambda's own string object.
*/
static void framealist(VM* vm, int level, ValueT* out)
{
  scm_Debug ar;
  if (!SCMDebug::getstack(vm, level, &ar))
  {
    *out = *Sfalseref;
    return;
  }
  Sgcvar1(vm, val);
  setnumi(val, level);
  alistput(vm, out, &framename[FN_DEPTH], val);
  CallFrame* frm = (CallFrame*)ar.i_frm;
  LambdaPtr lambda = closureref(frm->base)->lambda;
  if (lambda->source != NULL)
    setstr(val, lambda->source);
  else
    setstr(val, vm->strintern("=?", 2));
  alistput(vm, out, &framename[FN_SOURCE], val);
  *val = framename[lambda->vars == NULL ? FN_MAIN : FN_SCHEME];
  alistput(vm, out, &framename[FN_WHAT], val);
  setnumi(val, ar.currentline);
  alistput(vm, out, &framename[FN_LINE], val);
  setnumi(val, ar.linedefined);
  alistput(vm, out, &framename[FN_DEFLINE], val);
  *val = *(ar.istailcall ? Strueref : Sfalseref);
  alistput(vm, out, &framename[FN_TAIL], val);
  setnumi(val, ar.nparams);
  alistput(vm, out, &framename[FN_PARAMS], val);
  setnumi(val, ar.nups);
  alistput(vm, out, &framename[FN_OUTERS], val);
}

/*
** (debug-where)
** Description alist of the innermost inspected frame -- the interrupted
** computation inside a hook, the running code otherwise; #f when there
** is no frame.
*/
static ValueT scm_stub_debug_where(VM* vm)
{
  Sgcvar1(vm, out);
  framealist(vm, 0, out);
  return *out;
}

/*
** (debug-frames)
** Description alists of the inspected chain, innermost first.
*/
static ValueT scm_stub_debug_frames(VM* vm)
{
  Sgcvar2(vm, list, item);
  int depth = SCMDebug::stackdepth(vm);
  for (int level = depth - 1; level >= 0; level--)
  {
    framealist(vm, level, item);
    setpair(list, SCM::cons(vm, item, list));
  }
  return *list;
}

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

  /* The frame alist keys and what-values likewise. */
  for (int i = 0; i < FRAMENAMECOUNT; i++)
  {
    SymPtr sym = Intern(vm)->internsym(framenamestrs[i]);
    sym->markconst();
    setsym(&framename[i], sym);
  }

  const RegCProc procs[] = {
    RegCProc("debug-sethook", scm_stub_debug_sethook),
    RegCProc("debug-gethook", scm_stub_debug_gethook),
    RegCProc("debug-where", scm_stub_debug_where),
    RegCProc("debug-frames", scm_stub_debug_frames),
    RegCProc(NULL, -1)
  };
  regcfunc(vm, procs);
}

}
