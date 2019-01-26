
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

#include <jni.h>

#include "include/org_rocksdb_RocksDBExceptionTest.h"

#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksjni/portal.h"

/*
 *等级：组织摇滚乐测试
 *方法：RaiseException
 *签名：（）V
 **/

void Java_org_rocksdb_RocksDBExceptionTest_raiseException(JNIEnv* env,
                                                          jobject jobj) {
  rocksdb::RocksDBExceptionJni::ThrowNew(env, std::string("test message"));
}

/*
 *等级：组织摇滚乐测试
 *方法：RaiseExceptionWithStatusCode
 *签名：（）V
 **/

void Java_org_rocksdb_RocksDBExceptionTest_raiseExceptionWithStatusCode(
    JNIEnv* env, jobject jobj) {
  rocksdb::RocksDBExceptionJni::ThrowNew(env, "test message",
                                         rocksdb::Status::NotSupported());
}

/*
 *等级：组织摇滚乐测试
 *方法：raiseExceptionNomsgWithStatusCode
 *签名：（）V
 **/

void Java_org_rocksdb_RocksDBExceptionTest_raiseExceptionNoMsgWithStatusCode(
    JNIEnv* env, jobject jobj) {
  rocksdb::RocksDBExceptionJni::ThrowNew(env, rocksdb::Status::NotSupported());
}

/*
 *等级：组织摇滚乐测试
 *方法：RaiseExceptionWithStatusCoddoBCode
 *签名：（）V
 **/

void Java_org_rocksdb_RocksDBExceptionTest_raiseExceptionWithStatusCodeSubCode(
    JNIEnv* env, jobject jobj) {
  rocksdb::RocksDBExceptionJni::ThrowNew(
      env, "test message",
      rocksdb::Status::TimedOut(rocksdb::Status::SubCode::kLockTimeout));
}

/*
 *等级：组织摇滚乐测试
 *方法：raiseexceptionnomsgwithstatuscoddobcode
 *签名：（）V
 **/

void Java_org_rocksdb_RocksDBExceptionTest_raiseExceptionNoMsgWithStatusCodeSubCode(
    JNIEnv* env, jobject jobj) {
  rocksdb::RocksDBExceptionJni::ThrowNew(
      env, rocksdb::Status::TimedOut(rocksdb::Status::SubCode::kLockTimeout));
}

/*
 *等级：组织摇滚乐测试
 *方法：RaiseExceptionWithStatusDestate
 *签名：（）V
 **/

void Java_org_rocksdb_RocksDBExceptionTest_raiseExceptionWithStatusCodeState(
    JNIEnv* env, jobject jobj) {
  rocksdb::Slice state("test state");
  rocksdb::RocksDBExceptionJni::ThrowNew(env, "test message",
                                         rocksdb::Status::NotSupported(state));
}
