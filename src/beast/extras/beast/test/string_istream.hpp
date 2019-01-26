
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//
//版权所有（c）2013-2017 Vinnie Falco（Gmail.com上的Vinnie Dot Falco）
//
//在Boost软件许可证1.0版下分发。（见附件
//文件许可证_1_0.txt或复制到http://www.boost.org/license_1_0.txt）
//

#ifndef BEAST_TEST_STRING_ISTREAM_HPP
#define BEAST_TEST_STRING_ISTREAM_HPP

#include <beast/core/async_result.hpp>
#include <beast/core/bind_handler.hpp>
#include <beast/core/error.hpp>
#include <beast/websocket/teardown.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/throw_exception.hpp>
#include <string>

namespace beast {
namespace test {

/*从字符串中读取的同步流和异步流。

    这个类的行为就像一个套接字，除了写入的数据
    丢弃，当读取数据时，它来自提供的字符串
    施工时。
**/

class string_istream
{
    std::string s_;
    boost::asio::const_buffer cb_;
    boost::asio::io_service& ios_;
    std::size_t read_max_;

public:
    string_istream(boost::asio::io_service& ios,
            std::string s, std::size_t read_max =
                (std::numeric_limits<std::size_t>::max)())
        : s_(std::move(s))
        , cb_(boost::asio::buffer(s_))
        , ios_(ios)
        , read_max_(read_max)
    {
    }

    boost::asio::io_service&
    get_io_service()
    {
        return ios_;
    }

    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers)
    {
        error_code ec;
        auto const n = read_some(buffers, ec);
        if(ec)
            BOOST_THROW_EXCEPTION(system_error{ec});
        return n;
    }

    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers,
        error_code& ec)
    {
        auto const n = boost::asio::buffer_copy(
            buffers, cb_, read_max_);
        if(n > 0)
        {
            ec.assign(0, ec.category());
            cb_ = cb_ + n;
        }
        else
        {
            ec = boost::asio::error::eof;
        }
        return n;
    }

    template<class MutableBufferSequence, class ReadHandler>
    async_return_type<
        ReadHandler, void(error_code, std::size_t)>
    async_read_some(MutableBufferSequence const& buffers,
        ReadHandler&& handler)
    {
        auto const n = boost::asio::buffer_copy(
            buffers, boost::asio::buffer(s_));
        error_code ec;
        if(n > 0)
            s_.erase(0, n);
        else
            ec = boost::asio::error::eof;
        async_completion<ReadHandler,
            void(error_code, std::size_t)> init{handler};
        ios_.post(bind_handler(
            init.completion_handler, ec, n));
        return init.result.get();
    }

    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers)
    {
        error_code ec;
        auto const n = write_some(buffers, ec);
        if(ec)
            BOOST_THROW_EXCEPTION(system_error{ec});
        return n;
    }

    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers,
        error_code& ec)
    {
        ec.assign(0, ec.category());
        return boost::asio::buffer_size(buffers);
    }

    template<class ConstBuffeSequence, class WriteHandler>
    async_return_type<
        WriteHandler, void(error_code, std::size_t)>
    async_write_some(ConstBuffeSequence const& buffers,
        WriteHandler&& handler)
    {
        async_completion<WriteHandler,
            void(error_code, std::size_t)> init{handler};
        ios_.post(bind_handler(init.completion_handler,
            error_code{}, boost::asio::buffer_size(buffers)));
        return init.result.get();
    }

    friend
    void
    teardown(websocket::teardown_tag,
        string_istream&,
            boost::system::error_code& ec)
    {
        ec.assign(0, ec.category());
    }

    template<class TeardownHandler>
    friend
    void
    async_teardown(websocket::teardown_tag,
        string_istream& stream,
            TeardownHandler&& handler)
    {
        stream.get_io_service().post(
            bind_handler(std::move(handler),
                error_code{}));
    }
};

} //测试
} //野兽

#endif
