
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

#include <ripple/rpc/Status.h>

namespace ripple {
namespace RPC {

constexpr Status::Code Status::OK;

std::string Status::codeString () const
{
    if (!*this)
        return "";

    if (type_ == Type::none)
        return std::to_string (code_);

    if (type_ == Status::Type::TER)
    {
        std::string s1, s2;

        auto success = transResultInfo (toTER (), s1, s2);
        assert (success);
        (void) success;

        return s1 + ": " + s2;
    }

    if (type_ == Status::Type::error_code_i)
    {
        auto info = get_error_info (toErrorCode ());
        return info.token +  ": " + info.message;
    }

    assert (false);
    return "";
}

void Status::fillJson (Json::Value& value)
{
    if (!*this)
        return;

    auto& error = value[jss::error];
    error[jss::code] = code_;
    error[jss::message] = codeString ();

//还有消息吗？
    if (!messages_.empty ())
    {
        auto& messages = error[jss::data];
        for (auto& i: messages_)
            messages.append (i);
    }
}

std::string Status::message() const {
    std::string result;
    for (auto& m: messages_)
    {
        if (!result.empty())
            result += '/';
        result += m;
    }

    return result;
}

std::string Status::toString() const {
    if (*this)
        return codeString() + ":" + message();
    return "";
}

} //命名空间RPC
} //涟漪
