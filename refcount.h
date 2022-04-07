#pragma once

#include <vector>

namespace DummyScheme {
class DummyRefCount {
public:
	DummyRefCount() : refcount(0) { }
	virtual ~DummyRefCount() { }

	const DummyRefCount* acquire() { refcount++; return this; }
	int release() { return --refcount; }
	int refCount() const { return refcount; }

private:
	DummyRefCount(const DummyRefCount&); // no copy ctor
	DummyRefCount& operator = (const DummyRefCount&); // no assignments

	int refcount;
};

template<class T>
class DummyRefCountPtr {
public:
	DummyRefCountPtr():object(0) { }
	DummyRefCountPtr(T* object):object(0) { acquire(object); }
	DummyRefCountPtr(const DummyRefCountPtr& rhs):object(0) { acquire(rhs.object); }

	const DummyRefCountPtr& operator = (const DummyRefCountPtr& rhs) {
		acquire(rhs.object);
		return *this;
	}

	bool operator == (const DummyRefCountPtr& rhs) const {
		return object == rhs.object;
	}

	bool operator != (const DummyRefCountPtr& rhs) const {
		return object != rhs.object;
	}

	operator bool () const {
		return object != NULL;
	}

	~DummyRefCountPtr() {
		release();
	}

	T* operator -> () const { return object; }
	T* ptr() const { return object; }

private:
	void acquire(T* obj) {
		if (obj != NULL) {
			obj->acquire();
		}
		release();
		object = obj;
	}

	void release() {
		if ((object != NULL) && (object->release() == 0)) {
			delete object;
		}
	}

	T* object;
};
}
