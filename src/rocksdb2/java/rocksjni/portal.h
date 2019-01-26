
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

//此文件用于缓存那些常用的ID，并提供
//高效的门户（即一组静态功能）来访问Java代码
//来自C++。

#ifndef JAVA_ROCKSJNI_PORTAL_H_
#define JAVA_ROCKSJNI_PORTAL_H_

#include <jni.h>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/backupable_db.h"
#include "rocksdb/utilities/write_batch_with_index.h"
#include "rocksjni/comparatorjnicallback.h"
#include "rocksjni/loggerjnicallback.h"
#include "rocksjni/writebatchhandlerjnicallback.h"

//删除Windows上的宏
#ifdef DELETE
#undef DELETE
#endif

namespace rocksdb {

//检测jlong是否溢出大小
inline Status check_if_jlong_fits_size_t(const jlong& jvalue) {
  Status s = Status::OK();
  if (static_cast<uint64_t>(jvalue) > std::numeric_limits<size_t>::max()) {
    s = Status::InvalidArgument(Slice("jlong overflows 32 bit value."));
  }
  return s;
}

class JavaClass {
 public:
  /*
   *获取并初始化Java类
   *
   *@ PARAM Env指向Java环境的指针
   *@ PARAM-JCLAZZY名称：Java类的完全限定JNI名称
   *“Java/Lang/String”
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env, const char* jclazz_name) {
    jclass jclazz = env->FindClass(jclazz_name);
    assert(jclazz != nullptr);
    return jclazz;
  }
};

//本机类模板
template<class PTR, class DERIVED> class RocksDBNativeClass : public JavaClass {
};

//rockSmutableObject子类的本机类模板
template<class PTR, class DERIVED> class NativeRocksMutableObject
    : public RocksDBNativeClass<PTR, DERIVED> {
 public:

  /*
   *获取Java方法ID
   *RockSmutableObject setNativeHandle（long，boolean）方法
   *
   *@ PARAM Env指向Java环境的指针
   *@返回Java方法ID或null pTR，ROCKSMUTABLE对象类不能
   *被访问，或者如果NoSuchMethodError之一，
   *引发ExceptionInitializerError或OutOfMemoryError异常
   **/

  static jmethodID getSetNativeHandleMethod(JNIEnv* env) {
    static jclass jclazz = DERIVED::getJClass(env);
    if(jclazz == nullptr) {
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(
        jclazz, "setNativeHandle", "(JZ)V");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *在Java对象中设置C++对象指针句柄
   *
   *@ PARAM Env指向Java环境的指针
   *@ PARAM-JOBJ，在其中设置指针句柄的Java对象
   *@ PARAM-PTR C++对象指针
   如果C++对象的所有权是*@ PARAM JavaAuthsHy句柄JNIsTress
   *由Java对象管理
   *
   *如果Java异常悬而未决，返回true，否则为false
   **/

  static bool setHandle(JNIEnv* env, jobject jobj, PTR ptr,
      jboolean java_owns_handle) {
    assert(jobj != nullptr);
    static jmethodID mid = getSetNativeHandleMethod(env);
    if(mid == nullptr) {
return true;  //信号异常
    }

    env->CallVoidMethod(jobj, mid, reinterpret_cast<jlong>(ptr), java_owns_handle);
    if(env->ExceptionCheck()) {
return true;  //信号异常
    }

    return false;
  }
};

//Java异常模板
template<class DERIVED> class JavaException : public JavaClass {
 public:
  /*
   *使用所提供的消息创建和抛出Java异常
   *
   *@ PARAM Env指向Java环境的指针
   *@param msg异常消息
   *
   *@如果引发异常，返回true，否则返回false
   **/

  static bool ThrowNew(JNIEnv* env, const std::string& msg) {
    jclass jclazz = DERIVED::getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      std::cerr << "JavaException::ThrowNew - Error: unexpected exception!" << std::endl;
      return env->ExceptionCheck();
    }

    const jint rs = env->ThrowNew(jclazz, msg.c_str());
    if(rs != JNI_OK) {
//无法引发异常
      std::cerr << "JavaException::ThrowNew - Fatal: could not throw exception!" << std::endl;
      return env->ExceptionCheck();
    }

    return true;
  }
};

//org.rocksdb.rocksdb的门户类
class RocksDBJni : public RocksDBNativeClass<rocksdb::DB*, RocksDBJni> {
 public:
  /*
   *获取Java类Or.RoSDB.ROCKSDB
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/RocksDB");
  }
};

//org.rocksdb.status的门户类
class StatusJni : public RocksDBNativeClass<rocksdb::Status*, StatusJni> {
 public:
  /*
   *获取Java类Or.RoSDSDB状态
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/Status");
  }

  /*
   *创建一个新的JavaOr.RoSdB.Stand对象，其属性与
   *提供的C++ ROCKSDB:：状态对象
   *
   *@ PARAM Env指向Java环境的指针
   *@param status rocksdb:：status对象
   *
   *@返回对Java OrgRoSDB状态对象或NulLPTR的引用
   *如果发生异常
   **/

  static jobject construct(JNIEnv* env, const Status& status) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    jmethodID mid =
        env->GetMethodID(jclazz, "<init>", "(BBLjava/lang/String;)V");
    if(mid == nullptr) {
//引发异常：NoSuchMethodException或OutOfMemoryError
      return nullptr;
    }

//为Java转换状态状态
    jstring jstate = nullptr;
    if (status.getState() != nullptr) {
      const char* const state = status.getState();
      jstate = env->NewStringUTF(state);
      if(env->ExceptionCheck()) {
        if(jstate != nullptr) {
          env->DeleteLocalRef(jstate);
        }
        return nullptr;
      }
    }

    jobject jstatus =
        env->NewObject(jclazz, mid, toJavaStatusCode(status.code()),
            toJavaStatusSubCode(status.subcode()), jstate);
    if(env->ExceptionCheck()) {
//发生异常
      if(jstate != nullptr) {
        env->DeleteLocalRef(jstate);
      }
      return nullptr;
    }

    if(jstate != nullptr) {
      env->DeleteLocalRef(jstate);
    }

    return jstatus;
  }

//返回提供的等效org.rocksdb.status.code
//C++ ROCSDB:：：状态：代码枚举
  static jbyte toJavaStatusCode(const rocksdb::Status::Code& code) {
    switch (code) {
      case rocksdb::Status::Code::kOk:
        return 0x0;
      case rocksdb::Status::Code::kNotFound:
        return 0x1;
      case rocksdb::Status::Code::kCorruption:
        return 0x2;
      case rocksdb::Status::Code::kNotSupported:
        return 0x3;
      case rocksdb::Status::Code::kInvalidArgument:
        return 0x4;
      case rocksdb::Status::Code::kIOError:
        return 0x5;
      case rocksdb::Status::Code::kMergeInProgress:
        return 0x6;
      case rocksdb::Status::Code::kIncomplete:
        return 0x7;
      case rocksdb::Status::Code::kShutdownInProgress:
        return 0x8;
      case rocksdb::Status::Code::kTimedOut:
        return 0x9;
      case rocksdb::Status::Code::kAborted:
        return 0xA;
      case rocksdb::Status::Code::kBusy:
        return 0xB;
      case rocksdb::Status::Code::kExpired:
        return 0xC;
      case rocksdb::Status::Code::kTryAgain:
        return 0xD;
      default:
return 0x7F;  //未定义
    }
  }

//返回所提供的等效org.rocksdb.status.subcode
//C++：ROCSDB：：状态：子代码枚举
  static jbyte toJavaStatusSubCode(const rocksdb::Status::SubCode& subCode) {
    switch (subCode) {
      case rocksdb::Status::SubCode::kNone:
        return 0x0;
      case rocksdb::Status::SubCode::kMutexTimeout:
        return 0x1;
      case rocksdb::Status::SubCode::kLockTimeout:
        return 0x2;
      case rocksdb::Status::SubCode::kLockLimit:
        return 0x3;
      case rocksdb::Status::SubCode::kMaxSubCode:
        return 0x7E;
      default:
return 0x7F;  //未定义
    }
  }
};

//org.rocksdb.rocksdbeexception的门户类
class RocksDBExceptionJni :
    public JavaException<RocksDBExceptionJni> {
 public:
  /*
   *获取Java类Or.RoSDB.ROCKSDBBULL异常
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaException::getJClass(env, "org/rocksdb/RocksDBException");
  }

  /*
   *使用所提供的消息创建并抛出Java RooStbBeExpRebug
   *
   *@ PARAM Env指向Java环境的指针
   *@param msg异常消息
   *
   *@如果引发异常，返回true，否则返回false
   **/

  static bool ThrowNew(JNIEnv* env, const std::string& msg) {
    return JavaException::ThrowNew(env, msg);
  }

  /*
   *使用提供的状态创建并抛出Java RooStbBeExpRebug
   *
   *如果s.ok（）==true，则此函数不会引发任何异常。
   *
   *@ PARAM Env指向Java环境的指针
   *@param s异常的状态
   *
   *@如果引发异常，返回true，否则返回false
   **/

  static bool ThrowNew(JNIEnv* env, const Status& s) {
    assert(!s.ok());
    if (s.ok()) {
      return false;
    }

//获取RocksDBException类
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      std::cerr << "RocksDBExceptionJni::ThrowNew/class - Error: unexpected exception!" << std::endl;
      return env->ExceptionCheck();
    }

//获取org.rocksdb.rocksdbeexception的构造函数
    jmethodID mid =
        env->GetMethodID(jclazz, "<init>", "(Lorg/rocksdb/Status;)V");
    if(mid == nullptr) {
//引发异常：NoSuchMethodException或OutOfMemoryError
      std::cerr << "RocksDBExceptionJni::ThrowNew/cstr - Error: unexpected exception!" << std::endl;
      return env->ExceptionCheck();
    }

//获取Java状态对象
    jobject jstatus = StatusJni::construct(env, s);
    if(jstatus == nullptr) {
//发生异常
      std::cerr << "RocksDBExceptionJni::ThrowNew/StatusJni - Error: unexpected exception!" << std::endl;
      return env->ExceptionCheck();
    }

//建造岩石除外
    jthrowable rocksdb_exception = reinterpret_cast<jthrowable>(env->NewObject(jclazz, mid, jstatus));
    if(env->ExceptionCheck()) {
      if(jstatus != nullptr) {
        env->DeleteLocalRef(jstatus);
      }
      if(rocksdb_exception != nullptr) {
        env->DeleteLocalRef(rocksdb_exception);
      }
      std::cerr << "RocksDBExceptionJni::ThrowNew/NewObject - Error: unexpected exception!" << std::endl;
      return true;
    }

//抛掷石块
    const jint rs = env->Throw(rocksdb_exception);
    if(rs != JNI_OK) {
//无法引发异常
      std::cerr << "RocksDBExceptionJni::ThrowNew - Fatal: could not throw exception!" << std::endl;
      if(jstatus != nullptr) {
        env->DeleteLocalRef(jstatus);
      }
      if(rocksdb_exception != nullptr) {
        env->DeleteLocalRef(rocksdb_exception);
      }
      return env->ExceptionCheck();
    }

    if(jstatus != nullptr) {
      env->DeleteLocalRef(jstatus);
    }
    if(rocksdb_exception != nullptr) {
      env->DeleteLocalRef(rocksdb_exception);
    }

    return true;
  }

  /*
   *使用所提供的消息创建并抛出Java RooStbBeExpRebug
   *和状态
   *
   *如果s.ok（）==true，则此函数不会引发任何异常。
   *
   *@ PARAM Env指向Java环境的指针
   *@param msg异常消息
   *@param s异常的状态
   *
   *@如果引发异常，返回true，否则返回false
   **/

