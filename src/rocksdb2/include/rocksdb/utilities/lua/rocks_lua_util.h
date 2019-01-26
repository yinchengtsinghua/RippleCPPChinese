
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
//卢亚报头
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#ifdef LUA
#include <string>
#include <vector>

#include "rocksdb/utilities/lua/rocks_lua_custom_library.h"

namespace rocksdb {
namespace lua {
class LuaStateWrapper {
 public:
  explicit LuaStateWrapper(const std::string& lua_script) {
    lua_state_ = luaL_newstate();
    Init(lua_script, {});
  }
  LuaStateWrapper(
      const std::string& lua_script,
      const std::vector<std::shared_ptr<RocksLuaCustomLibrary>>& libraries) {
    lua_state_ = luaL_newstate();
    Init(lua_script, libraries);
  }
  lua_State* GetLuaState() const { return lua_state_; }
  ~LuaStateWrapper() { lua_close(lua_state_); }

 private:
  void Init(
      const std::string& lua_script,
      const std::vector<std::shared_ptr<RocksLuaCustomLibrary>>& libraries) {
    if (lua_state_) {
      luaL_openlibs(lua_state_);
      for (const auto& library : libraries) {
        luaL_openlib(lua_state_, library->Name(), library->Lib(), 0);
        library->CustomSetup(lua_state_);
      }
      luaL_dostring(lua_state_, lua_script.c_str());
    }
  }

  lua_State* lua_state_;
};
}  //命名空间Lua
}  //命名空间rocksdb
#endif  //卢阿
