#include "scmdebug.h"
#include "vm.h"

#include <cctype>

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

  HookGuard(VM* v, CallFrame* frm):vm(v), savedac(v), savedfrm(v->debug.pausedfrm)
  {
    vm->debug.allowhook = false;
    savedac.val = G(v)->ac0;
    v->debug.pausedfrm = frm;
  }
  ~HookGuard()
  {
    vm->debug.pausedfrm = savedfrm;
    vm->debug.allowhook = true;
    G(vm)->ac0 = savedac.val;
  }
};

/* Debugger evaluation runs with the hook engine muted: a breakpoint or
   line hook must not fire for the debugger's own expressions. */
struct NoHookGuard {
  VM* vm;
  bool saved;

  NoHookGuard(VM* v):vm(v), saved(v->debug.allowhook)
  {
    vm->debug.allowhook = false;
  }
  ~NoHookGuard()
  {
    vm->debug.allowhook = saved;
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
** Decode one line-table entry: 'line' is the running delta-decoded line
** and 'absi' the cursor into the anchor table, both advanced one
** instruction.  Returns the line of the instruction at index 'i'.
*/
static int nextline(LambdaPtr lambda, int i, int line, int* absi)
{
  byte d = lambda->lines[i];
  if ((signed char)d != ABSLINE)
    return line + (signed char)d;
  while (*absi < lambda->abslines.n && lambda->abslines.get(*absi).pc < i)
    (*absi)++;
  if (*absi < lambda->abslines.n && lambda->abslines.get(*absi).pc == i)
    return lambda->abslines.get(*absi).line;
  return line;
}

/*
** Last instruction line of a lambda; a chunk body reports 0, matching its
** "main" role.  The line table is decoded once, pc by pc.
*/
static int lastinstrline(LambdaPtr lambda)
{
  if (lambda->vars == NULL)
    return 0;
  if (lambda->lines == NULL)
    return lambda->defline;
  int line = lambda->defline;
  int max = line;
  int absi = 0;
  for (int i = 0; i < lambda->sizelines; i++)
  {
    line = nextline(lambda, i, line, &absi);
    if (line > max)
      max = line;
  }
  return max;
}

/*
** Fill 'ar' (zeroed) from one closure.  Top-level lambdas carry no
** variable table and are the chunk bodies, so they report as "main".
** 'currentline' comes from the frame when active, -1 otherwise.
*/
static void getclosureinfo(VM* vm, ClosurePtr clo, CallFrame* frm, scm_Debug* ar)
{
  (void)vm;
  LambdaPtr lambda = clo->lambda;

  ar->event = -1;
  ar->name = NULL;
  ar->namewhat = NULL;
  ar->what = lambda->vars == NULL ? "main" : "scheme";
  ar->source = lambda->source != NULL ? lambda->source->str : "=?";
  ar->srclen = (int)strlen(ar->source);
  ar->currentline = frm != NULL ? lambda->getline(frm->getpc()) : -1;
  ar->linedefined = lambda->defline;
  ar->lastlinedefined = lastinstrline(lambda);
  ar->nups = clo->n - 1;  /* outers[0] is the environment */
  ar->nparams = lambda->argnum - (lambda->argrest ? 1 : 0);
  ar->isvararg = lambda->argrest ? 1 : 0;
  ar->istailcall = frm != NULL && frm->tailcall ? 1 : 0;
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
  if (!isclosure(frm->base))
    return false;
  memset(ar, 0, sizeof(*ar));
  getclosureinfo(vm, closureref(frm->base), frm, ar);
  return true;
}

/*
** Fill 'ar' (zeroed) from a native procedure: what is "native", source
** "=[native]", both lines -1, nups 0.  Returns false on a value that is
** not a native procedure.
*/
static bool getnativeinfo(VM* vm, ValueT* fn, scm_Debug* ar)
{
  if (!isnativeproc(fn))
    return false;
  memset(ar, 0, sizeof(*ar));
  NativeProcObj* np = nativeprocref(fn);
  ar->event = -1;
  ar->name = NULL;
  ar->namewhat = NULL;
  ar->what = "native";
  ar->source = "=[native]";
  ar->srclen = (int)strlen(ar->source);
  ar->currentline = -1;
  ar->linedefined = -1;
  ar->lastlinedefined = -1;
  ar->nups = 0;
  ar->nparams = np->argnum - (np->argrest ? 1 : 0);
  ar->isvararg = np->argrest ? 1 : 0;
  ar->istailcall = 0;
  ar->i_frm = NULL;
  snprintf(ar->short_src, sizeof(ar->short_src), "%s", ar->source);
  return true;
}

void SCMDebug::traceback(VM* vm, Lbuffer* buf)
{
  buf->put("stack traceback:\n");
  for (CallFrame* frm = inspectedchain(vm); frm != NULL; frm = frm->prev)
  {
    FrameInfo info;
    if (!frameinfo(vm, frm, &info))
    {
      buf->put("\t<native>\n");
      continue;
    }
    buf->put("\t");
    buf->put(info.source);
    if (info.currentline > 0)
    {
      char ln[16] = {0};
      snprintf(ln, sizeof(ln), ":%d", info.currentline);
      buf->put(ln);
    }
    buf->put("\n");
    if (frm->tailcall)
      buf->put("\t(...tail calls...)\n");
  }
}

/*
** ── locals and outers ──
**
** The name/read/write helpers take a valid index (0 <= n < count from
** localcount/outercount); resolving levels and names is the caller's
** job, so these stay a plain slot access each.
*/

int SCMDebug::localcount(CallFrame* frm)
{
  LambdaVarsObj* vars = closureref(frm->base)->lambda->vars;
  return vars != NULL ? vars->local.n : 0;
}

int SCMDebug::outercount(CallFrame* frm)
{
  return closureref(frm->base)->n - 1;  /* outers[0] is the environment */
}

void SCMDebug::localname(CallFrame* frm, int n, ValueT* out)
{
  setsym(out, closureref(frm->base)->lambda->vars->reflocal(n));
}

void SCMDebug::outername(CallFrame* frm, int n, ValueT* out)
{
  setsym(out, closureref(frm->base)->lambda->vars->refbvar(n)->name);
}

void SCMDebug::getlocal(CallFrame* frm, int n, ValueT* out)
{
  ValueT* slot = frm->base + 1 + n;
  *out = isbox(slot) ? boxref(slot)->val : *slot;
}

void SCMDebug::setlocal(CallFrame* frm, int n, ValueT* val)
{
  ValueT* slot = frm->base + 1 + n;
  if (isbox(slot))
    boxref(slot)->val = *val;
  else
    *slot = *val;
}

void SCMDebug::getouter(CallFrame* frm, int n, ValueT* out)
{
  *out = boxref(&closureref(frm->base)->outers[n + 1])->val;
}

void SCMDebug::setouter(CallFrame* frm, int n, ValueT* val)
{
  boxref(&closureref(frm->base)->outers[n + 1])->val = *val;
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
  LambdaPtr lambda = closureref(ar.i_frm->base)->lambda;
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

/* Resolve the inspected frame at 'level'; false when out of range. */
static bool levelframe(VM* vm, int level, CallFrame** frm)
{
  scm_Debug ar;
  if (!SCMDebug::getstack(vm, level, &ar))
    return false;
  *frm = ar.i_frm;
  return true;
}

/* Push ((interned key) . val) onto the alist under construction. */
static void infoput(VM* vm, ValueT* out, const char* key, ValueT* val)
{
  Sgcvar2(vm, k, kv);
  setsym(k, Intern(vm)->internsym(key));
  setpair(kv, SCM::cons(vm, k, val));
  setpair(out, SCM::cons(vm, kv, out));
}

/* Table of the lines carrying instructions; #f for a native procedure. */
static void activelines(VM* vm, LambdaPtr lambda, ValueT* out)
{
  if (lambda == NULL || lambda->lines == NULL)
  {
    *out = *Sfalseref;
    return;
  }
  Sgcvar2(vm, tbl, key);
  sethashtable(tbl, Sr1(vm, HashTableObj, vm));
  HashTableObj* lines = hashtableref(tbl);
  int line = lambda->defline;
  int absi = 0;
  ValueT tv = *Strueref;
  for (int i = 0; i < lambda->sizelines; i++)
  {
    line = nextline(lambda, i, line, &absi);
    setnumi(key, line);
    lines->set(vm, key, &tv);
  }
  *out = *tbl;
}

/*
** (debug-getinfo level-or-proc)
** Full description alist of the frame at 'level' or of a function value:
** source short_src linedefined lastlinedefined what currentline nups
** nparams isvararg name namewhat istailcall func activelines.  #f when a
** level is out of range.
*/
static ValueT scm_stub_debug_getinfo(VM* vm, ValueT* target)
{
  static const char* METHOD = "debug-getinfo";
  AssertArg(vm, isnumi(target) || isclosure(target) || isnativeproc(target),
            METHOD, target, "not a level or function");

  bool levelform = isnumi(target);
  scm_Debug ar;
  if (levelform)
  {
    if (!SCMDebug::getstack(vm, numi(target), &ar))
      return *Sfalseref;
  }
  else if (isclosure(target))
    getclosureinfo(vm, closureref(target), NULL, &ar);
  else
    getnativeinfo(vm, target, &ar);

  Sgcvar4(vm, out, val, func, lines);
  *func = levelform ? *ar.i_frm->base : *target;
  setstr(val, G(vm)->ninstrintern(ar.source, ar.srclen));
  infoput(vm, out, "source", val);
  setstr(val, G(vm)->ninstrintern(ar.short_src, (int)strlen(ar.short_src)));
  infoput(vm, out, "short_src", val);
  setnumi(val, ar.linedefined);
  infoput(vm, out, "linedefined", val);
  setnumi(val, ar.lastlinedefined);
  infoput(vm, out, "lastlinedefined", val);
  setsym(val, Intern(vm)->internsym(ar.what));
  infoput(vm, out, "what", val);
  setnumi(val, ar.currentline);
  infoput(vm, out, "currentline", val);
  setnumi(val, ar.nups);
  infoput(vm, out, "nups", val);
  setnumi(val, ar.nparams);
  infoput(vm, out, "nparams", val);
  *val = *(ar.isvararg ? Strueref : Sfalseref);
  infoput(vm, out, "isvararg", val);
  *val = *Sfalseref;      /* names are not tracked */
  infoput(vm, out, "name", val);
  setstr(val, vm->strintern("", 0));
  infoput(vm, out, "namewhat", val);
  *val = *(ar.istailcall ? Strueref : Sfalseref);
  infoput(vm, out, "istailcall", val);
  infoput(vm, out, "func", func);
  if (isclosure(target) || levelform)
    activelines(vm, closureref(func)->lambda, lines);
  else
    *lines = *Sfalseref;
  infoput(vm, out, "activelines", lines);
  return *out;
}

/* Index of the outer named 'name' in the frame; -1 when absent. */
static int findouter(CallFrame* frm, ValueT* name)
{
  int count = SCMDebug::outercount(frm);
  for (int i = 0; i < count; i++)
  {
    ValueT nm;
    SCMDebug::outername(frm, i, &nm);
    if (symref(&nm) == symref(name))
      return i;
  }
  return -1;
}

/* Build the (name . value) alist of the frame closure's outers. */
static void outervals(VM* vm, CallFrame* frm, ValueT* out)
{
  Sgcvar3(vm, namev, val, entry);
  int count = SCMDebug::outercount(frm);
  for (int i = count - 1; i >= 0; i--)
  {
    SCMDebug::outername(frm, i, namev);
    SCMDebug::getouter(frm, i, val);
    setpair(entry, SCM::cons(vm, namev, val));
    setpair(out, SCM::cons(vm, entry, out));
  }
}

/*
** Name of parameter 'n' (0-based, rest parameter included) of an inactive
** function; false for a native procedure and for an out-of-range index.
*/
static bool funcparamname(ValueT* fn, int n, ValueT* out)
{
  if (!isclosure(fn))
    return false;
  LambdaPtr lambda = closureref(fn)->lambda;
  if (lambda->vars == NULL || n < 0 || n >= lambda->argnum)
    return false;
  setsym(out, lambda->vars->local.get(n).sym);
  return true;
}

/*
** (debug-getlocal level-or-proc index)
** Frame form: two values, the name and value of local 'index' (0-based)
** in the frame at 'level'; #f when the index is out of range, an error
** when the level is.  Function form: the name of parameter 'index', no
** value; #f for a native procedure or an out-of-range index.
*/
static ValueT scm_stub_debug_getlocal(VM* vm, ValueT* target, ValueT* index)
{
  static const char* METHOD = "debug-getlocal";
  AssertArg(vm, isnumi(target) || isclosure(target) || isnativeproc(target),
            METHOD, target, "not a level or function");
  AssertArg(vm, isnumi(index), METHOD, index, "not an integer");
  int n = numi(index);

  Sgcvar2(vm, name, val);
  if (!isnumi(target))
    return funcparamname(target, n, name) ? *name : *Sfalseref;

  CallFrame* frm = NULL;
  Assert(vm, levelframe(vm, numi(target), &frm), "%s: level out of range", METHOD);
  if (n < 0 || n >= SCMDebug::localcount(frm))
    return *Sfalseref;
  SCMDebug::localname(frm, n, name);
  SCMDebug::getlocal(frm, n, val);
  setpair(val, SCM::cons(vm, val, Snullref));
  setpair(val, SCM::cons(vm, name, val));
  return SCM::packvalues(vm, val);
}

/*
** (debug-setlocal level index value)
** Write local 'index' (0-based) of the frame at 'level', boxed slots
** written through; returns the local's name.  #f when the index is out
** of range, an error when the level is.
*/
static ValueT scm_stub_debug_setlocal(VM* vm, ValueT* level, ValueT* index, ValueT* val)
{
  static const char* METHOD = "debug-setlocal";
  AssertArg(vm, isnumi(level), METHOD, level, "not a level");
  AssertArg(vm, isnumi(index), METHOD, index, "not an integer");
  CallFrame* frm = NULL;
  Assert(vm, levelframe(vm, numi(level), &frm), "%s: level out of range", METHOD);
  int n = numi(index);
  if (n < 0 || n >= SCMDebug::localcount(frm))
    return *Sfalseref;
  SCMDebug::setlocal(frm, n, val);
  Sgcvar1(vm, name);
  SCMDebug::localname(frm, n, name);
  return *name;
}

/*
** (debug-outers [level])
** (name . value) alist of the frame closure's outers, environment
** excluded; #f when the level is out of range.
*/
static ValueT scm_stub_debug_outers(VM* vm, ValueT* rest)
{
  static const char* METHOD = "debug-outers";
  int level = 0;
  if (!isnull(rest))
  {
    ValueT* p = Scar(rest);
    rest = Scdr(rest);
    AssertArg(vm, isnumi(p), METHOD, p, "not a number");
    Assert(vm, isnull(rest), "%s: too much arguments", METHOD);
    level = numi(p);
  }
  Sgcvar1(vm, out);
  CallFrame* frm = NULL;
  if (!levelframe(vm, level, &frm))
    return *Sfalseref;
  outervals(vm, frm, out);
  return *out;
}

/*
** (debug-outer level name)
** Value of the named outer in the frame at 'level'; #f when the frame or
** the name is out of range.
*/
static ValueT scm_stub_debug_outer(VM* vm, ValueT* level, ValueT* name)
{
  static const char* METHOD = "debug-outer";
  AssertArg(vm, isnumi(level), METHOD, level, "not a number");
  AssertArg(vm, issym(name), METHOD, name, "not a symbol");
  CallFrame* frm = NULL;
  if (!levelframe(vm, numi(level), &frm))
    return *Sfalseref;
  int n = findouter(frm, name);
  ValueT out;
  if (n < 0)
    return *Sfalseref;
  SCMDebug::getouter(frm, n, &out);
  return out;
}

/*
** (debug-setouter! level name value)
** Write through to the named outer; #t on success, #f when the frame or
** the name is out of range.
*/
static ValueT scm_stub_debug_setouter(VM* vm, ValueT* level, ValueT* name, ValueT* val)
{
  static const char* METHOD = "debug-setouter!";
  AssertArg(vm, isnumi(level), METHOD, level, "not a number");
  AssertArg(vm, issym(name), METHOD, name, "not a symbol");
  CallFrame* frm = NULL;
  if (!levelframe(vm, numi(level), &frm))
    return *Sfalseref;
  int n = findouter(frm, name);
  if (n < 0)
    return *Sfalseref;
  SCMDebug::setouter(frm, n, val);
  return *Strueref;
}

static const char* skipspace(const char* p)
{
  while (*p != '\0' && isspace((unsigned char)*p))
    p++;
  return p;
}

/*
** Evaluate 'expr' with the frame's variables snapshotted into a
** temporary environment; the result lands in *out, errors propagate.
** defines and set!s inside the expression write the temporary table
** only.  Runs on its own root frame chain (ReserveStack, like
** schemehook) and restores the interrupted computation's ac0.
*/
static void evalinframe(VM* vm, CallFrame* frm, const char* expr, ValueT* out)
{
  ReserveStack _reservestk_(vm);
  NoHookGuard _nohook_(vm);
  StkVar savedac(vm);
  savedac.val = G(vm)->ac0;
  Sgcvar2(vm, env, clo);

  ClosurePtr closure = closureref(frm->base);
  sethashtable(env, Sr1(vm, HashTableObj, vm));
  HashTableObj* envt = hashtableref(env);

  /* snapshot the frame's locals and the closure's outers into the
     temporary environment (void-valued ones too: they shadow
     same-named globals like the real scope does) */
  int count = SCMDebug::localcount(frm);
  for (int i = 0; i < count; i++)
  {
    ValueT nm;
    ValueT val;
    SCMDebug::localname(frm, i, &nm);
    SCMDebug::getlocal(frm, i, &val);
    envt->set(vm, &nm, &val);
  }
  count = SCMDebug::outercount(frm);
  for (int i = 0; i < count; i++)
  {
    ValueT nm;
    ValueT val;
    SCMDebug::outername(frm, i, &nm);
    SCMDebug::getouter(frm, i, &val);
    envt->set(vm, &nm, &val);
  }

  /* scm_compile wraps the expression in one (begin ...) form and
     installs a metatable __index = genv; retarget __index to the frame
     closure's own environment (a sandboxed frame carries a custom env
     there) */
  *clo = scm_compile(vm, env, expr, (int)strlen(expr));
  hashtableref(envt->getmetatable())->set(vm, &G(vm)->indexmeta,
                                          &closure->outers[0]);

  SCM::invokecall(vm, clo, NULL, 0);
  *out = G(vm)->ac0;
}

/*
** (debug-eval expr [level])
** Evaluate 'expr' (a string) with the frame at 'level'.  defines and
** set!s write the temporary table only; set! on locals does not write
** back to the frame (use debug-setlocal!).  Errors propagate like any
** VM error.
*/
static ValueT scm_stub_debug_eval(VM* vm, ValueT* expr, ValueT* rest)
{
  static const char* METHOD = "debug-eval";
  AssertArg(vm, isstr(expr), METHOD, expr, "not a string");
  int level = 0;
  if (!isnull(rest))
  {
    ValueT* p = Scar(rest);
    rest = Scdr(rest);
    AssertArg(vm, isnumi(p), METHOD, p, "not a number");
    Assert(vm, isnull(rest), "%s: too much arguments", METHOD);
    level = numi(p);
  }
  CallFrame* frm = NULL;
  if (!levelframe(vm, level, &frm))
    return *Sfalseref;
  const char* body = skipspace(Ssstr(strref(expr)));
  Assert(vm, *body != '\0', "%s: empty expression", METHOD);
  Sgcvar1(vm, out);
  evalinframe(vm, frm, body, out);
  return *out;
}

/*
** (debug-traceback [msg])
** Traceback text of the inspected chain as a Scheme string; the optional
** msg is the first line.
*/
static ValueT scm_stub_debug_traceback(VM* vm, ValueT* rest)
{
  static const char* METHOD = "debug-traceback";
  Lbuffer buf(vm);
  if (!isnull(rest))
  {
    Assert(vm, isnull(Scdr(rest)), "%s: too much arguments", METHOD);
    ValueT* p = Scar(rest);
    AssertArg(vm, isstr(p), METHOD, p, "not a string");
    buf.put(vtstr(p), vtstrlen(p));
    buf.put('\n');
  }
  SCMDebug::traceback(vm, &buf);
  ValueT out;
  setstr(&out, G(vm)->ninstrintern(buf.buf, buf.count));
  return out;
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

/*
** ── interactive console ──
*/

/* Source file for one 'list' command.  The stream is closed on every
   return path; lines are printed as they are read, so the command needs
   no process-wide cache and no manually freed source buffer. */
struct SourceFile {
  SourceFile(const char* path):file(fopen(path, "r")) {}
  ~SourceFile() { if (file != NULL) fclose(file); }

  bool opened() const { return file != NULL; }
  char* readline(char* buf, int size) { return fgets(buf, size, file); }

private:
  SourceFile(const SourceFile&);
  SourceFile& operator=(const SourceFile&);
  FILE* file;
};

/* Prompts only on a terminal; piped command scripts stay free of noise.
   Standard platform split: POSIX isatty(0), Windows
   _isatty(_fileno(stdin)), otherwise assume a tty. */
static bool stdin_is_tty()
{
#if defined(SCM_PLATFORM_WINDOWS)
  return _isatty(_fileno(stdin)) != 0;
#elif defined(SCM_PLATFORM_APPLE) || defined(SCM_PLATFORM_LINUX)
  return isatty(0) != 0;
#else
  return true;
#endif
}

struct ConsoleState {
  ConsoleState(VM* v):vm(v), level(0), tty(false) {}

  VM* vm;
  int level;   /* inspected frame; 0 = innermost */
  bool tty;
};

/* Whole-token integer parse; false for empty or trailing junk. */
static bool parsenum(const char* s, int* out)
{
  if (*s == '\0')
    return false;
  char* end = NULL;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0')
    return false;
  *out = (int)v;
  return true;
}

/* write-semantics rendering of one value into stdout */
static void printvalue_stdout(VM* vm, ValueT* val)
{
  int len = 0;
  const char* s = scm_tostring(vm, val, &len);
  fwrite(s, 1, (size_t)len, stdout);
}

static bool currentframe(ConsoleState* c, scm_Debug* ar)
{
  return SCMDebug::getstack(c->vm, c->level, ar);
}

static void consoleerror(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

static void consolebt(ConsoleState* c, int limit)
{
  int depth = SCMDebug::stackdepth(c->vm);
  if (limit < 0 || limit > depth)
    limit = depth;
  for (int i = 0; i < limit; i++)
  {
    scm_Debug ar;
    if (!SCMDebug::getstack(c->vm, i, &ar))
      break;
    printf("%c #%d %s:%d", i == c->level ? '*' : ' ', i,
           ar.short_src, ar.currentline);
    if (ar.istailcall)
      printf(" (tail)");
    printf("\n");
  }
}

static void consolewhere(ConsoleState* c)
{
  scm_Debug ar;
  if (!currentframe(c, &ar))
  {
    consoleerror("no frame %d", c->level);
    return;
  }
  printf("#%d %s:%d defline %d", c->level, ar.short_src,
         ar.currentline, ar.linedefined);
  if (ar.istailcall)
    printf(" tail-call");
  printf("\n");
}

static void consolelocals(ConsoleState* c)
{
  scm_Debug ar;
  if (!currentframe(c, &ar))
  {
    consoleerror("no frame %d", c->level);
    return;
  }

  int count = SCMDebug::localcount(ar.i_frm);
  if (count == 0)
  {
    printf("(no locals)\n");
    return;
  }
  for (int i = 0; i < count; i++)
  {
    ValueT nm;
    ValueT val;
    SCMDebug::localname(ar.i_frm, i, &nm);
    SCMDebug::getlocal(ar.i_frm, i, &val);
    printf("  %s = ", Ssstr(symref(&nm)));
    printvalue_stdout(c->vm, &val);
    printf("\n");
  }
}

static void consoleouters(ConsoleState* c)
{
  scm_Debug ar;
  if (!currentframe(c, &ar))
  {
    consoleerror("no frame %d", c->level);
    return;
  }

  int count = SCMDebug::outercount(ar.i_frm);
  if (count == 0)
  {
    printf("(no outers)\n");
    return;
  }
  for (int i = 0; i < count; i++)
  {
    ValueT nm;
    ValueT val;
    SCMDebug::outername(ar.i_frm, i, &nm);
    SCMDebug::getouter(ar.i_frm, i, &val);
    printf("  %s = ", Ssstr(symref(&nm)));
    printvalue_stdout(c->vm, &val);
    printf("\n");
  }
}

static void consoleprint(ConsoleState* c, const char* expr)
{
  expr = skipspace(expr);
  if (*expr == '\0')
  {
    printf("usage: p expr\n");
    return;
  }
  scm_Debug ar;
  if (!currentframe(c, &ar))
  {
    consoleerror("no frame %d", c->level);
    return;
  }
  Sgcvar1(c->vm, out);
  evalinframe(c->vm, ar.i_frm, expr, out);
  printvalue_stdout(c->vm, out);
  printf("\n");
}

static void consoleset(ConsoleState* c, char* arg)
{
  char* name = arg;
  char* q = name;
  while (*q != '\0' && *q != '=' && !isspace((unsigned char)*q))
    q++;
  if (*q == '\0')
  {
    printf("usage: set name = expr\n");
    return;
  }

  char* expr;
  if (*q == '=')
  {
    *q = '\0';
    expr = q + 1;
  }
  else
  {
    *q = '\0';
    expr = (char*)skipspace(q + 1);
    if (*expr != '=')
    {
      printf("usage: set name = expr\n");
      return;
    }
    expr++;
  }
  expr = (char*)skipspace(expr);
  if (*expr == '\0')
  {
    printf("usage: set name = expr\n");
    return;
  }

  scm_Debug ar;
  if (!currentframe(c, &ar))
  {
    consoleerror("no frame %d", c->level);
    return;
  }

  int idx = -1;
  int count = SCMDebug::localcount(ar.i_frm);
  for (int i = 0; i < count; i++)
  {
    ValueT nm;
    SCMDebug::localname(ar.i_frm, i, &nm);
    if (strcmp(Ssstr(symref(&nm)), name) == 0)
    {
      idx = i;
      break;
    }
  }
  if (idx < 0)
  {
    consoleerror("no local named %s", name);
    return;
  }

  Sgcvar1(c->vm, val);
  evalinframe(c->vm, ar.i_frm, expr, val);
  SCMDebug::setlocal(ar.i_frm, idx, val);
  printf("%s = ", name);
  printvalue_stdout(c->vm, val);
  printf("\n");
}

static void consolelist(ConsoleState* c, char* arg)
{
  scm_Debug ar;
  if (!currentframe(c, &ar))
  {
    consoleerror("no frame %d", c->level);
    return;
  }

  int start = -1;
  int end = -1;
  if (*arg != '\0')
  {
    char* comma = strchr(arg, ',');
    if (comma != NULL)
    {
      *comma = '\0';
      if (!parsenum(comma + 1, &end))
      {
        printf("usage: list [start[,end]]\n");
        return;
      }
    }
    if (!parsenum(arg, &start))
    {
      printf("usage: list [start[,end]]\n");
      return;
    }
  }
  if (start <= 0)
  {
    start = ar.currentline > 5 ? ar.currentline - 5 : 1;
    if (start < 1)
      start = 1;
  }
  if (end <= 0)
    end = start + 9;

  SourceFile sf(ar.source);
  if (!sf.opened())
  {
    consoleerror("source not available: %s", ar.short_src);
    return;
  }

  char buf[1024] = {0};
  int lineno = 0;
  while (sf.readline(buf, sizeof(buf)) != NULL)
  {
    int n = (int)strlen(buf);
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r'))
      buf[--n] = '\0';
    lineno++;
    if (lineno >= start && lineno <= end)
      printf("%s%4d  %s\n", lineno == ar.currentline ? "=>" : "  ",
             lineno, buf);
  }
  if (lineno == 0)
    consoleerror("empty source: %s", ar.short_src);
}

static void consoledisasm(ConsoleState* c)
{
  scm_Debug ar;
  if (!currentframe(c, &ar))
  {
    consoleerror("no frame %d", c->level);
    return;
  }
  if (!isclosure(ar.i_frm->base))
  {
    consoleerror("not a Scheme frame");
    return;
  }
  LambdaPtr lambda = closureref(ar.i_frm->base)->lambda;
  c->vm->printccode0(stdout, lambda, ar.i_frm->getpc());
  fflush(stdout);
}

static void consolehelp()
{
  printf(
    "commands:\n"
    "  bt [n]              backtrace, innermost first\n"
    "  frame N             select the inspected frame\n"
    "  up                  select the caller frame\n"
    "  down                select the callee frame\n"
    "  locals | lv         locals of the selected frame\n"
    "  outers | ov         outers of the selected frame's closure\n"
    "  p expr | print expr evaluate in the selected frame\n"
    "  set name = expr     write a local of the selected frame\n"
    "  where               selected frame position\n"
    "  list [start[,end]]  source lines around the current line\n"
    "  disasm              disassemble the selected frame\n"
    "  c | continue        resume the program\n"
    "  q | quit            exit the program\n"
    "  h | ? | help        this help\n");
}

/* Returns false when the console should return to the program. */
static bool consolecommand(ConsoleState* c, const char* cmd, char* arg)
{
  if (strcmp(cmd, "c") == 0 || strcmp(cmd, "continue") == 0)
    return false;
  if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0)
  {
    fflush(stdout);
    exit(0);
    return false;
  }
  if (strcmp(cmd, "h") == 0 || strcmp(cmd, "?") == 0 || strcmp(cmd, "help") == 0)
    consolehelp();
  else if (strcmp(cmd, "bt") == 0)
  {
    int limit = -1;
    if (*arg != '\0' && !parsenum(arg, &limit))
      printf("usage: bt [n]\n");
    else
      consolebt(c, limit);
  }
  else if (strcmp(cmd, "frame") == 0)
  {
    int level = 0;
    if (!parsenum(arg, &level))
    {
      printf("usage: frame N\n");
    }
    else
    {
      int depth = SCMDebug::stackdepth(c->vm);
      if (level < 0 || level >= depth)
        consoleerror("no frame %d", level);
      else
        c->level = level;
    }
  }
  else if (strcmp(cmd, "up") == 0)
  {
    if (c->level + 1 >= SCMDebug::stackdepth(c->vm))
      consoleerror("no caller frame");
    else
      c->level++;
  }
  else if (strcmp(cmd, "down") == 0)
  {
    if (c->level <= 0)
      consoleerror("no callee frame");
    else
      c->level--;
  }
  else if (strcmp(cmd, "locals") == 0 || strcmp(cmd, "lv") == 0)
    consolelocals(c);
  else if (strcmp(cmd, "outers") == 0 || strcmp(cmd, "ov") == 0)
    consoleouters(c);
  else if (strcmp(cmd, "p") == 0 || strcmp(cmd, "print") == 0)
    consoleprint(c, arg);
  else if (strcmp(cmd, "set") == 0)
    consoleset(c, arg);
  else if (strcmp(cmd, "where") == 0)
    consolewhere(c);
  else if (strcmp(cmd, "list") == 0)
    consolelist(c, arg);
  else if (strcmp(cmd, "disasm") == 0)
    consoledisasm(c);
  else
    consoleerror("unknown command: %s (h for help)", cmd);
  return true;
}

/* One command with the evaluator's errors caught here: a bad expression
   must not unwind the console, and the interrupted program's frames are
   kept alive by the hook guard or by the console's live curfrm chain. */
static bool consolexec(ConsoleState* c, const char* cmd, char* arg)
{
  bool keep = true;
  TRY
  {
    keep = consolecommand(c, cmd, arg);
  }
  CATCH(err)
  {
    printf("error: %s\n", err);
  }
  return keep;
}

static bool getconsoleline(Lbuffer* line)
{
  line->reset();
  line->buf[0] = '\0';
  int ch;
  while ((ch = getchar()) != EOF && ch != '\n')
    line->put((char)ch);
  if (ch == EOF && line->count == 0)
    return false;
  return true;
}

static bool getcmdarg(Lbuffer* line, char** cmd, char** arg)
{
  *cmd = NULL;
  *arg = NULL;

  while (line->count > 0 && isspace((unsigned char)line->buf[line->count-1]))
    line->buf[--line->count] = '\0';

  char* p1 = (char*)skipspace(line->buf);
  if (*p1 == '\0')
    return false;

  *cmd = p1;
  char* p2 = p1;
  while (*p2 != '\0' && !isspace((unsigned char)*p2))
    p2++;
  if (*p2 != '\0')
  {
    *p2 = '\0';
    p2++;
  }
  *arg = (char*)skipspace(p2);
  return true;
}

/* Run the inspection console: read one command per line into a growable
   Lbuffer and execute it until `continue`, `quit` or end of input. */
void SCMDebug::console(VM* vm)
{
  int depth = SCMDebug::stackdepth(vm);
  if (depth == 0)
  {
    consoleerror("debug: no stack frames");
    return;
  }

  ConsoleState c(vm);
  c.tty = stdin_is_tty();
  printf("debug console (%d frame%s)\n", depth, depth == 1 ? "" : "s");
  consolewhere(&c);

  Lbuffer line(vm);
  for (;;)
  {
    if (c.tty)
    {
      printf("(debug) ");
      fflush(stdout);
    }

    if (!getconsoleline(&line))
      break;
    char* cmd = NULL;
    char* arg = NULL;
    if (!getcmdarg(&line, &cmd, &arg))
      continue;
    if (!consolexec(&c, cmd, arg))
      break;
  }
  fflush(stdout);
}

/*
** (debug)
** Enter the inspection console at the current position.  The primitive
** itself does not create a frame, so level 0 is its caller (or the
** interrupted frame while a hook is paused).
*/
static ValueT scm_stub_debug(VM* vm)
{
  SCMDebug::console(vm);
  return_void();
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
    RegCProc("debug-getinfo", scm_stub_debug_getinfo),
    RegCProc("debug-getlocal", scm_stub_debug_getlocal),
    RegCProc("debug-setlocal", scm_stub_debug_setlocal),
    RegCProc("debug-frames", scm_stub_debug_frames),
    RegCProc("debug-outers", scm_stub_debug_outers, true),
    RegCProc("debug-outer", scm_stub_debug_outer),
    RegCProc("debug-setouter!", scm_stub_debug_setouter),
    RegCProc("debug-eval", scm_stub_debug_eval, true),
    RegCProc("debug-traceback", scm_stub_debug_traceback, true),
    RegCProc("debug", scm_stub_debug),
    RegCProc(NULL, -1)
  };
  regcfunc(vm, procs);
}

}
