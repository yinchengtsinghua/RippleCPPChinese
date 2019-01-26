
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

#ifndef RIPPLE_SERVER_SIMPLEWRITER_H_INCLUDED
#define RIPPLE_SERVER_SIMPLEWRITER_H_INCLUDED

#include <ripple/server/Writer.h>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/write.hpp>
#include <utility>

namespace ripple {

///deprecated:序列化HTTP/1消息的编写器
class SimpleWriter : public Writer
{
    boost::beast::multi_buffer sb_;

public:
    template<bool isRequest, class Body, class Fields>
    explicit
    SimpleWriter(boost::beast::http::message<isRequest, Body, Fields> const& msg)
    {
        boost::beast::ostream(sb_) << msg;
    }

    bool
    complete() override
    {
        return sb_.size() == 0;
    }

    void
    consume (std::size_t bytes) override
    {
        sb_.consume(bytes);
    }

    bool
    prepare(std::size_t bytes,
        std::function<void(void)>) override
    {
        return true;
    }

    std::vector<boost::asio::const_buffer>
    data() override
    {
        auto const& buf = sb_.data();
        std::vector<boost::asio::const_buffer> result;
        result.reserve(std::distance(buf.begin(), buf.end()));
        for (auto const& b : buf)
            result.push_back(b);
        return result;
    }
};

} //涟漪

#endif
