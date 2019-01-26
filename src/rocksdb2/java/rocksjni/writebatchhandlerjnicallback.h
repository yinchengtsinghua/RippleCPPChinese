
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
//rocksdb:：writebatch:：handler。

#ifndef JAVA_ROCKSJNI_WRITEBATCHHANDLERJNICALLBACK_H_
#define JAVA_ROCKSJNI_WRITEBATCHHANDLERJNICALLBACK_H_

#include <jni.h>
#include "rocksdb/write_batch.h"

namespace rocksdb {
/*
 *该类充当C++之间的桥梁
 *和Java。此类中的方法将是
 *从ROCKSDB存储引擎（C++）调用
 *调用适当的Java方法。
 *这使得写批处理程序可以在Java中实现。
 **/

class WriteBatchHandlerJniCallback : public WriteBatch::Handler {
 public:
    WriteBatchHandlerJniCallback(
      JNIEnv* env, jobject jWriteBackHandler);
    ~WriteBatchHandlerJniCallback();
    void Put(const Slice& key, const Slice& value);
    void Merge(const Slice& key, const Slice& value);
    void Delete(const Slice& key);
    void DeleteRange(const Slice& beginKey, const Slice& endKey);
    void LogData(const Slice& blob);
    bool Continue();

 private:
    JNIEnv* m_env;
    jobject m_jWriteBatchHandler;
    jbyteArray sliceToJArray(const Slice& s);
    jmethodID m_jPutMethodId;
    jmethodID m_jMergeMethodId;
    jmethodID m_jDeleteMethodId;
    jmethodID m_jDeleteRangeMethodId;
    jmethodID m_jLogDataMethodId;
    jmethodID m_jContinueMethodId;
};
}  //命名空间rocksdb

#endif  //java_rocksjni_writebatchhandlerjnicallback_h_
