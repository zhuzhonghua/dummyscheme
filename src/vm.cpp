#include "vm.h"
#include "scmcompiler.h"
#include "scmmath.h"
#include "scmbasic.h"
#include "scmport.h"
#include "scmstring.h"
#include "scmvector.h"
#include "scmcode.h"
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstring>

using namespace Scheme;

#define lexassert(vm, cond, line, fmt, ...) do {       \
 if (!(cond)) lexerror(vm, line, fmt, ##__VA_ARGS__);  \
 } while(0)

#define lexerror(vm, line, fmt, ...) do {        \
 Print(fmt, ##__VA_ARGS__);                      \
 Print(" %s:%d: ", vm->gettopsource(), line);    \
 DebugAssertStop(*((int*)0) = 0);                \
 throw "lex error";                              \
 } while (0)

ValueT ValueT::undefined(VT_UNDEFINED);
ValueT ValueT::eofref(VT_EOF);

ValueT ValueT::voidref(VT_VOID);

ValueT ValueT::nullref(VT_NULL);
ValueT ValueT::trueref(VT_TRUE);
ValueT ValueT::falseref(VT_FALSE);

ValueT ValueT::linknext;
ValueT ValueT::linkreturn;
ArrayObj ArrayObj::empty;

void RefObject::finz(VM* vm)
{
//  if (marked & (1 << WEAKTAG))
//    vm->rmWeak(this);
  int size = this->getsize();
  this->~RefObject();
  vm->free(this, size);
}

void PairObj::finz(VM* vm)
{
  Cache(vm)->addref(this);
}

void PairObj::visit(VM* vm)
{
  Check(scar);
  Check(scdr);
}

#define SweepListNext(gclist, objgroup, gc, delpost)  \
{ RefPtr* lst = &(gclist);                            \
  while (*lst != NULL) {                              \
    if ((*lst)->isconst () ||                         \
        (*lst)->isgcmark(objgroup->curbit()))         \
      lst = &(*lst)->gcnxt;                           \
    else {                                            \
      RefPtr item = *lst; *lst = (*lst)->gcnxt;       \
      (gc)->toDel(item);  delpost ;                   \
    }}                                                \
}

RefPtr RefObjGroup::recobj(RefPtr ref)
{
  addref(ref, curIdx);
  ref->gcmark(bitmask[curIdx]);
  GC(vm)->condgc();
  return ref;
}

void RefObjGroup::fullsweep()
{
  SweepListNext(gclst[curIdx], ObjGroup(vm), GC(vm), );
  RefPtr* lst = &gclst[othIdx];
  while (*lst != NULL)
  {
    RefPtr item = *lst;
    lst = &(*lst)->gcnxt;
    GC(vm)->toDelOrMark(item);
  }
}

bool RefObjGroup::stepsweep()
{
  for(int i = 0; i < 10 && gclst[othIdx] != NULL; i++)
  {
    RefPtr item = gclst[othIdx];
    gclst[othIdx] = gclst[othIdx]->gcnxt;
    GC(vm)->toDelOrMark(item);
  }
  return gclst[othIdx] != NULL;
}

void IntArray::suffixset(int i, int v)
{
  if (i >= vec.size)
    vec_ensure(int, vm, &vec, vec_fill1(-1));
  set(i, v);
}

IntArray::~IntArray()
{
  vec_finz(int, vm, &vec);
}

ArrayObj::ArrayObj(VM* vm, int c)
{
  vec_init(ValueT, vm, &array, c, vec_fill2(ValueT()));
  array.n = c;
}

void ArrayObj::shrink(VM* vm)
{
  vec_shrink(ValueT, vm, &array);
}

int ArrayObj::add(VM* vm, ValueT* ele)
{
  vec_add2(ValueT, vm, array, ele);
  return array.n-1;
}

void ArrayObj::visit(VM* vm)
{
  VEC_FOR(i, &array)
    Check(array.getptr(i));
}

void ArrayObj::finz(VM* vm)
{
  vec_finz(ValueT, vm, &array);
}

StkGCVar::~StkGCVar()
{
  Stk(vm)->sv = sv->next;
}

void StackSegment::closeouterval(VM* vm, CallFrame* frm, ValueT* level)
{
  OuterVal** p = &outers;
  while ((*p) != NULL && (*p)->valp >= level)
  {
    OuterVal* next = (*p)->next;
    (*p)->close(vm, frm);
    *p = next;
  }
}

OuterVal* StackSegment::findouterval(VM* vm, ValueT* level)
{
  OuterVal** pp = &outers;
  OuterVal* p = NULL;
  while ((p = *pp) != NULL && p->valp >= level)
  {
    if (p->valp == level)
      return p;
    pp = &p->next;
  }
  p = Sr0(vm, OuterVal);
  p->valp = level;
  p->next = *pp;
  *pp = p;
  return p;
}

void CallFrame::visit(VM* vm)
{
  Check(prev);
  Check(seg);
  for (ValueT* b = base; b < top; b++)
    Check(b);
}

void Stack::stepmark()
{
  fullmark();
}

void Stack::fullmark()
{
  StkVar *ptr = sv;
  while (ptr)
  {
    Check(ptr->var);
    ptr = ptr->next;
  }
  Check(curfrm);
}

Stack::Stack(VM* v):vm(v),sv(NULL)
{
  curfrm = &basefrm;
  curfrm->seg = &baseseg;
  curfrm->base = curfrm->start = curfrm->top = curfrm->seg->first();
}

void Stack::setvoid(ValueT* s, ValueT* e)
{
  for (ValueT* n = s; n <= e; n++)
    *n = Svoidref;
}

CallFrame* Stack::rtnfrm(CallFrame* frm)
{
  return curfrm = frm->prev;
}

CallFrame* Stack::newfrm(CallFrame* frm, ValueT* base, int argnum, int arity)
{
  StackSegment* seg = frm->seg;
  CallFrame* newfrm = curfrm = Sr0(vm, CallFrame);
  newfrm->prev = frm;
  newfrm->seg = seg;
  newfrm->base = newfrm->start = base;
  newfrm->top = newfrm->base + 1 + arity;
  if (seg->frozen > 0 || newfrm->top >= seg->end())
  {
    seg = Sr0(vm, StackSegment);
    newfrm->seg = seg;
    newfrm->base = seg->first();
    newfrm->top = newfrm->base + 1 + arity;
    Assert(vm, newfrm->top < seg->end(), "alloc stack seg error in newfrm, arity: %d too big", arity);
    int i = 0;
    for (; i <= argnum; i++)
      *(newfrm->base + i) = base + i;
  }
  return newfrm;
}

ReserveStack::ReserveStack(VM* v):vm(v)
{
  curfrm = Stk(vm)->curfrm;
}

ReserveStack::~ReserveStack()
{
  Stk(vm)->curfrm = curfrm;
}

void CacheGroup::addref(PairPtr ref)
{
  ref->car(Snullref);
  ref->cdr(Snullref);
  ref->link(pairs);
  pairs = ref;
}

PairPtr CacheGroup::getonepair()
{
  PairPtr item = pairs;
  if (pairs != NULL)
    pairs = (PairPtr)pairs->gcnxt;
  return item;
}

void CacheGroup::fullsweep()
{
  while (pairs != NULL)
  {
    PairPtr p = pairs;
    pairs = (PairPtr)pairs->gcnxt;
    p->RefObject::finz(vm);
  }
}

void SGC::toDel(RefPtr ref)
{
  ref->finz(vm);
}

void SGC::startstep()
{
  Assert(vm, state == GCSNone, "internal error %d", state);
  Assert(vm, graylstLast == NULL, "internal error graylstlast not null");
  Assert(vm, graylst == NULL, "internal error graylst not null");
  state = GCSNone + 1;
  //Stk(vm)->startstep();
  Intern(vm)->startstep();
  ftouchptr = &SGC::addgraylst;
}

bool SGC::markgray()
{
  for (int i = 0; i < 10; i++)
  {
    if (graylst == NULL) return false;
    PairPtr pair = graylst;
    graylst = (PairPtr)graylst->gcnxt;
    if (graylst == NULL) graylstLast = NULL;
    Assert(vm, isref(pair->car()), "internal error gray lst car not ref");
    pair->car()->v.p->visit(vm);
    if (!isnull(pair->cdr()))
      pair->cdr()->v.p->visit(vm);
    Cache(vm)->addref(pair);
  }
  return true;
}

void SGC::addgraylst(RefPtr ref)
{
  ValueT vt;
  setref(&vt, ref);
  if (graylst == NULL)
  {
    PairPtr pair = vm->getonepairnor();
    graylst = graylstLast = pair;
    graylstLast->car(&vt);
  }
  else
  {
    if (isnull(graylstLast->cdr()))
      graylstLast->cdr(&vt);
    else
    {
      PairPtr pair = vm->getonepairnor();
      graylstLast->gcnxt = pair;
      graylstLast = pair;
      pair->car(&vt);
    }
  }
}

void SGC::singlestep()
{
  switch(state) {
  case GCSMarkStk:
    Stk(vm)->stepmark();
    state = state + 1;
    break;
  case GCSMarkReg:
    state = state + 1;
    break;
  case GCSMarkFrame:
    vm->stepmarkframes();
    state = state + 1;
    break;
  case GCSMarkGray:
    if (!markgray())
      state = state + 1;
    break;
  case GCSSweepObjGroup:
    if (!ObjGroup(vm)->stepsweep())
      state = state + 1;
    break;
  case GCSSweepInterns:
    if (!Intern(vm)->stepsweep())
      state = state + 1;
    break;
  case GCSSweepEnd:
    Intern(vm)->checkResize();
    state = state + 1;
    break;
  case GCSEnd:
    state = GCSNone;
    break;
  default:
    Error(vm, "unknown state %d", state);
    break;
  }
}

void SGC::sweepgray()
{
  while (graylst != NULL)
  {
    PairPtr pair = graylst;
    graylst = (PairPtr)graylst->gcnxt;
    toDelOrMark(pair->car()->v.p);
    if (!isnull(pair->cdr()))
      toDelOrMark(pair->cdr()->v.p);
    this->toDel(pair);
  }
  graylst = graylstLast = NULL;
}

void SGC::fullgc()
{
  Check(vm->iport);
  Check(vm->oport);
  ftouchptr = &SGC::touchchild;
  ObjGroup(vm)->setfullgcmask();
  Stk(vm)->fullmark();
  vm->regfullmark();
  Cache(vm)->fullsweep();
  ObjGroup(vm)->fullsweep();
  sweepgray();
  Intern(vm)->fullsweep();
  clearDel();
  ObjGroup(vm)->unsetfullgcmask();
  ftouchptr = &SGC::addgraylst;
  state = GCSNone;
  setthreshold();
}

void SGC::clearDel()
{
  while (toDelLst != NULL)
  {
    RefPtr item = toDelLst;
    toDelLst = toDelLst->gcnxt;
    item->finz(vm);
  }
}

void SGC::stepfullgc()
{
  startstep();
  while (state != GCSNone)
    singlestep();
}

void SGC::trace(RefPtr ptr)
{
  if (ptr && !ptr->isgcmark(ObjGroup(vm)->curbit()))
  {
    ptr->gcmark(ObjGroup(vm)->curbit());
    (this->*ftouchptr)(ptr);
  }
}

void SGC::toDelOrMark(RefPtr ref)
{
  if (ref->isgcmark(ObjGroup(vm)->curbit()))
    ObjGroup(vm)->addrefcur(ref);
  else
    this->toDel(ref);
}

void SGC::checkBarrier(RefPtr ptr)
{
  if (state != GCSNone && ptr->isgcmark(ObjGroup(vm)->curbit()))
    trace(ptr);
}


const int Interns::INIT_LEN = 128;
bool Interns::stepsweep()
{
  int i = bidx;
  if (i < blistlen)
  {
    bidx++;
    if (bucketlist[i] != NULL)
    {
      RefPtr bklst = bucketlist[i];
      SweepListNext(bklst, ObjGroup(vm), GC(vm), bcount--);
    }

    return true;
  }

  return false;
}

void Interns::fullsweep()
{
  for (int i = 0; i < blistlen; i++)
  {
    RefPtr bklst = bucketlist[i];
    SweepListNext(bklst, ObjGroup(vm), GC(vm), bcount--);
  }
}

void Interns::rehash(int oldl, int newl)
{
  for (int i = 0; i < oldl; i++)
  {
    RefPtr b = bucketlist[i], bnext = NULL;
    bucketlist[i] = NULL;

    for (; b != NULL; b = bnext)
    {
      bnext = b->gcnxt;

      int idx = ((SymPtr)b)->hash % newl;
      b->gcnxt = bucketlist[idx];
      bucketlist[idx] = (SymPtr)b;
    }
  }
}

void Interns::checkResize()
{
  if (bcount >= blistlen)
  {
    int newl = blistlen * 2;
    while (bcount >= blistlen) newl *= 2;
    bucketlist = (SymPtr*)vm->realloc(bucketlist, blistlen, sizeof(SymPtr) * newl);
    rehash(blistlen, newl);
    blistlen = newl;
  }
  else if (bcount < blistlen / 4 && blistlen > INIT_LEN)
  {
    int newl = blistlen / 2;
    while (bcount < newl / 4) newl /= 2;
    newl = std::max(newl, INIT_LEN);
    rehash(blistlen, newl);
    bucketlist = (SymPtr*)vm->realloc(bucketlist, blistlen, sizeof(SymPtr) * newl);
    blistlen = newl;
  }
}

