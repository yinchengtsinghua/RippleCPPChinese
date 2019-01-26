
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

#ifndef GFLAGS
#include <cstdio>
int main() {
  fprintf(stderr, "Please install gflags to run rocksdb tools\n");
  return 1;
}
#else

#include <gflags/gflags.h>

#include "db/db_impl.h"
#include "db/dbformat.h"
#include "monitoring/histogram.h"
#include "rocksdb/db.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/table.h"
#include "table/block_based_table_factory.h"
#include "table/get_context.h"
#include "table/internal_iterator.h"
#include "table/plain_table_factory.h"
#include "table/table_builder.h"
#include "util/file_reader_writer.h"
#include "util/testharness.h"
#include "util/testutil.h"

using GFLAGS::ParseCommandLineFlags;
using GFLAGS::SetUsageMessage;

namespace rocksdb {

namespace {
//生成一个键，我确定前4个字符，j确定
//最后4个字符。
static std::string MakeKey(int i, int j, bool through_db) {
  char buf[100];
  snprintf(buf, sizeof(buf), "%04d__key___%04d", i, j);
  if (through_db) {
    return std::string(buf);
  }
//如果我们直接查询表，它对内部键进行操作
//我们需要添加8个字节的内部
//将信息（行类型等）发送给用户密钥以生成内部
//关键。
  InternalKey key(std::string(buf), 0, ValueType::kTypeValue);
  return key.Encode().ToString();
}

uint64_t Now(Env* env, bool measured_by_nanosecond) {
  return measured_by_nanosecond ? env->NowNanos() : env->NowMicros();
}
}  //命名空间

//一个非常简单的基准。
//创建一个带有大致numkey1*numkey2键的表，
//如果键有numkey1前缀，则每个前缀都有numkey2编号
//可分辨键，后缀部分不同。
//如果_query_empty_keys=false，则查询现有的keys numkey1*numkey2
//时间随机。
//如果_query_empty_keys=true，则查询numkey1*numkey2随机空键。
//打印出总时间。
//如果通过“db=true”，将创建一个完整的db，查询将针对
//它。否则，操作将直接通过表级别进行。
//
//如果for terator=true，那么它不会每次只查询一个键，而是查询
//具有相同前缀的范围。
namespace {
void TableReaderBenchmark(Options& opts, EnvOptions& env_options,
                          ReadOptions& read_options, int num_keys1,
                          int num_keys2, int num_iter, int prefix_len,
                          bool if_query_empty_keys, bool for_iterator,
                          bool through_db, bool measured_by_nanosecond) {
  rocksdb::InternalKeyComparator ikc(opts.comparator);

  std::string file_name = test::TmpDir()
      + "/rocksdb_table_reader_benchmark";
  std::string dbname = test::TmpDir() + "/rocksdb_table_reader_bench_db";
  WriteOptions wo;
  Env* env = Env::Default();
  TableBuilder* tb = nullptr;
  DB* db = nullptr;
  Status s;
  const ImmutableCFOptions ioptions(opts);
  unique_ptr<WritableFileWriter> file_writer;
  if (!through_db) {
    unique_ptr<WritableFile> file;
    env->NewWritableFile(file_name, &file, env_options);

    std::vector<std::unique_ptr<IntTblPropCollectorFactory> >
        int_tbl_prop_collector_factories;

    file_writer.reset(new WritableFileWriter(std::move(file), env_options));
    int unknown_level = -1;
    tb = opts.table_factory->NewTableBuilder(
        TableBuilderOptions(ioptions, ikc, &int_tbl_prop_collector_factories,
                            CompressionType::kNoCompression,
                            CompressionOptions(),
                            /*lptr/*压缩\u dict*/，
                            假/*跳过过滤器*/, kDefaultColumnFamilyName,

                            unknown_level),
        /**列_family_id*/，file_writer.get（））；
  }否则{
    s=db:：open（opts，dbname，&db）；
    SalpTyk OK（s）；
    断言“真”（db！= null pTr）；
  }
  //填充略多于1米的键
  对于（int i=0；i<num_keys1；i++）
    对于（int j=0；j<num_keys2；j++）
      std:：string key=makekey（i*2，j，通过_db）；
      如果（！）通过x dB）{
        tb->add（键，键）；
      }否则{
        db->put（wo，key，key）；
      }
    }
  }
  如果（！）通过x dB）{
    TB-> FIX（）；
    文件_writer->close（）；
  }否则{
    db->flush（flushoptings（））；
  }

