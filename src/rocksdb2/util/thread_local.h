
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

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "util/autovector.h"
#include "port/port.h"

namespace rocksdb {

//将为本地存储线程调用的清理函数
//当发生下列情况之一时，指针（如果不是空值）：
//（1）线程终止
//（2）threadlocalptr被销毁
typedef void (*UnrefHandler)(void* ptr);

//threadlocalptr只存储指针类型的值。不同于
//通常的线程本地存储threadlocalptr能够
//区分来自不同线程和不同
//threadLocalptr实例。例如，如果一个普通线程
//变量A在dbimpl中声明，两个dbimpl对象将共享
//但是，在
//dbimpl的作用域可以避免这种冲突。因此，它的记忆
//用法为o（of threads*of threadLocalptr instances）。
class ThreadLocalPtr {
 public:
  explicit ThreadLocalPtr(UnrefHandler handler = nullptr);

  ~ThreadLocalPtr();

//返回存储在线程本地中的当前指针
  void* Get() const;

//为线程本地存储设置新的指针值。
  void Reset(void* ptr);

//自动交换提供的ptr并返回上一个值
  void* Swap(void* ptr);

//将存储值与预期值进行原子比较。设置新的
//仅当比较为真时，指向线程本地的指针值。
//否则，expected返回存储值。
//成功时返回真，失败时返回假
  bool CompareAndSwap(void* ptr, void*& expected);

//将所有线程本地数据重置为替换，并返回非nullptr
//所有现有线程的数据
  void Scrape(autovector<void*>* ptrs, void* const replacement);

  typedef std::function<void(void*, void*)> FoldFunc;
//通过对每个线程的本地值应用func来更新res。拿着一把锁
//阻止UNREF处理程序在此调用期间运行，但客户端必须
//仍然提供外部同步，因为拥有的线程可以
//在不进行内部锁定的情况下访问值，例如，通过get（）和reset（）。
  void Fold(FoldFunc func, void* res);

//添加到此处进行测试
//返回下一个可用的ID而不声明它
  static uint32_t TEST_PeekId();

//初始化threadlocalptr的静态单例。
//
//如果不调用此函数，则单例
//使用时自动初始化。
//
//调用此函数两次或在
//初始化将为no-op。
  static void InitSingletons();

  class StaticMeta;

private:

  static StaticMeta* Instance();

  const uint32_t id_;
};

}  //命名空间rocksdb