  static bool ThrowNew(JNIEnv* env, const std::string& msg, const Status& s) {
    assert(!s.ok());
    if (s.ok()) {
      return false;
    }

//获取RocksDBException类
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      std::cerr << "RocksDBExceptionJni::ThrowNew/class - Error: unexpected exception!" << std::endl;
      return env->ExceptionCheck();
    }

//获取org.rocksdb.rocksdbeexception的构造函数
    jmethodID mid =
        env->GetMethodID(jclazz, "<init>", "(Ljava/lang/String;Lorg/rocksdb/Status;)V");
    if(mid == nullptr) {
//引发异常：NoSuchMethodException或OutOfMemoryError
      std::cerr << "RocksDBExceptionJni::ThrowNew/cstr - Error: unexpected exception!" << std::endl;
      return env->ExceptionCheck();
    }

    jstring jmsg = env->NewStringUTF(msg.c_str());
    if(jmsg == nullptr) {
//引发异常：OutOfMemoryError
      std::cerr << "RocksDBExceptionJni::ThrowNew/msg - Error: unexpected exception!" << std::endl;
      return env->ExceptionCheck();
    }

//获取Java状态对象
    jobject jstatus = StatusJni::construct(env, s);
    if(jstatus == nullptr) {
//发生异常
      std::cerr << "RocksDBExceptionJni::ThrowNew/StatusJni - Error: unexpected exception!" << std::endl;
      if(jmsg != nullptr) {
        env->DeleteLocalRef(jmsg);
      }
      return env->ExceptionCheck();
    }

//建造岩石除外
    jthrowable rocksdb_exception = reinterpret_cast<jthrowable>(env->NewObject(jclazz, mid, jmsg, jstatus));
    if(env->ExceptionCheck()) {
      if(jstatus != nullptr) {
        env->DeleteLocalRef(jstatus);
      }
      if(jmsg != nullptr) {
        env->DeleteLocalRef(jmsg);
      }
      if(rocksdb_exception != nullptr) {
        env->DeleteLocalRef(rocksdb_exception);
      }
      std::cerr << "RocksDBExceptionJni::ThrowNew/NewObject - Error: unexpected exception!" << std::endl;
      return true;
    }

//抛掷石块
    const jint rs = env->Throw(rocksdb_exception);
    if(rs != JNI_OK) {
//无法引发异常
      std::cerr << "RocksDBExceptionJni::ThrowNew - Fatal: could not throw exception!" << std::endl;
      if(jstatus != nullptr) {
        env->DeleteLocalRef(jstatus);
      }
      if(jmsg != nullptr) {
        env->DeleteLocalRef(jmsg);
      }
      if(rocksdb_exception != nullptr) {
        env->DeleteLocalRef(rocksdb_exception);
      }
      return env->ExceptionCheck();
    }

    if(jstatus != nullptr) {
      env->DeleteLocalRef(jstatus);
    }
    if(jmsg != nullptr) {
      env->DeleteLocalRef(jmsg);
    }
    if(rocksdb_exception != nullptr) {
      env->DeleteLocalRef(rocksdb_exception);
    }

    return true;
  }
};

//java.lang.IllegalArgumentException的门户类
class IllegalArgumentExceptionJni :
    public JavaException<IllegalArgumentExceptionJni> {
 public:
  /*
   *获取Java类java.郎.
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaException::getJClass(env, "java/lang/IllegalArgumentException");
  }

  /*
   *创建并抛出具有提供状态的Java ILLalgAguMutExtExtExchange
   *
   *如果s.ok（）==true，则此函数不会引发任何异常。
   *
   *@ PARAM Env指向Java环境的指针
   *@param s异常的状态
   *
   *@如果引发异常，返回true，否则返回false
   **/

  static bool ThrowNew(JNIEnv* env, const Status& s) {
    assert(!s.ok());
    if (s.ok()) {
      return false;
    }

//获取IllegalArgumentException类
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      std::cerr << "IllegalArgumentExceptionJni::ThrowNew/class - Error: unexpected exception!" << std::endl;
      return env->ExceptionCheck();
    }

    return JavaException::ThrowNew(env, s.ToString());
  }
};


//org.rocksdb.options的门户类
class OptionsJni : public RocksDBNativeClass<
    rocksdb::Options*, OptionsJni> {
 public:
  /*
   *获取Java类Or.RoSDSDB选项
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/Options");
  }
};

//org.rocksdb.dboptions的门户类
class DBOptionsJni : public RocksDBNativeClass<
    rocksdb::DBOptions*, DBOptionsJni> {
 public:
  /*
   *获取Java类Or.RoSdB.dBo期权
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/DBOptions");
  }
};

class ColumnFamilyDescriptorJni : public JavaClass {
 public:
  /*
   *获取Java类Or.RoSdB.CulnFulky描述符
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "org/rocksdb/ColumnFamilyDescriptor");
  }

  /*
   *获取Java方法：CyrnFulyEngultExpReule:CynLoad家族名称
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getColumnFamilyNameMethod(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jclazz, "columnFamilyName", "()[B");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：CyrnFulySypDeultSuxCyrnCyfyOrthOp选项
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getColumnFamilyOptionsMethod(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jclazz, "columnFamilyOptions",
            "()Lorg/rocksdb/ColumnFamilyOptions;");
    assert(mid != nullptr);
    return mid;
  }
};

//org.rocksdb.columnfamilyOptions的门户类
class ColumnFamilyOptionsJni : public RocksDBNativeClass<
    rocksdb::ColumnFamilyOptions*, ColumnFamilyOptionsJni> {
 public:
  /*
   *获取Java类Or.RoSdB.CyrnCyfyOrthOp选项
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env,
        "org/rocksdb/ColumnFamilyOptions");
  }
};

//org.rocksdb.writeoptions的门户类
class WriteOptionsJni : public RocksDBNativeClass<
    rocksdb::WriteOptions*, WriteOptionsJni> {
 public:
  /*
   *获取Java类Or.RoSDSD.WrreOb选项
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/WriteOptions");
  }
};

//org.rocksdb.readoptions的门户类
class ReadOptionsJni : public RocksDBNativeClass<
    rocksdb::ReadOptions*, ReadOptionsJni> {
 public:
  /*
   *获取Java类Or.RoSDB.Read选项
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/ReadOptions");
  }
};

//org.rocksdb.writebatch的门户类
class WriteBatchJni : public RocksDBNativeClass<
    rocksdb::WriteBatch*, WriteBatchJni> {
 public:
  /*
   *获取Java类Or.RoSdB.WrreBog
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/WriteBatch");
  }
};

//org.rocksdb.writebatch.handler的门户类
class WriteBatchHandlerJni : public RocksDBNativeClass<
    const rocksdb::WriteBatchHandlerJniCallback*,
    WriteBatchHandlerJni> {
 public:
  /*
   *获取Java类Or.RoSdB.WrreButCH处理程序
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env,
        "org/rocksdb/WriteBatch$Handler");
  }

  /*
   *获取Java方法：
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getPutMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jclazz, "put", "([B[B)V");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：RealePoal.Huffer-*合并
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getMergeMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jclazz, "merge", "([B[B)V");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getDeleteMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jclazz, "delete", "([B)V");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：WrrePoal.Huffer-TyelDelangang.
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getDeleteRangeMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if (jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jclazz, "deleteRange", "([B[B)V");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：WrrePoal.Huffer-TyLog.
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getLogDataMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jclazz, "logData", "([B)V");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：RealePoal.Huffer-Tybe继续
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getContinueMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jclazz, "shouldContinue", "()Z");
    assert(mid != nullptr);
    return mid;
  }
};

//org.rocksdb.writebatchwithindex的门户类
class WriteBatchWithIndexJni : public RocksDBNativeClass<
    rocksdb::WriteBatchWithIndex*, WriteBatchWithIndexJni> {
 public:
  /*
   *获取Java类Or.RoSDSD.WrueBoffCh索引
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env,
        "org/rocksdb/WriteBatchWithIndex");
  }
};

//org.rocksdb.histogramdata的门户类
class HistogramDataJni : public JavaClass {
 public:
  /*
   *获取Java类Or.RoSDB.GracRAPDATA数据
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "org/rocksdb/HistogramData");
  }

  /*
   *获取Java方法：直方图数据构造函数
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getConstructorMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jclazz, "<init>", "(DDDDD)V");
    assert(mid != nullptr);
    return mid;
  }
};

//org.rocksdb.backupabledboptions的门户类
class BackupableDBOptionsJni : public RocksDBNativeClass<
    rocksdb::BackupableDBOptions*, BackupableDBOptionsJni> {
 public:
  /*
   *获取Java类Or.RoSDSD.ButuBabdBeOb选项
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env,
        "org/rocksdb/BackupableDBOptions");
  }
};

//org.rocksdb.backupengine的门户类
class BackupEngineJni : public RocksDBNativeClass<
    rocksdb::BackupEngine*, BackupEngineJni> {
 public:
  /*
   *获取Java类Or.RoSDB.Buffuable引擎
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/BackupEngine");
  }
};

//org.rocksdb.rocksiterator的门户类
class IteratorJni : public RocksDBNativeClass<
    rocksdb::Iterator*, IteratorJni> {
 public:
  /*
   *获取Java类Or.RoSDB.Roististor
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/RocksIterator");
  }
};

//org.rocksdb.filter的门户类
class FilterJni : public RocksDBNativeClass<
    std::shared_ptr<rocksdb::FilterPolicy>*, FilterJni> {
 public:
  /*
   *获取Java类Or.RoSDB过滤器
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/Filter");
  }
};

//org.rocksdb.columnfamilyhandle的门户类
class ColumnFamilyHandleJni : public RocksDBNativeClass<
    rocksdb::ColumnFamilyHandle*, ColumnFamilyHandleJni> {
 public:
  /*
   *获取Java类Or.RoSDSDB
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env,
        "org/rocksdb/ColumnFamilyHandle");
  }
};

//org.rocksdb.flushoptings的门户类
class FlushOptionsJni : public RocksDBNativeClass<
    rocksdb::FlushOptions*, FlushOptionsJni> {
 public:
  /*
   *获取Java类Or.RoSDB.Flash选项
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/FlushOptions");
  }
};

//org.rocksdb.comparatorOptions的门户类
class ComparatorOptionsJni : public RocksDBNativeClass<
    rocksdb::ComparatorJniCallbackOptions*, ComparatorOptionsJni> {
 public:
  /*
   *获取Java类Or.RoSDBB比较器选项
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/ComparatorOptions");
  }
};

//org.rocksdb.abstractComparator的门户类
class AbstractComparatorJni : public RocksDBNativeClass<
    const rocksdb::BaseComparatorJniCallback*,
    AbstractComparatorJni> {
 public:
  /*
   *获取Java类Or.RoSDB.AcExabor比较器
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env,
        "org/rocksdb/AbstractComparator");
  }

  /*
   *获取Java方法：比较器名
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getNameMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jclazz, "name", "()Ljava/lang/String;");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：比较器比较
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getCompareMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jclazz, "compare",
            "(Lorg/rocksdb/AbstractSlice;Lorg/rocksdb/AbstractSlice;)I");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：比较器
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getFindShortestSeparatorMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jclazz, "findShortestSeparator",
            "(Ljava/lang/String;Lorg/rocksdb/AbstractSlice;)Ljava/lang/String;");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：比较器
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getFindShortSuccessorMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jclazz, "findShortSuccessor",
            "(Ljava/lang/String;)Ljava/lang/String;");
    assert(mid != nullptr);
    return mid;
  }
};

//org.rocksdb.abstractslice的门户类
class AbstractSliceJni : public NativeRocksMutableObject<
    const rocksdb::Slice*, AbstractSliceJni> {
 public:
  /*
   *获取Java类Or.RoSDB.Actuple切片
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/AbstractSlice");
  }
};

//org.rocksdb.slice的门户类
class SliceJni : public NativeRocksMutableObject<
    const rocksdb::Slice*, AbstractSliceJni> {
 public:
  /*
   *获取Java类Or.RoSDB切片
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/Slice");
  }

  /*
   *构造一个切片对象
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回一个Java切片对象的引用，或者返回一个null pTr如果
   *发生异常
   **/

  static jobject construct0(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jclazz, "<init>", "()V");
    if(mid == nullptr) {
//访问方法时发生异常
      return nullptr;
    }
    
    jobject jslice = env->NewObject(jclazz, mid);
    if(env->ExceptionCheck()) {
      return nullptr;
    }

    return jslice;
  }
};