  唯一的读卡器；
  如果（！）通过x dB）{
    唯一访问文件>raf；
    s=env->newrandomaccessfile（文件名，&raf，env_选项）；
    如果（！）S.O.（））{
      fprintf（stderr，“创建文件错误：%s\n”，s.toString（）.c_str（））；
      退出（1）；
    }
    uint64_t文件大小；
    env->getfilesize（文件名和文件大小）；
    唯一的读卡器
        新建RandomAccessFileReader（std:：move（raf），文件名））；
    s=opts.table_factory->newTableReader（
        TableReaderOptions（IOOptions、env_Options、ikc）、Std:：Move（File_Reader），以及
        文件大小和表读卡器；
    如果（！）S.O.（））{
      fprintf（stderr，“打开表错误：%s\n”，s.toString（）.c_str（））；
      退出（1）；
    }
  }

  随机RND（301）；
  std：：字符串结果；
  HistogramImpl历史；

  for（int it=0；it<numiter；it++）
    对于（int i=0；i<num_keys1；i++）
      对于（int j=0；j<num_keys2；j++）
        int r1=rnd.uniform（num_keys1）*2；
        int r2=RND.均匀（num_keys2）；
        if（if_query_empty_keys）
          R1++；
          r2=数字键2*2-r2；
        }

        如果（！）F-迭代器）{
          //查询一个已有的键；
          std:：string key=makekey（r1，r2，通过_db）；
          uint64_t start_time=now（env，以_纳秒为单位测量）；
          如果（！）通过x dB）{
            pinnableslice值；
            合并上下文合并上下文；
            rangedelaggregator range_del_agg（ikc，/*快照*/);

            GetContext get_context(ioptions.user_comparator,
                                   ioptions.merge_operator, ioptions.info_log,
                                   ioptions.statistics, GetContext::kNotFound,
                                   Slice(key), &value, nullptr, &merge_context,
                                   &range_del_agg, env);
            s = table_reader->Get(read_options, key, &get_context);
          } else {
            s = db->Get(read_options, key, &result);
          }
          hist.Add(Now(env, measured_by_nanosecond) - start_time);
        } else {
          int r2_len;
          if (if_query_empty_keys) {
            r2_len = 0;
          } else {
            r2_len = rnd.Uniform(num_keys2) + 1;
            if (r2_len + r2 > num_keys2) {
              r2_len = num_keys2 - r2;
            }
          }
          std::string start_key = MakeKey(r1, r2, through_db);
          std::string end_key = MakeKey(r1, r2 + r2_len, through_db);
          uint64_t total_time = 0;
          uint64_t start_time = Now(env, measured_by_nanosecond);
          Iterator* iter = nullptr;
          InternalIterator* iiter = nullptr;
          if (!through_db) {
            iiter = table_reader->NewIterator(read_options);
          } else {
            iter = db->NewIterator(read_options);
          }
          int count = 0;
          for (through_db ? iter->Seek(start_key) : iiter->Seek(start_key);
               through_db ? iter->Valid() : iiter->Valid();
               through_db ? iter->Next() : iiter->Next()) {
            if (if_query_empty_keys) {
              break;
            }
//验证密钥；
            total_time += Now(env, measured_by_nanosecond) - start_time;
            assert(Slice(MakeKey(r1, r2 + count, through_db)) ==
                   (through_db ? iter->key() : iiter->key()));
            start_time = Now(env, measured_by_nanosecond);
            if (++count >= r2_len) {
              break;
            }
          }
          if (count != r2_len) {
            fprintf(
                stderr, "Iterator cannot iterate expected number of entries. "
                "Expected %d but got %d\n", r2_len, count);
            assert(false);
          }
          delete iter;
          total_time += Now(env, measured_by_nanosecond) - start_time;
          hist.Add(total_time);
        }
      }
    }
  }

  fprintf(
      stderr,
      "==================================================="
      "====================================================\n"
      "InMemoryTableSimpleBenchmark: %20s   num_key1:  %5d   "
      "num_key2: %5d  %10s\n"
      "==================================================="
      "===================================================="
      "\nHistogram (unit: %s): \n%s",
      opts.table_factory->Name(), num_keys1, num_keys2,
      for_iterator ? "iterator" : (if_query_empty_keys ? "empty" : "non_empty"),
      measured_by_nanosecond ? "nanosecond" : "microsecond",
      hist.ToString().c_str());
  if (!through_db) {
    env->DeleteFile(file_name);
  } else {
    delete db;
    db = nullptr;
    DestroyDB(dbname, opts);
  }
}
}  //命名空间
}  //命名空间rocksdb

