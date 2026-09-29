#include "vm.h"
#include "scmtable.h"

#include <string.h>

namespace Scheme {

HashTableObj::HashTableObj(VM* v) : vm(v), array(NULL), sizearray(0),
  lsizenode(0), node(NULL), firstfree(NULL)
{
}

uint HashTableObj::gethash(ValueT* v)
{
  switch (vttype(v)) {
  case VT_NUM_INTEGER:
    return (uint)numi(v);
  case VT_NUM_REAL: {
    uint h[2];
    double d = numreal(v);
    memcpy(h, &d, sizeof(h));
    return h[0] + h[1];
  }
  case VT_CHAR:
    return (uint)vtchar(v);
  case VT_FALSE:
    return 0;
  case VT_TRUE:
    return 1;
  case VT_NULL:
    return 2;
  case VT_EOF:
    return 3;
  case VT_VOID:
    return 4;
  case VT_UNDEFINED:
    return 5;
  case VT_REF_STR:
  case VT_REF_SYM: {
    StrObj* s = strref(v);
    if (s->hash < 0)
      s->hash = (int)SCM::hash(Ssstr(s), s->len, vm->seed);
    return (uint)s->hash;
  }
  default:
    return (uint)(size_t)refp(v);
  }
}

/* whether an integer key is a candidate for the array part (Lua arrayindex) */
int HashTableObj::arrayindex(ValueT* key)
{
  if (isnumi(key))
  {
    int k = numi(key);
    if (k >= 1 && !scmtoobig(k))
      return k;
  }
  return -1;
}

int HashTableObj::log2i(int x)
{
  int l = -1;
  while (x >= 1) { l++; x >>= 1; }
  return l;
}

TableNode* HashTableObj::mainposition(ValueT* key)
{
  uint h = gethash(key);
  h ^= h >> 16;
  h *= 0x7feb352d;
  h ^= h >> 15;
  h *= 0x846ca68b;
  h ^= h >> 16;
  return &node[h & (twoto(lsizenode) - 1)];
}

TableNode* HashTableObj::getfreepos()
{
  for (TableNode* n = firstfree; n >= node; n--)
  {
    if (isundefined(&n->key))
    {
      firstfree = n;
      return n;
    }
  }
  return NULL;
}

ValueT* HashTableObj::get(ValueT* key)
{
  if (isnumi(key))
  {
    int k = numi(key);
    if (1 <= k && k <= sizearray)
      return &array[k - 1]; /* in the array part (slot may hold undefined) */
  }
  if (!node)
    return NULL;
  TableNode* n = mainposition(key);
  for (;;)
  {
    if (isundefined(&n->key))
      return NULL;
    if (keyeq(key, &n->key))
      return &n->val;
    if (n->next == NULL)
      return NULL;
    n = n->next;
  }
}

bool HashTableObj::keyeq(ValueT* a, ValueT* b)
{
  if (isstr(a) && isstr(b) && strref(a)->equalp(strref(b)))
    return true;
  return SCM::eqp(a, b);
}

/* hash-part slot of key, or -1 if absent (Lua: invalid key for next) */
int HashTableObj::nodeindex(ValueT* key)
{
  if (!node)
    return -1;
  TableNode* n = mainposition(key);
  for (;;)
  {
    if (isundefined(&n->key))
      return -1;
    if (keyeq(key, &n->key))
      return (int)(n - node);
    if (n->next == NULL)
      return -1;
    n = n->next;
  }
}

/* Lua luaH_next: kv = #f means start; otherwise kv is the previous (key . val).
   Returns the next (key . val), or #f when traversal is done. */
ValueT HashTableObj::next(VM* vm, ValueT* kv)
{
  static const char* METHOD = "hash-table-next";
  int i;
  if (isfalse(kv))
    i = -1;
  else
  {
    AssertVT(vm, ispair(kv), kv, "%s: not a pair or #f", METHOD);
    ValueT key = *Scar(kv);
    int k = arrayindex(&key);
    if (k >= 1 && k <= sizearray)
      i = k - 1; /* locate in the array part */
    else
    {
      i = nodeindex(&key);
      if (i < 0)
        Error(vm, "%s: invalid key for traversal", METHOD);
      i += sizearray; /* hash elements are numbered after array ones */
    }
  }
  /* scan the array part */
  for (i++; i < sizearray; i++)
  {
    if (!isundefined(&array[i]))
    {
      ValueT k, out;
      setnumi(&k, i + 1);
      setpair(&out, SCM::cons(vm, &k, &array[i]));
      return out;
    }
  }
  /* scan the hash part */
  for (i -= sizearray; i < (node ? twoto(lsizenode) : 0); i++)
  {
    TableNode* n = &node[i];
    if (!isundefined(&n->key))
    {
      ValueT out;
      setpair(&out, SCM::cons(vm, &n->key, &n->val));
      return out;
    }
  }
  return *Sfalseref;
}

void HashTableObj::fixfirstfree()
{
  if (isundefined(&firstfree->key))
    return;
  for (TableNode* n = firstfree - 1; n >= node; n--)
  {
    if (isundefined(&n->key))
    {
      firstfree = n;
      return;
    }
  }
}

void HashTableObj::insertkey(ValueT* key, ValueT* val)
{
  TableNode* mp = mainposition(key);
  if (!isundefined(&mp->key))
  {
    TableNode* othern = mainposition(&mp->key);
    TableNode* freen = getfreepos();
    Assert(vm, freen, "hash table internal error: no free node");
    if (othern != mp)
    {
      while (othern->next != mp)
        othern = othern->next;
      othern->next = freen;
      *freen = *mp;
      mp->next = NULL;
      setundefined(&mp->key);
      setundefined(&mp->val);
    }
    else
    {
      freen->next = mp->next;
      mp->next = freen;
      mp = freen;
    }
  }
  mp->key = *key;
  mp->val = *val;
  fixfirstfree();
}

void HashTableObj::set(VM* vm, ValueT* key, ValueT* val)
{
  if (isnumi(key))
  {
    int k = numi(key);
    if (1 <= k && k <= sizearray)
    {
      array[k - 1] = *val;
      return; /* inside the array part: direct write */
    }
  }

  ValueT* slot = get(key);
  if (slot)
  {
    *slot = *val;
    GC(vm)->checkBarrier(this);
    return;
  }

  if (!node || (!isundefined(&mainposition(key)->key) && getfreepos() == NULL))
  {
    rehash(vm);
    set(vm, key, val);
    GC(vm)->checkBarrier(this);
    return;
  }

  insertkey(key, val);
  GC(vm)->checkBarrier(this);
}

static void computesizes(int nums[], int ntotal, int* narray, int* nhash);

void HashTableObj::numuse(int* narray, int* nhash)
{
  int nums[SCMMAXBITS + 1];
  int i, lg;
  int totaluse = 0;

  /* count elements in array part */
  for (i = 0, lg = 0; lg <= SCMMAXBITS; lg++)
  {
    int ttlg = twoto(lg);
    if (ttlg > sizearray)
    {
      ttlg = sizearray;
      if (i >= ttlg) break;
    }
    nums[lg] = 0;
    for (; i < ttlg; i++)
    {
      if (!isundefined(&array[i]))
      {
        nums[lg]++;
        totaluse++;
      }
    }
  }
  for (; lg <= SCMMAXBITS; lg++) nums[lg] = 0;
  *narray = totaluse;

  /* count elements in hash part, marking those that fit the array part */
  i = node ? twoto(lsizenode) : 0;
  while (i--)
  {
    TableNode* n = &node[i];
    if (!isundefined(&n->key))
    {
      int k = arrayindex(&n->key);
      if (k >= 0)
      {
        nums[log2i(k - 1) + 1]++;
        (*narray)++;
      }
      totaluse++;
    }
  }
  computesizes(nums, totaluse, narray, nhash);
}

static void computesizes(int nums[], int ntotal, int* narray, int* nhash)
{
  int i;
  int a = nums[0]; /* number of elements smaller than 2^0 */
  int na = a;      /* number of elements to go to the array part */
  int n = (na == 0) ? -1 : 0; /* (log of) optimal size for the array part */
  for (i = 1; a < *narray && *narray >= twoto(i - 1); i++)
  {
    if (nums[i] > 0)
    {
      a += nums[i];
      if (a >= twoto(i - 1)) /* more than half elements in use? */
      {
        n = i;
        na = a;
      }
    }
  }
  *nhash = ntotal - na;
  *narray = (n == -1) ? 0 : twoto(n);
}

void HashTableObj::setarrayvector(VM* vm, int size)
{
  int oldasize = sizearray;
  ValueT* newarray = (ValueT*)vm->alloc(size * sizeof(ValueT));
  for (int i = 0; i < size; i++)
    setundefined(&newarray[i]);
  if (oldasize > 0)
  {
    memcpy(newarray, array, oldasize * sizeof(ValueT));
    vm->free(array, oldasize * sizeof(ValueT));
  }
  array = newarray;
  sizearray = size;
}

/* build a fresh hash part; the old node array is freed by the caller (resize) */
void HashTableObj::setnodevector(VM* vm, int lsize)
{
  int size = twoto(lsize);
  node = (TableNode*)vm->alloc(size * sizeof(TableNode));
  new (node) TableNode[size];
  initnodes(size);
  firstfree = &node[size - 1];
  lsizenode = (byte)lsize;
}

/* rebuild the table with new array size and hash size (Lua resize) */
void HashTableObj::resize(VM* vm, int nasize, int nhsize)
{
  int oldasize = sizearray;
  int oldhsize = lsizenode;
  TableNode* nold = node;

  if (nasize > oldasize) /* array part must grow? */
    setarrayvector(vm, nasize);

  /* create a new hash part with the appropriate size */
  setnodevector(vm, nhsize);

  /* re-insert elements */
  if (nasize < oldasize) /* array part must shrink? */
  {
    sizearray = nasize;
    /* re-insert elements from the vanishing slice */
    for (int i = nasize; i < oldasize; i++)
    {
      if (!isundefined(&array[i]))
      {
        ValueT k, v;
        setnumi(&k, i + 1);
        v = array[i];
        set(vm, &k, &v); /* full set path: may land in the array part */
      }
    }
    /* shrink the array */
    if (nasize > 0)
    {
      ValueT* newarray = (ValueT*)vm->alloc(nasize * sizeof(ValueT));
      memcpy(newarray, array, nasize * sizeof(ValueT));
      vm->free(array, oldasize * sizeof(ValueT));
      array = newarray;
    }
    else
    {
      vm->free(array, oldasize * sizeof(ValueT));
      array = NULL;
    }
    sizearray = nasize;
  }

  if (nold)
  {
    for (int i = twoto(oldhsize) - 1; i >= 0; i--)
    {
      TableNode* old = nold + i;
      if (!isundefined(&old->key))
      {
        ValueT k = old->key;
        ValueT v = old->val;
        set(vm, &k, &v); /* full set path: array candidates re-enter array part */
      }
    }
    vm->free(nold, twoto(oldhsize) * sizeof(TableNode));
  }
}

void HashTableObj::rehash(VM* vm)
{
  int nasize, nhsize;
  numuse(&nasize, &nhsize); /* compute new sizes */
  resize(vm, nasize, log2i(nhsize) + 1);
}

void HashTableObj::initnodes(int size)
{
  for (int i = 0; i < size; i++)
  {
    node[i].next = NULL;
    setundefined(&node[i].key);
    setundefined(&node[i].val);
  }
}

void HashTableObj::visit(VM* vm)
{
  for (int i = 0; i < sizearray; i++)
  {
    if (!isundefined(&array[i]))
      Check(array[i]);
  }
  if (node)
  {
    int size = twoto(lsizenode);
    for (int i = 0; i < size; i++)
    {
      TableNode* n = &node[i];
      if (!isundefined(&n->key))
      {
        Check(n->key);
        Check(n->val);
      }
    }
  }
}

void HashTableObj::finz(VM* vm)
{
  if (array)
  {
    vm->free(array, sizearray * sizeof(ValueT));
    array = NULL;
  }
  if (node)
  {
    vm->free(node, twoto(lsizenode) * sizeof(TableNode));
    node = NULL;
  }
}

ValueT SCMTable::maketable(VM* vm)
{
  ValueT out;
  sethashtable(&out, Sr1(vm, HashTableObj, vm));
  return out;
}

ValueT SCMTable::ref(VM* vm, ValueT* ht, ValueT* key)
{
  static const char* METHOD = "hash-table-ref";
  AssertArg(vm, ishashtable(ht), METHOD, ht, " is not a hash-table");
  HashTableObj* table = hashtableref(ht);
  ValueT* slot = table->get(key);
  AssertVT(vm, slot && !isundefined(slot), key, "%s: key not found", METHOD);
  return *slot;
}

void SCMTable::set(VM* vm, ValueT* ht, ValueT* key, ValueT* val)
{
  static const char* METHOD = "hash-table-set!";
  AssertArg(vm, ishashtable(ht), METHOD, ht, " is not a hash-table");
  HashTableObj* table = hashtableref(ht);
  table->set(vm, key, val);
}

static ValueT scm_stub_make_hash_table(VM* vm, ValueT* args)
{
  static const char* METHOD = "make-hash-table";
  Assert(vm, isnull(args), "%s: too many arguments", METHOD);
  return SCMTable::maketable(vm);
}

static ValueT scm_stub_hash_table_ref(VM* vm, ValueT* ht, ValueT* key)
{
  return SCMTable::ref(vm, ht, key);
}

static ValueT scm_stub_hash_table_set(VM* vm, ValueT* ht, ValueT* key, ValueT* val)
{
  SCMTable::set(vm, ht, key, val);
  return Svoidref;
}

static ValueT scm_stub_hash_table_next(VM* vm, ValueT* ht, ValueT* kv)
{
  static const char* METHOD = "hash-table-next";
  AssertArg(vm, ishashtable(ht), METHOD, ht, " is not a hash-table");
  HashTableObj* table = hashtableref(ht);
  return table->next(vm, kv);
}

void SCMTable::init(VM* vm)
{
  const RegCProc hashes[] = {
    RegCProc("make-hash-table", scm_stub_make_hash_table, true),
    RegCProc("hash-table-ref", scm_stub_hash_table_ref),
    RegCProc("hash-table-set!", scm_stub_hash_table_set),
    RegCProc("hash-table-next", scm_stub_hash_table_next),
    RegCProc(NULL, -1)
  };
  regcfunc(vm, hashes);
}

};
