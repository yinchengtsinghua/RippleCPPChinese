
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
//此文件定义线程操作和状态的结构。
//线程操作用于描述
//在线程状态下执行压缩或刷新等线程
//用于描述较低级别的操作，如阅读/
//正在写入文件或等待互斥。操作和状态
//设计为独立的。通常，线程通常涉及
//在一个操作和任何特定时间点的一个状态下。

#pragma once

#include "rocksdb/thread_status.h"

#include <string>

namespace rocksdb {

#ifdef ROCKSDB_USING_THREAD_STATUS

//描述主线程操作的结构。
struct OperationInfo {
  const ThreadStatus::OperationType type;
  const std::string name;
};

//全局操作表。
//
//更新线程状态时，operationinfo的指针
//当前线程的状态数据将指向
//此全局表中的行。
//
//注意，它的设计并不像未来我们
//可以考虑将全局计数添加到operationinfo。
static OperationInfo global_operation_table[] = {
  {ThreadStatus::OP_UNKNOWN, ""},
  {ThreadStatus::OP_COMPACTION, "Compaction"},
  {ThreadStatus::OP_FLUSH, "Flush"}
};

struct OperationStageInfo {
  const ThreadStatus::OperationStage stage;
  const std::string name;
};

//表维护从阶段类型到阶段字符串的映射。
//注意，当
//关联的函数名已更改。
static OperationStageInfo global_op_stage_table[] = {
  {ThreadStatus::STAGE_UNKNOWN, ""},
  {ThreadStatus::STAGE_FLUSH_RUN,
      "FlushJob::Run"},
  {ThreadStatus::STAGE_FLUSH_WRITE_L0,
      "FlushJob::WriteLevel0Table"},
  {ThreadStatus::STAGE_COMPACTION_PREPARE,
      "CompactionJob::Prepare"},
  {ThreadStatus::STAGE_COMPACTION_RUN,
      "CompactionJob::Run"},
  {ThreadStatus::STAGE_COMPACTION_PROCESS_KV,
      "CompactionJob::ProcessKeyValueCompaction"},
  {ThreadStatus::STAGE_COMPACTION_INSTALL,
      "CompactionJob::Install"},
  {ThreadStatus::STAGE_COMPACTION_SYNC_FILE,
      "CompactionJob::FinishCompactionOutputFile"},
  {ThreadStatus::STAGE_PICK_MEMTABLES_TO_FLUSH,
      "MemTableList::PickMemtablesToFlush"},
  {ThreadStatus::STAGE_MEMTABLE_ROLLBACK,
      "MemTableList::RollbackMemtableFlush"},
  {ThreadStatus::STAGE_MEMTABLE_INSTALL_FLUSH_RESULTS,
      "MemTableList::InstallMemtableFlushResults"},
};

//描述状态的结构。
struct StateInfo {
  const ThreadStatus::StateType type;
  const std::string name;
};

//全局状态表。
//
//更新线程状态时，stateinfo的指针
//当前线程的状态数据将指向
//此全局表中的行。
static StateInfo global_state_table[] = {
  {ThreadStatus::STATE_UNKNOWN, ""},
  {ThreadStatus::STATE_MUTEX_WAIT, "Mutex Wait"},
};

struct OperationProperty {
  int code;
  std::string name;
};

static OperationProperty compaction_operation_properties[] = {
  {ThreadStatus::COMPACTION_JOB_ID, "JobID"},
  {ThreadStatus::COMPACTION_INPUT_OUTPUT_LEVEL, "InputOutputLevel"},
  {ThreadStatus::COMPACTION_PROP_FLAGS, "Manual/Deletion/Trivial"},
  {ThreadStatus::COMPACTION_TOTAL_INPUT_BYTES, "TotalInputBytes"},
  {ThreadStatus::COMPACTION_BYTES_READ, "BytesRead"},
  {ThreadStatus::COMPACTION_BYTES_WRITTEN, "BytesWritten"},
};

static OperationProperty flush_operation_properties[] = {
  {ThreadStatus::FLUSH_JOB_ID, "JobID"},
  {ThreadStatus::FLUSH_BYTES_MEMTABLES, "BytesMemtables"},
  {ThreadStatus::FLUSH_BYTES_WRITTEN, "BytesWritten"}
};

#else

struct OperationInfo {
};

struct StateInfo {
};

#endif  //使用线程状态的rocksdb_
}  //命名空间rocksdb
