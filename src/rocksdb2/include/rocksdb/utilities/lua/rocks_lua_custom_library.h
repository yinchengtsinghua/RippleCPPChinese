
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
#ifdef LUA

//卢亚报头
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace rocksdb {
namespace lua {
//用于定义可调用的自定义C库的类
//Lua脚本
class RocksLuaCustomLibrary {
 public:
  virtual ~RocksLuaCustomLibrary() {}
//C库的名称。此名称也将用作表
//（命名空间）在包含C库的Lua中。
  virtual const char* Name() const = 0;

//返回“static const struct lual_reg[]”，其中包括
//C函数。请注意，此静态数组的最后一个条目必须是
//nullptr，nullptr根据Lua的要求。
//
//有关如何实现LuaC库的更多详细信息，请参阅
//在官方lua文件中http://www.lua.org/pil/26.2.html
  virtual const struct luaL_Reg* Lib() const = 0;

//在创建库后立即调用的函数
//在卢阿州的最高层。此自定义设置功能
//允许开发人员将其他表或常量值放入
//相同的表/命名空间。
  virtual void CustomSetup(lua_State* L) const {}
};
}  //命名空间Lua
}  //命名空间rocksdb
#endif  //卢阿