//org.rocksdb.directslice的门户类
class DirectSliceJni : public NativeRocksMutableObject<
    const rocksdb::Slice*, AbstractSliceJni> {
 public:
  /*
   *获取Java类Or.RoSDSDB的直接切片
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/DirectSlice");
  }

  /*
   *构造DirectSicle对象
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回一个Java直接切片对象的引用，或者返回一个null pTR，如果是
   *发生异常
   **/

  static jobject construct0(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jclazz, "<init>", "()V");
    if(mid == nullptr) {
//访问方法时发生异常
      return nullptr;
    }

    jobject jdirect_slice = env->NewObject(jclazz, mid);
    if(env->ExceptionCheck()) {
      return nullptr;
    }

    return jdirect_slice;
  }
};

//java.util.list的门户类
class ListJni : public JavaClass {
 public:
  /*
   *获取Java类java. UTILITE列表
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getListClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "java/util/List");
  }

  /*
   *获取Java类java. UTIL.ARAYLIST
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getArrayListClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "java/util/ArrayList");
  }

  /*
   *获取Java类java. UTIL迭代器
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getIteratorClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "java/util/Iterator");
  }

  /*
   *获取Java方法：列表{迭代器
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getIteratorMethod(JNIEnv* env) {
    jclass jlist_clazz = getListClass(env);
    if(jlist_clazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jlist_clazz, "iterator", "()Ljava/util/Iterator;");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：迭代器
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getHasNextMethod(JNIEnv* env) {
    jclass jiterator_clazz = getIteratorClass(env);
    if(jiterator_clazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jiterator_clazz, "hasNext", "()Z");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：迭代器
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getNextMethod(JNIEnv* env) {
    jclass jiterator_clazz = getIteratorClass(env);
    if(jiterator_clazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jiterator_clazz, "next", "()Ljava/lang/Object;");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：ARARYLIST构造函数
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getArrayListConstructorMethodId(JNIEnv* env) {
    jclass jarray_list_clazz = getArrayListClass(env);
    if(jarray_list_clazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }
    static jmethodID mid =
        env->GetMethodID(jarray_list_clazz, "<init>", "(I)V");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *获取Java方法：
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getListAddMethodId(JNIEnv* env) {
    jclass jlist_clazz = getListClass(env);
    if(jlist_clazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jlist_clazz, "add", "(Ljava/lang/Object;)Z");
    assert(mid != nullptr);
    return mid;
  }
};

//java.lang.byte的门户类
class ByteJni : public JavaClass {
 public:
  /*
   *获取Java类java.郎.字节
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "java/lang/Byte");
  }

  /*
   *获取Java类字节[]
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getArrayJClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "[B");
  }

  /*
   *创建一个新的二维Java字节数组字节[]]
   *
   *@ PARAM Env指向Java环境的指针
   *@param len第一维度的大小
   *
   *@如果发生异常，返回对Java字节[] ]或null pTr的引用
   **/

  static jobjectArray new2dByteArray(JNIEnv* env, const jsize len) {
    jclass clazz = getArrayJClass(env);
    if(clazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    return env->NewObjectArray(len, clazz, nullptr);
  }

  /*
   *获取Java方法：字节
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getByteValueMethod(JNIEnv* env) {
    jclass clazz = getJClass(env);
    if(clazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(clazz, "byteValue", "()B");
    assert(mid != nullptr);
    return mid;
  }
};

//java.lang.StringBuilder的门户类
class StringBuilderJni : public JavaClass {
  public:
  /*
   *获取Java类java. Lang.StrugBuudid
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "java/lang/StringBuilder");
  }

  /*
   *获取Java方法：StrugBuuder-*追加
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getListAddMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jclazz, "append",
            "(Ljava/lang/String;)Ljava/lang/StringBuilder;");
    assert(mid != nullptr);
    return mid;
  }

  /*
   *将C样式字符串附加到StringBuilder
   *
   *@ PARAM Env指向Java环境的指针
   *@param jstring_builder引用java.lang.stringbuilder
   *@param c_str附加到StringBuilder的C样式字符串
   *
   *@返回对更新的StringBuilder的引用，或者返回nullptr，如果
   *发生异常
   **/

  static jobject append(JNIEnv* env, jobject jstring_builder,
      const char* c_str) {
    jmethodID mid = getListAddMethodId(env);
    if(mid == nullptr) {
//访问类或方法时发生异常
      return nullptr;
    }

    jstring new_value_str = env->NewStringUTF(c_str);
    if(new_value_str == nullptr) {
//引发异常：OutOfMemoryError
      return nullptr;
    }

    jobject jresult_string_builder =
        env->CallObjectMethod(jstring_builder, mid, new_value_str);
    if(env->ExceptionCheck()) {
//发生异常
      env->DeleteLocalRef(new_value_str);
      return nullptr;
    }

    return jresult_string_builder;
  }
};

//org.rocksdb.backupinfo的门户类
class BackupInfoJni : public JavaClass {
 public:
  /*
   *获取Java类Or.RoSDSD.BuffuFipe
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "org/rocksdb/BackupInfo");
  }

  /*
   *构造backupinfo对象
   *
   *@ PARAM Env指向Java环境的指针
   *@param backup？备份的ID
   *@param timestamp备份的时间戳
   *@param备份大小
   *@param number_files与备份相关的文件数
   *
   *@返回一个Java BuffuInFor对象的引用，或者返回一个null pTR
   *发生异常
   **/

  static jobject construct0(JNIEnv* env, uint32_t backup_id, int64_t timestamp,
      uint64_t size, uint32_t number_files) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid = env->GetMethodID(jclazz, "<init>", "(IJJI)V");
    if(mid == nullptr) {
//访问方法时发生异常
      return nullptr;
    }

    jobject jbackup_info =
        env->NewObject(jclazz, mid, backup_id, timestamp, size, number_files);
    if(env->ExceptionCheck()) {
      return nullptr;
    }

    return jbackup_info;
  }
};

class BackupInfoListJni {
 public:
  /*
   *将C++ STD:：vector＜BuffUnFiel}对象转换为
   *一个JavaRayList.org .ROCKSDB.BuffuFuff>对象
   *
   *@ PARAM Env指向Java环境的指针
   *@param backup_info是backupinfo的载体
   *
   *@返回对JavaRayayListar对象或NulLPTR的引用
   *如果发生异常
   **/

  static jobject getBackupInfo(JNIEnv* env,
      std::vector<BackupInfo> backup_infos) {
    jclass jarray_list_clazz = rocksdb::ListJni::getArrayListClass(env);
    if(jarray_list_clazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    jmethodID cstr_mid = rocksdb::ListJni::getArrayListConstructorMethodId(env);
    if(cstr_mid == nullptr) {
//访问方法时发生异常
      return nullptr;
    }

    jmethodID add_mid = rocksdb::ListJni::getListAddMethodId(env);
    if(add_mid == nullptr) {
//访问方法时发生异常
      return nullptr;
    }

//创建Java列表
    jobject jbackup_info_handle_list =
        env->NewObject(jarray_list_clazz, cstr_mid, backup_infos.size());
    if(env->ExceptionCheck()) {
//构造对象时发生异常
      return nullptr;
    }

//在Java列表中插入
    auto end = backup_infos.end();
    for (auto it = backup_infos.begin(); it != end; ++it) {
      auto backup_info = *it;

      jobject obj = rocksdb::BackupInfoJni::construct0(env,
          backup_info.backup_id,
          backup_info.timestamp,
          backup_info.size,
          backup_info.number_files);
      if(env->ExceptionCheck()) {
//构造对象时发生异常
        if(obj != nullptr) {
          env->DeleteLocalRef(obj);
        }
        if(jbackup_info_handle_list != nullptr) {
          env->DeleteLocalRef(jbackup_info_handle_list);
        }
        return nullptr;
      }

      jboolean rs =
          env->CallBooleanMethod(jbackup_info_handle_list, add_mid, obj);
      if(env->ExceptionCheck() || rs == JNI_FALSE) {
//调用方法时发生异常，或无法添加
        if(obj != nullptr) {
          env->DeleteLocalRef(obj);
        }
        if(jbackup_info_handle_list != nullptr) {
          env->DeleteLocalRef(jbackup_info_handle_list);
        }
        return nullptr;
      }
    }

    return jbackup_info_handle_list;
  }
};

//org.rocksdb.wbwirocksiterator的门户类
class WBWIRocksIteratorJni : public JavaClass {
 public:
  /*
   *获取Java类Or.RoSDB.WBWiRikStReistor
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "org/rocksdb/WBWIRocksIterator");
  }

  /*
   *获取Java字段：WBWRikStReReals**条目
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或字段ID不能，则返回Java字段ID或NulLPTR
   *被保留
   **/

  static jfieldID getWriteEntryField(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jfieldID fid =
        env->GetFieldID(jclazz, "entry",
            "Lorg/rocksdb/WBWIRocksIterator$WriteEntry;");
    assert(fid != nullptr);
    return fid;
  }

  /*
   *获取wbwirocksiterator项的值
   *
   *@ PARAM Env指向Java环境的指针
   *@param jwbwi_rocks_迭代器对wbwi迭代器的引用
   *
   *@返回对JavaWBWRikStRealActudi.WruteEnter对象的引用，或
   *如果发生异常，则为nullptr
   **/

  static jobject getWriteEntry(JNIEnv* env, jobject jwbwi_rocks_iterator) {
    assert(jwbwi_rocks_iterator != nullptr);

    jfieldID jwrite_entry_field = getWriteEntryField(env);
    if(jwrite_entry_field == nullptr) {
//访问字段时发生异常
      return nullptr;
    }

    jobject jwe = env->GetObjectField(jwbwi_rocks_iterator, jwrite_entry_field);
    assert(jwe != nullptr);
    return jwe;
  }
};

//org.rocksdb.wbwirocksiterator.writetype的门户类
class WriteTypeJni : public JavaClass {
 public:
    /*
     *获取wbwirocksiterator.writeType的Put枚举字段值
     *
     *@ PARAM Env指向Java环境的指针
     *
     *@返回对枚举字段值或nullptr的引用，如果
     *无法检索枚举字段值
     **/

    static jobject PUT(JNIEnv* env) {
      return getEnum(env, "PUT");
    }

    /*
     *获取wbwirocksiterator.writeType的合并枚举字段值
     *
     *@ PARAM Env指向Java环境的指针
     *
     *@返回对枚举字段值或nullptr的引用，如果
     *无法检索枚举字段值
     **/

    static jobject MERGE(JNIEnv* env) {
      return getEnum(env, "MERGE");
    }

    /*
     *获取wbwirocksiterator.writeType的删除枚举字段值
     *
     *@ PARAM Env指向Java环境的指针
     *
     *@返回对枚举字段值或nullptr的引用，如果
     *无法检索枚举字段值
     **/

    static jobject DELETE(JNIEnv* env) {
      return getEnum(env, "DELETE");
    }