SymPtr Interns::internsym(const char* str, int n)
{
  buf.count = 0;
  for (int i = 0; i < n; i++)
    //buf.put(toupper(str[i]));
    buf.put(str[i]);
  return intern(buf.buf, n);
}

SymPtr Interns::intern(const char* str, int n)
{
  uint h = SCM::hash(str, n, vm->seed);
  int idx = h % blistlen;
  SymPtr b = bucketlist[idx];
  while (b != NULL)
  {
    if (b->len == n && memcmp(Ssstr(b), str, n * sizeof(char)) == 0)
    {
      b->gcmark(ObjGroup(vm)->curbit());
      return b;
    }
    b = (SymPtr)b->gcnxt;
  }
  SymPtr sym = vm->newstr(str, n, h);
  sym->gcnxt = bucketlist[idx];
  b = bucketlist[idx] = sym;
  b->gcmark(ObjGroup(vm)->curbit());
  bcount++;
  return sym;
}

void Interns::init()
{
  bucketlist = (SymPtr*)vm->alloc(sizeof(SymPtr) * INIT_LEN);
  blistlen = INIT_LEN;
  bcount = 0;
}

Interns::~Interns()
{
  if (bucketlist)
    vm->free(bucketlist, sizeof(SymPtr) * blistlen);
  bucketlist = NULL;
  blistlen = 0;
}

#define BIND_CONST_TO_VT(kstr, kvt) {           \
 SymPtr sym = Intern(vm)->internsym(kstr);      \
 sym->markconst();                              \
 GEnv(vm)->newkey(sym, kvt);                    \
}

#define initreserve2(VT, SYM) {                 \
 SymPtr sym = Intern(vm)->internsym(SYM);       \
 sym->markconst();                              \
 setsym(&vm-> VT, sym);                         \
}

void GSymTable::init()
{
  lsizenode = MINLTSIZE;
  int size = twoto(lsizenode);
  slots = new (vm->alloc(size * sizeof(TSlot))) TSlot[size];
  lastfree = &slots[size - 1];
}

void GSymTable::newkey(SymPtr sym, ValueT* v)
{
  ValueT symp;
  setsym(&symp, sym);
  Sgcvar1(vm, dummy);
  setpair(dummy, SCM::cons(vm, &symp, v));
  insert(sym, pairref(dummy));
}

void GSymTable::newkeyorupdate(SymPtr sym, ValueT* val)
{
  PairPtr pair = getslot(sym);
  if (pair == NULL)
    newkey(sym, val);
  else
  {
    pair->cdr(val);
    GC(vm)->checkBarrier(pair);
  }
}

void GSymTable::insert(SymPtr sym, PairPtr tv)
{
  TSlot* node = getpos(sym);
  if (node->tv != NULL)
  {
    TSlot* freenode = getfreepos();
    if (!freenode)
    {
      rehash();
      insert(sym, tv);
      return;
    }
    PairPtr othertv = node->tv;
    SymPtr othersym = symref(othertv->car());
    TSlot* othernode = getpos(othersym);
    if (node == othernode)
    {
      DebugMem(Print("gsymtable insert collide same node {%s[hash %d]} VS {%s[hash %d]}\n",
                     sym->str, sym->hash, othersym->str, othersym->hash));
      freenode->settv(tv);
      while (node->next) node = node->next;
      node->next = freenode;
    }
    else
    {
      DebugMem(Print("gsymtable insert collide not same node {%s[hash %d]} VS {%s[hash %d]}\n",
                     sym->str, sym->hash, othersym->str, othersym->hash));
      node->settv(tv);
      freenode->tv = othertv;
      while (othernode->next) othernode = othernode->next;
      othernode->next = freenode;
    }
  }
  else
  {
    node->settv(tv);
    DebugMem(Print("gsymtable insert no colliding {%s[hash %d]}\n", sym->str, sym->hash));
  }
}

void GSymTable::rehash()
{
  int oldsize = twoto(lsizenode);
  lsizenode += 1;
  int size = twoto(lsizenode);
  TSlot* oldslots = slots;
  slots = new (vm->alloc(size * sizeof(TSlot))) TSlot[size];
  lastfree = &slots[size - 1];
  for (int i = 0; i < oldsize; i++)
  {
    TSlot* slot = &oldslots[i];
    insert(symref(slot->tv->car()), slot->tv);
  }
  vm->free(oldslots, oldsize * sizeof(TSlot));
  DebugMem(Print("gsymtable rehash oldsize %d newsize %d\n", oldsize, size));
}

TSlot* GSymTable::getfreepos()
{
  if (lastfree)
  {
    if (lastfree->tv == NULL) return lastfree;
    while (lastfree > slots)
    {
      lastfree--;
      if (lastfree->tv == NULL) return lastfree;
    }
  }
  return NULL;
}

TSlot* GSymTable::getpos(SymPtr sym)
{
  return &slots[sym->hash & (twoto(lsizenode) - 1)];
}

void GSymTable::getval(SymPtr sym, ValueT* v)
{
  PairPtr pair = getslot(sym);
  if (pair == NULL)
    setundefined(v);
  else
    *v = pair->cdr();
}

void GSymTable::setslot(SymPtr sym, ValueT* val)
{
  PairPtr pair = getslot(sym);
  Assert(vm, pair, "undefined global variable %s", Ssstr(sym));
  pair->cdr(val);
  GC(vm)->checkBarrier(pair);
}

PairPtr GSymTable::getslot(SymPtr sym)
{
  TSlot* node = getpos(sym);
  for (;;)
  {
    if (node->tv == NULL) return NULL;
    if (sym == symref(node->tv->car())) return node->tv;
    if (node->next == NULL) return NULL;
    node = node->next;
  }
  Error(vm, "table internal error: won't reach here %s", Ssstr(sym));
}

void SCM::tokwform(VM* vm, ValueT* kw, ValueT* v, int line, bool anno)
{
  if (anno)
  {
    int line2 = annotateline(v);
    setpair(v, SCM::cons(vm, v, Snullref));
    SCM::toAnnotation(vm, v, line2);
  }
  else
    setpair(v, SCM::cons(vm, v, Snullref));
  Sgcvar1(vm, v2);
  *v2 = kw;
  if (anno)
    SCM::toAnnotation(vm, v2, line);
  setpair(v, SCM::cons(vm, v2, v));
  if (anno)
    SCM::toAnnotation(vm, v, line);
}

void SCM::toAnnotation(VM* vm, ValueT* v, int line)
{
  AnnotationObj* rec = Sr0(vm, AnnotationObj);
  rec->vt = v;
  rec->line = line;
  setannotate(v, rec);
}

uint SCM::hash(const char* s, int l, int seed)
{
  int h = seed ^ l;
  for (; l > 0; l--)
    h ^= ((h<<5) + (h>>2) + (byte)(s[l - 1]));
  return h;
}

void SCM::assert(bool flag, const char* info, const char* fmt, ...)
{
  if (!flag)
  {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    throw info;
  }
}

bool SCM::strcaseeql(const char* str1, int len1, const char* str2)
{
  if (len1 != strlen(str2))
    return false;
  for (int i = 0; i < len1; i++)
    if (std::tolower(str1[i]) != tolower(str2[i]))
      return false;
  return true;
}

bool SCM::eqp(ValueT* p1, ValueT* p2)
{
  if (p1 == p2) return true;
  if (vttype(p1) != vttype(p2)) return false;
  if (isnumber(p1)) return SCMMath::numequalp(p1, p2);
  if (ischar(p1)) return vtchar(p1) == vtchar(p2);
  if (isboolean(p1) || isnull(p1)) return true;
  if (isref(p1)) return refp(p1) == refp(p2);
  return false;
}

bool SCM::eqvp(ValueT* p1, ValueT* p2)
{
  return eqp(p1, p2);
}

bool SCM::equalp(ValueT* p1, ValueT* p2)
{
  if (p1 == p2) return true;
  if (vttype(p1) != vttype(p2)) return false;
  if (isref(p1) && refp(p1) == refp(p2)) return true;
  if (ischar(p1)) return vtchar(p1) == vtchar(p2);
  if (isboolean(p1) || isnull(p1)) return true;
  if (isnumber(p1)) return SCMMath::numequalp(p1, p2);
  if (issym(p1)) return symref(p1)->equalp(symref(p2));
  if (isstr(p1)) return strref(p1)->equalp(strref(p2));
  if (ispair(p1))
  {
    for (ValueT* p1p = p1, *p2p = p2;
         !isnull(p1p) && !isnull(p2p); p1p = Scdr(p1p), p2p = Scdr(p2p))
    {
      if (vttype(p1p) != vttype(p2p))
        return false;
      if (ispair(p1p))
      {
        if (!equalp(Scar(p1p), Scar(p2p)))
          return false;
      }
      else
      {
        if (!equalp(p1p, p2p))
          return false;
        break;
      }
    }
    return true;
  }
  if (isarray(p1))
  {
    ArrayObj* p1v = arrayref(p1), *p2v = arrayref(p2);
    if (p1v->array.n != p2v->array.n)
      return false;
    int count = p1v->array.n;
    for (int i = 0; i < count; i++)
      if (!equalp(p1v->get(i), p2v->get(i)))
        return false;
    return true;
  }
  return false;
}

bool SCM::listp(ValueT* a)
{
  return thlength(a) >= 0;
}

int SCM::length(VM* vm, ValueT* a, const char* prompt)
{
  int len = 0;
  PAIR_FOR(p, a)
  {
    AssertVT(vm, ispair(p), a, prompt?prompt:"not a pair");
    len++;
  }
  return len;
}

ValueT* SCM::listlastlen(ValueT* a, int* len)
{
  *len = 0;
  PAIR_FOR(p, a)
  {
    *len = *len+1;
    if (isnull(Scdr(p)))
      return Scar(p);
  }
  return NULL;
}

ValueT* SCM::listlast(ValueT* a)
{
  PAIR_FOR(p, a)
    if (isnull(Scdr(p)))
      return Scar(p);
  return NULL;
}

int SCM::length(ValueT* a)
{
  int len = 0;
  PAIR_FOR(p, a)
    len++;
  return len;
}

int SCM::thlength(ValueT* p)
{
  int len = 0;
  ValueT* tortoise = p, * hare = p;
  do {
    if (isnull(hare))
      return len;
    if (!ispair(hare))
      return -1;
    hare = Scdr(hare);
    len++;
    if (isnull(hare))
      return len;
    if (!ispair(hare))
      return -1;
    hare = Scdr(hare);
    len++;
    tortoise = Scdr(tortoise);
  } while(tortoise != hare);
  return -1;
}

void SCM::list2vector(VM* vm, ValueT* lst, ValueT* out)
{
  int i = thlength(lst);
  if (i < 0)
    Serrorvt(vm, lst, "list?: not a list");
  ArrayObj* arr = NULL;
  if (i != 0)
    setarray(out, arr = Sr2(vm, ArrayObj, vm, i));
  else
    setarray(out, arr = Sr0(vm, ArrayObj));
  int j = 0;
  PAIR_FOR(p, lst)
    arr->set(j++, Scar(p));
}

void SCM::copystripanno(VM* vm, ValueT* out, ValueT* annos)
{
  if (isnull(annos))
  {
    setnull(out);
    return;
  }
  ValueT* annovt = annotatevt(annos);
  if (ispair(annovt))
  {
    setpair(out, SCM::cons(vm, Snullref, Snullref));
    copystripanno(vm, Scar(out), Scar(annovt));
    copystripanno(vm, Scdr(out), Scdr(annovt));
  }
  else if (isarray(annovt))
  {
    ArrayObj* arr = arrayref(annovt), * arrout;
    VecT<ValueT>* array = &arr->array;
    setarray(out, arrout = Sr2(vm, ArrayObj, vm, array->n));
    VEC_FOR(i, array)
      copystripanno(vm, arrout->array.getptr(i), array->getptr(i));
  }
  else
    *out = annovt;
}

ValueT* SCM::copylist(VM* vm, ValueT* out, ValueT* a)
{
  PAIR_FOR (ap, a)
  {
    if (ispair(ap))
    {
      setpair(out, SCM::cons(vm, Scar(ap), Snullref));
      out = Scdr(out);
    }
    else
    {
      *out = ap;
      break;
    }
  }
  return out;
}

void SCM::vector2cons(VM* vm, ValueT* out, ArrayObj* arr)
{
  *out = Snullref;
  int count = arr->array.n;
  if (count > 0)
  {
    for (int i = count-1; i>=0; i--)
      setpair(out, cons(vm, arr->get(i), out));
  }
}

PairPtr SCM::cons(VM* vm, ValueT* h, ValueT* t)
{
  PairPtr pair = Cache(vm)->getonepair();
  if (pair == NULL)
    return Sr2(vm, PairObj, h, t);
  else
  {
    pair->car(h);
    pair->cdr(t);
    return (PairPtr)ObjGroup(vm)->recobj(pair);
  }
}

