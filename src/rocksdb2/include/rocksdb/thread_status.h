
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
//此文件定义用于公开任何
//RocksDB相关螺纹。这种运行时状态可以通过
//getThreadList（）API。
//
//请注意，所有线程状态功能仍在开发中，并且
//因此，此时API和类定义可能会发生变化。
//将在API完成后删除此注释。

#pragma once

#include <stdint.h>
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#if !defined(ROCKSDB_LITE) && \
    !defined(NROCKSDB_THREAD_STATUS) && \
    defined(ROCKSDB_SUPPORT_THREAD_LOCAL)
#define ROCKSDB_USING_THREAD_STATUS
#endif

namespace rocksdb {

//Todo:（YHCHIANG）：一旦C++ 14可用，删除此函数
//正如std：：max将能够覆盖这一点。
//当前MS编译器不支持constexpr
template <int A, int B>
struct constexpr_max {
  static const int result = (A > B) ? A : B;
};

//描述线程当前状态的结构。
//活动线程的状态可以使用
//rocksdb:：getthreadlist（）。
struct ThreadStatus {
//线程的类型。
  enum ThreadType : int {
HIGH_PRIORITY = 0,  //高优先级线程池中的rocksdb bg线程
LOW_PRIORITY,  //低优先级线程池中的rocksdb bg线程
USER,  //用户线程（非rocksdb bg线程）
    NUM_THREAD_TYPES
  };

//用于引用线程操作的类型。
//线程操作描述线程的高级操作。
//例如压实和冲洗。
  enum OperationType : int {
    OP_UNKNOWN = 0,
    OP_COMPACTION,
    OP_FLUSH,
    NUM_OP_TYPES
  };

  enum OperationStage : int {
    STAGE_UNKNOWN = 0,
    STAGE_FLUSH_RUN,
    STAGE_FLUSH_WRITE_L0,
    STAGE_COMPACTION_PREPARE,
    STAGE_COMPACTION_RUN,
    STAGE_COMPACTION_PROCESS_KV,
    STAGE_COMPACTION_INSTALL,
    STAGE_COMPACTION_SYNC_FILE,
    STAGE_PICK_MEMTABLES_TO_FLUSH,
    STAGE_MEMTABLE_ROLLBACK,
    STAGE_MEMTABLE_INSTALL_FLUSH_RESULTS,
    NUM_OP_STAGES
  };

  enum CompactionPropertyType : int {
    COMPACTION_JOB_ID = 0,
    COMPACTION_INPUT_OUTPUT_LEVEL,
    COMPACTION_PROP_FLAGS,
    COMPACTION_TOTAL_INPUT_BYTES,
    COMPACTION_BYTES_READ,
    COMPACTION_BYTES_WRITTEN,
    NUM_COMPACTION_PROPERTIES
  };

  enum FlushPropertyType : int {
    FLUSH_JOB_ID = 0,
    FLUSH_BYTES_MEMTABLES,
    FLUSH_BYTES_WRITTEN,
    NUM_FLUSH_PROPERTIES
  };

//操作的最大属性数。
//此数字应设置为最大的num_xxx_属性。
  static const int kNumOperationProperties =
      constexpr_max<NUM_COMPACTION_PROPERTIES, NUM_FLUSH_PROPERTIES>::result;

//用于引用线程状态的类型。
//状态描述线程的低级操作
//例如读/写文件或等待互斥。
  enum StateType : int {
    STATE_UNKNOWN = 0,
    STATE_MUTEX_WAIT = 1,
    NUM_STATE_TYPES
  };

  ThreadStatus(const uint64_t _id,
               const ThreadType _thread_type,
               const std::string& _db_name,
               const std::string& _cf_name,
               const OperationType _operation_type,
               const uint64_t _op_elapsed_micros,
               const OperationStage _operation_stage,
               const uint64_t _op_props[],
               const StateType _state_type) :
      thread_id(_id), thread_type(_thread_type),
      db_name(_db_name),
      cf_name(_cf_name),
      operation_type(_operation_type),
      op_elapsed_micros(_op_elapsed_micros),
      operation_stage(_operation_stage),
      state_type(_state_type) {
    for (int i = 0; i < kNumOperationProperties; ++i) {
      op_properties[i] = _op_props[i];
    }
  }

//线程的唯一ID。
  const uint64_t thread_id;

//线程的类型，它可能是高优先级的，
//低优先级和用户
  const ThreadType thread_type;

//线程当前所在的数据库实例的名称
//参与。如果线程
//不涉及任何数据库操作。
  const std::string db_name;

//线程当前所在的列族的名称
//如果线程不涉及
//在任何列族中。
  const std::string cf_name;

//当前线程所涉及的操作（高级操作）。
  const OperationType operation_type;

//当前线程操作的运行时间（以微秒为单位）。
  const uint64_t op_elapsed_micros;

//显示线程所涉及的当前阶段的整数。
//在当前操作中。
  const OperationStage operation_stage;

//描述有关当前
//操作。op_properties[]中的同一字段可能具有不同的
//不同操作的含义。
  uint64_t op_properties[kNumOperationProperties];

//当前线程所涉及的状态（低级操作）。
  const StateType state_type;

//下面是一组用于解释的实用函数
//螺纹状态信息

  static const std::string& GetThreadTypeName(ThreadType thread_type);

//获取给定类型的操作的名称。
  static const std::string& GetOperationName(OperationType op_type);

  static const std::string MicrosToString(uint64_t op_elapsed_time);

//获取描述指定操作阶段的可读字符串。
  static const std::string& GetOperationStageName(
      OperationStage stage);

//获取
//指定的操作。
  static const std::string& GetOperationPropertyName(
      OperationType op_type, int i);

//转换给定操作的“i”th属性
//属性值。
  static std::map<std::string, uint64_t>
      InterpretOperationProperties(
          OperationType op_type, const uint64_t* op_properties);

//获取给定类型的状态的名称。
  static const std::string& GetStateName(StateType state_type);
};


}  //命名空间rocksdb