    /*
     *获取wbwirocksiterator.writetype的日志枚举字段值
     *
     *@ PARAM Env指向Java环境的指针
     *
     *@返回对枚举字段值或nullptr的引用，如果
     *无法检索枚举字段值
     **/

    static jobject LOG(JNIEnv* env) {
      return getEnum(env, "LOG");
    }

 private:
  /*
   *获取Java类Or.RoSDB.WBWiRikStRealTo.WryType
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "org/rocksdb/WBWIRocksIterator$WriteType");
  }

  /*
   *获取org.rocksdb.wbwirocksiterator.writetype的枚举字段
   *
   *@ PARAM Env指向Java环境的指针
   *@param name枚举字段的名称
   *
   *@返回对枚举字段值或nullptr的引用，如果
   *无法检索枚举字段值
   **/

  static jobject getEnum(JNIEnv* env, const char name[]) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    jfieldID jfid =
        env->GetStaticFieldID(jclazz, name,
            "Lorg/rocksdb/WBWIRocksIterator$WriteType;");
    if(env->ExceptionCheck()) {
//获取字段时发生异常
      return nullptr;
    } else if(jfid == nullptr) {
      return nullptr;
    }

    jobject jwrite_type = env->GetStaticObjectField(jclazz, jfid);
    assert(jwrite_type != nullptr);
    return jwrite_type;
  }
};

//org.rocksdb.wbwirocksiterator.writeEntry的门户类
class WriteEntryJni : public JavaClass {
 public:
  /*
   *获取Java类Or.RoSDB.WBWiRekStReRealTror
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

    static jclass getJClass(JNIEnv* env) {
      return JavaClass::getJClass(env, "org/rocksdb/WBWIRocksIterator$WriteEntry");
    }
};

//org.rocksdb.infologlevel的门户类
class InfoLogLevelJni : public JavaClass {
 public:
    /*
     *获取infologlevel的调试级枚举字段值
     *
     *@ PARAM Env指向Java环境的指针
     *
     *@返回对枚举字段值或nullptr的引用，如果
     *无法检索枚举字段值
     **/

    static jobject DEBUG_LEVEL(JNIEnv* env) {
      return getEnum(env, "DEBUG_LEVEL");
    }

    /*
     *获取infologlevel的信息级枚举字段值
     *
     *@ PARAM Env指向Java环境的指针
     *
     *@返回对枚举字段值或nullptr的引用，如果
     *无法检索枚举字段值
     **/

    static jobject INFO_LEVEL(JNIEnv* env) {
      return getEnum(env, "INFO_LEVEL");
    }

    /*
     *获取inforloglevel的warn_-level枚举字段值
     *
     *@ PARAM Env指向Java环境的指针
     *
     *@返回对枚举字段值或nullptr的引用，如果
     *无法检索枚举字段值
     **/

    static jobject WARN_LEVEL(JNIEnv* env) {
      return getEnum(env, "WARN_LEVEL");
    }

    /*
     *获取信息日志级别的错误\级别枚举字段值
     *
     *@ PARAM Env指向Java环境的指针
     *
     *@返回对枚举字段值或nullptr的引用，如果
     *无法检索枚举字段值
     **/

    static jobject ERROR_LEVEL(JNIEnv* env) {
      return getEnum(env, "ERROR_LEVEL");
    }

    /*
     *获取infologlevel的致命枚举字段值
     *
     *@ PARAM Env指向Java环境的指针
     *
     *@返回对枚举字段值或nullptr的引用，如果
     *无法检索枚举字段值
     **/

    static jobject FATAL_LEVEL(JNIEnv* env) {
      return getEnum(env, "FATAL_LEVEL");
    }

    /*
     *获取信息日志级别的头级枚举字段值
     *
     *@ PARAM Env指向Java环境的指针
     *
     *@返回对枚举字段值或nullptr的引用，如果
     *无法检索枚举字段值
     **/

    static jobject HEADER_LEVEL(JNIEnv* env) {
      return getEnum(env, "HEADER_LEVEL");
    }

 private:
  /*
   *获取Java类Or.RoSDB的信息级别
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaClass::getJClass(env, "org/rocksdb/InfoLogLevel");
  }

  /*
   *获取org.rocksdb.infologlevel的枚举字段
   *
   *@ PARAM Env指向Java环境的指针
   *@param name枚举字段的名称
   *
   *@返回对枚举字段值或nullptr的引用，如果
   *无法检索枚举字段值
   **/

  static jobject getEnum(JNIEnv* env, const char name[]) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    jfieldID jfid =
        env->GetStaticFieldID(jclazz, name, "Lorg/rocksdb/InfoLogLevel;");
    if(env->ExceptionCheck()) {
//获取字段时发生异常
      return nullptr;
    } else if(jfid == nullptr) {
      return nullptr;
    }

    jobject jinfo_log_level = env->GetStaticObjectField(jclazz, jfid);
    assert(jinfo_log_level != nullptr);
    return jinfo_log_level;
  }
};

//org.rocksdb.logger的门户类
class LoggerJni : public RocksDBNativeClass<
    std::shared_ptr<rocksdb::LoggerJniCallback>*, LoggerJni> {
 public:
  /*
   *获取Java类ORG/ROCKSDB/LogGER
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return RocksDBNativeClass::getJClass(env, "org/rocksdb/Logger");
  }

  /*
   *获取Java方法：Logger-ylog
   *
   *@ PARAM Env指向Java环境的指针
   *
   *如果类或方法ID不能，则返回Java方法ID或NulLPTR
   *被保留
   **/

  static jmethodID getLogMethodId(JNIEnv* env) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    static jmethodID mid =
        env->GetMethodID(jclazz, "log",
            "(Lorg/rocksdb/InfoLogLevel;Ljava/lang/String;)V");
    assert(mid != nullptr);
    return mid;
  }
};

//org.rocksdb.transactionlogiterator.batchresult的门户类
class BatchResultJni : public JavaClass {
  public:
  /*
   *获取Java类Or.RoSDSD.TraceActudiLogial.Boice结果
   *
   *@ PARAM Env指向Java环境的指针
   *
   *@返回Java类或NulLPTR，如果其中一个
   *ClassFormatError、ClassCircularityError、NoClassDeffoundError和
   *引发OutOfMemoryError或ExceptionIninitializerError异常
   **/

  static jclass getJClass(JNIEnv* env) {
    return JavaClass::getJClass(env,
        "org/rocksdb/TransactionLogIterator$BatchResult");
  }

  /*
   *创建一个新的JavaOr.RoSDSD.TraceActudiLogial.BACCHREST对象
   *与所提供的C++ ROCKSDB:：BACCHREST对象具有相同的属性
   *
   *@ PARAM Env指向Java环境的指针
   *@param batch_result rocksdb:：batch result对象
   *
   *@返回对Java的引用
   *org.rocksdb.transactionlogiterator.batchresult对象，
   *或nullptr（如果发生异常）
   **/

  static jobject construct(JNIEnv* env,
      rocksdb::BatchResult& batch_result) {
    jclass jclazz = getJClass(env);
    if(jclazz == nullptr) {
//访问类时发生异常
      return nullptr;
    }

    jmethodID mid = env->GetMethodID(
      jclazz, "<init>", "(JJ)V");
    if(mid == nullptr) {
//引发异常：NoSuchMethodException或OutOfMemoryError
      return nullptr;
    }

    jobject jbatch_result = env->NewObject(jclazz, mid,
      batch_result.sequence, batch_result.writeBatchPtr.get());
    if(jbatch_result == nullptr) {
//引发异常：InstantiationException或OutOfMemoryError
      return nullptr;
    }

    batch_result.writeBatchPtr.release();
    return jbatch_result;
  }
};

//org.rocksdb.compactionstopstyle的门户类
class CompactionStopStyleJni {
 public:
//为提供的返回等效的org.rocksdb.compactionstopstyle
//C++ ROCKSDB:
  static jbyte toJavaCompactionStopStyle(
      const rocksdb::CompactionStopStyle& compaction_stop_style) {
    switch(compaction_stop_style) {
      case rocksdb::CompactionStopStyle::kCompactionStopStyleSimilarSize:
        return 0x0;
      case rocksdb::CompactionStopStyle::kCompactionStopStyleTotalSize:
        return 0x1;
      default:
return 0x7F;  //未定义
    }
  }

//返回等价的C++ ROCSDB:：：
//提供JavaOrgRSDB.CascultStudio样式
  static rocksdb::CompactionStopStyle toCppCompactionStopStyle(
      jbyte jcompaction_stop_style) {
    switch(jcompaction_stop_style) {
      case 0x0:
        return rocksdb::CompactionStopStyle::kCompactionStopStyleSimilarSize;
      case 0x1:
        return rocksdb::CompactionStopStyle::kCompactionStopStyleTotalSize;
      default:
//未定义/默认
        return rocksdb::CompactionStopStyle::kCompactionStopStyleSimilarSize;
    }
  }
};

//org.rocksdb.compressionType的门户类
class CompressionTypeJni {
 public:
//返回所提供的等效org.rocksdb.compressionType
//C++ROCKSDB:：压缩类型枚举
  static jbyte toJavaCompressionType(
      const rocksdb::CompressionType& compression_type) {
    switch(compression_type) {
      case rocksdb::CompressionType::kNoCompression:
        return 0x0;
      case rocksdb::CompressionType::kSnappyCompression:
        return 0x1;
      case rocksdb::CompressionType::kZlibCompression:
        return 0x2;
      case rocksdb::CompressionType::kBZip2Compression:
        return 0x3;
      case rocksdb::CompressionType::kLZ4Compression:
        return 0x4;
      case rocksdb::CompressionType::kLZ4HCCompression:
        return 0x5;
      case rocksdb::CompressionType::kXpressCompression:
        return 0x6;
      case rocksdb::CompressionType::kZSTD:
        return 0x7;
      case rocksdb::CompressionType::kDisableCompressionOption:
      default:
        return 0x7F;
    }
  }

//返回等效的C++ ROCSDB:：压缩类型枚举
//提供Java OrrSDB压缩类型
  static rocksdb::CompressionType toCppCompressionType(
      jbyte jcompression_type) {
    switch(jcompression_type) {
      case 0x0:
        return rocksdb::CompressionType::kNoCompression;
      case 0x1:
        return rocksdb::CompressionType::kSnappyCompression;
      case 0x2:
        return rocksdb::CompressionType::kZlibCompression;
      case 0x3:
        return rocksdb::CompressionType::kBZip2Compression;
      case 0x4:
        return rocksdb::CompressionType::kLZ4Compression;
      case 0x5:
        return rocksdb::CompressionType::kLZ4HCCompression;
      case 0x6:
        return rocksdb::CompressionType::kXpressCompression;
      case 0x7:
        return rocksdb::CompressionType::kZSTD;
      case 0x7F:
      default:
        return rocksdb::CompressionType::kDisableCompressionOption;
    }
  }
};

//org.rocksdb.compactionpriority的门户类
class CompactionPriorityJni {
 public:
//为提供的返回等效的org.rocksdb.compactionpriority
//C++ ROCKSDB:
  static jbyte toJavaCompactionPriority(
      const rocksdb::CompactionPri& compaction_priority) {
    switch(compaction_priority) {
      case rocksdb::CompactionPri::kByCompensatedSize:
        return 0x0;
      case rocksdb::CompactionPri::kOldestLargestSeqFirst:
        return 0x1;
      case rocksdb::CompactionPri::kOldestSmallestSeqFirst:
        return 0x2;
      case rocksdb::CompactionPri::kMinOverlappingRatio:
        return 0x3;
      default:
return 0x0;  //未定义
    }
  }

//返回等价的C++ ROCSDB:：：
//提供Java OrgSDB压缩优先级
  static rocksdb::CompactionPri toCppCompactionPriority(
      jbyte jcompaction_priority) {
    switch(jcompaction_priority) {
      case 0x0:
        return rocksdb::CompactionPri::kByCompensatedSize;
      case 0x1:
        return rocksdb::CompactionPri::kOldestLargestSeqFirst;
      case 0x2:
        return rocksdb::CompactionPri::kOldestSmallestSeqFirst;
      case 0x3:
        return rocksdb::CompactionPri::kMinOverlappingRatio;
      default:
//未定义/默认
        return rocksdb::CompactionPri::kByCompensatedSize;
    }
  }
};

