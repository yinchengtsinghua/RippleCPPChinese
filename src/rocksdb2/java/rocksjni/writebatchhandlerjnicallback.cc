
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
//RocksDB：：比较器。

#include "rocksjni/writebatchhandlerjnicallback.h"
#include "rocksjni/portal.h"

namespace rocksdb {
WriteBatchHandlerJniCallback::WriteBatchHandlerJniCallback(
    JNIEnv* env, jobject jWriteBatchHandler)
    : m_env(env) {

//注意：我们希望访问JavaWrreButkHuffer-Lead实例
//跨多个方法调用，因此我们创建一个全局引用
  assert(jWriteBatchHandler != nullptr);
  m_jWriteBatchHandler = env->NewGlobalRef(jWriteBatchHandler);
  if(m_jWriteBatchHandler == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  m_jPutMethodId = WriteBatchHandlerJni::getPutMethodId(env);
  if(m_jPutMethodId == nullptr) {
//引发异常
    return;
  }

  m_jMergeMethodId = WriteBatchHandlerJni::getMergeMethodId(env);
  if(m_jMergeMethodId == nullptr) {
//引发异常
    return;
  }

  m_jDeleteMethodId = WriteBatchHandlerJni::getDeleteMethodId(env);
  if(m_jDeleteMethodId == nullptr) {
//引发异常
    return;
  }

  m_jDeleteRangeMethodId = WriteBatchHandlerJni::getDeleteRangeMethodId(env);
  if (m_jDeleteRangeMethodId == nullptr) {
//引发异常
    return;
  }

  m_jLogDataMethodId = WriteBatchHandlerJni::getLogDataMethodId(env);
  if(m_jLogDataMethodId == nullptr) {
//引发异常
    return;
  }

  m_jContinueMethodId = WriteBatchHandlerJni::getContinueMethodId(env);
  if(m_jContinueMethodId == nullptr) {
//引发异常
    return;
  }
}

void WriteBatchHandlerJniCallback::Put(const Slice& key, const Slice& value) {
  const jbyteArray j_key = sliceToJArray(key);
  if(j_key == nullptr) {
//引发异常
    if(m_env->ExceptionCheck()) {
      m_env->ExceptionDescribe();
    }
    return;
  }

  const jbyteArray j_value = sliceToJArray(value);
  if(j_value == nullptr) {
//引发异常
    if(m_env->ExceptionCheck()) {
      m_env->ExceptionDescribe();
    }
    if(j_key != nullptr) {
      m_env->DeleteLocalRef(j_key);
    }
    return;
  }

  m_env->CallVoidMethod(
      m_jWriteBatchHandler,
      m_jPutMethodId,
      j_key,
      j_value);
  if(m_env->ExceptionCheck()) {
//引发异常
    m_env->ExceptionDescribe();
    if(j_value != nullptr) {
      m_env->DeleteLocalRef(j_value);
    }
    if(j_key != nullptr) {
      m_env->DeleteLocalRef(j_key);
    }
    return;
  }

  if(j_value != nullptr) {
    m_env->DeleteLocalRef(j_value);
  }
  if(j_key != nullptr) {
    m_env->DeleteLocalRef(j_key);
  }
}

void WriteBatchHandlerJniCallback::Merge(const Slice& key, const Slice& value) {
  const jbyteArray j_key = sliceToJArray(key);
  if(j_key == nullptr) {
//引发异常
    if(m_env->ExceptionCheck()) {
      m_env->ExceptionDescribe();
    }
    return;
  }

  const jbyteArray j_value = sliceToJArray(value);
  if(j_value == nullptr) {
//引发异常
    if(m_env->ExceptionCheck()) {
      m_env->ExceptionDescribe();
    }
    if(j_key != nullptr) {
      m_env->DeleteLocalRef(j_key);
    }
    return;
  }

  m_env->CallVoidMethod(
      m_jWriteBatchHandler,
      m_jMergeMethodId,
      j_key,
      j_value);
  if(m_env->ExceptionCheck()) {
//引发异常
    m_env->ExceptionDescribe();
    if(j_value != nullptr) {
      m_env->DeleteLocalRef(j_value);
    }
    if(j_key != nullptr) {
      m_env->DeleteLocalRef(j_key);
    }
    return;
  }

  if(j_value != nullptr) {
    m_env->DeleteLocalRef(j_value);
  }
  if(j_key != nullptr) {
    m_env->DeleteLocalRef(j_key);
  }
}

void WriteBatchHandlerJniCallback::Delete(const Slice& key) {
  const jbyteArray j_key = sliceToJArray(key);
  if(j_key == nullptr) {
//引发异常
    if(m_env->ExceptionCheck()) {
      m_env->ExceptionDescribe();
    }
    return;
  }

  m_env->CallVoidMethod(
      m_jWriteBatchHandler,
      m_jDeleteMethodId,
      j_key);
  if(m_env->ExceptionCheck()) {
//引发异常
    m_env->ExceptionDescribe();
    if(j_key != nullptr) {
      m_env->DeleteLocalRef(j_key);
    }
    return;
  }

  if(j_key != nullptr) {
    m_env->DeleteLocalRef(j_key);
  }
}

void WriteBatchHandlerJniCallback::DeleteRange(const Slice& beginKey,
                                               const Slice& endKey) {
  const jbyteArray j_beginKey = sliceToJArray(beginKey);
  if (j_beginKey == nullptr) {
//引发异常
    if (m_env->ExceptionCheck()) {
      m_env->ExceptionDescribe();
    }
    return;
  }

  const jbyteArray j_endKey = sliceToJArray(beginKey);
  if (j_endKey == nullptr) {
//引发异常
    if (m_env->ExceptionCheck()) {
      m_env->ExceptionDescribe();
    }
    return;
  }

  m_env->CallVoidMethod(m_jWriteBatchHandler, m_jDeleteRangeMethodId,
                        j_beginKey, j_endKey);
  if (m_env->ExceptionCheck()) {
//引发异常
    m_env->ExceptionDescribe();
    if (j_beginKey != nullptr) {
      m_env->DeleteLocalRef(j_beginKey);
    }
    if (j_endKey != nullptr) {
      m_env->DeleteLocalRef(j_endKey);
    }
    return;
  }

  if (j_beginKey != nullptr) {
    m_env->DeleteLocalRef(j_beginKey);
  }

  if (j_endKey != nullptr) {
    m_env->DeleteLocalRef(j_endKey);
  }
}

void WriteBatchHandlerJniCallback::LogData(const Slice& blob) {
  const jbyteArray j_blob = sliceToJArray(blob);
  if(j_blob == nullptr) {
//引发异常
    if(m_env->ExceptionCheck()) {
      m_env->ExceptionDescribe();
    }
    return;
  }

  m_env->CallVoidMethod(
      m_jWriteBatchHandler,
      m_jLogDataMethodId,
      j_blob);
  if(m_env->ExceptionCheck()) {
//引发异常
    m_env->ExceptionDescribe();
    if(j_blob != nullptr) {
      m_env->DeleteLocalRef(j_blob);
    }
    return;
  }

  if(j_blob != nullptr) {
    m_env->DeleteLocalRef(j_blob);
  }
}

bool WriteBatchHandlerJniCallback::Continue() {
  jboolean jContinue = m_env->CallBooleanMethod(
      m_jWriteBatchHandler,
      m_jContinueMethodId);
  if(m_env->ExceptionCheck()) {
//引发异常
    m_env->ExceptionDescribe();
  }

  return static_cast<bool>(jContinue == JNI_TRUE);
}

/*
 *从切片中的数据创建Java字节数组
 *
 *调用此函数时
 *必须记住调用env->deleteLocalRef
 *完成后的结果
 *
 *@ PARAM是一个切片到一个Java字节数组
 *
 *@返回一个Java字节数组的引用，或者返回一个null pTr如果
 *发生异常
 **/

jbyteArray WriteBatchHandlerJniCallback::sliceToJArray(const Slice& s) {
  jbyteArray ja = m_env->NewByteArray(static_cast<jsize>(s.size()));
  if(ja == nullptr) {
//引发异常：OutOfMemoryError
    return nullptr;
  }

  m_env->SetByteArrayRegion(
      ja, 0, static_cast<jsize>(s.size()),
      const_cast<jbyte*>(reinterpret_cast<const jbyte*>(s.data())));
  if(m_env->ExceptionCheck()) {
    if(ja != nullptr) {
      m_env->DeleteLocalRef(ja);
    }
//引发异常：arrayindexoutofboundsException
    return nullptr;
  }

  return ja;
}

WriteBatchHandlerJniCallback::~WriteBatchHandlerJniCallback() {
  if(m_jWriteBatchHandler != nullptr) {
    m_env->DeleteGlobalRef(m_jWriteBatchHandler);
  }
}
}  //命名空间rocksdb