ValueT* SCM::lastappend(VM* vm, ValueT* h, ValueT* t)
{
  AssertVT(vm, ispair(h), h, "append: contract violation, list?");
  *Scdr(h) = t;
  if (isnull(t))
    return h;
  if (!ispair(t))
    return t;
  PAIR_FOR(p, t)
    if (isnull(Scdr(p)))
    {
      h = p;
      break;
    }
  return h;
}

ValueT* SCM::append(VM* vm, ValueT* h, ValueT* t)
{
  if (isnull(h))
  {
    *h = t;
    return h;
  }
  else
  {
    AssertVT(vm, ispair(h), h, "append: contract violation, list?");
    PAIR_FOR(ph, h)
      if (!ispair(Scdr(ph)))
      {
        h = ph;
        break;
      }
    return lastappend(vm, h, t);
  }
}

PairPtr SCM::list(VM* vm, ValueT* a)
{
  return cons(vm, a, Snullref);
}

void SCM::reverselist(VM* vm, ValueT* lst)
{
  if (isnull(lst)) return;
  AssertVT(vm, SCM::listp(lst), lst, "reverse: not a list");
  ValueT b = lst;
  ValueT a = Scdr(lst);
  pairref(&b)->cdr(Snullref);
  ValueT tmp;
  for (; !isnull(&a); b = a, a = tmp)
  {
    tmp = Scdr(&a);
    pairref(&a)->cdr(&b);
  }
  *lst = b;
}

PairPtr SCM::list(VM* vm, ValueT* a, ValueT* b)
{
  Sgcvar1(vm, pbv);
  setpair(pbv, list(vm, b));
  return cons(vm, a, pbv);
}

PairPtr SCM::list(VM* vm, ValueT* a, ValueT* b, ValueT* c)
{
  Sgcvar1(vm, pbv);
  setpair(pbv, list(vm, b, c));
  return cons(vm, a, pbv);
}

PairPtr SCM::list(VM* vm, ValueT* a, ValueT* b, ValueT* c, ValueT* d)
{
  Sgcvar1(vm, pbv);
  setpair(pbv, list(vm, b, c, d));
  return cons(vm, a, pbv);
}

NumBigObj* SCM::numbigdiv(VM* vm, NumBigObj* a, NumBigObj* b)
{
  NumBigDivRes res = SCMMath::numbigdivabs(vm, a, b);
  res.quot->sign = (a->sign == b->sign) ? 1 : -1;
  return res.quot;
}

NumBigObj* SCM::numbigmod(VM* vm, NumBigObj* a, NumBigObj* b)
{
  NumBigDivRes res = SCMMath::numbigdivabs(vm, a, b);
  res.rem->sign = a->sign;
  return res.rem;
}

scm_uint SCM::div128by64(scm_uint u_hi, scm_uint u_lo, scm_uint divisor, scm_uint* quot_lo)
{
  scm_uint rem = u_hi;
  scm_uint q = 0;
  scm_uint part1 = (rem << INT_BITS_HALF) | (u_lo >> INT_BITS_HALF);
  scm_uint q1 = part1 / divisor;
  rem = part1 % divisor;
  scm_uint part0 = (rem << INT_BITS_HALF) | (u_lo & UINT_MASK_LOW);
  scm_uint q0 = part0 / divisor;
  rem = part0 % divisor;
  *quot_lo = (q1 << 32) | q0;
  return rem;
}

void NumBigObj::tostr(VM* vm, Lbuffer* buf)
{
  buf->reset();
  if (iszero()) { buf->put('0'); return; }
  Sgcvar1(vm, tmpvt);
  NumBigObj* tmp = copy(vm);
  setnumbig(tmpvt, tmp);
  while (!tmp->iszero())
  {
    scm_uint rem = 0;
    for (int i = tmp->len - 1; i >= 0; --i)
    {
      scm_uint cur_limb = (scm_uint)tmp->data[i];
      scm_uint new_limb_quot = 0;
      rem = SCM::div128by64(rem, cur_limb, UINT_BASE, &new_limb_quot);
      tmp->data[i] = (scm_int)new_limb_quot;
    }
    tmp->normalize();
    bool is_last_chunk = tmp->iszero();
    for (int k = 0; k < UINT_BASE_LEN; ++k)
    {
      buf->put('0' + (rem % 10));
      rem /= 10;
      if (is_last_chunk && rem == 0)
        break;
    }
  }
  if (sign < 0) buf->put('-');
  buf->reverse();
}

int NumBigObj::bitlength()
{
  if (len == 1 && data[0] == 0) return 0;
  int top_idx = len - 1;
  scm_uint top_limb = (scm_uint)data[top_idx];
  scm_int bits = top_idx * INT_BITS;
  while (top_limb > 0)
  {
    bits++;
    top_limb >>= 1;
  }
  return bits;
}

NumBigObj* NumBigObj::addabs(VM* vm, NumBigObj* other)
{
  NumBigObj* res = vm->newnumbig(len+1, 1);
  scm_uint carry = 0;
  for (int i = 0; i < len; ++i)
  {
    scm_uint ua = (scm_uint)data[i];
    scm_uint ub = (i < other->len) ? (scm_uint)other->data[i] : 0;
    scm_uint sum = ua + ub + carry;
    carry = (sum < ua) || (sum == ua && carry > 0) ? 1 : 0;
    res->data[i] = (scm_int)sum;
  }
  if (carry)
    res->data[len] = (scm_int)carry;
  return res->normalize();
}

NumBigObj* NumBigObj::add(VM* vm, NumBigObj* other)
{
  if (iszero()) return other->copy(vm);
  if (other->iszero()) return this;
  if (sign == other->sign) {
    NumBigObj* r = addabs(vm, other);
    r->sign = sign;
    return r;
  }
  int c = cmpabs(other);
  if (c >= 0) {
    NumBigObj* r = subabs(vm, other);
    r->sign = sign;
    return r;
  }
  else {
    NumBigObj* r = other->subabs(vm, this);
    r->sign = other->sign;
    return r;
  }
}

NumBigObj* NumBigObj::subabs(VM* vm, NumBigObj* other)
{
  NumBigObj* res = vm->newnumbig(len, 1);
  scm_uint borrow = 0;
  for (int i = 0; i < len; ++i)
  {
    scm_uint ua = (scm_uint)data[i];
    scm_uint ub = (i < other->len) ? (scm_uint)other->data[i] : 0;
    scm_uint diff = ua - ub - borrow;
    borrow = (ua < ub) || (ua == ub && borrow > 0) ? 1 : 0;
    res->data[i] = (scm_int)diff;
  }
  return res->normalize();
}

NumBigObj* NumBigObj::mulabs(VM* vm, NumBigObj* other)
{
  if (iszero() || other->iszero()) return Sbigzero;
  int res_len = len + other->len;
  NumBigObj* res = vm->newnumbig(res_len, 1);
  for (int i = 0; i < len; ++i)
  {
    scm_uint ua = (scm_uint)data[i];
    if (ua == 0) continue;
    scm_uint ua_lo = ua & UINT_MASK_LOW;
    scm_uint ua_hi = ua >> 32;
    scm_uint carry = 0;
    for (int j = 0; j < other->len; ++j)
    {
      scm_uint ub = (scm_uint)other->data[j];
      scm_uint ub_lo = ub & UINT_MASK_LOW;
      scm_uint ub_hi = ub >> 32;
      scm_uint p0 = ua_lo * ub_lo; // 0-63
      scm_uint p1 = ua_lo * ub_hi; // 32-95
      scm_uint p2 = ua_hi * ub_lo; // 32-95
      scm_uint p3 = ua_hi * ub_hi; // 64-127
      scm_uint p1_p2 = p1 + p2;
      scm_uint p1_p2_carry = (p1_p2 < p1) ? 1 : 0;
      scm_uint p_low = p0 + (p1_p2 << 32);
      scm_uint p_high = p3 + (p1_p2 >> 32) + (p1_p2_carry << 32);
      if (p_low < p0) p_high += 1;
      scm_uint current_dest = (scm_uint)res->data[i + j];
      p_low += current_dest;
      if (p_low < current_dest) p_high += 1;
      p_low += carry;
      if (p_low < carry) p_high += 1;
      res->data[i + j] = (scm_int)p_low;
      carry = p_high;
    }
    scm_int k = i + other->len;
    while (carry > 0 && k < res_len)
    {
      scm_uint cur = (scm_uint)res->data[k];
      scm_uint sum = cur + carry;
      res->data[k] = (scm_int)sum;
      carry = (sum < cur) ? 1 : 0;
      k++;
    }
  }
  return res->normalize();
}

NumBigObj* NumBigObj::mul(VM* vm, NumBigObj* other)
{
  if (iszero() || other->iszero()) return Sbigzero;
  NumBigObj* res = mulabs(vm, other);
  res->sign = (sign == other->sign) ? 1 : -1;
  return res;
}

NumBigObj NumBigObj::zero(1, 1);

class ResetNumBigSign {
public:
  ResetNumBigSign(NumBigObj* n):num(n) {}
  ~ResetNumBigSign() { num->sign = -num->sign; }
  NumBigObj* num;
};

NumBigObj* NumBigObj::sub(VM* vm, NumBigObj* other)
{
  other->sign = -other->sign;
  ResetNumBigSign guard(other);
  return add(vm, other);
}

NumBigObj* NumBigObj::copy(VM* vm)
{
  NumBigObj* tmp = vm->newnumbig(len, sign);
  for (int i = 0; i < len; ++i) tmp->data[i] = data[i];
  return tmp;
}

void VM::stepmarkframes()
{
  //getgc()->checkTrace(frames);
}

void VM::regfullmark()
{
  //getgc()->checkTrace(frames);
}

void VM::checkgc()
{
  if (getgc()->state == GCSNone)
  {
    getobjgroup()->swapobjset();
    getgc()->startstep();
    getgc()->singlestep();
  }
  else
    getgc()->singlestep();
}

static ValueT scmcallcproc(VM* vm, NativeProcObj* np, ValueT* args)
{
  switch(np->argnum) {
  case 0: return (*np->cf.cp0)(vm);
  case 1: return (*np->cf.cp1)(vm, args);
  case 2: return (*np->cf.cp2)(vm, args, args+1);
  case 3: return (*np->cf.cp3)(vm, args, args+1, args+2);
  case 4: return (*np->cf.cp4)(vm, args, args+1, args+2, args+3);
  case 5: return (*np->cf.cp5)(vm, args, args+1, args+2, args+3, args+4);
  case 6: return (*np->cf.cp6)(vm, args, args+1, args+2, args+3, args+4, args+5);
  case 7: return (*np->cf.cp7)(vm, args, args+1, args+2, args+3, args+4, args+5, args+6);
  case 8: return (*np->cf.cp8)(vm, args, args+1, args+2, args+3, args+4, args+5, args+6, args+7);
  case 9: return (*np->cf.cp9)(vm, args, args+1, args+2, args+3, args+4, args+5, args+6, args+7, args+8);
  case 10: return (*np->cf.cp10)(vm, args, args+1, args+2, args+3, args+4, args+5, args+6, args+7, args+8, args+9);
  default:
    Error(vm, "error happened too many ffi arguments %s", Ssstr(np->var));
    return Svoidref;
  }
}

void VM::printccode(FILE *f, LambdaPtr lambda)
{
  if (lambda->vars)
  {
    if (lambda->vars->local.size > 0)
    {
      fprintf(f, "%-7s", "Local:");
      VEC_FOR(j, &lambda->vars->local)
        fprintf(f, "[%d,%s] ", (j+1), Ssstr(lambda->vars->local[j]));
      fprintf(f, "\n");
    }
    if (lambda->vars->ovar.size > 0)
    {
      fprintf(f, "%-7s", "Out:");
      VEC_FOR(j, &lambda->vars->ovar)
      {
        OuterVar* ov = lambda->vars->ovar.getptr(j);
        fprintf(f, "[%s,%d,%s] ", Ssstr(ov->name), ov->idx, ov->islocal?"true":"false");
      }
      fprintf(f, "\n");
    }
  }
  if (lambda->ks.size > 0)
  {
    fprintf(f, "%-7s", "Const:");
    VEC_FOR(j, &lambda->ks)
    {
      fprintf(f, "[%d,", j);
      OutputPortFileObj ofile;
      ofile.file = f;
      printvalue0(&ofile, lambda->ks.getptr(j), true);
      fprintf(f, "] ");
    }
    fprintf(f, "\n");
  }
  printccode0(f, lambda, -1);
}

#define CODE_FMT "%-20s"
#define PrintCode(CODE) fprintf(f, CODE_FMT, #CODE)
#define OFFSET_FMT "%-3d"
#define BLANK_FMT "    "
#define PrintOffset(A) fprintf(f, OFFSET_FMT "" BLANK_FMT "" BLANK_FMT, A)
#define PrintOffset2(A, B) fprintf(f, OFFSET_FMT " " OFFSET_FMT "" BLANK_FMT, A, B)
#define PrintOffset3(A, B, C) fprintf(f, OFFSET_FMT " " OFFSET_FMT " " OFFSET_FMT, A, B, C)