//org.rocksdb.accesshint的门户类
class AccessHintJni {
 public:
//返回提供的等效org.rocksdb.accesshint
//C++ ROCSDB:：dBase::Access
  static jbyte toJavaAccessHint(
      const rocksdb::DBOptions::AccessHint& access_hint) {
    switch(access_hint) {
      case rocksdb::DBOptions::AccessHint::NONE:
        return 0x0;
      case rocksdb::DBOptions::AccessHint::NORMAL:
        return 0x1;
      case rocksdb::DBOptions::AccessHint::SEQUENTIAL:
        return 0x2;
      case rocksdb::DBOptions::AccessHint::WILLNEED:
        return 0x3;
      default:
//未定义/默认
        return 0x1;
    }
  }

//返回等效的C+ROCSDB::dVals::Access
//提供Java OrgSDB.Access
  static rocksdb::DBOptions::AccessHint toCppAccessHint(jbyte jaccess_hint) {
    switch(jaccess_hint) {
      case 0x0:
        return rocksdb::DBOptions::AccessHint::NONE;
      case 0x1:
        return rocksdb::DBOptions::AccessHint::NORMAL;
      case 0x2:
        return rocksdb::DBOptions::AccessHint::SEQUENTIAL;
      case 0x3:
        return rocksdb::DBOptions::AccessHint::WILLNEED;
      default:
//未定义/默认
        return rocksdb::DBOptions::AccessHint::NORMAL;
    }
  }
};

//org.rocksdb.walrecoverymode的门户类
class WALRecoveryModeJni {
 public:
//返回所提供的等效org.rocksdb.walrecoverymode
//C++ ROCKSDB:：WALRecoveryMode枚举
  static jbyte toJavaWALRecoveryMode(
      const rocksdb::WALRecoveryMode& wal_recovery_mode) {
    switch(wal_recovery_mode) {
      case rocksdb::WALRecoveryMode::kTolerateCorruptedTailRecords:
        return 0x0;
      case rocksdb::WALRecoveryMode::kAbsoluteConsistency:
        return 0x1;
      case rocksdb::WALRecoveryMode::kPointInTimeRecovery:
        return 0x2;
      case rocksdb::WALRecoveryMode::kSkipAnyCorruptedRecords:
        return 0x3;
      default:
//未定义/默认
        return 0x2;
    }
  }

//返回等效的C++ROCSDB:：WALRecoveryMode枚举
//提供Java OrgSDB语言
  static rocksdb::WALRecoveryMode toCppWALRecoveryMode(jbyte jwal_recovery_mode) {
    switch(jwal_recovery_mode) {
      case 0x0:
        return rocksdb::WALRecoveryMode::kTolerateCorruptedTailRecords;
      case 0x1:
        return rocksdb::WALRecoveryMode::kAbsoluteConsistency;
      case 0x2:
        return rocksdb::WALRecoveryMode::kPointInTimeRecovery;
      case 0x3:
        return rocksdb::WALRecoveryMode::kSkipAnyCorruptedRecords;
      default:
//未定义/默认
        return rocksdb::WALRecoveryMode::kPointInTimeRecovery;
    }
  }
};

