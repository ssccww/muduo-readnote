// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include "muduo/base/noncopyable.h"

#include <assert.h>
#include <pthread.h>

namespace muduo
{
// 每个线程可以通过调用ThreadLocalSingleton<T>:: instance 来获得一个线程独立的T类型变量的引用
// 该对象线程回收时会自动析构
template<typename T> 
class ThreadLocalSingleton : noncopyable
{
 public:
  ThreadLocalSingleton() = delete;
  ~ThreadLocalSingleton() = delete;

  // 每个线程在调用
  static T& instance()
  {
    if (!t_value_)
    {
      t_value_ = new T();
      deleter_.set(t_value_);
    }
    return *t_value_;
  }

  static T* pointer()
  {
    return t_value_;
  }

 private:
 // 帮助线程在析构时释放T对象的内存
  static void destructor(void* obj)
  {
    assert(obj == t_value_);
    // 完全类型：
    /*
       例如 class A; 只声明而未定义，依然可以定义对象
    */
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1]; // 不完全类型会在编译阶段报错
    T_must_be_complete_type dummy; (void) dummy;
    delete t_value_;
    t_value_ = 0;
  }

  class Deleter
  {
   public:
    Deleter()
    {
      pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
    }

    ~Deleter()
    {
      pthread_key_delete(pkey_);
    }

    void set(T* newObj)
    {
      assert(pthread_getspecific(pkey_) == NULL);
      pthread_setspecific(pkey_, newObj);
    }

    pthread_key_t pkey_;
  };

  static __thread T* t_value_;
  static Deleter deleter_;
};

template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}  // namespace muduo
#endif  // MUDUO_BASE_THREADLOCALSINGLETON_H
