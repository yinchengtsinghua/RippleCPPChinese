
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

#ifndef ROCKSDB_LITE
#ifndef GFLAGS
#include <cstdio>
int main() {
  fprintf(stderr, "Please install gflags to run rocksdb tools\n");
  return 1;
}
#else

#include <cstdio>
#include <atomic>

#include <gflags/gflags.h>

#include "db/write_batch_internal.h"
#include "rocksdb/db.h"
#include "rocksdb/types.h"
#include "util/testutil.h"

//运行线程执行Put。
//另一个线程使用GetUpdateSince API继续获取更新。
//选项：
//--num_inserts=第一个线程应执行的插入数。
//--wal_ttl=运行的wal ttl。

using namespace rocksdb;

using GFLAGS::ParseCommandLineFlags;
using GFLAGS::SetUsageMessage;

struct DataPumpThread {
  size_t no_records;
DB* db; //假设数据库已打开。
};

static std::string RandomString(Random* rnd, int len) {
  std::string r;
  test::RandomString(rnd, len, &r);
  return r;
}

static void DataPumpThreadBody(void* arg) {
  DataPumpThread* t = reinterpret_cast<DataPumpThread*>(arg);
  DB* db = t->db;
  Random rnd(301);
  size_t i = 0;
  while(i++ < t->no_records) {
    if(!db->Put(WriteOptions(), Slice(RandomString(&rnd, 500)),
                Slice(RandomString(&rnd, 500))).ok()) {
      fprintf(stderr, "Error in put\n");
      exit(1);
    }
  }
}

struct ReplicationThread {
  std::atomic<bool> stop;
  DB* db;
  volatile size_t no_read;
};

static void ReplicationThreadBody(void* arg) {
  ReplicationThread* t = reinterpret_cast<ReplicationThread*>(arg);
  DB* db = t->db;
  unique_ptr<TransactionLogIterator> iter;
  SequenceNumber currentSeqNum = 1;
  while (!t->stop.load(std::memory_order_acquire)) {
    iter.reset();
    Status s;
    while(!db->GetUpdatesSince(currentSeqNum, &iter).ok()) {
      if (t->stop.load(std::memory_order_acquire)) {
        return;
      }
    }
    fprintf(stderr, "Refreshing iterator\n");
    for(;iter->Valid(); iter->Next(), t->no_read++, currentSeqNum++) {
      BatchResult res = iter->GetBatch();
      if (res.sequence != currentSeqNum) {
        fprintf(stderr,
                "Missed a seq no. b/w %ld and %ld\n",
                (long)currentSeqNum,
                (long)res.sequence);
        exit(1);
      }
    }
  }
}

DEFINE_uint64(num_inserts, 1000, "the num of inserts the first thread should"
              " perform.");
DEFINE_uint64(wal_ttl_seconds, 1000, "the wal ttl for the run(in seconds)");
DEFINE_uint64(wal_size_limit_MB, 10, "the wal size limit for the run"
              "(in MB)");

int main(int argc, const char** argv) {
  SetUsageMessage(
      std::string("\nUSAGE:\n") + std::string(argv[0]) +
      " --num_inserts=<num_inserts> --wal_ttl_seconds=<WAL_ttl_seconds>" +
      " --wal_size_limit_MB=<WAL_size_limit_MB>");
  ParseCommandLineFlags(&argc, const_cast<char***>(&argv), true);

  Env* env = Env::Default();
  std::string default_db_path;
  env->GetTestDirectory(&default_db_path);
  default_db_path += "db_repl_stress";
  Options options;
  options.create_if_missing = true;
  options.WAL_ttl_seconds = FLAGS_wal_ttl_seconds;
  options.WAL_size_limit_MB = FLAGS_wal_size_limit_MB;
  DB* db;
  DestroyDB(default_db_path, options);

  Status s = DB::Open(options, default_db_path, &db);

  if (!s.ok()) {
    fprintf(stderr, "Could not open DB due to %s\n", s.ToString().c_str());
    exit(1);
  }

  DataPumpThread dataPump;
  dataPump.no_records = FLAGS_num_inserts;
  dataPump.db = db;
  env->StartThread(DataPumpThreadBody, &dataPump);

  ReplicationThread replThread;
  replThread.db = db;
  replThread.no_read = 0;
  replThread.stop.store(false, std::memory_order_release);

  env->StartThread(ReplicationThreadBody, &replThread);
  while(replThread.no_read < FLAGS_num_inserts);
  replThread.stop.store(true, std::memory_order_release);
  if (replThread.no_read < dataPump.no_records) {
//不。读取应=>插入。
    fprintf(stderr,
            "No. of Record's written and read not same\nRead : %" ROCKSDB_PRIszt
            " Written : %" ROCKSDB_PRIszt "\n",
            replThread.no_read, dataPump.no_records);
    exit(1);
  }
  fprintf(stderr, "Successful!\n");
  exit(0);
}

#endif  //GFLAGS

#else  //摇滚乐
#include <stdio.h>
int main(int argc, char** argv) {
  fprintf(stderr, "Not supported in lite mode.\n");
  return 1;
}
#endif  //摇滚乐