DEFINE_bool(query_empty, false, "query non-existing keys instead of existing "
            "ones.");
DEFINE_int32(num_keys1, 4096, "number of distinguish prefix of keys");
DEFINE_int32(num_keys2, 512, "number of distinguish keys for each prefix");
DEFINE_int32(iter, 3, "query non-existing keys instead of existing ones");
DEFINE_int32(prefix_len, 16, "Prefix length used for iterators and indexes");
DEFINE_bool(iterator, false, "For test iterator");
DEFINE_bool(through_db, false, "If enable, a DB instance will be created and "
            "the query will be against DB. Otherwise, will be directly against "
            "a table reader.");
DEFINE_bool(mmap_read, true, "Whether use mmap read");
DEFINE_string(table_factory, "block_based",
              "Table factory to use: `block_based` (default), `plain_table` or "
              "`cuckoo_hash`.");
DEFINE_string(time_unit, "microsecond",
              "The time unit used for measuring performance. User can specify "
              "`microsecond` (default) or `nanosecond`");

int main(int argc, char** argv) {
  SetUsageMessage(std::string("\nUSAGE:\n") + std::string(argv[0]) +
                  " [OPTIONS]...");
  ParseCommandLineFlags(&argc, &argv, true);

  std::shared_ptr<rocksdb::TableFactory> tf;
  rocksdb::Options options;
  if (FLAGS_prefix_len < 16) {
    options.prefix_extractor.reset(rocksdb::NewFixedPrefixTransform(
        FLAGS_prefix_len));
  }
  rocksdb::ReadOptions ro;
  rocksdb::EnvOptions env_options;
  options.create_if_missing = true;
  options.compression = rocksdb::CompressionType::kNoCompression;

  if (FLAGS_table_factory == "cuckoo_hash") {
#ifndef ROCKSDB_LITE
    options.allow_mmap_reads = FLAGS_mmap_read;
    env_options.use_mmap_reads = FLAGS_mmap_read;
    rocksdb::CuckooTableOptions table_options;
    table_options.hash_table_ratio = 0.75;
    tf.reset(rocksdb::NewCuckooTableFactory(table_options));
#else
    fprintf(stderr, "Plain table is not supported in lite mode\n");
    exit(1);
#endif  //摇滚乐
  } else if (FLAGS_table_factory == "plain_table") {
#ifndef ROCKSDB_LITE
    options.allow_mmap_reads = FLAGS_mmap_read;
    env_options.use_mmap_reads = FLAGS_mmap_read;

    rocksdb::PlainTableOptions plain_table_options;
    plain_table_options.user_key_len = 16;
    plain_table_options.bloom_bits_per_key = (FLAGS_prefix_len == 16) ? 0 : 8;
    plain_table_options.hash_table_ratio = 0.75;

    tf.reset(new rocksdb::PlainTableFactory(plain_table_options));
    options.prefix_extractor.reset(rocksdb::NewFixedPrefixTransform(
        FLAGS_prefix_len));
#else
    fprintf(stderr, "Cuckoo table is not supported in lite mode\n");
    exit(1);
#endif  //摇滚乐
  } else if (FLAGS_table_factory == "block_based") {
    tf.reset(new rocksdb::BlockBasedTableFactory());
  } else {
    fprintf(stderr, "Invalid table type %s\n", FLAGS_table_factory.c_str());
  }

  if (tf) {
//如果用户提供的选项无效，只需返回到微秒。
    bool measured_by_nanosecond = FLAGS_time_unit == "nanosecond";

    options.table_factory = tf;
    rocksdb::TableReaderBenchmark(options, env_options, ro, FLAGS_num_keys1,
                                  FLAGS_num_keys2, FLAGS_iter, FLAGS_prefix_len,
                                  FLAGS_query_empty, FLAGS_iterator,
                                  FLAGS_through_db, measured_by_nanosecond);
  } else {
    return 1;
  }

  return 0;
}

#endif  //GFLAGS
