
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include "port/win/win_thread.h"

#include <assert.h>
#include <process.h> //新一代
#include <windows.h>

#include <stdexcept>
#include <system_error>
#include <thread>

namespace rocksdb {
namespace port {

struct WindowsThread::Data {

  std::function<void()> func_;
  uintptr_t             handle_;

  Data(std::function<void()>&& func) :
    func_(std::move(func)),
    handle_(0) {
  }

  Data(const Data&) = delete;
  Data& operator=(const Data&) = delete;

  static unsigned int __stdcall ThreadProc(void* arg);
};


void WindowsThread::Init(std::function<void()>&& func) {

  data_.reset(new Data(std::move(func)));

  data_->handle_ = _beginthreadex(NULL,
0,    //堆栈大小
    &Data::ThreadProc,
    data_.get(),
0,   //初始化标志
    &th_id_);

  if (data_->handle_ == 0) {
    throw std::system_error(std::make_error_code(
      std::errc::resource_unavailable_try_again),
      "Unable to create a thread");
  }
}

WindowsThread::WindowsThread() :
  data_(nullptr),
  th_id_(0)
{}


WindowsThread::~WindowsThread() {
//必须连接或分离
//在毁灭之前。
//这与std:：thread相同
  if (data_) {
    if (joinable()) {
      assert(false);
      std::terminate();
    }
    data_.reset();
  }
}

WindowsThread::WindowsThread(WindowsThread&& o) noexcept :
  WindowsThread() {
  *this = std::move(o);
}

WindowsThread& WindowsThread::operator=(WindowsThread&& o) noexcept {

  if (joinable()) {
    assert(false);
    std::terminate();
  }

  data_ = std::move(o.data_);

//根据规范，两个实例将具有相同的ID
  th_id_ = o.th_id_;

  return *this;
}

bool WindowsThread::joinable() const {
  return (data_ && data_->handle_ != 0);
}

WindowsThread::native_handle_type WindowsThread::native_handle() const {
  return reinterpret_cast<native_handle_type>(data_->handle_);
}

unsigned WindowsThread::hardware_concurrency() {
  return std::thread::hardware_concurrency();
}

void WindowsThread::join() {

  if (!joinable()) {
    assert(false);
    throw std::system_error(
      std::make_error_code(std::errc::invalid_argument),
      "Thread is no longer joinable");
  }

  if (GetThreadId(GetCurrentThread()) == th_id_) {
    assert(false);
    throw std::system_error(
      std::make_error_code(std::errc::resource_deadlock_would_occur),
      "Can not join itself");
  }

  auto ret = WaitForSingleObject(reinterpret_cast<HANDLE>(data_->handle_),
    INFINITE);
  if (ret != WAIT_OBJECT_0) {
    auto lastError = GetLastError();
    assert(false);
    throw std::system_error(static_cast<int>(lastError),
      std::system_category(),
      "WaitForSingleObjectFailed");
  }

  CloseHandle(reinterpret_cast<HANDLE>(data_->handle_));
  data_->handle_ = 0;
}

bool WindowsThread::detach() {

  if (!joinable()) {
    assert(false);
    throw std::system_error(
      std::make_error_code(std::errc::invalid_argument),
      "Thread is no longer available");
  }

  BOOL ret = CloseHandle(reinterpret_cast<HANDLE>(data_->handle_));
  data_->handle_ = 0;

  return (ret == TRUE);
}

void  WindowsThread::swap(WindowsThread& o) {
  data_.swap(o.data_);
  std::swap(th_id_, o.th_id_);
}

unsigned int __stdcall  WindowsThread::Data::ThreadProc(void* arg) {
  auto data = reinterpret_cast<WindowsThread::Data*>(arg);
  data->func_();
  _endthreadex(0);
  return 0;
}
} //命名空间端口
} //命名空间rocksdb
