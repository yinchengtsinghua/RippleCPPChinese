
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

#include <memory>
#include <functional>
#include <type_traits>

namespace rocksdb {
namespace port {

//此类是std:：thread的替换
//我们不喜欢std:：thread的两个原因：
//--它动态地分配其内部，自动
//线程终止时释放，而不是在
//对象。这使得很难控制记忆的来源
//分配
//-这实现了PIMPL，因此我们可以轻松地替换
//如有必要，在我们的私有版本中使用。
class WindowsThread {

  struct Data;

  std::unique_ptr<Data>  data_;
  unsigned int           th_id_;

  void Init(std::function<void()>&&);

public:

  typedef void* native_handle_type;

//无线程构造
  WindowsThread();

//模板构造函数
//
//这个模板化的构造函数完成了几件事
//
//-允许类整体不是模板
//
//-采用“通用”参考来支持值和值
//
//-因为这个构造函数在很多方面都是一个catchall案例，
//可能会阻止我们同时使用默认的移动块。
//此外，它还可以避免复制因子删除。解决这个问题
//我们确保这一个至少有一个论点并消除
//它来自当windowsthread是第一个时的重载选择
//争论。
//
//-使用fx（ax…）构造，类型/参数数量可变。
//
//—将可调用对象及其参数和构造集合在一起
//单个可调用实体
//
//-使用std：：函数将其转换为规范模板
//同时检查签名一致性以确保
//提供了所有必要的参数，并允许pimpl
//实施。
  template<class Fn,
    class... Args,
    class = typename std::enable_if<
      !std::is_same<typename std::decay<Fn>::type,
                    WindowsThread>::value>::type>
  explicit WindowsThread(Fn&& fx, Args&&... ax) :
      WindowsThread() {

//使用活页夹创建单个可调用实体
    auto binder = std::bind(std::forward<Fn>(fx),
      std::forward<Args>(ax)...);
//使用std：：函数利用类型擦除
//所以我们仍然可以在PIMPL中隐藏实现
//这还确保绑定器签名符合
    std::function<void()> target = binder;

    Init(std::move(target));
  }


  ~WindowsThread();

  WindowsThread(const WindowsThread&) = delete;

  WindowsThread& operator=(const WindowsThread&) = delete;

  WindowsThread(WindowsThread&&) noexcept;

  WindowsThread& operator=(WindowsThread&&) noexcept;

  bool joinable() const;

  unsigned int get_id() const { return th_id_; }

  native_handle_type native_handle() const;

  static unsigned hardware_concurrency();

  void join();

  bool detach();

  void swap(WindowsThread&);
};
} //命名空间端口
} //命名空间rocksdb

namespace std {
  inline
  void swap(rocksdb::port::WindowsThread& th1, 
    rocksdb::port::WindowsThread& th2) {
    th1.swap(th2);
  }
} //命名空间性病

