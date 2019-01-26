
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Rippled的一部分：https://github.com/ripple/rippled
    版权所有（c）2012，2013 Ripple Labs Inc.

    使用、复制、修改和/或分发本软件的权限
    特此授予免费或不收费的目的，前提是
    版权声明和本许可声明出现在所有副本中。

    本软件按“原样”提供，作者不作任何保证。
    关于本软件，包括
    适销性和适用性。在任何情况下，作者都不对
    任何特殊、直接、间接或后果性损害或任何损害
    因使用、数据或利润损失而导致的任何情况，无论是在
    合同行为、疏忽或其他侵权行为
    或与本软件的使用或性能有关。
**/

//==============================================================

#ifndef RIPPLE_JSON_OUTPUT_H_INCLUDED
#define RIPPLE_JSON_OUTPUT_H_INCLUDED

#include <boost/beast/core/string.hpp>
#include <functional>

namespace Json {

class Value;

using Output = std::function <void (boost::beast::string_view const&)>;

inline
Output stringOutput (std::string& s)
{
    return [&](boost::beast::string_view const& b) { s.append (b.data(), b.size()); };
}

/*在O（n）时间内将JSON值的最小表示形式写入输出。

    数据被传输到输出端，因此只有少量的内存
    使用。对于非常大的json：：值，这可能非常重要。
 **/

void outputJson (Json::Value const&, Output const&);

/*返回o（n）时间内json：：值的最小字符串表示形式。

    这需要为输出的完整大小分配内存。
    如果可能，请使用outputjson（）。
 **/

std::string jsonAsString (Json::Value const&);

} //杰森

#endif
