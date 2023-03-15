#include "refcount.h"

namespace Dummy {

RefGC* RefGC::instance = NULL;

RefGC* RefGC::inst()
{
  if (instance == NULL) instance = new RefGC();

  return instance;
}

RefGC::RefGC()
{
  maxRef = 16;
  checkTimes = 0;
}

void* RefGC::newRef(std::size_t size)
{
  void* ptr = std::malloc(size);
  RefGC::inst()->allRef.insert((RefCount*)ptr);
  return ptr;
}

void RefGC::rlzRef(void* ptr)
{
  std::free(ptr);
  RefGC::inst()->allRef.erase((RefCount*)ptr);
}

void RefGC::inUse(RefCountP* ptr)
{
  RefGC::inst()->allInUse.insert(ptr);
}

void RefGC::rmUse(RefCountP* ptr)
{
  RefGC::inst()->allInUse.erase(ptr);
}

void RefGC::trace(RefCountP& p)
{
  if (p) p.mark(true);
}

void RefGC::checkGC()
{
  RefGC* gc = RefGC::inst();
  gc->checkTimes++;
  if (gc->allRef.size() > 0 &&
      (gc->allRef.size() >= gc->maxRef || gc->checkTimes >= 1024*8))
  {
    collect();
    gc->maxRef = gc->allRef.size() > 0 ? gc->allRef.size() * 2 : 16;
    gc->checkTimes = 0;
  }
}

void RefGC::collect()
{
  mark();
  sweep();
}

void RefGC::mark()
{
  RefGC* gc = RefGC::inst();
  // mark in use true
  for (std::set<RefCountP*>::iterator it = gc->allInUse.begin();
       it != gc->allInUse.end(); it++)
  {
    RefCountP* p = *it;
    if (p->mark()) continue;

    p->mark(true);
  }
}

void RefGC::sweep()
{
  std::set<RefCount*> toBeRlz;
  for (std::set<RefCount*>::iterator it = RefGC::inst()->allRef.begin();
       it != RefGC::inst()->allRef.end(); it++)
  {
    RefCount* p = *it;
    if (!p->mark())
    {
      p->dead(true);
      toBeRlz.insert(p);
    }
    else
    {
      p->mark(false);
    }
  }

  for (std::set<RefCount*>::iterator it = toBeRlz.begin();
       it != toBeRlz.end(); it++)
  {
    delete *it;
  }
}

};
