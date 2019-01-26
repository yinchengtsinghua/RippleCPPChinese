
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

#include <sstream>

#include "rocksdb/env.h"
#include "rocksdb/thread_status.h"
#include "util/string_util.h"
#include "util/thread_operation.h"

namespace rocksdb {

#ifdef ROCKSDB_USING_THREAD_STATUS
const std::string& ThreadStatus::GetThreadTypeName(
    ThreadStatus::ThreadType thread_type) {
  static std::string thread_type_names[NUM_THREAD_TYPES + 1] = {
      "High Pri", "Low Pri", "User", "Unknown"};
  if (thread_type < 0 || thread_type >= NUM_THREAD_TYPES) {
return thread_type_names[NUM_THREAD_TYPES];  //“未知”
  }
  return thread_type_names[thread_type];
}

const std::string& ThreadStatus::GetOperationName(
    ThreadStatus::OperationType op_type) {
  if (op_type < 0 || op_type >= NUM_OP_TYPES) {
    return global_operation_table[OP_UNKNOWN].name;
  }
  return global_operation_table[op_type].name;
}

const std::string& ThreadStatus::GetOperationStageName(
    ThreadStatus::OperationStage stage) {
  if (stage < 0 || stage >= NUM_OP_STAGES) {
    return global_op_stage_table[STAGE_UNKNOWN].name;
  }
  return global_op_stage_table[stage].name;
}

const std::string& ThreadStatus::GetStateName(
    ThreadStatus::StateType state_type) {
  if (state_type < 0 || state_type >= NUM_STATE_TYPES) {
    return global_state_table[STATE_UNKNOWN].name;
  }
  return global_state_table[state_type].name;
}

const std::string ThreadStatus::MicrosToString(uint64_t micros) {
  if (micros == 0) {
    return "";
  }
  const int kBufferLen = 100;
  char buffer[kBufferLen];
  AppendHumanMicros(micros, buffer, kBufferLen, false);
  return std::string(buffer);
}

const std::string& ThreadStatus::GetOperationPropertyName(
    ThreadStatus::OperationType op_type, int i) {
  static const std::string empty_str = "";
  switch (op_type) {
    case ThreadStatus::OP_COMPACTION:
      if (i >= NUM_COMPACTION_PROPERTIES) {
        return empty_str;
      }
      return compaction_operation_properties[i].name;
    case ThreadStatus::OP_FLUSH:
      if (i >= NUM_FLUSH_PROPERTIES) {
        return empty_str;
      }
      return flush_operation_properties[i].name;
    default:
      return empty_str;
  }
}

std::map<std::string, uint64_t>
    ThreadStatus::InterpretOperationProperties(
    ThreadStatus::OperationType op_type,
    const uint64_t* op_properties) {
  int num_properties;
  switch (op_type) {
    case OP_COMPACTION:
      num_properties = NUM_COMPACTION_PROPERTIES;
      break;
    case OP_FLUSH:
      num_properties = NUM_FLUSH_PROPERTIES;
      break;
    default:
      num_properties = 0;
  }

  std::map<std::string, uint64_t> property_map;
  for (int i = 0; i < num_properties; ++i) {
    if (op_type == OP_COMPACTION &&
        i == COMPACTION_INPUT_OUTPUT_LEVEL) {
      property_map.insert(
          {"BaseInputLevel", op_properties[i] >> 32});
      property_map.insert(
          {"OutputLevel", op_properties[i] % (uint64_t(1) << 32U)});
    } else if (op_type == OP_COMPACTION &&
               i == COMPACTION_PROP_FLAGS) {
      property_map.insert(
          {"IsManual", ((op_properties[i] & 2) >> 1)});
      property_map.insert(
          {"IsDeletion", ((op_properties[i] & 4) >> 2)});
      property_map.insert(
          {"IsTrivialMove", ((op_properties[i] & 8) >> 3)});
    } else {
      property_map.insert(
          {GetOperationPropertyName(op_type, i), op_properties[i]});
    }
  }
  return property_map;
}


#else

const std::string& ThreadStatus::GetThreadTypeName(
    ThreadStatus::ThreadType thread_type) {
  static std::string dummy_str = "";
  return dummy_str;
}

const std::string& ThreadStatus::GetOperationName(
    ThreadStatus::OperationType op_type) {
  static std::string dummy_str = "";
  return dummy_str;
}

const std::string& ThreadStatus::GetOperationStageName(
    ThreadStatus::OperationStage stage) {
  static std::string dummy_str = "";
  return dummy_str;
}

const std::string& ThreadStatus::GetStateName(
    ThreadStatus::StateType state_type) {
  static std::string dummy_str = "";
  return dummy_str;
}

const std::string ThreadStatus::MicrosToString(
    uint64_t op_elapsed_time) {
  static std::string dummy_str = "";
  return dummy_str;
}

const std::string& ThreadStatus::GetOperationPropertyName(
    ThreadStatus::OperationType op_type, int i) {
  static std::string dummy_str = "";
  return dummy_str;
}

std::map<std::string, uint64_t>
    ThreadStatus::InterpretOperationProperties(
    ThreadStatus::OperationType op_type,
    const uint64_t* op_properties) {
  return std::map<std::string, uint64_t>();
}

#endif  //使用线程状态的rocksdb_
}  //命名空间rocksdb
