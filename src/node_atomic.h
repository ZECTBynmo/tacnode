#ifndef NODE_ATOMIC_H_
#define NODE_ATOMIC_H_

#include "atomic_ops.h"
#include "node_internals.h"
#include "uv.h" // kind of annoying, ngx_queue_t is exposed by uv.h

namespace node {

template <class T>
class AtomicValue {
public:
  AtomicValue(): value_(0) {
  }

  AtomicValue(T value): value_((AO_t) value) {
    BUILD_BUG_ON(sizeof(value) > sizeof(value_));
  }

  T Get() {
    return (T) AO_load_read(&value_);
  }

  T Swap(T value) {
    AO_t old_value, new_value;

    new_value = (AO_t) value;
    do
      old_value = value_;
    while (!AO_compare_and_swap_full(&value_, old_value, new_value));

    return (T) old_value;
  }

private:
  DISALLOW_COPY_AND_ASSIGN(AtomicValue);
  AO_t value_;
};


class Mutex {
public:
  Mutex(): mutex_(&mutex_storage_) {
    if (uv_mutex_init(&mutex_storage_)) abort();
  }

  ~Mutex() {
    // Acquire and release the mutex before destroying it
    // to make sure it's not in use by another thread.
    uv_mutex_t* mutex = mutex_.Swap(NULL);
    uv_mutex_lock(mutex);
    uv_mutex_unlock(mutex);
    uv_mutex_destroy(mutex);
  }

  class ScopedLock {
  public:
    ScopedLock(Mutex& mutex): parent_(mutex) {
      uv_mutex_lock(parent_.mutex_.Get());
    }

    ~ScopedLock() {
      uv_mutex_unlock(parent_.mutex_.Get());
    }

  private:
    DISALLOW_IMPLICIT_CONSTRUCTORS(ScopedLock);
    Mutex& parent_;
  };

private:
  DISALLOW_COPY_AND_ASSIGN(Mutex);
  AtomicValue<uv_mutex_t*> mutex_;
  uv_mutex_t mutex_storage_;
};


typedef ngx_queue_t AtomicQueueEntry;


template <class T>
class AtomicQueue {
public:
  AtomicQueue() {
    ngx_queue_init(&queue_);
  }

  void Insert(T* item) {
    Mutex::ScopedLock scoped_lock(mutex_);
    ngx_queue_t* q = GetEntry(item);
    ngx_queue_insert_head(&queue_, q);
  }

  void Remove(T* item) {
    Mutex::ScopedLock scoped_lock(mutex_);
    ngx_queue_t* q = GetEntry(item);
    ngx_queue_remove(q);
  }

  T* Pop() {
    Mutex::ScopedLock scoped_lock(mutex_);

    if (ngx_queue_empty(&queue_))
      return NULL;

    ngx_queue_t* q = ngx_queue_head(&queue_);
    ngx_queue_remove(q);

    return GetItem(q);
  }

private:
  ngx_queue_t queue_;
  Mutex mutex_;

  ngx_queue_t* GetEntry(T* item) {
    char* addr = reinterpret_cast<char*>(item) + offset_of(T, queue_entry_);
    return reinterpret_cast<ngx_queue_t*>(addr);
  }

  T* GetItem(ngx_queue_t* q) {
    // gcc doesn't like that ngx_queue_data() uses a NULL offset
    return container_of(q, T, queue_entry_);
  }
};

} // namespace node

#endif // NODE_ATOMIC_H_
