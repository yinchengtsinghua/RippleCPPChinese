
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
//该文件实现了Java和C++之间的“桥梁”。
//ROCKSDB:：切片。

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string>

#include "include/org_rocksdb_AbstractSlice.h"
#include "include/org_rocksdb_Slice.h"
#include "include/org_rocksdb_DirectSlice.h"
#include "rocksdb/slice.h"
#include "rocksjni/portal.h"

//<editor fold desc=“org.rocksdb.abstractslice>

/*
 *类：org-rocksdb-abstractslice
 *方法：createneslicefromstring
 *签名：（ljava/lang/string；）j
 **/

jlong Java_org_rocksdb_AbstractSlice_createNewSliceFromString(
    JNIEnv * env, jclass jcls, jstring jstr) {
  const auto* str = env->GetStringUTFChars(jstr, nullptr);
  if(str == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }

  const size_t len = strlen(str);

//注意：buf将在
//java-org-rocksdb-slice-disposeinternalbuf或
//或java-org-rocksdb-directslice-dispositeinternalbuf方法
  char* buf = new char[len + 1];
  memcpy(buf, str, len);
  buf[len] = 0;
  env->ReleaseStringUTFChars(jstr, str);

  const auto* slice = new rocksdb::Slice(buf);
  return reinterpret_cast<jlong>(slice);
}

/*
 *类：org-rocksdb-abstractslice
 *方法：size0
 *签字：（j）I
 **/

jint Java_org_rocksdb_AbstractSlice_size0(
    JNIEnv* env, jobject jobj, jlong handle) {
  const auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  return static_cast<jint>(slice->size());
}

/*
 *类：org-rocksdb-abstractslice
 *方法：空0
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_AbstractSlice_empty0(
    JNIEnv* env, jobject jobj, jlong handle) {
  const auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  return slice->empty();
}

/*
 *类：org-rocksdb-abstractslice
 *方法：ToString0
 *签名：（jz）ljava/lang/string；
 **/

jstring Java_org_rocksdb_AbstractSlice_toString0(
    JNIEnv* env, jobject jobj, jlong handle, jboolean hex) {
  const auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  const std::string s = slice->ToString(hex);
  return env->NewStringUTF(s.c_str());
}

/*
 *类：org-rocksdb-abstractslice
 *方法：比较0
 *签字：（JJ）I；
 **/

jint Java_org_rocksdb_AbstractSlice_compare0(
    JNIEnv* env, jobject jobj, jlong handle, jlong otherHandle) {
  const auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  const auto* otherSlice =
    reinterpret_cast<rocksdb::Slice*>(otherHandle);
  return slice->compare(*otherSlice);
}

/*
 *类：org-rocksdb-abstractslice
 *方法：StartsWith0
 *签字：（JJ）Z；
 **/

jboolean Java_org_rocksdb_AbstractSlice_startsWith0(
    JNIEnv* env, jobject jobj, jlong handle, jlong otherHandle) {
  const auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  const auto* otherSlice =
    reinterpret_cast<rocksdb::Slice*>(otherHandle);
  return slice->starts_with(*otherSlice);
}

/*
 *类：org-rocksdb-abstractslice
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_AbstractSlice_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  delete reinterpret_cast<rocksdb::Slice*>(handle);
}

//< /编辑器折叠>

//<editor fold desc=“org.rocksdb.slice>

/*
 *课程：组织摇滚乐
 *方法：CreateNewslice0
 *签名：（[BI）J
 **/

jlong Java_org_rocksdb_Slice_createNewSlice0(
    JNIEnv * env, jclass jcls, jbyteArray data, jint offset) {
  const jsize dataSize = env->GetArrayLength(data);
  const int len = dataSize - offset;

//注意：buf将在java_-org_-rocksdb_-slice_-disposeinternalbuf方法中删除。
  jbyte* buf = new jbyte[len];
  env->GetByteArrayRegion(data, offset, len, buf);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    return 0;
  }

  const auto* slice = new rocksdb::Slice((const char*)buf, len);
  return reinterpret_cast<jlong>(slice);
}

/*
 *课程：组织摇滚乐
 *方法：创建新许可证1
 *签名：（[b）J
 **/

jlong Java_org_rocksdb_Slice_createNewSlice1(
    JNIEnv * env, jclass jcls, jbyteArray data) {
  jbyte* ptrData = env->GetByteArrayElements(data, nullptr);
  if(ptrData == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }
  const int len = env->GetArrayLength(data) + 1;

//注意：buf将在java_-org_-rocksdb_-slice_-disposeinternalbuf方法中删除。
  char* buf = new char[len];
  memcpy(buf, ptrData, len - 1);
  buf[len-1] = '\0';

  const auto* slice =
      new rocksdb::Slice(buf, len - 1);

  env->ReleaseByteArrayElements(data, ptrData, JNI_ABORT);

  return reinterpret_cast<jlong>(slice);
}

/*
 *课程：组织摇滚乐
 *方法：数据0
 *签字：（j）[b
 **/

