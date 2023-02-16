#pragma once

namespace Dummy {
class RefCount {
public:
	RefCount() : refcount(0) { }
	virtual ~RefCount() { }

	const RefCount* acquire() { refcount++; return this; }
	int release() { return --refcount; }
	int refCount() const { return refcount; }

private:
	RefCount(const RefCount&); // no copy ctor
	RefCount& operator = (const RefCount&); // no assignments

	int refcount;
};

template<class T>
class RefCountPtr {
public:
	RefCountPtr() :object(0) { }
	RefCountPtr(T* object) :object(0) { acquire(object); }
	RefCountPtr(const RefCountPtr& rhs) :object(0) { acquire(rhs.object); }

	const RefCountPtr& operator = (const RefCountPtr& rhs) {
		acquire(rhs.object);
		return *this;
	}

	bool operator == (const RefCountPtr& rhs) const {
		return object == rhs.object;
	}

	bool operator != (const RefCountPtr& rhs) const {
		return object != rhs.object;
	}

	operator bool() const {
		return object != NULL;
	}

	~RefCountPtr() {
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
};