void VM::printccode0(FILE *f, LambdaPtr lambda, int pc)
{
  for (int n = lambda->getcodestart(); n >= 0; n--)
  {
    if (pc == n)
      fprintf(f, "[PC]");
    fprintf(f, "[%d\t]", n);
    Instruction i = lambda->fetchcode()->get(n);
    byte op = GET_OP(i);
    switch(op) {
    case OP_CONSEXT: {
      int A, B, C; getcode_cons2(i, A, B, C);
      PrintCode(CONSEXT);
      PrintOffset3(A, B, C);
      fprintf(f, "\t; set %d as (cons %d %d)", A, B, C);
      break;
    }
    case OP_CONS: {
      int A; getcode_cons(i, A);
      PrintCode(CONS);
      PrintOffset(A);
      fprintf(f, "\t\t; set %d as (cons %d %d)", A, A, A+1);
      break;
    }
    case OP_LIST: {
      int A; getcode_list(i, A);
      PrintCode(LIST);
      PrintOffset(A);
      fprintf(f, "\t; set %d as (list %d)", A, A);
      break;
    }
    case OP_LISTK: {
      int A, B; getcode_listk(i, A, B);
      PrintCode(LISTK);
      PrintOffset(A);
      PrintOffset(B);
      fprintf(f, "\t; set %d as ", A);
      fprintf(f, "(list ");
      OutputPortFileObj ofile;
      ofile.file = f;
      printvalue0(&ofile, this->kconst[B], true);
      fprintf(f, " %d)", A);
      break;
    }
    case OP_LIST2VEC: {
      int A; getcode_list2vec(i, A);
      PrintCode(LIST2VECTOR);
      PrintOffset(A);
      fprintf(f, "\t; set %d as (list->vector %d)", A, A);
      break;
    }
    case OP_APPEND: {
      int A; getcode_append(i, A);
      PrintCode(APPEND);
      PrintOffset(A);
      fprintf(f, "\t; set %d as (append %d %d)", A, A, A+1);
      break;
    }
    case OP_APPEND0: {
      int A; getcode_append0(i, A);
      PrintCode(APPEND0);
      PrintOffset(A);
      fprintf(f, "\t; set %d as (append %d)", A, A);
      break;
    }
    case OP_APPENDEXT: {
      int A, B, C; getcode_append2(i, A, B, C);
      PrintCode(APPENDEXT);
      PrintOffset(A);
      PrintOffset(B);
      PrintOffset(C);
      fprintf(f, "\t; set %d as (append %d %d)", A, B, C);
      break;
    }
    case OP_ASSIGN: {
      int target, from;getcode_assign(i, target, from);
      PrintCode(ASSIGN);
      PrintOffset2(target, from);
      fprintf(f, "\t; set %d as ", target);
      OutputPortFileObj ofile;
      ofile.file = f;
      printvalue0(&ofile, lambda->getk(from), true);
      break;
    }
    case OP_TAILCALLAPP: {
      int k, len;getcode_tailcallapp(i, k, len);
      PrintCode(TAILCALLAPP);
      PrintOffset2(k, len);
      fprintf(f, "\t; tail call %d upto len %d", k, len);
      break;
    }
    case OP_CALLAPP: {
      int k, len;getcode_callapp(i, k, len);
      PrintCode(CALLAPP);
      PrintOffset2(k, len);
      fprintf(f, "\t; set %d as the result of calling %d upto len %d", k, k, len);
      break;
    }
    case OP_RETURN: {
      PrintCode(RETURN);
      break;
    }
    case OP_JUMPLABEL: {
      int target;getcode_jmplabel(i, target);
      PrintCode(JUMPLABEL);
      PrintOffset(target);
      break;
    }
    case OP_VARREFGLOBAL: {
      int target, from;getcode_varrefglobal(i, target, from);
      ValueT* k = lambda->getk(from);
      SymPtr sym = symref(k);
      PrintCode(VARREFGLOBAL);
      PrintOffset2(target, from);
      fprintf(f, "\t; set %d as %s", target, Ssstr(sym));
      break;
    }
    case OP_VARREFLOCAL: {
      int target, from;getcode_varreflocal(i, target, from);
      SymPtr var = lambda->vars->reflocal(from);
      PrintCode(VARREFLOCAL);
      PrintOffset2(target, from);
      fprintf(f, "\t; set %d as %s", target, Ssstr(var));
      break;
    }
    case OP_VARREFOVAR: {
      int target, from;getcode_varrefovar(i, target, from);
      OuterVar* ovar = lambda->vars->refovar(from);
      PrintCode(VARREFOVAR);
      PrintOffset2(target, from);
      fprintf(f, "\t; set %d as %s", target, Ssstr(ovar->name));
      break;
    }
    case OP_DEFGLOBAL: {
      int target, from;getcode_defglobal(i, target, from);
      ValueT* k = lambda->getk(target);
      SymPtr sym = symref(k);
      PrintCode(DEFGLOBAL);
      PrintOffset2(target, from);
      fprintf(f, "\t; %s", Ssstr(sym));
      break;
    }
    case OP_SETLOCAL: {
      int target, from;getcode_setlocal(i, target, from);
      PrintCode(SETLOCAL);
      PrintOffset2(target, from);
      SymPtr var = lambda->vars->reflocal(target);
      fprintf(f, "\t; set %s(%d) from %d", Ssstr(var), target, from);
      break;
    }
    case OP_SETOVAR: {
      int target, from;getcode_setovar(i, target, from);
      OuterVar* ovar = lambda->vars->refovar(target);
      PrintCode(SETOVAR);
      PrintOffset2(target, from);
      fprintf(f, "\t; set %s from %d", Ssstr(ovar->name), from);
      break;
    }
    case OP_SETGLOBAL: {
      int target, from;getcode_setglobal(i, target, from);
      ValueT* k = lambda->getk(target);
      SymPtr sym = symref(k);
      PrintCode(SETGLOBAL);
      PrintOffset2(target, from);
      fprintf(f, "\t; set %s from %d", Ssstr(sym), from);
      break;
    }
    case OP_IFFALSEJUMP: {
      int A, B;getcode_iffalsejmp(i, A, B);
      PrintCode(IFFALSEJUMP);
      PrintOffset2(A, B);
      fprintf(f, "\t; if %d == #f then jumpto %d", B, A);
      break;
    }
    case OP_LAMBDA: {
      int target, k;getcode_lambda(i, target, k);
      PrintCode(LAMBDA);
      PrintOffset2(target, k);
      fprintf(f, "\t; set %d as a closure from lambda %d", target, k);
      break;
    }
    default: Error(this, "unknown op %d", op);
    }
    fprintf(f, "\n");
  }
}

#define pcnext() --pc
#define stkvt(offset) (base+(offset))

void VM::printframe()
{
  Stack* stk = Stk(this);
  int n = 0;
  CallFrame* frm  = stk->curfrm;
  while (!stk->isbasefrm(frm))
  {
    ValueT* base = frm->base;
    ValueT* basevt = stkvt(0);
    ClosurePtr call = closureref(basevt);
    LambdaPtr lambda = call->lambda;
    fprintf(stderr, "\n");
    fprintf(stderr, "Frame %d\n", n);
    printccode0(stderr, lambda, frm->pc);
    fprintf(stderr, "Frame %d End\n\n", n++);
    frm = frm->prev;
  }
}

struct CallAppState {
  CallAppState(): callcc(false), unwind(NULL), fromapply(false) {}
  bool callcc;
  UnWindFunc unwind;
  bool fromapply;
};

static void shrinkarity(VM* vm, ValueT* base, int len, int argnum)
{
  if (len >= argnum)
  {
    setpair(stkvt(len), SCM::cons(vm, stkvt(len), Snullref));
    for (int i = len-1; i >= argnum; i--)
      setpair(stkvt(i), SCM::cons(vm, stkvt(i), stkvt(i+1)));
  }
  else
    setnull(stkvt(argnum));
}

static void expandarity(VM* vm, ValueT* base, int argnum, bool argrest, const char* fname)
{
  int len = 1;
  Sgcvar1(vm, arity);
  *arity = stkvt(len);
  if (argrest)
  {
    for (int i = len; i < argnum; i++, arity = Scdr(arity))
    {
      Assert(vm, ispair(arity), "%s: arity mismatch, provide %d, need at least %d",
             fname, i, argnum-1);
      *stkvt(i) = Scar(arity);
    }
    *stkvt(argnum) = arity;
  }
  else
  {
    for (int i = len; i <= argnum; i++, arity = Scdr(arity))
    {
      Assert(vm, ispair(arity), "%s: arity mismatch, provide %d, need %d",
             fname, i, argnum);
      *stkvt(i) = Scar(arity);
    }
    Assert(vm, isnull(arity), "%s: arity mismatch, provide more then need %d",
           fname, argnum);
  }
}

static void ensurearity(VM* vm, ValueT* base, int len, int argnum, bool argrest, const char* fname, bool fromapply)
{
  if (fromapply)
    expandarity(vm, base, argnum, argrest, fname);
  else
  {
    if (argrest)
    {
      Assert(vm, len >= argnum - 1, "%s: arity mismatch, provide %d need at least %d", fname, len, argnum - 1);
      shrinkarity(vm, base, len, argnum);
    }
    else
      Assert(vm, len == argnum, "%s: arity mismatch, provide %d need %d", fname, len, argnum);
  }
}

static void shrinkapplyarity(VM* vm, ValueT* base, int* olen)
{
  int len = *olen;
  Assert(vm, len >= 2, "apply: arity mismatch, provide %d need at least 2", len);
  Stack* stk = Stk(vm);
  ValueT* lastarg = stkvt(len);
  AssertVT(vm, SCM::listp(lastarg), lastarg, "apply: contract violation list?");
  for (int i = len-1; i >= 2; i--)
    setpair(stkvt(i), SCM::cons(vm, stkvt(i), stkvt(i+1)));
  *stkvt(0) = stkvt(1);
  *stkvt(1) = stkvt(2);
  stk->setvoid(stkvt(2), stkvt(len));
  *olen = 1;
}

static void flatshrinkapplyarity(VM* vm, ValueT* base, ValueT* arg)
{
  // flatten
  int arglen = SCM::length(vm, arg);
  Assert(vm, arglen >= 2, "apply: arity mismatch, provide %d need at least 2", arglen);
  Sgcvar2(vm, arrarg, out);
  ArrayObj* arr = NULL;
  setarray(arrarg, arr = Sr2(vm, ArrayObj, vm, arglen));
  int i = 0;
  PAIR_FOR(p, arg)
    arr->set(i++, Scar(p));
  // shrink
  ValueT* lastarg = arr->get(i-1);
  AssertVT(vm, SCM::listp(lastarg), lastarg, "apply: contract violation list?");
  for (int j = arglen-2; j >= 1; j--)
  {
    ValueT* ki = arr->get(j), *ki1 = arr->get(j+1);
    setpair(ki, SCM::cons(vm, ki, ki1));
  }
  *stkvt(0) = arr->get(0);
  *stkvt(1) = arr->get(1);
}

static void checkcallcc(VM* vm, CallFrame* oldfrm, ValueT* base, int len, CallAppState* callstate)
{
  static const char* METHOD = "call-with-current-continuation";
  Stack* stk = Stk(vm);
  ValueT* cc = stkvt(1);
  if (callstate->fromapply)
  {
    AssertVT(vm, ispair(cc), cc, "%s: internal error in apply", METHOD);
    AssertVT(vm, isnull(Scdr(cc)), cc, "%s: internal only needs one parameter", METHOD);
    *cc = Scar(cc);
    callstate->fromapply = false;
  }
  else
    Assert(vm, len==1, "%s: needs only 1 argument", METHOD);
  bool argrest = false;
  if (isclosure(cc))
  {
    ClosurePtr ccc = closureref(cc);
    Assert(vm, ccc->lambda->argnum == 1 || (ccc->lambda->argnum == 2 && ccc->lambda->argrest),
           "%s: closure object parameter wrong", METHOD);
    argrest = ccc->lambda->argrest;
  }
  else if (isnativeproc(cc))
  {
    NativeProcObj* ccproc = nativeprocref(cc);
    Assert(vm, ccproc->argnum == 1 || (ccproc->argnum == 2 && ccproc->argrest),
           "%s: closure object parameter wrong", METHOD);
    argrest = ccproc->argrest;
  }
  else
    Error(vm, "%s: needs closure object", METHOD);
  *stkvt(0) = cc;
  setcontinuation(stkvt(1), Sr2(vm, ContinuationObj, oldfrm, base));
  if (argrest) *stkvt(2) = Snullref;
  callstate->callcc = true;
}

static void closeiport(VM* vm, CallFrame* frm, ValueT* val)
{
  InputPortObj* iport = iportref(val);
  iport->close();
}

static void closeoport(VM* vm, CallFrame* frm, ValueT* val)
{
  OutputPortObj* oport = oportref(val);
  oport->close();
}

static void closeoportstr(VM* vm, CallFrame* frm, ValueT* val)
{
  OutputPortStrObj* oport = oportstrref(val);
  oport->close();
  StrObj* str = vm->strintern(oport->strbuf.buf, oport->strbuf.count);
  ValueT out;
  setstr(&out, str);
  vm->ac0 = *frm->start = out;
}