//org.rocksdb.tickertype的门户类
class TickerTypeJni {
 public:
//返回提供的等效org.rocksdb.tickertype
//C++ ROCSDB：：
  static jbyte toJavaTickerType(
      const rocksdb::Tickers& tickers) {
    switch(tickers) {
      case rocksdb::Tickers::BLOCK_CACHE_MISS:
        return 0x0;
      case rocksdb::Tickers::BLOCK_CACHE_HIT:
        return 0x1;
      case rocksdb::Tickers::BLOCK_CACHE_ADD:
        return 0x2;
      case rocksdb::Tickers::BLOCK_CACHE_ADD_FAILURES:
        return 0x3;
      case rocksdb::Tickers::BLOCK_CACHE_INDEX_MISS:
        return 0x4;
      case rocksdb::Tickers::BLOCK_CACHE_INDEX_HIT:
        return 0x5;
      case rocksdb::Tickers::BLOCK_CACHE_INDEX_ADD:
        return 0x6;
      case rocksdb::Tickers::BLOCK_CACHE_INDEX_BYTES_INSERT:
        return 0x7;
      case rocksdb::Tickers::BLOCK_CACHE_INDEX_BYTES_EVICT:
        return 0x8;
      case rocksdb::Tickers::BLOCK_CACHE_FILTER_MISS:
        return 0x9;
      case rocksdb::Tickers::BLOCK_CACHE_FILTER_HIT:
        return 0xA;
      case rocksdb::Tickers::BLOCK_CACHE_FILTER_ADD:
        return 0xB;
      case rocksdb::Tickers::BLOCK_CACHE_FILTER_BYTES_INSERT:
        return 0xC;
      case rocksdb::Tickers::BLOCK_CACHE_FILTER_BYTES_EVICT:
        return 0xD;
      case rocksdb::Tickers::BLOCK_CACHE_DATA_MISS:
        return 0xE;
      case rocksdb::Tickers::BLOCK_CACHE_DATA_HIT:
        return 0xF;
      case rocksdb::Tickers::BLOCK_CACHE_DATA_ADD:
        return 0x10;
      case rocksdb::Tickers::BLOCK_CACHE_DATA_BYTES_INSERT:
        return 0x11;
      case rocksdb::Tickers::BLOCK_CACHE_BYTES_READ:
        return 0x12;
      case rocksdb::Tickers::BLOCK_CACHE_BYTES_WRITE:
        return 0x13;
      case rocksdb::Tickers::BLOOM_FILTER_USEFUL:
        return 0x14;
      case rocksdb::Tickers::PERSISTENT_CACHE_HIT:
        return 0x15;
      case rocksdb::Tickers::PERSISTENT_CACHE_MISS:
        return 0x16;
      case rocksdb::Tickers::SIM_BLOCK_CACHE_HIT:
        return 0x17;
      case rocksdb::Tickers::SIM_BLOCK_CACHE_MISS:
        return 0x18;
      case rocksdb::Tickers::MEMTABLE_HIT:
        return 0x19;
      case rocksdb::Tickers::MEMTABLE_MISS:
        return 0x1A;
      case rocksdb::Tickers::GET_HIT_L0:
        return 0x1B;
      case rocksdb::Tickers::GET_HIT_L1:
        return 0x1C;
      case rocksdb::Tickers::GET_HIT_L2_AND_UP:
        return 0x1D;
      case rocksdb::Tickers::COMPACTION_KEY_DROP_NEWER_ENTRY:
        return 0x1E;
      case rocksdb::Tickers::COMPACTION_KEY_DROP_OBSOLETE:
        return 0x1F;
      case rocksdb::Tickers::COMPACTION_KEY_DROP_RANGE_DEL:
        return 0x20;
      case rocksdb::Tickers::COMPACTION_KEY_DROP_USER:
        return 0x21;
      case rocksdb::Tickers::COMPACTION_RANGE_DEL_DROP_OBSOLETE:
        return 0x22;
      case rocksdb::Tickers::NUMBER_KEYS_WRITTEN:
        return 0x23;
      case rocksdb::Tickers::NUMBER_KEYS_READ:
        return 0x24;
      case rocksdb::Tickers::NUMBER_KEYS_UPDATED:
        return 0x25;
      case rocksdb::Tickers::BYTES_WRITTEN:
        return 0x26;
      case rocksdb::Tickers::BYTES_READ:
        return 0x27;
      case rocksdb::Tickers::NUMBER_DB_SEEK:
        return 0x28;
      case rocksdb::Tickers::NUMBER_DB_NEXT:
        return 0x29;
      case rocksdb::Tickers::NUMBER_DB_PREV:
        return 0x2A;
      case rocksdb::Tickers::NUMBER_DB_SEEK_FOUND:
        return 0x2B;
      case rocksdb::Tickers::NUMBER_DB_NEXT_FOUND:
        return 0x2C;
      case rocksdb::Tickers::NUMBER_DB_PREV_FOUND:
        return 0x2D;
      case rocksdb::Tickers::ITER_BYTES_READ:
        return 0x2E;
      case rocksdb::Tickers::NO_FILE_CLOSES:
        return 0x2F;
      case rocksdb::Tickers::NO_FILE_OPENS:
        return 0x30;
      case rocksdb::Tickers::NO_FILE_ERRORS:
        return 0x31;
      case rocksdb::Tickers::STALL_L0_SLOWDOWN_MICROS:
        return 0x32;
      case rocksdb::Tickers::STALL_MEMTABLE_COMPACTION_MICROS:
        return 0x33;
      case rocksdb::Tickers::STALL_L0_NUM_FILES_MICROS:
        return 0x34;
      case rocksdb::Tickers::STALL_MICROS:
        return 0x35;
      case rocksdb::Tickers::DB_MUTEX_WAIT_MICROS:
        return 0x36;
      case rocksdb::Tickers::RATE_LIMIT_DELAY_MILLIS:
        return 0x37;
      case rocksdb::Tickers::NO_ITERATORS:
        return 0x38;
      case rocksdb::Tickers::NUMBER_MULTIGET_CALLS:
        return 0x39;
      case rocksdb::Tickers::NUMBER_MULTIGET_KEYS_READ:
        return 0x3A;
      case rocksdb::Tickers::NUMBER_MULTIGET_BYTES_READ:
        return 0x3B;
      case rocksdb::Tickers::NUMBER_FILTERED_DELETES:
        return 0x3C;
      case rocksdb::Tickers::NUMBER_MERGE_FAILURES:
        return 0x3D;
      case rocksdb::Tickers::BLOOM_FILTER_PREFIX_CHECKED:
        return 0x3E;
      case rocksdb::Tickers::BLOOM_FILTER_PREFIX_USEFUL:
        return 0x3F;
      case rocksdb::Tickers::NUMBER_OF_RESEEKS_IN_ITERATION:
        return 0x40;
      case rocksdb::Tickers::GET_UPDATES_SINCE_CALLS:
        return 0x41;
      case rocksdb::Tickers::BLOCK_CACHE_COMPRESSED_MISS:
        return 0x42;
      case rocksdb::Tickers::BLOCK_CACHE_COMPRESSED_HIT:
        return 0x43;
      case rocksdb::Tickers::BLOCK_CACHE_COMPRESSED_ADD:
        return 0x44;
      case rocksdb::Tickers::BLOCK_CACHE_COMPRESSED_ADD_FAILURES:
        return 0x45;
      case rocksdb::Tickers::WAL_FILE_SYNCED:
        return 0x46;
      case rocksdb::Tickers::WAL_FILE_BYTES:
        return 0x47;
      case rocksdb::Tickers::WRITE_DONE_BY_SELF:
        return 0x48;
      case rocksdb::Tickers::WRITE_DONE_BY_OTHER:
        return 0x49;
      case rocksdb::Tickers::WRITE_TIMEDOUT:
        return 0x4A;
      case rocksdb::Tickers::WRITE_WITH_WAL:
        return 0x4B;
      case rocksdb::Tickers::COMPACT_READ_BYTES:
        return 0x4C;
      case rocksdb::Tickers::COMPACT_WRITE_BYTES:
        return 0x4D;
      case rocksdb::Tickers::FLUSH_WRITE_BYTES:
        return 0x4E;
      case rocksdb::Tickers::NUMBER_DIRECT_LOAD_TABLE_PROPERTIES:
        return 0x4F;
      case rocksdb::Tickers::NUMBER_SUPERVERSION_ACQUIRES:
        return 0x50;
      case rocksdb::Tickers::NUMBER_SUPERVERSION_RELEASES:
        return 0x51;
      case rocksdb::Tickers::NUMBER_SUPERVERSION_CLEANUPS:
        return 0x52;
      case rocksdb::Tickers::NUMBER_BLOCK_COMPRESSED:
        return 0x53;
      case rocksdb::Tickers::NUMBER_BLOCK_DECOMPRESSED:
        return 0x54;
      case rocksdb::Tickers::NUMBER_BLOCK_NOT_COMPRESSED:
        return 0x55;
      case rocksdb::Tickers::MERGE_OPERATION_TOTAL_TIME:
        return 0x56;
      case rocksdb::Tickers::FILTER_OPERATION_TOTAL_TIME:
        return 0x57;
      case rocksdb::Tickers::ROW_CACHE_HIT:
        return 0x58;
      case rocksdb::Tickers::ROW_CACHE_MISS:
        return 0x59;
      case rocksdb::Tickers::READ_AMP_ESTIMATE_USEFUL_BYTES:
        return 0x5A;
      case rocksdb::Tickers::READ_AMP_TOTAL_READ_BYTES:
        return 0x5B;
      case rocksdb::Tickers::NUMBER_RATE_LIMITER_DRAINS:
        return 0x5C;
      case rocksdb::Tickers::TICKER_ENUM_MAX:
        return 0x5D;
      
      default:
//未定义/默认
        return 0x0;
    }
  }

//返回等效的C++ROCSDB:：
//提供Java OrgSDB.TICKER型
  static rocksdb::Tickers toCppTickers(jbyte jticker_type) {
    switch(jticker_type) {
      case 0x0:
        return rocksdb::Tickers::BLOCK_CACHE_MISS;
      case 0x1:
        return rocksdb::Tickers::BLOCK_CACHE_HIT;
      case 0x2:
        return rocksdb::Tickers::BLOCK_CACHE_ADD;
      case 0x3:
        return rocksdb::Tickers::BLOCK_CACHE_ADD_FAILURES;
      case 0x4:
        return rocksdb::Tickers::BLOCK_CACHE_INDEX_MISS;
      case 0x5:
        return rocksdb::Tickers::BLOCK_CACHE_INDEX_HIT;
      case 0x6:
        return rocksdb::Tickers::BLOCK_CACHE_INDEX_ADD;
      case 0x7:
        return rocksdb::Tickers::BLOCK_CACHE_INDEX_BYTES_INSERT;
      case 0x8:
        return rocksdb::Tickers::BLOCK_CACHE_INDEX_BYTES_EVICT;
      case 0x9:
        return rocksdb::Tickers::BLOCK_CACHE_FILTER_MISS;
      case 0xA:
        return rocksdb::Tickers::BLOCK_CACHE_FILTER_HIT;
      case 0xB:
        return rocksdb::Tickers::BLOCK_CACHE_FILTER_ADD;
      case 0xC:
        return rocksdb::Tickers::BLOCK_CACHE_FILTER_BYTES_INSERT;
      case 0xD:
        return rocksdb::Tickers::BLOCK_CACHE_FILTER_BYTES_EVICT;
      case 0xE:
        return rocksdb::Tickers::BLOCK_CACHE_DATA_MISS;
      case 0xF:
        return rocksdb::Tickers::BLOCK_CACHE_DATA_HIT;
      case 0x10:
        return rocksdb::Tickers::BLOCK_CACHE_DATA_ADD;
      case 0x11:
        return rocksdb::Tickers::BLOCK_CACHE_DATA_BYTES_INSERT;
      case 0x12:
        return rocksdb::Tickers::BLOCK_CACHE_BYTES_READ;
      case 0x13:
        return rocksdb::Tickers::BLOCK_CACHE_BYTES_WRITE;
      case 0x14:
        return rocksdb::Tickers::BLOOM_FILTER_USEFUL;
      case 0x15:
        return rocksdb::Tickers::PERSISTENT_CACHE_HIT;
      case 0x16:
        return rocksdb::Tickers::PERSISTENT_CACHE_MISS;
      case 0x17:
        return rocksdb::Tickers::SIM_BLOCK_CACHE_HIT;
      case 0x18:
        return rocksdb::Tickers::SIM_BLOCK_CACHE_MISS;
      case 0x19:
        return rocksdb::Tickers::MEMTABLE_HIT;
      case 0x1A:
        return rocksdb::Tickers::MEMTABLE_MISS;
      case 0x1B:
        return rocksdb::Tickers::GET_HIT_L0;
      case 0x1C:
        return rocksdb::Tickers::GET_HIT_L1;
      case 0x1D:
        return rocksdb::Tickers::GET_HIT_L2_AND_UP;
      case 0x1E:
        return rocksdb::Tickers::COMPACTION_KEY_DROP_NEWER_ENTRY;
      case 0x1F:
        return rocksdb::Tickers::COMPACTION_KEY_DROP_OBSOLETE;
      case 0x20:
        return rocksdb::Tickers::COMPACTION_KEY_DROP_RANGE_DEL;
      case 0x21:
        return rocksdb::Tickers::COMPACTION_KEY_DROP_USER;
      case 0x22:
        return rocksdb::Tickers::COMPACTION_RANGE_DEL_DROP_OBSOLETE;
      case 0x23:
        return rocksdb::Tickers::NUMBER_KEYS_WRITTEN;
      case 0x24:
        return rocksdb::Tickers::NUMBER_KEYS_READ;
      case 0x25:
        return rocksdb::Tickers::NUMBER_KEYS_UPDATED;
      case 0x26:
        return rocksdb::Tickers::BYTES_WRITTEN;
      case 0x27:
        return rocksdb::Tickers::BYTES_READ;
      case 0x28:
        return rocksdb::Tickers::NUMBER_DB_SEEK;
      case 0x29:
        return rocksdb::Tickers::NUMBER_DB_NEXT;
      case 0x2A:
        return rocksdb::Tickers::NUMBER_DB_PREV;
      case 0x2B:
        return rocksdb::Tickers::NUMBER_DB_SEEK_FOUND;
      case 0x2C:
        return rocksdb::Tickers::NUMBER_DB_NEXT_FOUND;
      case 0x2D:
        return rocksdb::Tickers::NUMBER_DB_PREV_FOUND;
      case 0x2E:
        return rocksdb::Tickers::ITER_BYTES_READ;
      case 0x2F:
        return rocksdb::Tickers::NO_FILE_CLOSES;
      case 0x30:
        return rocksdb::Tickers::NO_FILE_OPENS;
      case 0x31:
        return rocksdb::Tickers::NO_FILE_ERRORS;
      case 0x32:
        return rocksdb::Tickers::STALL_L0_SLOWDOWN_MICROS;
      case 0x33:
        return rocksdb::Tickers::STALL_MEMTABLE_COMPACTION_MICROS;
      case 0x34:
        return rocksdb::Tickers::STALL_L0_NUM_FILES_MICROS;
      case 0x35:
        return rocksdb::Tickers::STALL_MICROS;
      case 0x36:
        return rocksdb::Tickers::DB_MUTEX_WAIT_MICROS;
      case 0x37:
        return rocksdb::Tickers::RATE_LIMIT_DELAY_MILLIS;
      case 0x38:
        return rocksdb::Tickers::NO_ITERATORS;
      case 0x39:
        return rocksdb::Tickers::NUMBER_MULTIGET_CALLS;
      case 0x3A:
        return rocksdb::Tickers::NUMBER_MULTIGET_KEYS_READ;
      case 0x3B:
        return rocksdb::Tickers::NUMBER_MULTIGET_BYTES_READ;
      case 0x3C:
        return rocksdb::Tickers::NUMBER_FILTERED_DELETES;
      case 0x3D:
        return rocksdb::Tickers::NUMBER_MERGE_FAILURES;
      case 0x3E:
        return rocksdb::Tickers::BLOOM_FILTER_PREFIX_CHECKED;
      case 0x3F:
        return rocksdb::Tickers::BLOOM_FILTER_PREFIX_USEFUL;
      case 0x40:
        return rocksdb::Tickers::NUMBER_OF_RESEEKS_IN_ITERATION;
      case 0x41:
        return rocksdb::Tickers::GET_UPDATES_SINCE_CALLS;
      case 0x42:
        return rocksdb::Tickers::BLOCK_CACHE_COMPRESSED_MISS;
      case 0x43:
        return rocksdb::Tickers::BLOCK_CACHE_COMPRESSED_HIT;
      case 0x44:
        return rocksdb::Tickers::BLOCK_CACHE_COMPRESSED_ADD;
      case 0x45:
        return rocksdb::Tickers::BLOCK_CACHE_COMPRESSED_ADD_FAILURES;
      case 0x46:
        return rocksdb::Tickers::WAL_FILE_SYNCED;
      case 0x47:
        return rocksdb::Tickers::WAL_FILE_BYTES;
      case 0x48:
        return rocksdb::Tickers::WRITE_DONE_BY_SELF;
      case 0x49:
        return rocksdb::Tickers::WRITE_DONE_BY_OTHER;
      case 0x4A:
        return rocksdb::Tickers::WRITE_TIMEDOUT;
      case 0x4B:
        return rocksdb::Tickers::WRITE_WITH_WAL;
      case 0x4C:
        return rocksdb::Tickers::COMPACT_READ_BYTES;
      case 0x4D:
        return rocksdb::Tickers::COMPACT_WRITE_BYTES;
      case 0x4E:
        return rocksdb::Tickers::FLUSH_WRITE_BYTES;
      case 0x4F:
        return rocksdb::Tickers::NUMBER_DIRECT_LOAD_TABLE_PROPERTIES;
      case 0x50:
        return rocksdb::Tickers::NUMBER_SUPERVERSION_ACQUIRES;
      case 0x51:
        return rocksdb::Tickers::NUMBER_SUPERVERSION_RELEASES;
      case 0x52:
        return rocksdb::Tickers::NUMBER_SUPERVERSION_CLEANUPS;
      case 0x53:
        return rocksdb::Tickers::NUMBER_BLOCK_COMPRESSED;
      case 0x54:
        return rocksdb::Tickers::NUMBER_BLOCK_DECOMPRESSED;
      case 0x55:
        return rocksdb::Tickers::NUMBER_BLOCK_NOT_COMPRESSED;
      case 0x56:
        return rocksdb::Tickers::MERGE_OPERATION_TOTAL_TIME;
      case 0x57:
        return rocksdb::Tickers::FILTER_OPERATION_TOTAL_TIME;
      case 0x58:
        return rocksdb::Tickers::ROW_CACHE_HIT;
      case 0x59:
        return rocksdb::Tickers::ROW_CACHE_MISS;
      case 0x5A:
        return rocksdb::Tickers::READ_AMP_ESTIMATE_USEFUL_BYTES;
      case 0x5B:
        return rocksdb::Tickers::READ_AMP_TOTAL_READ_BYTES;
      case 0x5C:
        return rocksdb::Tickers::NUMBER_RATE_LIMITER_DRAINS;
      case 0x5D:
        return rocksdb::Tickers::TICKER_ENUM_MAX;

      default:
//未定义/默认
        return rocksdb::Tickers::BLOCK_CACHE_MISS;
    }
  }
};

