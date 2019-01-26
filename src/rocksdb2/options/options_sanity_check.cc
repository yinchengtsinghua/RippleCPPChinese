
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

#include "options/options_sanity_check.h"

namespace rocksdb {

namespace {
OptionsSanityCheckLevel SanityCheckLevelHelper(
    const std::unordered_map<std::string, OptionsSanityCheckLevel>& smap,
    const std::string& name) {
  auto iter = smap.find(name);
  return iter != smap.end() ? iter->second : kSanityLevelExactMatch;
}
}

OptionsSanityCheckLevel DBOptionSanityCheckLevel(
    const std::string& option_name) {
  return SanityCheckLevelHelper(sanity_level_db_options, option_name);
}

OptionsSanityCheckLevel CFOptionSanityCheckLevel(
    const std::string& option_name) {
  return SanityCheckLevelHelper(sanity_level_cf_options, option_name);
}

OptionsSanityCheckLevel BBTOptionSanityCheckLevel(
    const std::string& option_name) {
  return SanityCheckLevelHelper(sanity_level_bbt_options, option_name);
}

}  //命名空间rocksdb

#endif  //！摇滚乐
