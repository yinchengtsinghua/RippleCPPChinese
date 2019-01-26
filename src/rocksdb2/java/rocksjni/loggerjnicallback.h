
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
//ROCKSDB:：记录器

#ifndef JAVA_ROCKSJNI_LOGGERJNICALLBACK_H_
#define JAVA_ROCKSJNI_LOGGERJNICALLBACK_H_

#include <jni.h>
#include <memory>
#include <string>
#include "port/port.h"
#include "rocksdb/env.h"

namespace rocksdb {

  class LoggerJniCallback : public Logger {
   public:
     LoggerJniCallback(JNIEnv* env, jobject jLogger);
     virtual ~LoggerJniCallback();

     using Logger::SetInfoLogLevel;
     using Logger::GetInfoLogLevel;
//用指定的格式将条目写入日志文件。
     virtual void Logv(const char* format, va_list ap);
//使用指定的日志级别将条目写入日志文件
//格式。任何级别低于内部日志级别的日志
//其中*个（请参见@setinfologlevel和@getinfologlevel）将不会
//印刷的。
     virtual void Logv(const InfoLogLevel log_level,
         const char* format, va_list ap);

   private:
     JavaVM* m_jvm;
     jobject m_jLogger;
     jmethodID m_jLogMethodId;
     jobject m_jdebug_level;
     jobject m_jinfo_level;
     jobject m_jwarn_level;
     jobject m_jerror_level;
     jobject m_jfatal_level;
     jobject m_jheader_level;
     std::unique_ptr<char[]> format_str(const char* format, va_list ap) const;
  };
}  //命名空间rocksdb

#endif  //java_rocksjni_loggerjnicallback_h_
