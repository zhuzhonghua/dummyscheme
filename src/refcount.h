#pragma once

#include <set>

namespace Dummy {

class RefCount;
class RefCountP;

class RefGC {
public:
  static void newRef(RefCount* ptr) {
    RefGC::inst()->collect();
    RefGC::inst()->allRef.insert(ptr);
  }

  static void rlzRef(RefCount* ptr) {
    RefGC::inst()->allRef.erase(ptr);
    delete ptr;
  }

  void inUse(RefCountP* ptr);
  void rmUse(RefCountP* ptr);

  void checkGC();

  void collect();
  void mark();
  void sweep();
public:
  static void trace(RefCountP& p);
  static RefGC* inst();
protected:
  RefGC() {}
  static RefGC* instance;
protected:
  std::set<RefCount*> allRef;
  std::set<RefCountP*> allInUse;
  int maxRef;
  int checkTimes;
};

class RefCount {
public:
	RefCount() : refcount(0), marked(false), white(false) { }
	virtual ~RefCount() { }

	const RefCount* acquire() { refcount++; return this; }
	int release() { return --refcount; }
	int refCount() const { return refcount; }
  bool mark() const { return marked; }
  void mark(bool v) { marked = v; }

  bool dead() const { return white; }
  void dead(bool v) { white = v; }

  virtual void trace() = 0;
private:
//  static void* operator new(std::size_t size);
//  static void operator delete(void* ptr);
private:
	RefCount(const RefCount&); // no copy ctor
	RefCount& operator = (const RefCount&); // no assignments

	int refcount;
  bool marked;
  bool white;
};

class RefCountP {
public:
  virtual void mark(bool v) = 0;
  virtual bool mark() = 0;
  virtual operator bool() const = 0;
};

template<class T>
class RefCountPtr : public RefCountP{
public:
  RefCountPtr() :object(0) { }
	RefCountPtr(T* object) :object(0) { acquire(object); }
	RefCountPtr(const RefCountPtr& rhs) :object(0) { acquire(rhs.object); }

	const RefCountPtr& operator = (const RefCountPtr& rhs) {
		acquire(rhs.object);
		return *this;
	}

  bool operator < (const RefCountPtr& rhs) const {
    return object < rhs.object;
  }

	bool operator == (const RefCountPtr& rhs) const {
		return object == rhs.object;
	}

	bool operator != (const RefCountPtr& rhs) const {
		return object != rhs.object;
	}

	operator bool() const {
//    return object != NULL;
    if (!object) return false;
    else return object->operator bool();
	}

  bool mark() {
    return object != NULL ? object->mark() : true;
  }

  void mark(bool v) {
    if (object != NULL && !object->mark()) {
      object->RefCount::mark(v);
      object->trace();
    }
  }

	~RefCountPtr() {
		release();
	}

	T* operator -> () const { return object; }
	T* ptr() const { return object; }

protected:
	void acquire(T* obj) {
		if (obj != NULL) {
			obj->acquire();
		}
		release();
		object = obj;
	}

	void release() {
		if (object != NULL && object->release() == 0 && !object->dead()) {
      RefGC::rlzRef(object);
		}
	}

	T* object;
};

template<class T>
class RefMemberPtr : public RefCountPtr<T>{
public:
  RefMemberPtr():RefCountPtr<T>() { }
  RefMemberPtr(T* object): RefCountPtr<T>(object) {}
  RefMemberPtr(const RefCountPtr<T>& rhs): RefCountPtr<T>(rhs) {}

  const RefMemberPtr& operator = (const RefCountPtr<T>& rhs) {
		RefCountPtr<T>::operator = (rhs);
		return *this;
	}

};

template<class T>
class RefVarPtr : public RefCountPtr<T> {
public:
  RefVarPtr():RefCountPtr<T>() {
    RefGC::inst()->inUse(this);
  }

  RefVarPtr(T* object): RefCountPtr<T>(object) {
    RefGC::inst()->inUse(this);
  }

  RefVarPtr(const RefVarPtr<T>& rhs): RefCountPtr<T>(rhs) {
    RefGC::inst()->inUse(this);
  }

  RefVarPtr(const RefCountPtr<T>& rhs): RefCountPtr<T>(rhs) {
    RefGC::inst()->inUse(this);
  }

  const RefVarPtr& operator = (const RefCountPtr<T>& rhs) {
		RefCountPtr<T>::operator = (rhs);
		return *this;
	}

  const RefVarPtr& operator = (T* object) {
		RefCountPtr<T>::acquire(object);
		return *this;
	}

  ~RefVarPtr() {
    RefGC::inst()->rmUse(this);
  }
};


};
