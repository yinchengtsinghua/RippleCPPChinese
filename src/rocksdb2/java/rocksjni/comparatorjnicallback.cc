
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

#include "rocksjni/comparatorjnicallback.h"
#include "rocksjni/portal.h"

namespace rocksdb {
BaseComparatorJniCallback::BaseComparatorJniCallback(
    JNIEnv* env, jobject jComparator,
    const ComparatorJniCallbackOptions* copt)
    : mtx_compare(new port::Mutex(copt->use_adaptive_mutex)),
    mtx_findShortestSeparator(new port::Mutex(copt->use_adaptive_mutex)) {
//注：比较器方法可以通过多个线程访问，
//所以我们引用的是JVM而不是ENV
  const jint rs = env->GetJavaVM(&m_jvm);
  if(rs != JNI_OK) {
//引发异常
    return;
  }

//注意：我们希望访问Java比较器实例
//跨多个方法调用，因此我们创建一个全局引用
  assert(jComparator != nullptr);
  m_jComparator = env->NewGlobalRef(jComparator);
  if(m_jComparator == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

//注意：比较器的名称在其生命周期中不会改变，
//所以我们把它缓存在一个全局变量中
  jmethodID jNameMethodId = AbstractComparatorJni::getNameMethodId(env);
  if(jNameMethodId == nullptr) {
//引发异常：NoSuchMethodException或OutOfMemoryError
    return;
  }
  jstring jsName = (jstring)env->CallObjectMethod(m_jComparator, jNameMethodId);
  if(env->ExceptionCheck()) {
//引发异常
    return;
  }
  jboolean has_exception = JNI_FALSE;
  m_name = JniUtil::copyString(env, jsName,
&has_exception);  //同时发布jsname
  if (has_exception == JNI_TRUE) {
//引发异常
    return;
  }

  m_jCompareMethodId = AbstractComparatorJni::getCompareMethodId(env);
  if(m_jCompareMethodId == nullptr) {
//引发异常：NoSuchMethodException或OutOfMemoryError
    return;
  }

  m_jFindShortestSeparatorMethodId =
    AbstractComparatorJni::getFindShortestSeparatorMethodId(env);
  if(m_jFindShortestSeparatorMethodId == nullptr) {
//引发异常：NoSuchMethodException或OutOfMemoryError
    return;
  }

  m_jFindShortSuccessorMethodId =
    AbstractComparatorJni::getFindShortSuccessorMethodId(env);
  if(m_jFindShortSuccessorMethodId == nullptr) {
//引发异常：NoSuchMethodException或OutOfMemoryError
    return;
  }
}

const char* BaseComparatorJniCallback::Name() const {
  return m_name.c_str();
}

int BaseComparatorJniCallback::Compare(const Slice& a, const Slice& b) const {
  jboolean attached_thread = JNI_FALSE;
  JNIEnv* env = JniUtil::getJniEnv(m_jvm, &attached_thread);
  assert(env != nullptr);

//TODO（adamretter）：可以使用线程缓存切片对象
//避免锁定的局部变量。可以使此配置取决于
//性能。
  mtx_compare->Lock();

  bool pending_exception =
      AbstractSliceJni::setHandle(env, m_jSliceA, &a, JNI_FALSE);
  if(pending_exception) {
    if(env->ExceptionCheck()) {
//从sethandle或子代引发的异常
env->ExceptionDescribe(); //将异常打印到stderr
    }
    JniUtil::releaseJniEnv(m_jvm, attached_thread);
    return 0;
  }

  pending_exception =
      AbstractSliceJni::setHandle(env, m_jSliceB, &b, JNI_FALSE);
  if(pending_exception) {
    if(env->ExceptionCheck()) {
//从sethandle或子代引发的异常
env->ExceptionDescribe(); //将异常打印到stderr
    }
    JniUtil::releaseJniEnv(m_jvm, attached_thread);
    return 0;
  }
  
  jint result =
    env->CallIntMethod(m_jComparator, m_jCompareMethodId, m_jSliceA,
      m_jSliceB);

  mtx_compare->Unlock();

  if(env->ExceptionCheck()) {
//从callintMethod引发的异常
env->ExceptionDescribe(); //将异常打印到stderr
result = 0; //我们无法从Java回调中得到结果，所以使用0
  }

  JniUtil::releaseJniEnv(m_jvm, attached_thread);

  return result;
}

void BaseComparatorJniCallback::FindShortestSeparator(
  std::string* start, const Slice& limit) const {
  if (start == nullptr) {
    return;
  }

  jboolean attached_thread = JNI_FALSE;
  JNIEnv* env = JniUtil::getJniEnv(m_jvm, &attached_thread);
  assert(env != nullptr);

  const char* startUtf = start->c_str();
  jstring jsStart = env->NewStringUTF(startUtf);
  if(jsStart == nullptr) {
//无法构造字符串
    if(env->ExceptionCheck()) {
env->ExceptionDescribe(); //将异常打印到stderr
    }
    JniUtil::releaseJniEnv(m_jvm, attached_thread);
    return;
  }
  if(env->ExceptionCheck()) {
//引发异常：OutOfMemoryError
env->ExceptionDescribe(); //将异常打印到stderr
    env->DeleteLocalRef(jsStart);
    JniUtil::releaseJniEnv(m_jvm, attached_thread);
    return;
  }

//TODO（adamretter）：可以使用线程本地缓存切片对象
//变量以避免锁定。可以使此配置取决于
//性能。
  mtx_findShortestSeparator->Lock();

  bool pending_exception =
      AbstractSliceJni::setHandle(env, m_jSliceLimit, &limit, JNI_FALSE);
  if(pending_exception) {
    if(env->ExceptionCheck()) {
//从sethandle或子代引发的异常
env->ExceptionDescribe(); //将异常打印到stderr
    }
    if(jsStart != nullptr) {
      env->DeleteLocalRef(jsStart);
    }
    JniUtil::releaseJniEnv(m_jvm, attached_thread);
    return;
  }

  jstring jsResultStart =
    (jstring)env->CallObjectMethod(m_jComparator,
      m_jFindShortestSeparatorMethodId, jsStart, m_jSliceLimit);

  mtx_findShortestSeparator->Unlock();

  if(env->ExceptionCheck()) {
//从CallObjectMethod引发的异常
env->ExceptionDescribe();  //将异常打印到stderr
    env->DeleteLocalRef(jsStart);
    JniUtil::releaseJniEnv(m_jvm, attached_thread);
    return;
  }

  env->DeleteLocalRef(jsStart);

  if (jsResultStart != nullptr) {
//从结果开始更新
    jboolean has_exception = JNI_FALSE;
    std::string result = JniUtil::copyString(env, jsResultStart,
&has_exception);  //同时发布JSresultstart
    if (has_exception == JNI_TRUE) {
      if (env->ExceptionCheck()) {
env->ExceptionDescribe();  //将异常打印到stderr
      }
      JniUtil::releaseJniEnv(m_jvm, attached_thread);
      return;
    }

    *start = result;
  }

  JniUtil::releaseJniEnv(m_jvm, attached_thread);
}

void BaseComparatorJniCallback::FindShortSuccessor(std::string* key) const {
  if (key == nullptr) {
    return;
  }

  jboolean attached_thread = JNI_FALSE;
  JNIEnv* env = JniUtil::getJniEnv(m_jvm, &attached_thread);
  assert(env != nullptr);

  const char* keyUtf = key->c_str();
  jstring jsKey = env->NewStringUTF(keyUtf);
  if(jsKey == nullptr) {
//无法构造字符串
    if(env->ExceptionCheck()) {
env->ExceptionDescribe(); //将异常打印到stderr
    }
    JniUtil::releaseJniEnv(m_jvm, attached_thread);
    return;
  } else if(env->ExceptionCheck()) {
//引发异常：OutOfMemoryError
env->ExceptionDescribe(); //将异常打印到stderr
    env->DeleteLocalRef(jsKey);
    JniUtil::releaseJniEnv(m_jvm, attached_thread);
    return;
  }

  jstring jsResultKey =
    (jstring)env->CallObjectMethod(m_jComparator,
      m_jFindShortSuccessorMethodId, jsKey);

  if(env->ExceptionCheck()) {
//从CallObjectMethod引发的异常
env->ExceptionDescribe(); //将异常打印到stderr
    env->DeleteLocalRef(jsKey);
    JniUtil::releaseJniEnv(m_jvm, attached_thread);
    return;
  }

  env->DeleteLocalRef(jsKey);

  if (jsResultKey != nullptr) {
//用result更新key，还发布jresultkey。
    jboolean has_exception = JNI_FALSE;
    std::string result = JniUtil::copyString(env, jsResultKey, &has_exception);
    if (has_exception == JNI_TRUE) {
      if (env->ExceptionCheck()) {
env->ExceptionDescribe();  //将异常打印到stderr
      }
      JniUtil::releaseJniEnv(m_jvm, attached_thread);
      return;
    }

    *key = result;
  }

  JniUtil::releaseJniEnv(m_jvm, attached_thread);
}

BaseComparatorJniCallback::~BaseComparatorJniCallback() {
  jboolean attached_thread = JNI_FALSE;
  JNIEnv* env = JniUtil::getJniEnv(m_jvm, &attached_thread);
  assert(env != nullptr);

  if(m_jComparator != nullptr) {
    env->DeleteGlobalRef(m_jComparator);
  }

  JniUtil::releaseJniEnv(m_jvm, attached_thread);
}

ComparatorJniCallback::ComparatorJniCallback(
    JNIEnv* env, jobject jComparator,
    const ComparatorJniCallbackOptions* copt) :
    BaseComparatorJniCallback(env, jComparator, copt) {
  m_jSliceA = env->NewGlobalRef(SliceJni::construct0(env));
  if(m_jSliceA == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  m_jSliceB = env->NewGlobalRef(SliceJni::construct0(env));
  if(m_jSliceB == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  m_jSliceLimit = env->NewGlobalRef(SliceJni::construct0(env));
  if(m_jSliceLimit == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
}

ComparatorJniCallback::~ComparatorJniCallback() {
  jboolean attached_thread = JNI_FALSE;
  JNIEnv* env = JniUtil::getJniEnv(m_jvm, &attached_thread);
  assert(env != nullptr);

  if(m_jSliceA != nullptr) {
    env->DeleteGlobalRef(m_jSliceA);
  }

  if(m_jSliceB != nullptr) {
    env->DeleteGlobalRef(m_jSliceB);
  }

  if(m_jSliceLimit != nullptr) {
    env->DeleteGlobalRef(m_jSliceLimit);
  }

  JniUtil::releaseJniEnv(m_jvm, attached_thread);
}

DirectComparatorJniCallback::DirectComparatorJniCallback(
    JNIEnv* env, jobject jComparator,
    const ComparatorJniCallbackOptions* copt) :
    BaseComparatorJniCallback(env, jComparator, copt) {
  m_jSliceA = env->NewGlobalRef(DirectSliceJni::construct0(env));
  if(m_jSliceA == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  m_jSliceB = env->NewGlobalRef(DirectSliceJni::construct0(env));
  if(m_jSliceB == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  m_jSliceLimit = env->NewGlobalRef(DirectSliceJni::construct0(env));
  if(m_jSliceLimit == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
}

DirectComparatorJniCallback::~DirectComparatorJniCallback() {
  jboolean attached_thread = JNI_FALSE;
  JNIEnv* env = JniUtil::getJniEnv(m_jvm, &attached_thread);
  assert(env != nullptr);

  if(m_jSliceA != nullptr) {
    env->DeleteGlobalRef(m_jSliceA);
  }

  if(m_jSliceB != nullptr) {
    env->DeleteGlobalRef(m_jSliceB);
  }

  if(m_jSliceLimit != nullptr) {
    env->DeleteGlobalRef(m_jSliceLimit);
  }

  JniUtil::releaseJniEnv(m_jvm, attached_thread);
}
}  //命名空间rocksdb
