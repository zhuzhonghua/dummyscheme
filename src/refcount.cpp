#include "refcount.h"

namespace Dummy {

RefGC* RefGC::instance;

RefGC* RefGC::inst()
{
  if (!instance) instance = new RefGC();
  return instance;
}

void RefGC::rlzRef(RefCount* ptr)
{
  RefGC::inst()->allRef.erase(ptr);
  delete ptr;
}

void RefGC::inUse(RefCountP* ptr)
{
  allInUse.insert(ptr);
}

void RefGC::rmUse(RefCountP* ptr)
{
  allInUse.erase(ptr);
}

void RefGC::trace(RefCountP& p)
{
  if (p) p.mark(true);
}

void RefGC::checkGC()
{
  checkTimes++;
  if (allRef.size() > 0 &&
      (allRef.size() >= maxRef || checkTimes >= 1024*8))
  {
    collect();
    maxRef = allRef.size() > 0 ? allRef.size() * 2 : 16;
    checkTimes = 0;
  }
}

void RefGC::collect()
{
  mark();
  sweep();
}

void RefGC::mark()
{
  typedef std::set<RefCountP*>::iterator RefPSetItr;
  // mark in use true
  for (RefPSetItr it = allInUse.begin(); it != allInUse.end(); it++)
  {
    RefCountP* p = *it;
    if (p->mark()) continue;

    p->mark(true);
  }
}

void RefGC::sweep()
{
  std::set<RefCount*> toBeRlz;
  typedef std::set<RefCount*>::iterator RefSetItr;

  for (RefSetItr it = allRef.begin(); it != allRef.end(); it++)
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

  for (RefSetItr it = toBeRlz.begin(); it != toBeRlz.end(); it++)
  {
    RefCount* p = *it;
    RefGC::rlzRef(p);
  }
}

};