//org.rocksdb.histogramtype的门户类
class HistogramTypeJni {
 public:
//为提供的返回等效的org.rocksdb.histogramtype
//C++ ROCKSDB:：Histograms枚举
  static jbyte toJavaHistogramsType(
      const rocksdb::Histograms& histograms) {
    switch(histograms) {
      case rocksdb::Histograms::DB_GET:
        return 0x0;
      case rocksdb::Histograms::DB_WRITE:
        return 0x1;
      case rocksdb::Histograms::COMPACTION_TIME:
        return 0x2;
      case rocksdb::Histograms::SUBCOMPACTION_SETUP_TIME:
        return 0x3;
      case rocksdb::Histograms::TABLE_SYNC_MICROS:
        return 0x4;
      case rocksdb::Histograms::COMPACTION_OUTFILE_SYNC_MICROS:
        return 0x5;
      case rocksdb::Histograms::WAL_FILE_SYNC_MICROS:
        return 0x6;
      case rocksdb::Histograms::MANIFEST_FILE_SYNC_MICROS:
        return 0x7;
      case rocksdb::Histograms::TABLE_OPEN_IO_MICROS:
        return 0x8;
      case rocksdb::Histograms::DB_MULTIGET:
        return 0x9;
      case rocksdb::Histograms::READ_BLOCK_COMPACTION_MICROS:
        return 0xA;
      case rocksdb::Histograms::READ_BLOCK_GET_MICROS:
        return 0xB;
      case rocksdb::Histograms::WRITE_RAW_BLOCK_MICROS:
        return 0xC;
      case rocksdb::Histograms::STALL_L0_SLOWDOWN_COUNT:
        return 0xD;
      case rocksdb::Histograms::STALL_MEMTABLE_COMPACTION_COUNT:
        return 0xE;
      case rocksdb::Histograms::STALL_L0_NUM_FILES_COUNT:
        return 0xF;
      case rocksdb::Histograms::HARD_RATE_LIMIT_DELAY_COUNT:
        return 0x10;
      case rocksdb::Histograms::SOFT_RATE_LIMIT_DELAY_COUNT:
        return 0x11;
      case rocksdb::Histograms::NUM_FILES_IN_SINGLE_COMPACTION:
        return 0x12;
      case rocksdb::Histograms::DB_SEEK:
        return 0x13;
      case rocksdb::Histograms::WRITE_STALL:
        return 0x14;
      case rocksdb::Histograms::SST_READ_MICROS:
        return 0x15;
      case rocksdb::Histograms::NUM_SUBCOMPACTIONS_SCHEDULED:
        return 0x16;
      case rocksdb::Histograms::BYTES_PER_READ:
        return 0x17;
      case rocksdb::Histograms::BYTES_PER_WRITE:
        return 0x18;
      case rocksdb::Histograms::BYTES_PER_MULTIGET:
        return 0x19;
      case rocksdb::Histograms::BYTES_COMPRESSED:
        return 0x1A;
      case rocksdb::Histograms::BYTES_DECOMPRESSED:
        return 0x1B;
      case rocksdb::Histograms::COMPRESSION_TIMES_NANOS:
        return 0x1C;
      case rocksdb::Histograms::DECOMPRESSION_TIMES_NANOS:
        return 0x1D;
      case rocksdb::Histograms::READ_NUM_MERGE_OPERANDS:
        return 0x1E;
      case rocksdb::Histograms::HISTOGRAM_ENUM_MAX:
        return 0x1F;

      default:
//未定义/默认
        return 0x0;
    }
  }

//返回等效的C++ ROCSDB:：直方图枚举
//提供JavaOr.RoSDSDB组织风格
  static rocksdb::Histograms toCppHistograms(jbyte jhistograms_type) {
    switch(jhistograms_type) {
      case 0x0:
        return rocksdb::Histograms::DB_GET;
      case 0x1:
        return rocksdb::Histograms::DB_WRITE;
      case 0x2:
        return rocksdb::Histograms::COMPACTION_TIME;
      case 0x3:
        return rocksdb::Histograms::SUBCOMPACTION_SETUP_TIME;
      case 0x4:
        return rocksdb::Histograms::TABLE_SYNC_MICROS;
      case 0x5:
        return rocksdb::Histograms::COMPACTION_OUTFILE_SYNC_MICROS;
      case 0x6:
        return rocksdb::Histograms::WAL_FILE_SYNC_MICROS;
      case 0x7:
        return rocksdb::Histograms::MANIFEST_FILE_SYNC_MICROS;
      case 0x8:
        return rocksdb::Histograms::TABLE_OPEN_IO_MICROS;
      case 0x9:
        return rocksdb::Histograms::DB_MULTIGET;
      case 0xA:
        return rocksdb::Histograms::READ_BLOCK_COMPACTION_MICROS;
      case 0xB:
        return rocksdb::Histograms::READ_BLOCK_GET_MICROS;
      case 0xC:
        return rocksdb::Histograms::WRITE_RAW_BLOCK_MICROS;
      case 0xD:
        return rocksdb::Histograms::STALL_L0_SLOWDOWN_COUNT;
      case 0xE:
        return rocksdb::Histograms::STALL_MEMTABLE_COMPACTION_COUNT;
      case 0xF:
        return rocksdb::Histograms::STALL_L0_NUM_FILES_COUNT;
      case 0x10:
        return rocksdb::Histograms::HARD_RATE_LIMIT_DELAY_COUNT;
      case 0x11:
        return rocksdb::Histograms::SOFT_RATE_LIMIT_DELAY_COUNT;
      case 0x12:
        return rocksdb::Histograms::NUM_FILES_IN_SINGLE_COMPACTION;
      case 0x13:
        return rocksdb::Histograms::DB_SEEK;
      case 0x14:
        return rocksdb::Histograms::WRITE_STALL;
      case 0x15:
        return rocksdb::Histograms::SST_READ_MICROS;
      case 0x16:
        return rocksdb::Histograms::NUM_SUBCOMPACTIONS_SCHEDULED;
      case 0x17:
        return rocksdb::Histograms::BYTES_PER_READ;
      case 0x18:
        return rocksdb::Histograms::BYTES_PER_WRITE;
      case 0x19:
        return rocksdb::Histograms::BYTES_PER_MULTIGET;
      case 0x1A:
        return rocksdb::Histograms::BYTES_COMPRESSED;
      case 0x1B:
        return rocksdb::Histograms::BYTES_DECOMPRESSED;
      case 0x1C:
        return rocksdb::Histograms::COMPRESSION_TIMES_NANOS;
      case 0x1D:
        return rocksdb::Histograms::DECOMPRESSION_TIMES_NANOS;
      case 0x1E:
        return rocksdb::Histograms::READ_NUM_MERGE_OPERANDS;
      case 0x1F:
        return rocksdb::Histograms::HISTOGRAM_ENUM_MAX;

      default:
//未定义/默认
        return rocksdb::Histograms::DB_GET;
    }
  }
};

//org.rocksdb.statslevel的门户类
class StatsLevelJni {
 public:
//返回所提供的等效org.rocksdb.statsLevel
//C++ROCKSDB:：StaseLead枚举
  static jbyte toJavaStatsLevel(
      const rocksdb::StatsLevel& stats_level) {
    switch(stats_level) {
      case rocksdb::StatsLevel::kExceptDetailedTimers:
        return 0x0;
      case rocksdb::StatsLevel::kExceptTimeForMutex:
        return 0x1;
      case rocksdb::StatsLevel::kAll:
        return 0x2;

      default:
//未定义/默认
        return 0x0;
    }
  }

//返回等效的C++ROCSDB:：StaseStaleEnUM
//提供Java OrgRSDSDB状态级别
  static rocksdb::StatsLevel toCppStatsLevel(jbyte jstats_level) {
    switch(jstats_level) {
      case 0x0:
        return rocksdb::StatsLevel::kExceptDetailedTimers;
      case 0x1:
        return rocksdb::StatsLevel::kExceptTimeForMutex;
      case 0x2:
        return rocksdb::StatsLevel::kAll;

      default:
//未定义/默认
        return rocksdb::StatsLevel::kExceptDetailedTimers;
    }
  }
};

//与rocksdb和jni一起工作的各种实用功能
class JniUtil {
 public:
    /*
     *从中获取对JNIENV的引用
     ＊JVM
     *
     *如果当前线程未连接到javavm
     *然后将其附加以检索JNIENV
     *
     *如果连接了一个螺纹，以后必须手动连接。
     *通过调用javavm:：DetachCurrentThread释放。
     *这可以通过始终将呼叫与此匹配来处理。
     *函数，调用@link jniutil:：releasejniev（javavm*，jboolean）
     *
     *@param jvm（in）指向javavm实例的指针
     *@param附加（out）一个指向布尔值的指针，
     *如果必须附加线程，则将设置为jni_true。
     *
     *@如果出现致命错误，返回指向jnienv或nullptr的指针
     *发生，无法检索JNIENV
     **/

    static JNIEnv* getJniEnv(JavaVM* jvm, jboolean* attached) {
      assert(jvm != nullptr);

      JNIEnv *env;
      const jint env_rs = jvm->GetEnv(reinterpret_cast<void**>(&env),
          JNI_VERSION_1_2);

      if(env_rs == JNI_OK) {
//当前线程已附加，返回jnienv
        *attached = JNI_FALSE;
        return env;
      } else if(env_rs == JNI_EDETACHED) {
//当前线程未附加，尝试附加
        const jint rs_attach = jvm->AttachCurrentThread(reinterpret_cast<void**>(&env), NULL);
        if(rs_attach == JNI_OK) {
          *attached = JNI_TRUE;
          return env;
        } else {
//错误，无法附加线程
          std::cerr << "JniUtil::getJinEnv - Fatal: could not attach current thread to JVM!" << std::endl;
          return nullptr;
        }
      } else if(env_rs == JNI_EVERSION) {
//错误，JDK不支持JNI_版本_1_2+
        std::cerr << "JniUtil::getJinEnv - Fatal: JDK does not support JNI_VERSION_1_2" << std::endl;
        return nullptr;
      } else {
        std::cerr << "JniUtil::getJinEnv - Fatal: Unknown error: env_rs=" << env_rs << std::endl;
        return nullptr;
      }
    }

    /*
     *对应于@link jniutil:：getjnienv（javavm*，jboolean*）
     *
     *如果当前线程以前是JVM线程，则将其从JVM中分离。
     *附加
     *
     *@param jvm（in）指向javavm实例的指针
     *@param attached（in）jni_true如果我们以前必须附加线程
     *到javavm获取jnienv
     **/

    static void releaseJniEnv(JavaVM* jvm, jboolean& attached) {
      assert(jvm != nullptr);
      if(attached == JNI_TRUE) {
        const jint rs_detach = jvm->DetachCurrentThread();
        assert(rs_detach == JNI_OK);
        if(rs_detach != JNI_OK) {
          std::cerr << "JniUtil::getJinEnv - Warn: Unable to detach current thread from JVM!" << std::endl;
        }
      }
    }

    /*
     *将Java String []复制到C++ STD:：vector < STD::String >
     *
     *@ PARAM-Env（in）指向Java环境的指针
     *@ PARAM JSS（in）要复制的Java字符串数组
     *@param has_exception（out）将设置为jni_true
     *如果OutOfMemoryError或ArrayIndexOutOfBoundsException
     *发生异常
     *
     *@返回一个STD:：vector < STD:String >，包含Java字符串的副本
     **/

    static std::vector<std::string> copyStrings(JNIEnv* env,
        jobjectArray jss, jboolean* has_exception) {
          return rocksdb::JniUtil::copyStrings(env, jss,
              env->GetArrayLength(jss), has_exception);
    }

