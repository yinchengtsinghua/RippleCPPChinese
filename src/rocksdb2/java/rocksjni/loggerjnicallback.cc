
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
//RocksDB：：记录器。

#include "include/org_rocksdb_Logger.h"

#include "rocksjni/loggerjnicallback.h"
#include "rocksjni/portal.h"
#include <cstdarg>
#include <cstdio>

namespace rocksdb {

LoggerJniCallback::LoggerJniCallback(
    JNIEnv* env, jobject jlogger) {
//注意：记录器方法可以由多个线程访问，
//所以我们引用的是JVM而不是ENV
  const jint rs = env->GetJavaVM(&m_jvm);
  if(rs != JNI_OK) {
//引发异常
    return;
  }

//注意：我们希望访问Java记录器实例
//跨多个方法调用，因此我们创建一个全局引用
  assert(jlogger != nullptr);
  m_jLogger = env->NewGlobalRef(jlogger);
  if(m_jLogger == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
  m_jLogMethodId = LoggerJni::getLogMethodId(env);
  if(m_jLogMethodId == nullptr) {
//引发异常：NoSuchMethodException或OutOfMemoryError
    return;
  }

  jobject jdebug_level = InfoLogLevelJni::DEBUG_LEVEL(env);
  if(jdebug_level == nullptr) {
//引发异常：NoSuchFieldError，ExceptionInInitializerError
//或内存不足错误
    return;
  }
  m_jdebug_level = env->NewGlobalRef(jdebug_level);
  if(m_jdebug_level == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  jobject jinfo_level = InfoLogLevelJni::INFO_LEVEL(env);
  if(jinfo_level == nullptr) {
//引发异常：NoSuchFieldError，ExceptionInInitializerError
//或内存不足错误
    return;
  }
  m_jinfo_level = env->NewGlobalRef(jinfo_level);
  if(m_jinfo_level == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  jobject jwarn_level = InfoLogLevelJni::WARN_LEVEL(env);
  if(jwarn_level == nullptr) {
//引发异常：NoSuchFieldError，ExceptionInInitializerError
//或内存不足错误
    return;
  }
  m_jwarn_level = env->NewGlobalRef(jwarn_level);
  if(m_jwarn_level == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  jobject jerror_level = InfoLogLevelJni::ERROR_LEVEL(env);
  if(jerror_level == nullptr) {
//引发异常：NoSuchFieldError，ExceptionInInitializerError
//或内存不足错误
    return;
  }
  m_jerror_level = env->NewGlobalRef(jerror_level);
  if(m_jerror_level == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  jobject jfatal_level = InfoLogLevelJni::FATAL_LEVEL(env);
  if(jfatal_level == nullptr) {
//引发异常：NoSuchFieldError，ExceptionInInitializerError
//或内存不足错误
    return;
  }
  m_jfatal_level = env->NewGlobalRef(jfatal_level);
  if(m_jfatal_level == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  jobject jheader_level = InfoLogLevelJni::HEADER_LEVEL(env);
  if(jheader_level == nullptr) {
//引发异常：NoSuchFieldError，ExceptionInInitializerError
//或内存不足错误
    return;
  }
  m_jheader_level = env->NewGlobalRef(jheader_level);
  if(m_jheader_level == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
}

void LoggerJniCallback::Logv(const char* format, va_list ap) {
//我们实现这个方法是因为它是虚拟的，但是我们没有
//使用它是因为我们需要了解日志级别。
}

void LoggerJniCallback::Logv(const InfoLogLevel log_level,
    const char* format, va_list ap) {
  if (GetInfoLogLevel() <= log_level) {

//确定信息级Java枚举实例
    jobject jlog_level;
    switch (log_level) {
      case rocksdb::InfoLogLevel::DEBUG_LEVEL:
        jlog_level = m_jdebug_level;
        break;
      case rocksdb::InfoLogLevel::INFO_LEVEL:
        jlog_level = m_jinfo_level;
        break;
      case rocksdb::InfoLogLevel::WARN_LEVEL:
        jlog_level = m_jwarn_level;
        break;
      case rocksdb::InfoLogLevel::ERROR_LEVEL:
        jlog_level = m_jerror_level;
        break;
      case rocksdb::InfoLogLevel::FATAL_LEVEL:
        jlog_level = m_jfatal_level;
        break;
      case rocksdb::InfoLogLevel::HEADER_LEVEL:
        jlog_level = m_jheader_level;
        break;
      default:
        jlog_level = m_jfatal_level;
        break;
    }

    assert(format != nullptr);
    assert(ap != nullptr);
    const std::unique_ptr<char[]> msg = format_str(format, ap);

//将MSG传递给Java回调处理程序
    jboolean attached_thread = JNI_FALSE;
    JNIEnv* env = JniUtil::getJniEnv(m_jvm, &attached_thread);
    assert(env != nullptr);

    jstring jmsg = env->NewStringUTF(msg.get());
    if(jmsg == nullptr) {
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
      env->DeleteLocalRef(jmsg);
      JniUtil::releaseJniEnv(m_jvm, attached_thread);
      return;
    }

    env->CallVoidMethod(m_jLogger, m_jLogMethodId, jlog_level, jmsg);
    if(env->ExceptionCheck()) {
//引发异常
env->ExceptionDescribe(); //将异常打印到stderr
      env->DeleteLocalRef(jmsg);
      JniUtil::releaseJniEnv(m_jvm, attached_thread);
      return;
    }

    env->DeleteLocalRef(jmsg);
    JniUtil::releaseJniEnv(m_jvm, attached_thread);
  }
}

std::unique_ptr<char[]> LoggerJniCallback::format_str(const char* format, va_list ap) const {
  va_list ap_copy;

  va_copy(ap_copy, ap);
const size_t required = vsnprintf(nullptr, 0, format, ap_copy) + 1; //'\0'的额外空间
  va_end(ap_copy);

  std::unique_ptr<char[]> buf(new char[required]);

  va_copy(ap_copy, ap);
  vsnprintf(buf.get(), required, format, ap_copy);
  va_end(ap_copy);

  return buf;
}

LoggerJniCallback::~LoggerJniCallback() {
  jboolean attached_thread = JNI_FALSE;
  JNIEnv* env = JniUtil::getJniEnv(m_jvm, &attached_thread);
  assert(env != nullptr);

  if(m_jLogger != nullptr) {
    env->DeleteGlobalRef(m_jLogger);
  }

  if(m_jdebug_level != nullptr) {
    env->DeleteGlobalRef(m_jdebug_level);
  }

  if(m_jinfo_level != nullptr) {
    env->DeleteGlobalRef(m_jinfo_level);
  }

  if(m_jwarn_level != nullptr) {
    env->DeleteGlobalRef(m_jwarn_level);
  }

  if(m_jerror_level != nullptr) {
    env->DeleteGlobalRef(m_jerror_level);
  }

  if(m_jfatal_level != nullptr) {
    env->DeleteGlobalRef(m_jfatal_level);
  }

  if(m_jheader_level != nullptr) {
    env->DeleteGlobalRef(m_jheader_level);
  }

  JniUtil::releaseJniEnv(m_jvm, attached_thread);
}

}  //命名空间rocksdb

/*
 *类别：org-rocksdb-logger
 *方法：CreateNewLoggerOptions
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Logger_createNewLoggerOptions(
    JNIEnv* env, jobject jobj, jlong joptions) {
  auto* sptr_logger = new std::shared_ptr<rocksdb::LoggerJniCallback>(
      new rocksdb::LoggerJniCallback(env, jobj));

//设置日志级别
  auto* options = reinterpret_cast<rocksdb::Options*>(joptions);
  sptr_logger->get()->SetInfoLogLevel(options->info_log_level);

  return reinterpret_cast<jlong>(sptr_logger);
}

/*
 *类别：org-rocksdb-logger
 *方法：createnewloggerdboptions
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Logger_createNewLoggerDbOptions(
    JNIEnv* env, jobject jobj, jlong jdb_options) {
  auto* sptr_logger = new std::shared_ptr<rocksdb::LoggerJniCallback>(
    new rocksdb::LoggerJniCallback(env, jobj));

//设置日志级别
  auto* db_options = reinterpret_cast<rocksdb::DBOptions*>(jdb_options);
  sptr_logger->get()->SetInfoLogLevel(db_options->info_log_level);

  return reinterpret_cast<jlong>(sptr_logger);
}

/*
 *类别：org-rocksdb-logger
 *方法：setInfologLevel
 *签字：（JB）V
 **/

void Java_org_rocksdb_Logger_setInfoLogLevel(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jlog_level) {
  auto* handle =
      reinterpret_cast<std::shared_ptr<rocksdb::LoggerJniCallback> *>(jhandle);
  handle->get()->
      SetInfoLogLevel(static_cast<rocksdb::InfoLogLevel>(jlog_level));
}

/*
 *类别：org-rocksdb-logger
 *方法：信息日志级别
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_Logger_infoLogLevel(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* handle =
      reinterpret_cast<std::shared_ptr<rocksdb::LoggerJniCallback> *>(jhandle);
  return static_cast<jbyte>(handle->get()->GetInfoLogLevel());
}

/*
 *类别：org-rocksdb-logger
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_Logger_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* handle =
      reinterpret_cast<std::shared_ptr<rocksdb::LoggerJniCallback> *>(jhandle);
delete handle;  //删除std:：shared_ptr
}