static void callwithfile(VM* vm, ValueT* base, int len, CallAppState* callstate, ValueT** procp, ValueT** filep, const char* METHOD)
{
  Stack* stk = Stk(vm);
  ValueT* cw = stkvt(1);
  ValueT* proc = NULL, *file = NULL;
  if (callstate->fromapply)
  {
    AssertVT(vm, ispair(cw), cw, "%s: internal error in apply", METHOD);
    file = Scar(cw);
    cw = Scdr(cw);
    AssertVT(vm, ispair(cw), cw, "%s: needs 2 arguments", METHOD);
    proc = Scar(cw);
    AssertVT(vm, isnull(Scdr(cw)), cw, "%s: two much arguments other than 2", METHOD);
    callstate->fromapply = false;
  }
  else
  {
    Assert(vm, len==2, "%s: needs 2 arguments, not %d", METHOD, len);
    file = stkvt(1);
    proc = stkvt(2);
  }
  AssertVT(vm, isstr(file), file, "%s: not a string", METHOD);
  if (isnativeproc(proc))
  {
    NativeProcObj* ccproc = nativeprocref(proc);
    Assert(vm, ccproc->argnum == 1 || (ccproc->argnum == 2 && ccproc->argrest),
           "%s: closure object parameter wrong", METHOD);
  }
  else if (isclosure(proc))
  {
    ClosurePtr ccc = closureref(proc);
    Assert(vm, ccc->lambda->argnum == 1 || (ccc->lambda->argnum == 2 && ccc->lambda->argrest),
           "%s: closure object parameter wrong", METHOD);
  }
  else
    AssertVT(vm, iscontinuation(proc), proc, "%s: not a closure", METHOD);
  *procp = proc;
  *filep = file;
}

static void callwithinputfile(VM* vm, CallFrame* frm, ValueT* base, int* olen, CallAppState* callstate)
{
  const static char* METHOD = "call-with-input-file";
  Stack* stk = Stk(vm);
  ValueT* cw = stkvt(1);
  ValueT* proc = NULL, *file = NULL;
  callwithfile(vm, base, *olen, callstate, &proc, &file, METHOD);
  StrPtr fn = strref(file);
  const char* filename = Ssstr(fn);
  FILE* fhandle = fopen(filename, "r");
  if (fhandle == NULL)
  {
    Print("%s: error read file %s", METHOD, filename);
    throw "ReadError: failed to read file";
  }
  *stkvt(0) = proc;
  InputPortObj* iport = NULL;
  setiport(stkvt(1), iport = Sr0(vm, InputPortObj));
  iport->file = fhandle;
  iport->fname = fn;
  callstate->unwind = &closeiport;
  *olen = 1;
}

static void callwithoutputfile(VM* vm, CallFrame* frm, ValueT* base, int* olen, CallAppState* callstate)
{
  const static char* METHOD = "call-with-output-file";
  Stack* stk = Stk(vm);
  ValueT* cw = stkvt(1);
  ValueT* proc = NULL, *file = NULL;
  callwithfile(vm, base, *olen, callstate, &proc, &file, METHOD);
  StrPtr fn = strref(file);
  const char* filename = Ssstr(fn);
  FILE* fhandle = fopen(filename, "w");
  if (fhandle == NULL)
  {
    Print("%s: cannot create file %s", METHOD, filename);
    throw "Error: failed ";
  }
  *stkvt(0) = proc;
  OutputPortFileObj* oport = NULL;
  setoport(stkvt(1), oport = Sr0(vm, OutputPortFileObj));
  oport->file = fhandle;
  oport->fname = fn;
  callstate->unwind = &closeoport;
  *olen = 1;
}

static void callwithoutputstr(VM* vm, CallFrame* frm, ValueT* base, int* olen, CallAppState* callstate)
{
  int len = *olen;
  const static char* METHOD = "call-with-output-string";
  Stack* stk = Stk(vm);
  ValueT* cw = stkvt(1);
  ValueT* proc = NULL;
  if (callstate->fromapply)
  {
    AssertVT(vm, ispair(cw), cw, "%s: internal error in apply", METHOD);
    proc = Scar(cw);
    AssertVT(vm, isnull(Scdr(cw)), cw, "%s: two much arguments", METHOD);
    callstate->fromapply = false;
  }
  else
  {
    Assert(vm, len==1, "%s: needs 1 arguments, not %d", METHOD, len);
    proc = stkvt(1);
  }
  if (isnativeproc(proc))
  {
    NativeProcObj* ccproc = nativeprocref(proc);
    Assert(vm, ccproc->argnum == 1 || (ccproc->argnum == 2 && ccproc->argrest),
           "%s: closure object parameter wrong", METHOD);
  }
  else if (isclosure(proc))
  {
    ClosurePtr ccc = closureref(proc);
    Assert(vm, ccc->lambda->argnum == 1 || (ccc->lambda->argnum == 2 && ccc->lambda->argrest),
           "%s: closure object parameter wrong", METHOD);
  }
  *stkvt(0) = proc;
  setoport(stkvt(1), Sr1(vm, OutputPortStrObj, vm));
  callstate->unwind = &closeoportstr;
  *olen = 1;
}

static CallFrame* ctorclosurefrm(VM* vm, Instruction i, CallFrame* frm, ValueT* base, int len, CallAppState* callstate)
{
  Stack* stk = Stk(vm);
  ClosurePtr newcall = closureref(base);
  ensurearity(vm, base, len, newcall->lambda->argnum, newcall->lambda->argrest, "", callstate->fromapply);
  if (GET_OP(i) == OP_CALLAPP || callstate->callcc || callstate->unwind)
  {
    frm = stk->newfrm(frm, base, newcall->lambda->argnum, newcall->lambda->top);
    frm->start = base;
  }
  else
  {
    frm->seg->closeouterval(vm, frm, frm->base);
    frm->top = frm->base + 1 + newcall->lambda->top;
    if (frm->seg->frozen > 0 || frm->top >= frm->seg->end())
    {
      StackSegment* seg = Sr0(vm, StackSegment);
      frm->seg = seg;
      frm->base = seg->first();
      frm->top = frm->base + 1 + newcall->lambda->top;
      Assert(vm, frm->top < seg->end(), "alloc stack seg error in ctorfrm, arity: %d too big", newcall->lambda->top);
    }
    int j = 0;
    for (; j <= newcall->lambda->argnum; j++)
      *(frm->base+j) = base+j;
  }
  stk->setvoid(frm->base+newcall->lambda->argnum+1, frm->top-1);
  return frm;
}

static void checkcalliofile(VM* vm, CallFrame* frm, ValueT* val, CallAppState* callstate)
{
  if (callstate->unwind)
  {
    OuterVal* ov = frm->seg->findouterval(vm, val);
    ov->unwind = callstate->unwind;
  }
}

void VM::execute(CallFrame* frm)
{
  Stack* stk = Stk(this);
  ValueT* base = frm->base;
  ClosurePtr call = closureref(base);
  LambdaPtr lambda = call->lambda;
  int pc = lambda->getcodestart();
  Instruction i;
  int op = 0;
 loop:
  if (pc < 0)
    return;
  frm->setpc(pc);
  i = lambda->fetchi(pc);
  //checkgc();
  int icode = GET_OP(i);
  switch(icode) {
  case OP_CONSEXT: {
    int A, B, C; getcode_cons2(i, A, B, C);
    setpair(stkvt(A), SCM::cons(this, stkvt(B), stkvt(C)));
    break;
  }
  case OP_CONS: {
    int A; getcode_cons(i, A);
    setpair(stkvt(A), SCM::cons(this, stkvt(A), stkvt(A+1)));
    break;
  }
  case OP_LIST: {
    int A; getcode_list(i, A);
    setpair(stkvt(A), SCM::list(this, stkvt(A)));
    break;
  }
  case OP_LISTK: {
    int A, B; getcode_listk(i, A, B);
    setpair(stkvt(A), SCM::list(this, this->kconst[B], stkvt(A)));
    break;
  }
  case OP_LIST2VEC: {
    int A; getcode_list2vec(i, A);
    Sgcvar1(this, out);
    SCM::list2vector(this, stkvt(A), out);
    arrayref(out)->setimmutable();
    *stkvt(A) = out;
    break;
  }
  case OP_APPEND0: {
    int A; getcode_append0(i, A);
    SCM::append(this, stkvt(A), Snullref);
    break;
  }
  case OP_APPEND: {
    int A; getcode_append(i, A);
    SCM::append(this, stkvt(A), stkvt(A+1));
    break;
  }
  case OP_APPENDEXT: {
    int A, B, C; getcode_append2(i, A, B, C);
    SCM::append(this, stkvt(B), stkvt(C));
    *stkvt(A) = stkvt(B);
    break;
  }
  case OP_ASSIGN: {
    int target, from;getcode_assign(i, target, from);
    *stkvt(target) = lambda->getk(from);
    break;
  }
  case OP_JUMPLABEL:
    getcode_jmplabel(i, pc);
    goto loop;
  case OP_VARREFGLOBAL: {
    int target, from;getcode_varrefglobal(i, target, from);
    ValueT* k = lambda->getk(from);
    SymPtr name = symref(k);
    ValueT val;
    GEnv(this)->getval(name, &val);
    Assert(this, !isundefined(&val), "undefined global var %s", Ssstr(name));
    *stkvt(target) = val;
    break;
  }
  case OP_VARREFLOCAL: {
    int target, from;getcode_varreflocal(i, target, from);
    ValueT* val = stkvt(1 + from);
    SymPtr var = lambda->vars->reflocal(from);
    Assert(this, !isundefined(val), "undefined local variable %s", Ssstr(var));
    *stkvt(target) = val;
    break;
  }
  case OP_VARREFOVAR: {
    int target, from;getcode_varrefovar(i, target, from);
    OuterVar* ov = lambda->vars->refovar(from);
    OuterVal* ovl = call->outers[from];
    ValueT* val = ovl->valp;
    Assert(this, !isundefined(val), "undefined ref var %s", Ssstr(ov->name));
    *stkvt(target) = val;
    break;
  }
  case OP_DEFGLOBAL: {
    int target, from;getcode_defglobal(i, target, from);
    ValueT* k = lambda->getk(target);
    SymPtr name = symref(k);
    ValueT* val = stkvt(from);
    Assert(this, !isundefined(val), "undefined value to define global variable %s", Ssstr(name));
    GEnv(this)->newkeyorupdate(name, val);
    *val = Svoidref;
    break;
  }
  case OP_SETLOCAL: {
    int A, B;getcode_setlocal(i, A, B);
    ValueT* val = stkvt(B);
    SymPtr var = lambda->vars->reflocal(A);
    Assert(this, !isundefined(val), "undefined val for var %s", Ssstr(var));
    *stkvt(A+1) = val;
    break;
  }
  case OP_SETOVAR: {
    int A, B;getcode_setovar(i, A, B);
    OuterVar* ov = lambda->vars->refovar(A);
    ValueT* val = stkvt(B);
    Assert(this, !isundefined(val), "undefined val for var %s", Ssstr(ov->name));
    OuterVal* ovl = call->outers[ov->idx];
    *ovl->valp = val;
    break;
  }
  case OP_SETGLOBAL: {
    int target, from;getcode_setglobal(i, target, from);
    ValueT* k = lambda->getk(target);
    SymPtr name = symref(k);
    ValueT* val = stkvt(from);
    Assert(this, !isundefined(val), "undefined value to set! global variable %s", Ssstr(name));
    GEnv(this)->setslot(name, val);
    *val = Svoidref;
    break;
  }
  case OP_IFFALSEJUMP: {
    int A, B;getcode_iffalsejmp(i, A, B);
    ValueT* val = stkvt(B);
    if (isfalse(val))
    {
      pc = A;
      goto loop;
    }
    break;
  }
  case OP_LAMBDA: {
    int target, k;getcode_lambda(i, target, k);
    LambdaPtr newlambda = lambda->getl(k);
    int on = newlambda->vars->ovar.n;
    ValueT* tval = stkvt(target);
    ClosurePtr clo = NULL;
    setclosure(tval, clo = newclosure(on));
    clo->lambda = newlambda;
    clo->initouters(this, frm->seg, base, call->outers);
    break;
  }
  case OP_TAILCALLAPP:
  case OP_CALLAPP: {
    int k, len;GET_OPAB(i, k, len);
    ValueT* proc = stkvt(k);
    CallAppState callstate;
    recallapp:
    if (isnativeproc(proc))
    {
      NativeProcObj* nproc = nativeprocref(proc);
      if (nproc->iscomplex())
      {
        switch(nproc->complexid) {
        case NATIVE_COMPLEX_APPLY: {
          if (callstate.fromapply)
            flatshrinkapplyarity(this, proc, proc+len);
          else
          {
            shrinkapplyarity(this, proc, &len);
            callstate.fromapply = true;
          }
          goto recallapp;
        }
        case NATIVE_COMPLEX_CALLCC: {
          checkcallcc(this, frm, proc, len, &callstate);
          goto recallapp;
        }
        case NATIVE_COMPLEX_CALL_WITH_IN_FILE: {
          callwithinputfile(this, frm, proc, &len, &callstate);
          goto recallapp;
        }
        case NATIVE_COMPLEX_CALL_WITH_OUT_FILE: {
          callwithoutputfile(this, frm, proc, &len, &callstate);
          goto recallapp;
        }
        case NATIVE_COMPLEX_CALL_WITH_OUT_STR: {
          callwithoutputstr(this, frm, proc, &len, &callstate);
          goto recallapp;
        }
        default:
          Error(this, "not supported complex native proc %d yet\n", nproc->complexid);
          break;
        }
      }
      else
      {
        ensurearity(this, proc, len, nproc->argnum, nproc->argrest, Ssstr(nproc->var), callstate.fromapply);
        *proc = scmcallcproc(this, nproc, proc+1);
        if (callstate.unwind) callstate.unwind(this, frm, proc+1);
        if (icode == OP_CALLAPP)
          break;
      }
    }
    else
    {
      if (isclosure(proc))
      {
        frm = ctorclosurefrm(this, i, frm, proc, len, &callstate);
        checkcalliofile(this, frm, proc+1, &callstate);
        base = frm->base;
        call = closureref(base);
        lambda = call->lambda;
        pc = lambda->getcodestart();
        goto loop;
      }
      else if (iscontinuation(proc))
      {
        ContinuationPtr cont = continuationref(proc);
        frm = stk->curfrm = cont->frm;
        checkcalliofile(this, frm, proc+1, &callstate);
        ac0 = *cont->base = proc+1;
        base = frm->base;
        call = closureref(base);
        lambda = call->lambda;
        pc = frm->getpc();
        Assert(this, len == 1, "return error in call continuation, len=%d", len);
        break;
      }
      else
        ErrorVT(this, proc, "not a procedure");
    }
  }
  case OP_RETURN: {
    ac0 = *frm->start = stkvt((1+(lambda->vars?lambda->vars->local.n:0)));
    frm->seg->closeouterval(this, frm, frm->base);
    frm = stk->rtnfrm(frm);
    if (!stk->isbasefrm(frm))
    {
      base = frm->base;
      call = closureref(base);
      lambda = call->lambda;
      pc = frm->getpc();
    }
    else
      pc = -1;
    break;
  }
  default:
    Error(this, "unknown execution op %d", icode);
  }
  pcnext();
  goto loop;
}

