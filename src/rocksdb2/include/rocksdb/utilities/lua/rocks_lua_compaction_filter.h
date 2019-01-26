
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

#pragma once

#if defined(LUA) && !defined(ROCKSDB_LITE)
//卢亚报头
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <mutex>
#include <string>
#include <vector>

#include "rocksdb/compaction_filter.h"
#include "rocksdb/env.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/lua/rocks_lua_custom_library.h"
#include "rocksdb/utilities/lua/rocks_lua_util.h"

namespace rocksdb {
namespace lua {

struct RocksLuaCompactionFilterOptions {
//字符串中的Lua脚本，用于实现所有必需的CompactionFilter
//虚拟功能。指定的lua脚本必须实现以下内容
//函数，即名称和筛选器，如下所述。
//
//0。name函数只返回一个字符串，该字符串表示
//Lua脚本。如果name函数中有任何erorr，
//将使用空字符串。
//--实例
//函数名（）
//返回“defaultLuaCompactionFilter”
//结束
//
//
//1。脚本必须包含一个名为filter的函数，该函数实现
//compactionfilter:：filter（），接受三个输入参数，并返回
//以下三个值为API：
//
//功能过滤器（级别、键、现有值）
//…
//返回被过滤，被更改，新值
//结束
//
//注意，如果ignore_value设置为true，那么filter应该实现
//以下API：
//
//功能过滤器（级别，键）
//…
//返回被过滤
//结束
//
//如果filter（）函数中有任何错误，那么它将保留
//输入键/值对。
//
//--输入
//函数必须接受三个参数（integer、string、string）。
//从哪个映射到“级别”、“键”和“现有值”
//ROCKSDB
//
//--输出
//函数必须返回三个值（布尔值、布尔值、字符串）。
//-被过滤：如果第一个返回值为真，则表示
//应过滤输入键/值对。
//-是否已更改：如果第二个返回值为真，则表示
//需要更改现有的_值，以及结果值
//存储在第三个返回值中。
//-new_value：如果第二个返回值为true，则第三个返回值为
//返回值存储输入键/值对的新值。
//
//——实例
//--保留所有键值对的筛选器
//功能过滤器（级别、键、现有值）
//返回false，false，“”
//结束
//
//--保留所有键并将其值更改为“rocks”的筛选器
//功能过滤器（级别、键、现有值）
//返回假，真，“石头”
//结束

  std::string lua_script;

//如果设置为true，则现有的_值将不会传递到筛选器
//函数，过滤函数只需要返回一个布尔值
//指示是否筛选出此密钥的标志。
//
//功能过滤器（级别，键）
//…
//返回被过滤
//结束
  bool ignore_value = false;

//用于确定是否忽略快照的布尔标志。
  bool ignore_snapshots = false;

//当指定一个非空指针时，第一个“错误限制”过滤器
//将包括与Lua相关的每个CompactionFilter的错误。
//在这个日志中。
  std::shared_ptr<Logger> error_log;

//将打印每个CompactionFilter的错误数
//记录错误。
  int error_limit_per_filter = 1;

//允许Lua CompactionFilter的字符串到Lual_Reg数组映射
//使用自定义C库。字符串将用作库
//在Lua的名字。
  std::vector<std::shared_ptr<RocksLuaCustomLibrary>> libraries;

////////////////////////////////////////////////
//尚不支持
//实现的“lua_脚本”中lua函数的名称
//compactionfilter:：filtermergeoperand（）。功能必须
//三个输入参数（integer、string、string），映射到“level”，
//“key”和“operand”从rocksdb传递。此外，
//函数必须返回一个布尔值，指示
//过滤输入键/操作数。
//
//默认：默认实现总是返回false。
//@请参见CompactionFilter:：FilterMergeOperand
};

class RocksLuaCompactionFilterFactory : public CompactionFilterFactory {
 public:
  explicit RocksLuaCompactionFilterFactory(
      const RocksLuaCompactionFilterOptions opt);

  virtual ~RocksLuaCompactionFilterFactory() {}

  std::unique_ptr<CompactionFilter> CreateCompactionFilter(
      const CompactionFilter::Context& context) override;

//更改lua脚本，以便在此之后进行下一次压缩
//函数调用将使用新的lua脚本。
  void SetScript(const std::string& new_script);

//获取当前的lua脚本
  std::string GetScript();

  const char* Name() const override;

 private:
  RocksLuaCompactionFilterOptions opt_;
  std::string name_;
//保护“opt_uuu”使其动态变化的锁。
  std::mutex opt_mutex_;
};

//调用Lua脚本以执行CompactionFilter的包装类
//功能。
class RocksLuaCompactionFilter : public rocksdb::CompactionFilter {
 public:
  explicit RocksLuaCompactionFilter(const RocksLuaCompactionFilterOptions& opt)
      : options_(opt),
        lua_state_wrapper_(opt.lua_script, opt.libraries),
        error_count_(0),
        name_("") {}

  virtual bool Filter(int level, const Slice& key, const Slice& existing_value,
                      std::string* new_value,
                      bool* value_changed) const override;
//尚不支持
  virtual bool FilterMergeOperand(int level, const Slice& key,
                                  const Slice& operand) const override {
    return false;
  }
  virtual bool IgnoreSnapshots() const override;
  virtual const char* Name() const override;

 protected:
  void LogLuaError(const char* format, ...) const;

  RocksLuaCompactionFilterOptions options_;
  LuaStateWrapper lua_state_wrapper_;
  mutable int error_count_;
  mutable std::string name_;
};

}  //命名空间Lua
}  //命名空间rocksdb
#endif  //定义（lua）&！已定义（RocksDB-Lite）
