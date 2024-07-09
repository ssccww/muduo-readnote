// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/Exception.h"
#include "muduo/base/CurrentThread.h"

namespace muduo
{

Exception::Exception(string msg)
  : message_(std::move(msg)),
    // 输入参数为一个bool值，为true时进行函数名称的转换，为false时不进行
    stack_(CurrentThread::stackTrace(/*demangle=*/false))
{
}

}  // namespace muduo
