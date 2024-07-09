// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include "muduo/base/Types.h"

namespace muduo
{
/*
t_cachedTid：一个线程局部变量，用于缓存线程的唯一标识符（TID）。
t_tidString：一个线程局部字符数组，用于存储线程的标识符字符串。
t_tidStringLength：一个线程局部变量，表示线程标识符字符串的长度。
t_threadName：一个线程局部指针，指向线程的名称。
cacheTid()：用于缓存线程的唯一标识符（TID）的函数。
tid()：获取当前线程的唯一标识符（TID）的函数，如果 t_cachedTid 为 0，则调用 cacheTid() 缓存 TID。
tidString()：返回线程的标识符字符串的函数，用于日志记录。
tidStringLength()：返回线程标识符字符串的长度的函数，用于日志记录。
name()：返回当前线程的名称。
isMainThread()：判断当前线程是否是主线程的函数。
sleepUsec(int64_t usec)：用于让当前线程休眠一段时间（以微秒为单位），用于测试。
stackTrace(bool demangle)：生成当前线程的调用栈信息，可以选择是否进行符号解析（demangle）。
*/
namespace CurrentThread
{
  // internal
  extern __thread int t_cachedTid; //保存线程的id
  extern __thread char t_tidString[32];
  extern __thread int t_tidStringLength;
  extern __thread const char* t_threadName;
  void cacheTid();

  inline int tid()
  {
    //__builtin_expect ：GCC 提供的一个内建函数，用于提示编译器代码中条件表达式的概率，从而帮助编译器进行优化
    // 在这里只有第一次调用时t_cachedTid == 0成立，所以这里的代码表示认为执行到此处时t_cachedTid == 0的概率很低，if分支的语句大概率不被执行，会以此优化编译的代码
    if (__builtin_expect(t_cachedTid == 0, 0)) 
    {
      cacheTid();
    }
    return t_cachedTid;
  }

  inline const char* tidString() // for logging
  {
    return t_tidString;
  }

  inline int tidStringLength() // for logging
  {
    return t_tidStringLength;
  }

  inline const char* name()
  {
    return t_threadName;
  }

  bool isMainThread();

  void sleepUsec(int64_t usec);  // for testing

  string stackTrace(bool demangle);
}  // namespace CurrentThread
}  // namespace muduo

#endif  // MUDUO_BASE_CURRENTTHREAD_H
