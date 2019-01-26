
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
//演示如何使用CompactFiles、EventListener和
//和getColumnFamilyMetadata API实现自定义压缩算法。

#include <mutex>
#include <string>
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"

using namespace rocksdb;
std::string kDBPath = "/tmp/rocksdb_compact_files_example";
struct CompactionTask;

//这是外部压缩算法的一个示例接口。
//压实算法可在心墙外侧实现
//通过使用RockSDB提供的可插拔压缩API进行编码。
class Compactor : public EventListener {
 public:
//根据指定的数据库选择并返回压缩任务
//和柱族。打电话的人有责任
//销毁返回的compactiontask。返回“nullptr”
//如果找不到合适的压缩任务。
  virtual CompactionTask* PickCompaction(
      DB* db, const std::string& cf_name) = 0;

//在后台计划并运行指定的压缩任务。
  virtual void ScheduleCompaction(CompactionTask *task) = 0;
};

//描述压缩任务的示例结构。
struct CompactionTask {
  CompactionTask(
      DB* _db, Compactor* _compactor,
      const std::string& _column_family_name,
      const std::vector<std::string>& _input_file_names,
      const int _output_level,
      const CompactionOptions& _compact_options,
      bool _retry_on_fail)
          : db(_db),
            compactor(_compactor),
            column_family_name(_column_family_name),
            input_file_names(_input_file_names),
            output_level(_output_level),
            compact_options(_compact_options),
            retry_on_fail(_retry_on_fail) {}
  DB* db;
  Compactor* compactor;
  const std::string& column_family_name;
  std::vector<std::string> input_file_names;
  int output_level;
  CompactionOptions compact_options;
  bool retry_on_fail;
};

//一种简单的压缩算法，它总是压缩所有内容
//尽可能达到最高水平。
class FullCompactor : public Compactor {
 public:
  explicit FullCompactor(const Options options) : options_(options) {
    compact_options_.compression = options_.compression;
    compact_options_.output_file_size_limit =
        options_.target_file_size_base;
  }

//当刷新发生时，它决定是否触发压缩。如果
//触发的“写入”停止为真，它还将设置的重试标志
//将任务压缩为真。
  void OnFlushCompleted(
      DB* db, const FlushJobInfo& info) override {
    CompactionTask* task = PickCompaction(db, info.cf_name);
    if (task != nullptr) {
      if (info.triggered_writes_stop) {
        task->retry_on_fail = true;
      }
//在不同的线程中调度压缩。
      ScheduleCompaction(task);
    }
  }

//尽可能选择包含所有文件的压缩。
  CompactionTask* PickCompaction(
      DB* db, const std::string& cf_name) override {
    ColumnFamilyMetaData cf_meta;
    db->GetColumnFamilyMetaData(&cf_meta);

    std::vector<std::string> input_file_names;
    for (auto level : cf_meta.levels) {
      for (auto file : level.files) {
        if (file.being_compacted) {
          return nullptr;
        }
        input_file_names.push_back(file.name);
      }
    }
    return new CompactionTask(
        db, this, cf_name, input_file_names,
        options_.num_levels - 1, compact_options_, false);
  }

//在后台计划指定的压缩任务。
  void ScheduleCompaction(CompactionTask* task) override {
    options_.env->Schedule(&FullCompactor::CompactFiles, task);
  }

  static void CompactFiles(void* arg) {
    std::unique_ptr<CompactionTask> task(
        reinterpret_cast<CompactionTask*>(arg));
    assert(task);
    assert(task->db);
    Status s = task->db->CompactFiles(
        task->compact_options,
        task->input_file_names,
        task->output_level);
    printf("CompactFiles() finished with status %s\n", s.ToString().c_str());
    if (!s.ok() && !s.IsIOError() && task->retry_on_fail) {
//如果压缩任务的重试失败=真失败，
//如果原因是
//不是IO错误。
      CompactionTask* new_task = task->compactor->PickCompaction(
          task->db, task->column_family_name);
      task->compactor->ScheduleCompaction(new_task);
    }
  }

 private:
  Options options_;
  CompactionOptions compact_options_;
};

int main() {
  Options options;
  options.create_if_missing = true;
//禁用rocksdb背景压缩。
  options.compaction_style = kCompactionStyleNone;
//实验用小型减速和停止触发器。
  options.level0_slowdown_writes_trigger = 3;
  options.level0_stop_writes_trigger = 5;
  options.IncreaseParallelism(5);
  options.listeners.emplace_back(new FullCompactor(options));

  DB* db = nullptr;
  DestroyDB(kDBPath, options);
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());
  assert(db);

//如果后台压缩不起作用，则写入将停止
//因为options.level0_stop_writes_trigger
  for (int i = 1000; i < 99999; ++i) {
    db->Put(WriteOptions(), std::to_string(i),
                            std::string(500, 'a' + (i % 26)));
  }

//验证值是否仍然存在
  std::string value;
  for (int i = 1000; i < 99999; ++i) {
    db->Get(ReadOptions(), std::to_string(i),
                           &value);
    assert(value == std::string(500, 'a' + (i % 26)));
  }

//关闭数据库。
  delete db;

  return 0;
}