    /*
     *将Java String []复制到C++ STD:：vector < STD::String >
     *
     *@ PARAM-Env（in）指向Java环境的指针
     *@ PARAM JSS（in）要复制的Java字符串数组
     *@ PARAM-JSSYLLN（in）要复制的Java字符串数组的长度
     *@param has_exception（out）将设置为jni_true
     *如果OutOfMemoryError或ArrayIndexOutOfBoundsException
     *发生异常
     *
     *@返回一个STD:：vector < STD:String >，包含Java字符串的副本
     **/

    static std::vector<std::string> copyStrings(JNIEnv* env,
        jobjectArray jss, const jsize jss_len, jboolean* has_exception) {
      std::vector<std::string> strs;
      for (jsize i = 0; i < jss_len; i++) {
        jobject js = env->GetObjectArrayElement(jss, i);
        if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
          *has_exception = JNI_TRUE;
          return strs;
        }

        jstring jstr = static_cast<jstring>(js);
        const char* str = env->GetStringUTFChars(jstr, nullptr);
        if(str == nullptr) {
//引发异常：OutOfMemoryError
          env->DeleteLocalRef(js);
          *has_exception = JNI_TRUE;
          return strs;
        }

        strs.push_back(std::string(str));

        env->ReleaseStringUTFChars(jstr, str);
        env->DeleteLocalRef(js);
      }

      *has_exception = JNI_FALSE;
      return strs;
    }

    /*
     *将JString复制到std:：string
     *并释放原始jstring
     *
     *如果发生异常，则jnienv:：exceptioncheck（）。
     *将被呼叫
     *
     *@ PARAM-Env（in）指向Java环境的指针
     *@ PARAM-JS（in）要复制的Java字符串
     *@param has_exception（out）将设置为jni_true
     *如果发生OutOfMemoryError异常
     *
     *@返回jstring的std:string副本，或
     *空std：：string if has_exception==jni_true
     **/

    static std::string copyString(JNIEnv* env, jstring js,
        jboolean* has_exception) {
      const char *utf = env->GetStringUTFChars(js, nullptr);
      if(utf == nullptr) {
//引发异常：OutOfMemoryError
        env->ExceptionCheck();
        *has_exception = JNI_TRUE;
        return std::string();
      } else if(env->ExceptionCheck()) {
//引发异常
        env->ReleaseStringUTFChars(js, utf);
        *has_exception = JNI_TRUE;
        return std::string();
      }

      std::string name(utf);
      env->ReleaseStringUTFChars(js, utf);
      *has_exception = JNI_FALSE;
      return name;
    }

    /*
     *将std:：string中的字节复制到jbytearray
     *
     *@ PARAM Env指向Java环境的指针
     *@param bytes要复制的字节
     *
     *@如果出现异常，返回Java字节[]或nulpPTR
     **/

    static jbyteArray copyBytes(JNIEnv* env, std::string bytes) {
      const jsize jlen = static_cast<jsize>(bytes.size());

      jbyteArray jbytes = env->NewByteArray(jlen);
      if(jbytes == nullptr) {
//引发异常：OutOfMemoryError
        return nullptr;
      }

      env->SetByteArrayRegion(jbytes, 0, jlen,
        const_cast<jbyte*>(reinterpret_cast<const jbyte*>(bytes.c_str())));
      if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
        env->DeleteLocalRef(jbytes);
        return nullptr;
      }

      return jbytes;
    }

    /*
     *给出一个JavaBy[]]，它是java.郎格字符串的数组。
     *如果每个字符串都是字节[]，则传递的函数'string_fn'
     *将对每个字符串调用，结果是
     *调用传递函数“collector”，
     *
     *@ PARAM-Env（in）指向Java环境的指针
     *@ PARAM JByTexSstring（in）以字节表示的Java数组
     *@param string_fn（in）为每个字符串调用的转换函数
     *@param collector_fn（in）为结果调用的收集器
     *每个“字符串”，
     *@param has_exception（out）将设置为jni_true
     *如果arrayIndexOutOfBoundsException或OutOfMemoryError
     *发生异常
     **/

    template <typename T> static void byteStrings(JNIEnv* env,
        jobjectArray jbyte_strings,
        std::function<T(const char*, const size_t)> string_fn,
        std::function<void(size_t, T)> collector_fn,
        jboolean *has_exception) {
      const jsize jlen = env->GetArrayLength(jbyte_strings);

      for(jsize i = 0; i < jlen; i++) {
        jobject jbyte_string_obj = env->GetObjectArrayElement(jbyte_strings, i);
        if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
*has_exception = JNI_TRUE;  //信号误差
          return;
        }

        jbyteArray jbyte_string_ary =
            reinterpret_cast<jbyteArray>(jbyte_string_obj);
        T result = byteString(env, jbyte_string_ary, string_fn, has_exception);

        env->DeleteLocalRef(jbyte_string_obj);

        if(*has_exception == JNI_TRUE) {
//引发异常：OutOfMemoryError
          return;
        }

        collector_fn(i, result);
      }

      *has_exception = JNI_FALSE;
    }

    /*
     *给定一个Java字符串，它表示为Java字节数组字节[]，
     *传递的函数'string_fn'将在字符串上调用
     *结果返回
     *
     *@ PARAM-Env（in）指向Java环境的指针
     *@ PARAM JyByTyString（in）以字节表示的Java字符串
     *@param string_fn（in）要调用字符串的转换函数
     *@param has_exception（out）将设置为jni_true
     *如果发生OutOfMemoryError异常
     **/

    template <typename T> static T byteString(JNIEnv* env,
        jbyteArray jbyte_string_ary,
        std::function<T(const char*, const size_t)> string_fn,
        jboolean* has_exception) {
      const jsize jbyte_string_len = env->GetArrayLength(jbyte_string_ary);
      jbyte* jbyte_string =
          env->GetByteArrayElements(jbyte_string_ary, nullptr);
      if(jbyte_string == nullptr) {
//引发异常：OutOfMemoryError
        *has_exception = JNI_TRUE;
return nullptr;  //信号误差
      }

      T result =
          string_fn(reinterpret_cast<char *>(jbyte_string), jbyte_string_len);

      env->ReleaseByteArrayElements(jbyte_string_ary, jbyte_string, JNI_ABORT);

      *has_exception = JNI_FALSE;
      return result;
    }

    /*
     *将一个STD::vector < String >转换成一个Java字节[][]，其中每个Java字符串
     *表示为Java字节数组字节[]。
     *
     *@ PARAM Env指向Java环境的指针
     *@param strings字符串的向量
     *
     *@返回一个以字节表示的字符串数组
     **/

    static jobjectArray stringsBytes(JNIEnv* env, std::vector<std::string> strings) {
      jclass jcls_ba = ByteJni::getArrayJClass(env);
      if(jcls_ba == nullptr) {
//发生异常
        return nullptr;
      }

      const jsize len = static_cast<jsize>(strings.size());

      jobjectArray jbyte_strings = env->NewObjectArray(len, jcls_ba, nullptr);
      if(jbyte_strings == nullptr) {
//引发异常：OutOfMemoryError
        return nullptr;
      }

      for (jsize i = 0; i < len; i++) {
        std::string *str = &strings[i];
        const jsize str_len = static_cast<jsize>(str->size());

        jbyteArray jbyte_string_ary = env->NewByteArray(str_len);
        if(jbyte_string_ary == nullptr) {
//引发异常：OutOfMemoryError
          env->DeleteLocalRef(jbyte_strings);
          return nullptr;
        }

        env->SetByteArrayRegion(
          jbyte_string_ary, 0, str_len,
          const_cast<jbyte*>(reinterpret_cast<const jbyte*>(str->c_str())));
        if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
          env->DeleteLocalRef(jbyte_string_ary);
          env->DeleteLocalRef(jbyte_strings);
          return nullptr;
        }

        env->SetObjectArrayElement(jbyte_strings, i, jbyte_string_ary);
        if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
//或ArrayStoreException
          env->DeleteLocalRef(jbyte_string_ary);
          env->DeleteLocalRef(jbyte_strings);
          return nullptr;
        }

        env->DeleteLocalRef(jbyte_string_ary);
      }

      return jbyte_strings;
    }

    /*
     *键和值的操作助手
     *例如writebatch->put
     *
     *TODO（ar）可以扩展到覆盖返回的rocksDB：：status
     *来自“op”，用于rocksdb->put等。
     **/

    static void kv_op(
        std::function<void(rocksdb::Slice, rocksdb::Slice)> op,
        JNIEnv* env, jobject jobj,
        jbyteArray jkey, jint jkey_len,
        jbyteArray jentry_value, jint jentry_value_len) {
      jbyte* key = env->GetByteArrayElements(jkey, nullptr);
      if(env->ExceptionCheck()) {
//引发异常：OutOfMemoryError
        return;
      }

      jbyte* value = env->GetByteArrayElements(jentry_value, nullptr);
      if(env->ExceptionCheck()) {
//引发异常：OutOfMemoryError
        if(key != nullptr) {
          env->ReleaseByteArrayElements(jkey, key, JNI_ABORT);
        }
        return;
      }

      rocksdb::Slice key_slice(reinterpret_cast<char*>(key), jkey_len);
      rocksdb::Slice value_slice(reinterpret_cast<char*>(value),
          jentry_value_len);

      op(key_slice, value_slice);

      if(value != nullptr) {
        env->ReleaseByteArrayElements(jentry_value, value, JNI_ABORT);
      }
      if(key != nullptr) {
        env->ReleaseByteArrayElements(jkey, key, JNI_ABORT);
      }
    }

    /*
     *用于键操作的助手
     *例如writebatch->delete
     *
     *TODO（ar）可以扩展到覆盖返回的rocksDB：：status
     *来自“op”，用于rocksdb->delete等。
     **/

    static void k_op(
        std::function<void(rocksdb::Slice)> op,
        JNIEnv* env, jobject jobj,
        jbyteArray jkey, jint jkey_len) {
      jbyte* key = env->GetByteArrayElements(jkey, nullptr);
      if(env->ExceptionCheck()) {
//引发异常：OutOfMemoryError
        return;
      }

      rocksdb::Slice key_slice(reinterpret_cast<char*>(key), jkey_len);

      op(key_slice);

      if(key != nullptr) {
        env->ReleaseByteArrayElements(jkey, key, JNI_ABORT);
      }
    }

    /*
     *值操作助手
     *例如writebatchwithindex->getfrombatch
     **/

    static jbyteArray v_op(
        std::function<rocksdb::Status(rocksdb::Slice, std::string*)> op,
        JNIEnv* env, jbyteArray jkey, jint jkey_len) {
      jbyte* key = env->GetByteArrayElements(jkey, nullptr);
      if(env->ExceptionCheck()) {
//引发异常：OutOfMemoryError
        return nullptr;
      }

      rocksdb::Slice key_slice(reinterpret_cast<char*>(key), jkey_len);

      std::string value;
      rocksdb::Status s = op(key_slice, &value);

      if(key != nullptr) {
        env->ReleaseByteArrayElements(jkey, key, JNI_ABORT);
      }

      if (s.IsNotFound()) {
        return nullptr;
      }

      if (s.ok()) {
        jbyteArray jret_value =
            env->NewByteArray(static_cast<jsize>(value.size()));
        if(jret_value == nullptr) {
//引发异常：OutOfMemoryError
          return nullptr;
        }

        env->SetByteArrayRegion(jret_value, 0, static_cast<jsize>(value.size()),
                                const_cast<jbyte*>(reinterpret_cast<const jbyte*>(value.c_str())));
        if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
          if(jret_value != nullptr) {
            env->DeleteLocalRef(jret_value);
          }
          return nullptr;
        }

        return jret_value;
      }

      rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
      return nullptr;
    }
};

}  //命名空间rocksdb
#endif  //Java_Rocksjni_门户