jbyteArray Java_org_rocksdb_Slice_data0(
    JNIEnv* env, jobject jobj, jlong handle) {
  const auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  const jsize len = static_cast<jsize>(slice->size());
  const jbyteArray data = env->NewByteArray(len);
  if(data == nullptr) {
//引发异常：OutOfMemoryError
    return nullptr;
  }
  
  env->SetByteArrayRegion(data, 0, len,
    const_cast<jbyte*>(reinterpret_cast<const jbyte*>(slice->data())));
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    env->DeleteLocalRef(data);
    return nullptr;
  }

  return data;
}

/*
 *课程：组织摇滚乐
 *方法：Clear0
 *签字：（JZJ）V
 **/

void Java_org_rocksdb_Slice_clear0(
    JNIEnv * env, jobject jobj, jlong handle, jboolean shouldRelease,
    jlong internalBufferOffset) {
  auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  if(shouldRelease == JNI_TRUE) {
    const char* buf = slice->data_ - internalBufferOffset;
    delete [] buf;
  }
  slice->clear();
}

/*
 *课程：组织摇滚乐
 *方法：移除Prefix0
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Slice_removePrefix0(
    JNIEnv * env, jobject jobj, jlong handle, jint length) {
  auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  slice->remove_prefix(length);
}

/*
 *课程：组织摇滚乐
 *方法：处理内部UF
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Slice_disposeInternalBuf(
    JNIEnv * env, jobject jobj, jlong handle, jlong internalBufferOffset) {
  const auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  const char* buf = slice->data_ - internalBufferOffset;
  delete [] buf;
}

//< /编辑器折叠>

//<editor fold desc=“org.rocksdb.directslice>

/*
 *课程：org_rocksdb_directslice
 *方法：createnewdirectslice0
 *签名：（ljava/nio/bytebuffer；i）j
 **/

jlong Java_org_rocksdb_DirectSlice_createNewDirectSlice0(
    JNIEnv* env, jclass jcls, jobject data, jint length) {
  assert(data != nullptr);
  void* data_addr = env->GetDirectBufferAddress(data);
  if(data_addr == nullptr) {
//错误：未定义内存区域，给定的对象不是直接的
//jvm不支持对直接缓冲区的java.nio.buffer或jni访问
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument(
            "Could not access DirectBuffer"));
    return 0;
  }

  const auto* ptrData =
     reinterpret_cast<char*>(data_addr);
  const auto* slice = new rocksdb::Slice(ptrData, length);
  return reinterpret_cast<jlong>(slice);
}

/*
 *课程：org_rocksdb_directslice
 *方法：CreateNewDirectSlice1
 *签名：（ljava/nio/bytebuffer；）j
 **/

jlong Java_org_rocksdb_DirectSlice_createNewDirectSlice1(
    JNIEnv* env, jclass jcls, jobject data) {
  void* data_addr = env->GetDirectBufferAddress(data);
  if(data_addr == nullptr) {
//错误：未定义内存区域，给定的对象不是直接的
//jvm不支持对直接缓冲区的java.nio.buffer或jni访问
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument(
            "Could not access DirectBuffer"));
    return 0;
  }

  const auto* ptrData = reinterpret_cast<char*>(data_addr);
  const auto* slice = new rocksdb::Slice(ptrData);
  return reinterpret_cast<jlong>(slice);
}

/*
 *课程：org_rocksdb_directslice
 *方法：数据0
 *签名：（j）ljava/lang/object；
 **/

jobject Java_org_rocksdb_DirectSlice_data0(
    JNIEnv* env, jobject jobj, jlong handle) {
  const auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  return env->NewDirectByteBuffer(const_cast<char*>(slice->data()),
    slice->size());
}

/*
 *课程：org_rocksdb_directslice
 *方法：Get0
 *签名：（Ji）B
 **/

jbyte Java_org_rocksdb_DirectSlice_get0(
    JNIEnv* env, jobject jobj, jlong handle, jint offset) {
  const auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  return (*slice)[offset];
}

/*
 *课程：org_rocksdb_directslice
 *方法：Clear0
 *签字：（JZJ）V
 **/

void Java_org_rocksdb_DirectSlice_clear0(
    JNIEnv* env, jobject jobj, jlong handle,
    jboolean shouldRelease, jlong internalBufferOffset) {
  auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  if(shouldRelease == JNI_TRUE) {
    const char* buf = slice->data_ - internalBufferOffset;
    delete [] buf;
  }
  slice->clear();
}

/*
 *课程：org_rocksdb_directslice
 *方法：移除Prefix0
 *签名：（Ji）V
 **/

void Java_org_rocksdb_DirectSlice_removePrefix0(
    JNIEnv* env, jobject jobj, jlong handle, jint length) {
  auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  slice->remove_prefix(length);
}

/*
 *课程：org_rocksdb_directslice
 *方法：处理内部UF
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DirectSlice_disposeInternalBuf(
    JNIEnv* env, jobject jobj, jlong handle, jlong internalBufferOffset) {
  const auto* slice = reinterpret_cast<rocksdb::Slice*>(handle);
  const char* buf = slice->data_ - internalBufferOffset;
  delete [] buf;
}

//< /编辑器折叠>
