
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
//该文件实现Java和C++之间的“桥接”并启用
//调用C++ ROCKSDB:：从Java方面统计方法。

#include <jni.h>
#include <memory>
#include <set>

#include "include/org_rocksdb_Statistics.h"
#include "rocksjni/portal.h"
#include "rocksjni/statisticsjni.h"
#include "rocksdb/statistics.h"

/*
 *类别：Org_RocksDB_Statistics
 *方法：新闻统计
 *签字：（）J
 **/

jlong Java_org_rocksdb_Statistics_newStatistics__(JNIEnv* env, jclass jcls) {
  return Java_org_rocksdb_Statistics_newStatistics___3BJ(
      env, jcls, nullptr, 0);
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：新闻统计
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Statistics_newStatistics__J(
    JNIEnv* env, jclass jcls, jlong jother_statistics_handle) {
  return Java_org_rocksdb_Statistics_newStatistics___3BJ(
      env, jcls, nullptr, jother_statistics_handle);
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：新闻统计
 *签名：（[b）J
 **/

jlong Java_org_rocksdb_Statistics_newStatistics___3B(
    JNIEnv* env, jclass jcls, jbyteArray jhistograms) {
  return Java_org_rocksdb_Statistics_newStatistics___3BJ(
      env, jcls, jhistograms, 0);
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：新闻统计
 *签字：（[BJ）J
 **/

jlong Java_org_rocksdb_Statistics_newStatistics___3BJ(
    JNIEnv* env, jclass jcls, jbyteArray jhistograms,
    jlong jother_statistics_handle) {

  std::shared_ptr<rocksdb::Statistics>* pSptr_other_statistics = nullptr;
  if (jother_statistics_handle > 0) {
    pSptr_other_statistics =
        reinterpret_cast<std::shared_ptr<rocksdb::Statistics>*>(
            jother_statistics_handle);
  }

  std::set<uint32_t> histograms;
  if (jhistograms != nullptr) {
    const jsize len = env->GetArrayLength(jhistograms);
    if (len > 0) {
      jbyte* jhistogram = env->GetByteArrayElements(jhistograms, nullptr);
      if (jhistogram == nullptr ) {
//引发异常：OutOfMemoryError
        return 0;
      }

      for (jsize i = 0; i < len; i++) {
        const rocksdb::Histograms histogram =
            rocksdb::HistogramTypeJni::toCppHistograms(jhistogram[i]);
        histograms.emplace(histogram);
      }

      env->ReleaseByteArrayElements(jhistograms, jhistogram, JNI_ABORT);
    }
  }

  std::shared_ptr<rocksdb::Statistics> sptr_other_statistics = nullptr;
  if (pSptr_other_statistics != nullptr) {
      sptr_other_statistics =   *pSptr_other_statistics;
  }

  auto* pSptr_statistics = new std::shared_ptr<rocksdb::StatisticsJni>(
      new rocksdb::StatisticsJni(sptr_other_statistics, histograms));

  return reinterpret_cast<jlong>(pSptr_statistics);
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_Statistics_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  if(jhandle > 0) {
    auto* pSptr_statistics =
        reinterpret_cast<std::shared_ptr<rocksdb::Statistics>*>(jhandle);
    delete pSptr_statistics;
  }
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：StatsLevel
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_Statistics_statsLevel(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* pSptr_statistics =
      reinterpret_cast<std::shared_ptr<rocksdb::Statistics>*>(jhandle);
  assert(pSptr_statistics != nullptr);
  return rocksdb::StatsLevelJni::toJavaStatsLevel(pSptr_statistics->get()->stats_level_);
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：设置状态级别
 *签字：（JB）V
 **/

void Java_org_rocksdb_Statistics_setStatsLevel(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jstats_level) {
  auto* pSptr_statistics =
      reinterpret_cast<std::shared_ptr<rocksdb::Statistics>*>(jhandle);
  assert(pSptr_statistics != nullptr);
  auto stats_level = rocksdb::StatsLevelJni::toCppStatsLevel(jstats_level);
  pSptr_statistics->get()->stats_level_ = stats_level;
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：gettickercount
 *签字：（JB）J
 **/

jlong Java_org_rocksdb_Statistics_getTickerCount(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jticker_type) {
  auto* pSptr_statistics =
      reinterpret_cast<std::shared_ptr<rocksdb::Statistics>*>(jhandle);
  assert(pSptr_statistics != nullptr);
  auto ticker = rocksdb::TickerTypeJni::toCppTickers(jticker_type);
  return pSptr_statistics->get()->getTickerCount(ticker);
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：getandresetickercount
 *签字：（JB）J
 **/

jlong Java_org_rocksdb_Statistics_getAndResetTickerCount(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jticker_type) {
  auto* pSptr_statistics =
      reinterpret_cast<std::shared_ptr<rocksdb::Statistics>*>(jhandle);
  assert(pSptr_statistics != nullptr);
  auto ticker = rocksdb::TickerTypeJni::toCppTickers(jticker_type);
  return pSptr_statistics->get()->getAndResetTickerCount(ticker);
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：获取柱状图数据
 *签名：（jb）lorg/rocksdb/histogramdata；
 **/

jobject Java_org_rocksdb_Statistics_getHistogramData(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jhistogram_type) {
  auto* pSptr_statistics =
      reinterpret_cast<std::shared_ptr<rocksdb::Statistics>*>(jhandle);
  assert(pSptr_statistics != nullptr);

rocksdb::HistogramData data;  //ToDO（AR）可能更好地构造一个使用PtR到C++新的GracReDATA的Java对象包装器
  auto histogram = rocksdb::HistogramTypeJni::toCppHistograms(jhistogram_type);
  pSptr_statistics->get()->histogramData(
      static_cast<rocksdb::Histograms>(histogram), &data);

  jclass jclazz = rocksdb::HistogramDataJni::getJClass(env);
  if(jclazz == nullptr) {
//访问类时发生异常
    return nullptr;
  }

  jmethodID mid = rocksdb::HistogramDataJni::getConstructorMethodId(
      env);
  if(mid == nullptr) {
//访问方法时发生异常
    return nullptr;
  }

  return env->NewObject(
      jclazz,
      mid, data.median, data.percentile95,data.percentile99, data.average,
      data.standard_deviation);
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：GetHistogramString
 *签名：（jb）ljava/lang/string；
 **/

jstring Java_org_rocksdb_Statistics_getHistogramString(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jhistogram_type) {
  auto* pSptr_statistics =
      reinterpret_cast<std::shared_ptr<rocksdb::Statistics>*>(jhandle);
  assert(pSptr_statistics != nullptr);
  auto histogram = rocksdb::HistogramTypeJni::toCppHistograms(jhistogram_type);
  auto str = pSptr_statistics->get()->getHistogramString(histogram);
  return env->NewStringUTF(str.c_str());
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：重置
 *签名：（j）v
 **/

void Java_org_rocksdb_Statistics_reset(
    JNIEnv* env, jobject jobj, jlong jhandle) {
   auto* pSptr_statistics =
      reinterpret_cast<std::shared_ptr<rocksdb::Statistics>*>(jhandle);
  assert(pSptr_statistics != nullptr);
  rocksdb::Status s = pSptr_statistics->get()->Reset();
  if (!s.ok()) {
   rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：Org_RocksDB_Statistics
 *方法：ToString
 *签名：（j）ljava/lang/string；
 **/

jstring Java_org_rocksdb_Statistics_toString(
    JNIEnv* env, jobject jobj, jlong jhandle) {
   auto* pSptr_statistics =
      reinterpret_cast<std::shared_ptr<rocksdb::Statistics>*>(jhandle);
  assert(pSptr_statistics != nullptr);
  auto str = pSptr_statistics->get()->ToString();
  return env->NewStringUTF(str.c_str());
}
