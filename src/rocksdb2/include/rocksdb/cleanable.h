
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//迭代器从源中生成一个键/值对序列。
//下面的类定义接口。多个实现
//由本图书馆提供。特别是，提供了迭代器
//访问表或数据库的内容。
//
//多个线程可以在迭代器上调用const方法，而不需要
//外部同步，但如果任何线程可以调用
//非常量方法，访问同一迭代器的所有线程都必须使用
//外部同步。

#ifndef INCLUDE_ROCKSDB_CLEANABLE_H_
#define INCLUDE_ROCKSDB_CLEANABLE_H_

namespace rocksdb {

class Cleanable {
 public:
  Cleanable();
  ~Cleanable();

//不允许复制构造函数和复制分配。
  Cleanable(Cleanable&) = delete;
  Cleanable& operator=(Cleanable&) = delete;

//允许移动构造函数和移动分配。
  Cleanable(Cleanable&&);
  Cleanable& operator=(Cleanable&&);

//允许客户端注册函数/arg1/arg2三倍于
//将在销毁此迭代器时调用。
//
//请注意，与前面的所有方法不同，此方法是
//不是抽象的，因此客户端不应重写它。
  typedef void (*CleanupFunction)(void* arg1, void* arg2);
  void RegisterCleanup(CleanupFunction function, void* arg1, void* arg2);
  void DelegateCleanupsTo(Cleanable* other);
//docleanup并重置指针以便重用
  inline void Reset() {
    DoCleanup();
    cleanup_.function = nullptr;
    cleanup_.next = nullptr;
  }

 protected:
  struct Cleanup {
    CleanupFunction function;
    void* arg1;
    void* arg2;
    Cleanup* next;
  };
  Cleanup cleanup_;
//它也成为C的所有者
  void RegisterCleanup(Cleanup* c);

 private:
//执行所有清理。它不会重置指针。制造它
//私有的
//防止误用
  inline void DoCleanup() {
    if (cleanup_.function != nullptr) {
      (*cleanup_.function)(cleanup_.arg1, cleanup_.arg2);
      for (Cleanup* c = cleanup_.next; c != nullptr;) {
        (*c->function)(c->arg1, c->arg2);
        Cleanup* next = c->next;
        delete c;
        c = next;
      }
    }
  }
};

}  //命名空间rocksdb

#endif  //包括“RocksDB”可清洁“U H”