#define addbuff(b,p,e)                            \
{ size_t t = (size_t)(e);                         \
  memcpy(b + p, &t, sizeof(t)); p += sizeof(t); } \

void VM::makeseed()
{
  char buff[3 * sizeof(size_t)];
  int h = time(NULL);
  int p = 0;
  addbuff(buff, p, this);
  addbuff(buff, p, &h);
  addbuff(buff, p, &intern);
  Assert(this, p == sizeof(buff), "internal makeseed error");
  seed = SCM::hash(buff, p, h);
  srand(h);
  Debug(Print("vm make seed %d time %d\n", seed, h));
}

void VM::regNative(const char* name, CProc f, int argn, bool rest)
{
  SymPtr sym = getintern()->internsym(name);
  PairPtr pair = getgenv()->getslot(sym);
  Assert(this, !pair, "key already exists %s", name);
  Sgcvar1(this, cf);
  setnativeproc(cf, Sr4(this, NativeProcObj, sym, f, argn, rest));
  getgenv()->newkey(sym, cf);
  DebugReg(Print("register global native proc %s[hash %d]\n", name, sym->hash));
}

void VM::regComplex(const char* name, int id)
{
  Assert(this, id >= 0 && id < NATIVE_COMPLEX_MAX, "wrong id of complex proc %d", id);
  SymPtr sym = getintern()->internsym(name);
  PairPtr pair = getgenv()->getslot(sym);
  Assert(this, !pair, "key already exists %s", name);
  Sgcvar1(this, cf);
  setnativeproc(cf, Sr2(this, NativeProcObj, sym, id));
  getgenv()->newkey(sym, cf);
  DebugReg(Print("register global complex native proc %s[hash %d]\n", name, Sshash(sym)));
}

static void constvalinit(VM* vm)
{
  ArrayObj::empty.setimmutable();

  BIND_CONST_TO_VT("null", Snullref);
  BIND_CONST_TO_VT("true", Strueref);
  BIND_CONST_TO_VT("#t", Strueref);
  BIND_CONST_TO_VT("false", Sfalseref);
  BIND_CONST_TO_VT("#f", Sfalseref);

  initreserve2(definevt, "define");
  initreserve2(setvt, "set!");
  initreserve2(beginvt, "begin");
  initreserve2(ifvt, "if");
  initreserve2(lambdavt, "lambda");
  initreserve2(syntaxrvt, "syntax-rules");
  initreserve2(syntaxerrvt, "syntax-error");
  initreserve2(defsyntaxvt, "define-syntax");
  initreserve2(ellipsisvt, "...");
  initreserve2(quotevt, "quote");
  initreserve2(uquotevt, "unquote");
  initreserve2(uquotesvt, "unquote-splicing");
  initreserve2(qquotevt, "quasiquote");

  vm->kconst[K_NULL] = Snullref;
  vm->kconst[K_UNDEFINED] = Sundefined;
  vm->kconst[K_UNQUOTE] = &vm->uquotevt;
  vm->kconst[K_QQUOTE] = &vm->qquotevt;
  vm->kconst[K_UNQUOTES] = &vm->uquotesvt;
}

void VM::init()
{
  iport = Sr0(this, InputPortObj);
  OutputPortFileObj* ofile = NULL;
  oport = ofile = Sr0(this, OutputPortFileObj);
  ofile->file = stderr;
  getintern()->init();
  getgenv()->init();
  constvalinit(this);
  SCMBasic::init(this);
  SCMMath::init(this);
  SCMPort::init(this);
  SCMStr::init(this);
  SCMVector::init(this);
  regComplex("apply", NATIVE_COMPLEX_APPLY);
  regComplex("call-with-current-continuation", NATIVE_COMPLEX_CALLCC);
  regComplex("call-with-input-file", NATIVE_COMPLEX_CALL_WITH_IN_FILE);
  regComplex("call-with-output-file", NATIVE_COMPLEX_CALL_WITH_OUT_FILE);
  regComplex("call-with-output-string", NATIVE_COMPLEX_CALL_WITH_OUT_STR);
}

void VM::getuniquesym(SymPtr sym, ValueT* out)
{
  Lbuffer buff(this);
  char temp[32] = {0};
  snprintf(temp, sizeof(temp), "-%d-%d-", rand(), ++uniquen);
  buff.put(Ssstr(sym));
  buff.put(temp);
  buff.put(hygiene_id_post);
  setsym(out, this->ninstrintern(buff.buf, buff.count));
}

VM::VM(ScmAlloc a):
  frealloc(a), gc(this), stk(this), objGroup(this),
  intern(this), cacheGroup(this),
  genv(this),uniquen(0)
{
  makeseed();
  init();
  GC(this)->setthreshold();
}

void* VM::shrinkvec(void* ptr, int *psize, int fn, int esize)
{
  int olds = esize * (*psize);
  int news = esize * fn;
  Assert(this, news <= olds, "error when shrink vector");
  void* newptr = this->realloc(ptr, olds, news);
  *psize = fn;
  return newptr;
}

void* VM::growvec(void *ptr, int *psize, int ne, int esize, int limit, const char* what)
{
  int size = *psize;
  if (ne + 1 <= size)
    return ptr;
  Assert(this, size < limit, "too many %s (limit: %d)", what, limit);
  size *= 2;
  if (ne + 1 >= size)
    size *= 2;
  if (size < MINVEC_SIZE)
    size = MINVEC_SIZE;
  if (size >= limit)
    size = limit;
  Assert(this, ne + 1 <= size, "%s:reach limit need:%d size:%d limit:%d", what, ne, size, limit);
  void *newptr = this->realloc(ptr, (*psize) * esize, size * esize);
  *psize = size;
  return newptr;
}

void* VM::realloc(void* ptr, size_t osize, size_t nsize)
{
  long debt = getgc()->debt();
  void* block = frealloc(ptr, nsize);
  if (block == NULL)
  {
    getgc()->fullgc();
    block = frealloc(ptr, nsize);
  }
  if (osize < nsize) memset((byte*)block + osize, 0, nsize - osize);
  getgc()->debt(debt + nsize - osize);
  debt = getgc()->debt();
  DebugMem(Print("realloc mem(%u) -> %p old(%u) -> %p \n", nsize, block, osize, ptr));
  return block;
}

void* VM::alloc(size_t size)
{
  long debt = getgc()->debt();
  void* block = frealloc(NULL, size);
  if (block == NULL)
  {
    getgc()->fullgc();
    block = frealloc(NULL, size);
  }
  memset(block, 0, size);
  getgc()->debt(debt + size);
  debt = getgc()->debt();
  DebugMem(Print("alloc mem(%u) -> %p\n", size, block));
  return block;
}

StrObj* VM::newstr(int len, int h)
{
  int size = StrObj::totalsize(len);
  return new (alloc(size)) StrObj(len, h);
}

StrObj* VM::newstr(char c, int len, int h)
{
  int size = StrObj::totalsize(len);
  return new (alloc(size)) StrObj(c, len, h);
}

StrObj* VM::newstr(const char* str, int len, int h)
{
  int size = StrObj::totalsize(len);
  return new (alloc(size)) StrObj(str, len, h);
}

NumBigObj* VM::newnumbig(int len, char sign)
{
  int size = NumBigObj::totalsize(len);
  return (NumBigObj*)getobjgroup()->recobj(new (alloc(size)) NumBigObj(len, sign));
}

ClosurePtr VM::newclosure(int no)
{
  int size = ClosureObj::totalsize(no);
  return (ClosurePtr)getobjgroup()->recobj(new (alloc(size)) ClosureObj(no));
}

PairPtr VM::getonepairnor()
{
  PairPtr pair = Cache(this)->getonepair();
  if (pair == NULL)
    pair = new (alloc(sizeof(PairObj))) PairObj();
  pair->gcnxt = NULL;
  return pair;
}

StrObj* VM::strintern(const char* str)
{
  return strintern(str, strlen(str));
}

StrObj* VM::ninstrintern(const char* str, int len)
{
  return (StrObj*)getobjgroup()->recobj(newstr(str, len, -1));
}

StrObj* VM::strintern(const char* str, int len)
{
  if (len <= MAXSHORTLEN)
    return getintern()->intern(str, len);
  else
    return ninstrintern(str, len);
}

StrObj* VM::strintern(int len)
{
  return (StrObj*)getobjgroup()->recobj(newstr(len, -1));
}

StrObj* VM::strintern(int len, char c)
{
  return (StrObj*)getobjgroup()->recobj(newstr(c, len, -1));
}

const char* VM::gettopsource()
{
  return gettoplambda()->source->str;
}

LambdaPtr VM::gettoplambda()
{
  Stack* stk = Stk(this);
  CallFrame* frm  = stk->curfrm;
  ValueT* base = frm->base;
  ValueT* basevt = stkvt(0);
  ClosurePtr call = closureref(basevt);
  return call->lambda;
}

bool VM::dolex(Lexer* lex, StrPtr source)
{
  ReserveStack _reservestk_(this);
  Stack* stk = Stk(this);
  Sgcvar3(this, expr, lambdavt, _dummy);
  LambdaPtr lambda = NULL;
  setlambda(lambdavt, lambda = Sr0(this, LambdaObj));
  lambda->source = source;
  setundefined(expr);
  lex->readOne(expr);
  if (!isundefined(expr))
  {
    lambda->defline = annotateline(expr);

#ifdef DebugVT
    Print("\n");
    printvalue0(expr, true);
    Print("\n");
#endif

    SCompiler compiler(this, lambda, NULL, _dummy);
    compiler.prevline = lambda->defline;
    compiler.compile(1, expr, Sreturn, true);

#ifdef DebugCCode
    Print("<--Compiled Code-->\n");
    printccode(lambda);
    Print("<--Compiled Code End-->\n");
#endif

    ac0 = Sundefined;
    CallFrame* frm = stk->curfrm;
    frm = stk->newfrm(frm, frm->top, 0, lambda->top);
    ClosurePtr closure = newclosure(0);
    setclosure(frm->base, closure);
    closure->lambda = lambda;
    stk->setvoid(frm->base+1, frm->top-1);
    execute(frm);

#ifdef DebugVT
    if (!isvoid(&ac0) && !isundefined(&ac0))
    {
      printvalue0(&ac0, true);
      Print("\n");
    }
#endif

    return true;
  }
  return false;
}

void VM::loadfile(const char* fname)
{
  ReaderF reader(fname);
  Lexer lex(this, &reader);
  Sgcvar1(this, source);
  setstr(source, strintern(fname));
  bool flag = false;
  do {
    flag = dolex(&lex, strref(source));
  } while(flag);
  GC(this)->fullgc();
}

void VM::dorepl()
{
  ReaderI reader(this);
  Sgcvar1(this, source);
  setstr(source, strintern("=stdin"));
 loop:
  TRY
  {
    Lexer lex(this, &reader);
    reader.lexer = &lex;
    dolex(&lex, strref(source));
    fflush(stderr);
  }
  CATCH(err)
  {
    Print("\n%s\n", err);
    GC(this)->fullgc();
  }
  goto loop;
}

void VM::printvalue(ValueT* val)
{
  switch (vttype(val)) {
  case VT_REF_PAIR:
  case VT_REF_ARRAY:
    oport->writechar('\'');
    break;
  }
  printvalue0(this->oport, val);
}

