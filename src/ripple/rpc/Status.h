
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

#ifndef RIPPLE_RPC_STATUS_H_INCLUDED
#define RIPPLE_RPC_STATUS_H_INCLUDED

#include <ripple/protocol/TER.h>
#include <ripple/protocol/ErrorCodes.h>
#include <cassert>

namespace ripple {
namespace RPC {

/*状态表示可能失败的操作的结果。

    它包装了旧代码ter和错误代码i，提供了统一的
    接口和将附加信息附加到现有状态的方法
    返回。

    状态还可用于用JSON-RPC 2.0填充JSON：：值。
    错误响应：请参阅http://www.jsonrpc.org/specification_error_对象
 **/

struct Status : public std::exception
{
public:
    enum class Type {none, TER, error_code_i};
    using Code = int;
    using Strings = std::vector <std::string>;

    static constexpr Code OK = 0;

    Status () = default;

//启用“if”只允许整数（而不允许枚举）。防止枚举收缩。
    template <typename T,
        typename = std::enable_if_t<std::is_integral<T>::value>>
    Status (T code, Strings d = {})
        : type_ (Type::none), code_ (code), messages_ (std::move (d))
    {
    }

    Status (TER ter, Strings d = {})
        : type_ (Type::TER), code_ (TERtoInt (ter)), messages_ (std::move (d))
    {
    }

    Status (error_code_i e, Strings d = {})
        : type_ (Type::error_code_i), code_ (e), messages_ (std::move (d))
    {
    }

    Status (error_code_i e, std::string const& s)
        : type_ (Type::error_code_i), code_ (e), messages_ ({s})
    {
    }

    /*以字符串形式返回整数状态代码的表示形式。
       如果状态为OK，则结果为空字符串。
    **/

    std::string codeString () const;

    /*如果状态为“不确定”，则返回“真”。*/
    operator bool() const
    {
        return code_ != OK;
    }

    /*如果状态为“正常”，则返回“真”。*/
    bool operator !() const
    {
        return ! bool (*this);
    }

    /*将状态作为ter返回。
        只有当type（）==type：：ter时才能调用此函数。*/

    TER toTER () const
    {
        assert (type_ == Type::TER);
        return TER::fromInt (code_);
    }

    /*以错误代码返回状态。
        这只能在type（）==type:：error_code_i时调用。*/

    error_code_i toErrorCode() const
    {
        assert (type_ == Type::error_code_i);
        return error_code_i (code_);
    }

    /*将状态应用于JSONObject
     **/

    template <class Object>
    void inject (Object& object)
    {
        if (auto ec = toErrorCode())
        {
            if (messages_.empty())
                inject_error (ec, object);
            else
                inject_error (ec, message(), object);
        }
    }

    Strings const& messages() const
    {
        return messages_;
    }

    /*如果有的话，返回第一条消息。*/
    std::string message() const;

    Type type() const
    {
        return type_;
    }

    std::string toString() const;

    /*用rpc 2.0响应填充json：：值。
        如果状态为OK，则filljson不起作用。
        当前未使用。*/

    void fillJson(Json::Value&);

private:
    Type type_ = Type::none;
    Code code_ = OK;
    Strings messages_;
};

} //命名空间RPC
} //涟漪

#endif
