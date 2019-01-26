
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2016，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#if defined(LUA) && !defined(ROCKSDB_LITE)
#include "rocksdb/utilities/lua/rocks_lua_compaction_filter.h"

extern "C" {
#include <luaconf.h>
}

#include "rocksdb/compaction_filter.h"

namespace rocksdb {
namespace lua {

const std::string kFilterFunctionName = "Filter";
const std::string kNameFunctionName = "Name";

void RocksLuaCompactionFilter::LogLuaError(const char* format, ...) const {
  if (options_.error_log.get() != nullptr &&
      error_count_ < options_.error_limit_per_filter) {
    error_count_++;

    va_list ap;
    va_start(ap, format);
    options_.error_log->Logv(InfoLogLevel::ERROR_LEVEL, format, ap);
    va_end(ap);
  }
}

bool RocksLuaCompactionFilter::Filter(int level, const Slice& key,
                                      const Slice& existing_value,
                                      std::string* new_value,
                                      bool* value_changed) const {
  auto* lua_state = lua_state_wrapper_.GetLuaState();
//将正确的函数推入Lua堆栈
  lua_getglobal(lua_state, kFilterFunctionName.c_str());

  int error_no = 0;
  int num_input_values;
  int num_return_values;
  if (options_.ignore_value == false) {
//将输入参数推送到Lua堆栈中
    lua_pushnumber(lua_state, level);
    lua_pushlstring(lua_state, key.data(), key.size());
    lua_pushlstring(lua_state, existing_value.data(), existing_value.size());
    num_input_values = 3;
    num_return_values = 3;
  } else {
//如果“忽略”值设置为“真”，则只放置两个参数
//期望一个回报值
    lua_pushnumber(lua_state, level);
    lua_pushlstring(lua_state, key.data(), key.size());
    num_input_values = 2;
    num_return_values = 1;
  }

//执行lua调用
  if ((error_no =
           lua_pcall(lua_state, num_input_values, num_return_values, 0)) != 0) {
    LogLuaError("[Lua] Error(%d) in Filter function --- %s", error_no,
                lua_tostring(lua_state, -1));
//从堆栈中弹出Lua错误
    lua_pop(lua_state, 1);
    return false;
  }

//由于Luaeu-PCall的成功，可以保证
//Lua堆栈中的三个元素是返回的三个值。

  bool has_error = false;
  const int kIndexIsFiltered = -num_return_values;
  const int kIndexValueChanged = -num_return_values + 1;
  const int kIndexNewValue = -num_return_values + 2;

//检查三个返回值的类型
//IS-滤波
  if (!lua_isboolean(lua_state, kIndexIsFiltered)) {
    LogLuaError(
        "[Lua] Error in Filter function -- "
        "1st return value (is_filtered) is not a boolean "
        "while a boolean is expected.");
    has_error = true;
  }

  if (options_.ignore_value == false) {
//更改值
    if (!lua_isboolean(lua_state, kIndexValueChanged)) {
      LogLuaError(
          "[Lua] Error in Filter function -- "
          "2nd return value (value_changed) is not a boolean "
          "while a boolean is expected.");
      has_error = true;
    }
//纽尔值
    if (!lua_isstring(lua_state, kIndexNewValue)) {
      LogLuaError(
          "[Lua] Error in Filter function -- "
          "3rd return value (new_value) is not a string "
          "while a string is expected.");
      has_error = true;
    }
  }

  if (has_error) {
    lua_pop(lua_state, num_return_values);
    return false;
  }

//获取返回值
  bool is_filtered = false;
  if (!has_error) {
    is_filtered = lua_toboolean(lua_state, kIndexIsFiltered);
    if (options_.ignore_value == false) {
      *value_changed = lua_toboolean(lua_state, kIndexValueChanged);
      if (*value_changed) {
        const char* new_value_buf = lua_tostring(lua_state, kIndexNewValue);
        const size_t new_value_size = lua_strlen(lua_state, kIndexNewValue);
//注意，Lua ToString返回的任何字符串在
//它的末端，bu/t里面可以有其他的0
        assert(new_value_buf[new_value_size] == '\0');
        assert(strlen(new_value_buf) <= new_value_size);
        new_value->assign(new_value_buf, new_value_size);
      }
    } else {
      *value_changed = false;
    }
  }
//弹出三个返回值。
  lua_pop(lua_state, num_return_values);
  return is_filtered;
}

const char* RocksLuaCompactionFilter::Name() const {
  if (name_ != "") {
    return name_.c_str();
  }
  auto* lua_state = lua_state_wrapper_.GetLuaState();
//将正确的函数推入Lua堆栈
  lua_getglobal(lua_state, kNameFunctionName.c_str());

//执行调用（0个参数，1个结果）
  int error_no;
  if ((error_no = lua_pcall(lua_state, 0, 1, 0)) != 0) {
    LogLuaError("[Lua] Error(%d) in Name function --- %s", error_no,
                lua_tostring(lua_state, -1));
//从堆栈中弹出Lua错误
    lua_pop(lua_state, 1);
    return name_.c_str();
  }

//检查返回值
  if (!lua_isstring(lua_state, -1)) {
    LogLuaError(
        "[Lua] Error in Name function -- "
        "return value is not a string while string is expected");
  } else {
    const char* name_buf = lua_tostring(lua_state, -1);
    const size_t name_size __attribute__((unused)) = lua_strlen(lua_state, -1);
    assert(name_buf[name_size] == '\0');
    assert(strlen(name_buf) <= name_size);
    name_ = name_buf;
  }
  lua_pop(lua_state, 1);
  return name_.c_str();
}

/*尚不支持
bool rocksluacompactionfilter:：filtermergeoperand（）。
    int level，const slice&key，const slice&operand）const_
  auto*lua_state=lua_state_wrapper_u.getLuaState（）；
  //将正确的函数推入lua堆栈
  lua_getglobal（lua_状态，“filtermergeoperand”）；

  //将输入参数推送到Lua堆栈中
  Lua_PushNumber（Lua_状态，级别）；
  lua pushlstring（lua_状态，key.data（），key.size（））；
  lua pushlstring（lua_状态，operand.data（），operand.size（））；

  //执行调用（3个参数，1个结果）
  输入错误；
  如果（（错误_no=lua _pcall（lua _状态，3，1，0））！= 0）{
    logluaError（filtermergeoperand函数中的[lua]错误（%d）---%s“，
        错误_no，lua_to字符串（lua_状态，-1））；
    //从堆栈中弹出lua错误
    Luaou Pop（Luaou州，1）；
    返回错误；
  }

  bool被_filtered=false；
  //检查返回值
  如果（！）Lua_是布尔值（Lua_状态，-1））
    logLuaError（“filterMergeOperand函数中的[Lua]错误--”
                “返回值不是布尔值，而应为布尔值”）；
  }否则{
    _filtered=lua_toboolean（lua_状态，-1）；
  }

  Luaou Pop（Luaou州，1）；

  返回被过滤；
}
**/


bool RocksLuaCompactionFilter::IgnoreSnapshots() const {
  return options_.ignore_snapshots;
}

RocksLuaCompactionFilterFactory::RocksLuaCompactionFilterFactory(
    const RocksLuaCompactionFilterOptions opt)
    : opt_(opt) {
  auto filter = CreateCompactionFilter(CompactionFilter::Context());
  name_ = std::string("RocksLuaCompactionFilterFactory::") +
          std::string(filter->Name());
}

std::unique_ptr<CompactionFilter>
RocksLuaCompactionFilterFactory::CreateCompactionFilter(
    const CompactionFilter::Context& context) {
  std::lock_guard<std::mutex> lock(opt_mutex_);
  return std::unique_ptr<CompactionFilter>(new RocksLuaCompactionFilter(opt_));
}

std::string RocksLuaCompactionFilterFactory::GetScript() {
  std::lock_guard<std::mutex> lock(opt_mutex_);
  return opt_.lua_script;
}

void RocksLuaCompactionFilterFactory::SetScript(const std::string& new_script) {
  std::lock_guard<std::mutex> lock(opt_mutex_);
  opt_.lua_script = new_script;
}

const char* RocksLuaCompactionFilterFactory::Name() const {
  return name_.c_str();
}

}  //命名空间Lua
}  //命名空间rocksdb
#endif  //定义（lua）&！已定义（RocksDB-Lite）