void VM::printvalue0(OutputPortObj* oport, ValueT* val, bool stripanno)
{
  switch(vttype(val)) {
  case VT_REF_STR: {
    StrPtr strp = strref(val);
    const char* s = Ssstr(strp);
    oport->writechar('"');
    for (int i = 0; i < Sslen(strp); i++)
    {
      char c = strp->str[i];
      switch (c)
      {
      case '"':
        oport->writestr("\\\"");
        break;
      case '\\':
        oport->writestr("\\\\");
        break;
      case '\a':
        oport->writestr("\\a");
        break;
      case '\b':
        oport->writestr("\\b");
        break;
      case '\f':
        oport->writestr("\\f");
        break;
      case '\n':
        oport->writestr("\\n");
        break;
      case '\r':
        oport->writestr("\\r");
        break;
      case '\t':
        oport->writestr("\\t");
        break;
      case '\v':
        oport->writestr("\\v");
        break;
      default:
        if (isprint(c)) oport->writechar(c);
        else
        {
          char buf[5] = {0};
          snprintf(buf, sizeof(buf), "\\%03d", c);
          oport->writestr(buf);
        }
        break;
      }
    }
    oport->writechar('"');
    break;
  }
  case VT_CHAR:{
    char c = vtchar(val);
    switch(c) {
    case '\n':
      oport->writestr("#\\newline");
      break;
    case '\r':
      oport->writestr("#\\return");
      break;
    case ' ':
      oport->writestr("#\\space");
      break;
    case '\t':
      oport->writestr("#\\tab");
      break;
    case '\a':
      oport->writestr("#\\alarm");
      break;
    case '\b':
      oport->writestr("#\\backspace");
      break;
    case 127:
      oport->writestr("#\\delete");
      break;
    case 27:
      oport->writestr("#\\escape");
      break;
    case 0:
      oport->writestr("#\\null");
      break;
    default: {
      char buf[8] = {0};
      snprintf(buf, sizeof(buf), "#\\%c", c);
      oport->writestr(buf);
      break;
    }
    }
    break;
  }
  case VT_NUM_INTEGER: {
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), scm_int_fmt, numi(val));
    oport->writestr(buf);
    break;
  }
  case VT_NUM_REAL: {
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "%f", numreal(val));
    oport->writestr(buf);
    break;
  }
  case VT_REF_NUM_COMPLEX: {
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "%f", numcomplexreal(val));
    oport->writestr(buf);
    scm_float imag = numcompleximag(val);
    if (imag > 0)
      oport->writechar('+');
    char buf2[64] = {0};
    snprintf(buf2, sizeof(buf2), "%fi", imag);
    oport->writestr(buf2);
    break;
  }
  case VT_REF_NUM_BIG: {
    NumBigObj* b = numbigref(val);
    Lbuffer tmp(this);
    b->tostr(this, &tmp);
    oport->writestr(tmp.buf, tmp.count);
    //for (int i = 0; i < tmp.count; i++)
    //  fprintf(f, "%c", tmp.buf[i]);
    break;
  }
  case VT_UNDEFINED:
    oport->writestr("<undefined>");
    break;
  case VT_VOID:
    oport->writestr("<void>");
    break;
  case VT_EOF:
    oport->writestr("<eof>");
    break;
  case VT_NULL:
    oport->writestr("()");
    break;
  case VT_TRUE:
    oport->writestr("#t");
    break;
  case VT_FALSE:
    oport->writestr("#f");
    break;
  case VT_REF_NATIVE:
    oport->writestr("#<procedure:");
    oport->writestr(Ssstr(nativeprocref(val)->var));
    oport->writechar('>');
    break;
  case VT_REF_CONTINUATION: {
    oport->writestr("#<procedure:cont>");
    break;
  }
  case VT_REF_CLOSURE:{
    //ClosurePtr cptr = closureref(val);
    oport->writestr("#<procedure>");
    break;
  }
  case VT_REF_HYGIENE_SYM:
    oport->writestr(Ssstr(hygienesymref(val)->sym));
    oport->writechar('|');
    break;
  case VT_REF_SYM:
    oport->writestr(Ssstr(symref(val)));
    break;
  case VT_REF_PAIR:
    if (isformquote(this, val))
    {
      oport->writechar('\'');
      printvalue0(oport, Scadr(val), stripanno);
    }
    else if (isformqquote(this, val))
    {
      oport->writechar('`');
      printvalue0(oport, Scadr(val), stripanno);
    }
    else if (isformuquote(this, val))
    {
      oport->writechar(',');
      printvalue0(oport, Scadr(val), stripanno);
    }
    else if (isformuquotes(this, val))
    {
      oport->writestr(",@");
      printvalue0(oport, Scadr(val), stripanno);
    }
    else
    {
      ValueT* tortoise = val, * hare = val;
      oport->writechar('(');
      do {
        printvalue0(oport, Scar(hare), stripanno);
        hare = Scdr(hare);
        if (!ispair(hare)) break;
        oport->writechar(' ');
        printvalue0(oport, Scar(hare), stripanno);
        hare = Scdr(hare);
        if (!ispair(hare)) break;
        oport->writechar(' ');
        tortoise = Scdr(tortoise);
      } while (tortoise != hare);
      if (tortoise == hare)
        Serror("cycle list");
      if (!isnull(hare))
      {
        oport->writestr(" . ");
        printvalue0(oport, hare, stripanno);
      }
      oport->writechar(')');
    }
    break;
  case VT_REF_ARRAY:{
    oport->writestr("#(");
    ArrayObj* arr = arrayref(val);
    int count = arr->array.n;
    for(int i = 0; i < count; i++)
    {
      printvalue0(oport, arr->get(i), stripanno);
      if (i == count - 1) break;
      oport->writechar(' ');
    }
    oport->writechar(')');
    break;
  }
  case VT_REF_ANNOTATION: {
    AnnotationObj* anno = annotateref(val);
    if (stripanno)
    {
      Sgcvar1(this, out);
      SCM::copystripanno(this, out, val);
      printvalue0(oport, out);
    }
    else
    {
      oport->writestr("(annotation line:");
      char buf[64] = {0};
      snprintf(buf, sizeof(buf), "%d,", anno->line);
      oport->writestr(buf);
      printvalue0(oport, &anno->vt);
      oport->writechar(')');
    }
    break;
  }
  case VT_REF_SYNTAX:
  case VT_REF_SYNTAX_RULES:
    oport->writestr("#<syntax-procedure>");
    break;
  case VT_REF_IPORT: {
    InputPortObj* iport = iportref(val);
    if (iport->fname)
    {
      oport->writestr("#<input-port:");
      oport->writestr(Ssstr(iport->fname));
      oport->writechar('>');
    }
    else
      oport->writestr("#<input-port>");
    break;
  }
  case VT_REF_OPORT:
    oport->writestr("#<output-port>");
    break;
  default:
    Error(this, "printvalue unknown type %d", vttype(val));
  }
}

int LambdaObj::addk(VM* vm, ValueT* v)
{
  VEC_FOR(i, &ks)
    if (SCM::equalp(v, ks.getptr(i)))
      return i;
  vec_add2(ValueT, vm, ks, v);
  return ks.n-1;
}

int LambdaObj::addl(VM* vm, LambdaPtr l)
{
  vec_add1(LambdaPtr, vm, ksl, l);
  return ksl.n-1;
}

void LambdaObj::saveline(VM* vm, int pc, int line, int preline)
{
  int diff = line - preline;
  if (pc >= sizelines)
    lines = (byte*)vm->growvec(lines, &sizelines, pc, sizeof(byte), INT_MAX, "opcodes lines");
  if (abs(diff) >= MLINEDIFF)
  {
    AbsLine al;
    al.pc = pc;
    al.line = line;
    vec_add2(AbsLine, vm, abslines, al);
    diff = ABSLINE;
  }
  lines[pc] = diff;
}

int LambdaObj::pushcode(VM* vm, Instruction i)
{
  vec_add1(Instruction, vm, code, i);
  return code.n-1;
}

void LambdaObj::visit(VM* vm)
{
  VEC_FOR(i, &ksl) Check(ksl.get(i));
  VEC_FOR(i, &ks) Check(ks.getptr(i));
  if (vars) vars->visit(vm);
  Check(source);
}

void LambdaObj::shrink(VM* vm)
{
  vec_shrink(LambdaPtr, vm, &ksl);
  vec_shrink(ValueT, vm, &ks);
  vec_shrink(Instruction, vm, &code);
  if (vars) vars->shrink(vm);
  if (lines) lines = (byte*)vm->shrinkvec(lines, &sizelines, code.n, sizeof(byte));
  vec_shrink(AbsLine, vm, &abslines);
}

void LambdaObj::finz(VM* vm)
{
  int coden = code.n;
  vec_finz(LambdaPtr, vm, &ksl);
  vec_finz(ValueT, vm, &ks);
  vec_finz(Instruction, vm, &code);
  if (vars) vars->finz(vm);
  if (lines)
  {
    vm->free(lines, coden * sizeof(byte));
    lines = NULL;
  }
  vec_finz(AbsLine, vm, &abslines);
}

void LambdaVarsObj::visit(VM* vm)
{
  VEC_FOR(i, &local) Check(local.get(i));
  VEC_FOR(i, &syntax) Check(syntax.get(i));
  VEC_FOR(i, &ovar) ovar.getptr(i)->visit(vm);
}

void LambdaVarsObj::shrink(VM* vm)
{
  vec_shrink(SymPtr, vm, &local);
  vec_shrink(OuterVar, vm, &ovar);
  vec_shrink(SyntaxPtr, vm, &syntax);
}

void LambdaVarsObj::finz(VM* vm)
{
  vec_finz(SymPtr, vm, &local);
  vec_finz(OuterVar, vm, &ovar);
  vec_finz(SyntaxPtr, vm, &syntax);
}

int LambdaVarsObj::addovar(VM* vm, SymPtr sym, int idx, bool islocal)
{
  vec_ensure(OuterVar, vm, &ovar, vec_fill2(OuterVar()));
  OuterVar* ov = ovar.getptr(ovar.n++);
  ov->name = sym;
  ov->idx = idx;
  ov->islocal = islocal;
  return ovar.n - 1;
}

int LambdaVarsObj::lookovar(SymPtr sym)
{
  VEC_FOR(i, &ovar)
    if (ovar.get(i).name == sym)
      return i;
  return -1;
}

void LambdaVarsObj::addsyntax(VM* vm, SyntaxPtr syn)
{
  vec_ensure(SyntaxPtr, vm, &syntax, vec_fill1(NULL));
  syntax.set(syntax.n++, syn);
}

int LambdaVarsObj::looklocal(SymPtr sym)
{
  VEC_FOR(i, &local)
    if (local.get(i) == sym)
      return i;
  return -1;
}

int LambdaVarsObj::addlocal(VM* vm, SymPtr sym)
{
  vec_add1(SymPtr, vm, local, sym);
  return local.n - 1;
}

int LambdaVarsObj::looksyntax(SymPtr sym, SyntaxPtr* sp)
{
  VEC_FOR(i, &syntax)
  {
    SyntaxPtr syn = syntax.get(i);
    if (syn->name == sym)
    {
      *sp = syn;
      return i;
    }
  }
  return -1;
}

class UnreadGuard {
public:
  Lexer* lex;
  UnreadGuard(Lexer* l): lex(l) {}
  ~UnreadGuard() { if (lex) lex->unread(); }
};

void InputPortObj::read(VM* vm, ValueT* vt)
{
  *vt = Seofref;
  ReaderPort rp(this);
  Lexer lex(vm, &rp);
  lex.anno = false;
  UnreadGuard guard(&lex);
  lex.readOne(vt);
}

void InputPortObj::finz(VM* vm)
{
  close();
  RefObject::finz(vm);
}

int OutputPortFileObj::write(VM* vm, ValueT* vt)
{
  if (!file) return -1;
  vm->printvalue0(this, vt, false);
  return 0;
}

int OutputPortStrObj::write(VM* vm, ValueT* vt)
{
  if (closed) return -1;
  vm->printvalue0(this, vt, false);
  return 0;
}

void OuterVal::close(VM* vm, CallFrame* frm)
{
  if (unwind)
  {
    unwind(vm, frm, valp);
    unwind = NULL;
  }
  val = *valp;
  valp = &val;
}

void ClosureObj::visit(VM* vm)
{
  Check(lambda);
  for (int i = 0; i < n ; i++)
    if (outers[i])
      Check(outers[i]);
}

void ClosureObj::initouters(VM* vm, StackSegment* seg, ValueT* base, OuterVal** encouter)
{
  LambdaVarsObj* vars = lambda->vars;
  for (int i = 0; i < n; i++)
  {
    OuterVar* ov = vars->refovar(i);
    if (ov->islocal)
      outers[i] = seg->findouterval(vm, base + 1 + ov->idx);
    else
      outers[i] = encouter[ov->idx];
  }
}

void ContinuationObj::finz(VM* vm)
{
  for (CallFrame* s = frm; s != NULL; s = s->prev)
    s->seg->frozen--;
  RefObject::finz(vm);
}

ContinuationObj::ContinuationObj(CallFrame* s, ValueT* b):frm(s), base(b)
{
  for (s = frm; s != NULL; s = s->prev)
    s->seg->frozen++;
}

