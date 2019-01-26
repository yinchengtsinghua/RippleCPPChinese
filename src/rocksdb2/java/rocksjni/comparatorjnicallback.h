
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
//这个文件实现了Java和C++之间的回调“桥”。
//rocksdb:：comparator和rocksdb:：directcomparator。

#ifndef JAVA_ROCKSJNI_COMPARATORJNICALLBACK_H_
#define JAVA_ROCKSJNI_COMPARATORJNICALLBACK_H_

#include <jni.h>
#include <string>
#include "rocksdb/comparator.h"
#include "rocksdb/slice.h"
#include "port/port.h"

namespace rocksdb {

struct ComparatorJniCallbackOptions {
//使用自适应互斥体，该互斥体在用户空间中旋转，然后再诉诸
//到内核。当互斥体不是
//竞争激烈。但是，如果互斥是热的，我们可能会结束
//浪费旋转时间。
//默认：假
  bool use_adaptive_mutex;

  ComparatorJniCallbackOptions() : use_adaptive_mutex(false) {
  }
};

/*
 *该类充当C++之间的桥梁
 *和Java。此类中的方法将是
 *从ROCKSDB存储引擎（C++）调用
 *然后回调到适当的Java方法
 *这使得比较器可以在Java中实现。
 *
 *这个比较器的设计缓存了Java切片
 *在比较和查找短分隔符中使用的对象
 *方法回调。而不是为每个回调创建新对象
 *在这些函数中，通过sethandle重用，我们有很多
 *更快；不幸的是，这意味着我们必须
 *在每个方法的区域中引入独立的锁定
 *分别通过mutex mtx_compare和mtx_findshortestseparator
 **/

class BaseComparatorJniCallback : public Comparator {
 public:
    BaseComparatorJniCallback(
      JNIEnv* env, jobject jComparator,
      const ComparatorJniCallbackOptions* copt);
    virtual ~BaseComparatorJniCallback();
    virtual const char* Name() const;
    virtual int Compare(const Slice& a, const Slice& b) const;
    virtual void FindShortestSeparator(
      std::string* start, const Slice& limit) const;
    virtual void FindShortSuccessor(std::string* key) const;

 private:
//用于比较法中的同步
    port::Mutex* mtx_compare;
//用于findshortestseparator方法中的同步
    port::Mutex* mtx_findShortestSeparator;
    jobject m_jComparator;
    std::string m_name;
    jmethodID m_jCompareMethodId;
    jmethodID m_jFindShortestSeparatorMethodId;
    jmethodID m_jFindShortSuccessorMethodId;

 protected:
    JavaVM* m_jvm;
    jobject m_jSliceA;
    jobject m_jSliceB;
    jobject m_jSliceLimit;
};

class ComparatorJniCallback : public BaseComparatorJniCallback {
 public:
      ComparatorJniCallback(
        JNIEnv* env, jobject jComparator,
        const ComparatorJniCallbackOptions* copt);
      ~ComparatorJniCallback();
};

class DirectComparatorJniCallback : public BaseComparatorJniCallback {
 public:
      DirectComparatorJniCallback(
        JNIEnv* env, jobject jComparator,
        const ComparatorJniCallbackOptions* copt);
      ~DirectComparatorJniCallback();
};
}  //命名空间rocksdb

#endif  //java_rocksjni_comparatorjnicallback_h_
