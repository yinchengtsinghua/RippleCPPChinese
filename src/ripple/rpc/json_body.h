
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

#ifndef RIPPLE_RPC_JSON_BODY_H
#define RIPPLE_RPC_JSON_BODY_H

#include <ripple/json/json_value.h>
#include <ripple/json/to_string.h>

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/http/message.hpp>

namespace ripple {

///body保存json
struct json_body
{
    explicit json_body() = default;

    using value_type = Json::Value;

    class reader
    {
        using dynamic_buffer_type = boost::beast::multi_buffer;

        dynamic_buffer_type buffer_;

    public:
        using const_buffers_type =
            typename dynamic_buffer_type::const_buffers_type;

        using is_deferred = std::false_type;

        template<bool isRequest, class Fields>
        explicit
        reader(boost::beast::http::message<
            isRequest, json_body, Fields> const& m)
        {
            stream(m.body,
                [&](void const* data, std::size_t n)
                {
                    buffer_.commit(boost::asio::buffer_copy(
                        buffer_.prepare(n), boost::asio::buffer(data, n)));
                });
        }

        void
        init(boost::beast::error_code&) noexcept
        {
        }

        boost::optional<std::pair<const_buffers_type, bool>>
        get(boost::beast::error_code& ec)
        {
            return {{buffer_.data(), false}};
        }

        void
        finish(boost::beast::error_code&)
        {
        }
    };

    class writer
    {
        std::string body_string_;

    public:
        using const_buffers_type =
            boost::asio::const_buffer;

        template <bool isRequest, class Fields>
        explicit
        writer(
            boost::beast::http::header<isRequest, Fields> const& fields,
            value_type const& value)
                : body_string_(to_string(value))
        {
        }

        void
        init(boost::beast::error_code& ec)
        {
            ec.assign(0, ec.category());
        }

        boost::optional<std::pair<const_buffers_type, bool>>
        get(boost::beast::error_code& ec)
        {
            ec.assign(0, ec.category());
            return {{const_buffers_type{
                body_string_.data(), body_string_.size()}, false}};
        }
    };
};

} //涟漪

#endif