int ReaderI::fillbuff(int initn)
{
  if (lexer->aheadToken == TOKEN_UNKNOWN)
    prompt('>');
  else
    prompt(0);
  buff.reset();
  n = 0;
  int c;
  while ((c = getc(stdin)) != EOF && c != '\n')
    buff.put(c);
  if (c == '\n') buff.put(c);
  if (buff.count > 0)  return (n=initn, buff.buf[0]);
  return -1;
}

const int Lbuffer::INIT_LEN = 128;
Lbuffer::Lbuffer(VM* v):vm(v)
{
  size = INIT_LEN;
  count = 0;
  buf = (char*)vm->alloc(INIT_LEN);
  Assert(vm, buf, "out of memory alloc fail, %d", size);
}

void Lbuffer::close()
{
  if (buf)
  {
    vm->free(buf, size);
    buf = NULL;
    size = 0;
    count =0;
  }
}

Lbuffer::~Lbuffer()
{
  close();
}

void Lbuffer::put(char c)
{
  if (count + 2 >= size)
  {
    int ns = size * 2;
    buf = (char*)vm->realloc(buf, size, ns);
    Assert(vm, buf, "out of memory realloc fail, old:%d, new:%d", size, ns);
    size = ns;
  }
  buf[count++] = c;
  buf[count] = 0;
}

void Lexer::plusline()
{
  lexassert(vm, ++linenum < INT_MAX, linenum, "too many lines");
  int old = curchar;
  nextc();
  // \n\r or \r\n
  // not \n\n
  if ((curchar == '\n' || curchar == '\r') && curchar != old)
    nextc();
}

int Lexer::dLex()
{
  int token = TOKEN_UNKNOWN;
 loop:
  switch(curchar) {
  case READ_END:
    return TOKEN_END;
  case '\n': case '\r':
    plusline();
    goto loop;
  case ' ': case '\f': case '\t': case '\v':
    nextc();
    goto loop;
  case ';':
    do {
      nextc();
    } while(curchar != READ_END && curchar != '\n' && curchar != '\r');
    goto loop;
  case '\'':
		token = TOKEN_QUOTE;
    nextc();
    break;
  case '`':
		token = TOKEN_QUASIQUOTE;
    nextc();
		break;
  case ',':{
    token = TOKEN_UNQUOTE;
    nextc();
    if (curchar != READ_END && curchar == '@')
    {
      token = TOKEN_UNQUOTE_SPLICING;
      nextc();
    }
    break;
  }
  case '"':
    token = readString();
    break;
  case '(':
		token = TOKEN_LEFT_PAREN;
    startline = linenum;
    nextc();
    break;
	case ')':
		token = TOKEN_RIGHT_PAREN;
    nextc();
    break;
  case '[':
  case ']':
  case '{':
  case '}':
		lexerror(vm, startline, "unexpected char near %c", curchar);
    return TOKEN_UNKNOWN;
  case '.': {
    nextc();
    if (curchar != READ_END)
    {
      switch(curchar) {
      case ')':
      case '(':
      case '[':
      case ']':
      case '{':
      case '}':
        lexerror(vm, linenum, "unexpected char near %c", curchar);
        token = TOKEN_UNKNOWN;
        break;
      case CASE_BLANK:
        token = TOKEN_DOT;
        break;
      case CASE_09DIGIT:
        token = readNum('.');
        break;
      default:
        token = readSymbol('.');
        break;
      }
    }
    break;
  }
  case '+':case '-': {
    startline = linenum;
    int oldc = curchar;
    nextc();
    if (curchar != READ_END)
    {
      switch(curchar) {
      case CASE_BLANK:
      case ')':
      case ';':
        buff.reset();buff.put(oldc);
        token = TOKEN_SYMBOL;
        break;
      default:
        token = readNum(oldc);
        break;
      }
    }
    else
    {
      lexerror(vm, startline, "unexpected end");
      token = TOKEN_UNKNOWN;
    }
    break;
  }
  case CASE_09DIGIT:
    token = readNum();
    break;
  case '#':{
    startline = linenum;
    nextc();
    if (curchar != READ_END)
    {
      switch(curchar) {
      case '\\':
        token = readChar();
        break;
      case '(':
        token = TOKEN_VECTOR;
        nextc();
        break;
      case 't':
      case 'f':
        token = readBool();
        break;
      case 'b':case 'B':case 'o':case 'O':
      case 'd':case 'D':case 'x':case 'X':
      case 'i':case 'I':case 'e':case 'E':
        token = readNum('#');
        break;
      default:
        lexerror(vm, startline, "unexpected char near %c", curchar);
        return TOKEN_UNKNOWN;
      }
    }
    break;
  }
  default:{
    char c = curchar;
    nextc();
    token = readSymbol(c);
    break;
  }
  }
  return token;
}

int Lexer::readChar()
{
  startline = linenum;
  nextc();
  if (curchar == READ_END)
    return TOKEN_UNKNOWN;
  buff.reset();
  buff.put(curchar);
  nextc();
 loop:
  if (curchar != READ_END)
  {
    switch(curchar){
    case CASE_BLANK:
    case ')':
    case ';':
      break;
		default:
      buff.put(curchar);
      nextc();
      goto loop;
		}
	}
  if (buff.count == 1)
    readC = buff.buf[0];
  else
  {
    if (SCM::strcaseeql(buff.buf, buff.count, "newline"))
      readC = '\n';
    else if (SCM::strcaseeql(buff.buf, buff.count, "return"))
      readC = '\r';
    else if (SCM::strcaseeql(buff.buf, buff.count, "space"))
      readC = ' ';
    else if (SCM::strcaseeql(buff.buf, buff.count, "tab"))
      readC = '\t';
    else if (SCM::strcaseeql(buff.buf, buff.count, "alarm"))
      readC = '\a';
    else if (SCM::strcaseeql(buff.buf, buff.count, "backspace"))
      readC = '\b';
    else if (SCM::strcaseeql(buff.buf, buff.count, "delete"))
      readC = 127;
    else if (SCM::strcaseeql(buff.buf, buff.count, "escape"))
      readC = 27;
    else if (SCM::strcaseeql(buff.buf, buff.count, "null"))
      readC = 0;
    else if (SCM::strcaseeql(buff.buf, buff.count, "nul"))
      readC = 0;
    else
      return TOKEN_UNKNOWN;
  }
  return TOKEN_CHAR;
}

int Lexer::readBool()
{
  startline = linenum;
  if (curchar == 't')
    readT = true;
  else
    readT = false;
  nextc();
  if (curchar != READ_END)
  {
    switch(curchar){
    case CASE_BLANK:
    case ')':
    case ';':
      return TOKEN_BOOL;
    default:
      return TOKEN_UNKNOWN;
    }
  }
  else
    return TOKEN_UNKNOWN;
}

int Lexer::readSymbol(char init)
{
  startline = linenum;
  buff.reset();
  if (init) buff.put(init);
 loop:
  if (curchar != READ_END)
  {
    switch(curchar){
    case ')':
    case CASE_BLANK:
    case ';':
      break;
		default:
      buff.put(curchar);
      nextc();
      goto loop;
		}
	}
	return TOKEN_SYMBOL;
}

int Lexer::readSymbol()
{
  return readSymbol(0);
}

int Lexer::readString()
{
  startline = linenum;
  buff.reset();
  nextc();
 read:
  if (curchar != READ_END)
  {
    if (curchar == '\"')
      goto outloop;
    if (curchar == '\\')
    {
      nextc();
      if (curchar == READ_END)
        goto outloop;
      switch(curchar) {
      case 'a': curchar = '\a'; break;
      case 'b': curchar = '\b'; break;
      case 'f': curchar = '\f'; break;
      case 'n': curchar = '\n'; break;
      case 'r': curchar = '\r'; break;
      case 't': curchar = '\t'; break;
      case 'v': curchar = '\v'; break;
      case '\n':
      case '\r':
        plusline();
        curchar = '\n'; break;
      default:
        // check escape char
        break;
      }
    }

    buff.put(curchar);
    nextc();
    goto read;
  }
 outloop:
  if (curchar != '\"')
    return TOKEN_UNKNOWN;
  nextc();
	return TOKEN_STRING;
}

int Lexer::readNum(char init)
{
  startline = linenum;
  buff.reset();
  if (init) buff.put(init);
 loop:
  if (curchar != READ_END)
  {
    switch(curchar) {
    case CASE_BLANK:
    case ')':
    case ';':
      break;
    default:
      buff.put(curchar);
      nextc();
      goto loop;
    }
  }
  return TOKEN_NUM;
}

int Lexer::readNum()
{
  return readNum(0);
}

void Lexer::readOne(ValueT* v)
{
  aheadToken = dLex();
  readValueT(v);
  aheadToken = TOKEN_UNKNOWN;
}

void Lexer::readVector(ValueT* v)
{
  int line = startline;
  aheadToken = dLex();
  if (aheadToken != TOKEN_RIGHT_PAREN)
  {
    lexassert(vm, aheadToken != TOKEN_DOT, line, "illegal form, not proper list in vector");
    ArrayObj* arr = NULL;
    setarray(v, arr = Sr0(vm, ArrayObj));
    int idx = arr->add(vm, Snullref);
    ValueT* v2 = arr->get(idx);
    readValueT(v2);
    aheadToken = dLex();
    while (aheadToken != TOKEN_RIGHT_PAREN)
    {
      lexassert(vm, aheadToken != TOKEN_DOT, line, "illegal form, not proper list in vector");
      idx = arr->add(vm, Snullref);
      v2 = arr->get(idx);
      readValueT(v2);
      aheadToken = dLex();
    }
    arr->shrink(vm);
  }
  else
    setarray(v, &ArrayObj::empty);
  arrayref(v)->setimmutable();
  if (anno)
    SCM::toAnnotation(vm, v, line);
}

void Lexer::readListT0(ValueT* v)
{
  int line = startline;
  aheadToken = dLex();
  if (aheadToken == TOKEN_RIGHT_PAREN)
  {
    setnull(v);
    return;
  }
  if (aheadToken == TOKEN_DOT)
  {
    aheadToken = dLex();
    readValueT(v);
    aheadToken = dLex();
    lexassert(vm, aheadToken == TOKEN_RIGHT_PAREN, line, "must have only one item after dot");
    return;
  }
  readValueT(v);
  Sgcvar1(vm, v2);
  readListT0(v2);
  setpair(v, SCM::cons(vm, v, v2));
  if (anno)
    SCM::toAnnotation(vm, v, line);
}

void Lexer::readListT(ValueT* v)
{
  int line = startline;
  aheadToken = dLex();
  if (aheadToken == TOKEN_RIGHT_PAREN)
  {
    setnull(v);
    if (anno)
      SCM::toAnnotation(vm, v, line);
  }
  else
  {
    lexassert(vm, aheadToken != TOKEN_DOT, line, "illegal form: dot cannot be the first item");
    readValueT(v);
    Sgcvar1(vm, v2);
    readListT0(v2);
    setpair(v, SCM::cons(vm, v, v2));
    if (anno)
      SCM::toAnnotation(vm, v, line);
  }
}

void Lexer::readValueT(ValueT* v)
{
  int line = startline;
  switch(aheadToken) {
	case TOKEN_NUM:
    if (!SCMMath::str2num(vm, buff.buf, buff.count, v))
      lexerror(vm, line, "unexpected num token %.*s", buff.count, buff.buf);
    if (anno)
      SCM::toAnnotation(vm, v, line);
    break;
  case TOKEN_CHAR:
    setchar(v, readC);
    if (anno)
      SCM::toAnnotation(vm, v, line);
    break;
  case TOKEN_BOOL:
    if (readT) *v = Strueref;
    else *v = Sfalseref;
    if (anno)
      SCM::toAnnotation(vm, v, line);
    break;
	case TOKEN_STRING:
    setstr(v, vm->strintern(buff.buf, buff.count));
    strref(v)->setimmutable();
    if (anno)
      SCM::toAnnotation(vm, v, line);
    break;
	case TOKEN_SYMBOL:
    setsym(v, Intern(vm)->internsym(buff.buf, buff.count));
    if (anno)
      SCM::toAnnotation(vm, v, line);
    break;
  case TOKEN_LEFT_PAREN:
		readListT(v);
    break;
  case TOKEN_VECTOR:
    readVector(v);
    break;
	case TOKEN_QUOTE: {
    aheadToken = dLex();
    readValueT(v);
    SCM::tokwform(vm, &vm->quotevt, v, line, anno);
    break;
  }
	case TOKEN_UNQUOTE: {
    aheadToken = dLex();
    readValueT(v);
    SCM::tokwform(vm, &vm->uquotevt, v, line, anno);
    break;
  }
	case TOKEN_QUASIQUOTE: {
    aheadToken = dLex();
    readValueT(v);
    SCM::tokwform(vm, &vm->qquotevt, v, line, anno);
    break;
  }
	case TOKEN_UNQUOTE_SPLICING: {
    aheadToken = dLex();
    readValueT(v);
    SCM::tokwform(vm, &vm->uquotesvt, v, line, anno);
    break;
  }
  case TOKEN_END:
    return;
	default:
		lexerror(vm, line, "read error unknown token");
    break;
	}
}
