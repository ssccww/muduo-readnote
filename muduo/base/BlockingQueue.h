// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"

#include <deque>
#include <assert.h>

namespace muduo
{

//提供了一个线程安全的阻塞队列
/*
使用 std::deque<T> 作为底层容器来存储元素。
提供了两个 put 方法，一个接受左值引用（const T&），另一个接受右值引用（T&&），用于向队列中添加元素。右值引用版本的 put 使用了移动语义来避免不必要的拷贝操作。
提供了 take 方法用于从队列中取出元素。在队列为空时，使用条件变量 notEmpty_ 进行等待，直到队列非空时才返回元素。
提供了 drain 方法用于将队列中的所有元素取出并返回一个新的 std::deque<T>，同时清空原队列。
提供了 size 方法用于获取队列中元素的数量。
*/
template<typename T>
class BlockingQueue : noncopyable //不可复制
{
 public:
  using queue_type = std::deque<T>;

  BlockingQueue()
    : mutex_(),
      notEmpty_(mutex_),
      queue_()
  {
  }

  void put(const T& x)
  {
    MutexLockGuard lock(mutex_);
    queue_.push_back(x);
    notEmpty_.notify(); // wait morphing saves us
    // http://www.domaigne.com/blog/computing/condvars-signal-with-mutex-locked-or-not/
  }

  void put(T&& x)
  {
    MutexLockGuard lock(mutex_);
    queue_.push_back(std::move(x));
    notEmpty_.notify();
  }

  T take()
  {
    MutexLockGuard lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty())
    {
      notEmpty_.wait();
    }
    assert(!queue_.empty());
    T front(std::move(queue_.front()));
    queue_.pop_front();
    return front;
  }

  queue_type drain()
  {
    std::deque<T> queue;
    {
      MutexLockGuard lock(mutex_);
      queue = std::move(queue_);
      assert(queue_.empty());
    }
    return queue;
  }

  size_t size() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.size();
  }

 private:
  mutable MutexLock mutex_;
  Condition         notEmpty_ GUARDED_BY(mutex_);
  queue_type        queue_ GUARDED_BY(mutex_);
};  // __attribute__ ((aligned (64)));

}  // namespace muduo

#endif  // MUDUO_BASE_BLOCKINGQUEUE_H
