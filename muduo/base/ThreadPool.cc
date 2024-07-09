// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/ThreadPool.h"

#include "muduo/base/Exception.h"

#include <assert.h>
#include <stdio.h>

using namespace muduo;
/*
构造函数 ThreadPool::ThreadPool(const string& nameArg):

初始化了成员变量 mutex_、notEmpty_、notFull_，分别用于线程同步和条件变量控制。
设置了线程池的名称 name_，最大队列大小 maxQueueSize_ 和运行状态 running_。

析构函数 ThreadPool::~ThreadPool():
如果线程池处于运行状态 (running_ 为 true)，则调用 stop() 方法停止线程池。

void ThreadPool::start(int numThreads):
开启线程池，创建 numThreads 个线程并启动。
每个线程执行 ThreadPool::runInThread 方法。
如果 numThreads 为 0 且存在 threadInitCallback_ 回调函数，则调用回调函数。

void ThreadPool::stop():
停止线程池，将 running_ 设置为 false。
通知所有等待在 notEmpty_ 和 notFull_ 条件变量上的线程。
等待所有线程结束 (join() 操作)。

size_t ThreadPool::queueSize() const:
返回当前任务队列 queue_ 的大小。

void ThreadPool::run(Task task):
如果线程池为空（没有线程），直接执行任务 task。
否则，加锁并等待任务队列 queue_ 不满且线程池处于运行状态，然后将任务加入队列并通知等待中的线程。

ThreadPool::Task ThreadPool::take():
从任务队列中取出任务并返回。如果队列为空且线程池正在运行，则等待直到有任务可取。
取出任务后，如果设置了最大队列大小 maxQueueSize_，则通知等待中的线程队列不满。

bool ThreadPool::isFull() const:
检查任务队列是否已满。

void ThreadPool::runInThread():
线程执行函数，不断从任务队列中取出任务并执行，直到线程池停止运行或出现异常。
*/
ThreadPool::ThreadPool(const string& nameArg)
  : mutex_(),
    notEmpty_(mutex_),
    notFull_(mutex_),
    name_(nameArg),
    maxQueueSize_(0),
    running_(false)
{
}

ThreadPool::~ThreadPool()
{
  if (running_)
  {
    stop();
  }
}

void ThreadPool::start(int numThreads)
{
  assert(threads_.empty());
  running_ = true;
  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i)
  {
    char id[32];
    snprintf(id, sizeof id, "%d", i+1);
    threads_.emplace_back(new muduo::Thread(
          std::bind(&ThreadPool::runInThread, this), name_+id));
    threads_[i]->start();
  }
  if (numThreads == 0 && threadInitCallback_)
  {
    threadInitCallback_();
  }
}

void ThreadPool::stop()
{
  {
  MutexLockGuard lock(mutex_);
  running_ = false;
  notEmpty_.notifyAll();
  notFull_.notifyAll();
  }
  for (auto& thr : threads_)
  {
    thr->join();
  }
}

size_t ThreadPool::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return queue_.size();
}

void ThreadPool::run(Task task)
{
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    while (isFull() && running_)
    {
      notFull_.wait();
    }
    if (!running_) return;
    assert(!isFull());

    queue_.push_back(std::move(task));
    notEmpty_.notify();
  }
}

ThreadPool::Task ThreadPool::take()
{
  MutexLockGuard lock(mutex_);
  // always use a while-loop, due to spurious wakeup
  while (queue_.empty() && running_)
  {
    notEmpty_.wait();
  }
  Task task;
  if (!queue_.empty())
  {
    task = queue_.front();
    queue_.pop_front();
    if (maxQueueSize_ > 0)
    {
      notFull_.notify();
    }
  }
  return task;
}

bool ThreadPool::isFull() const
{
  mutex_.assertLocked();
  return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
  try
  {
    if (threadInitCallback_)
    {
      threadInitCallback_();
    }
    while (running_)
    {
      Task task(take());
      if (task)
      {
        task();
      }
    }
  }
  catch (const Exception& ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    abort();
  }
  catch (const std::exception& ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  }
  catch (...)
  {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
    throw; // rethrow
  }
}

