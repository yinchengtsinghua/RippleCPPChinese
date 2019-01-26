
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

#ifndef BEAST_TEST_FAIL_STREAM_HPP
#define BEAST_TEST_FAIL_STREAM_HPP

#include <beast/core/async_result.hpp>
#include <beast/core/bind_handler.hpp>
#include <beast/core/error.hpp>
#include <beast/core/detail/type_traits.hpp>
#include <beast/websocket/teardown.hpp>
#include <beast/test/fail_counter.hpp>
#include <boost/optional.hpp>

namespace beast {
namespace test {

/*失败的流包装。

    在第n个操作上，流将失败，并指定
    错误代码，或无效参数的默认错误代码。
**/

template<class NextLayer>
class fail_stream
{
    boost::optional<fail_counter> fc_;
    fail_counter* pfc_;
    NextLayer next_layer_;

public:
    using next_layer_type =
        typename std::remove_reference<NextLayer>::type;

    using lowest_layer_type =
        typename get_lowest_layer<next_layer_type>::type;

    fail_stream(fail_stream&&) = delete;
    fail_stream(fail_stream const&) = delete;
    fail_stream& operator=(fail_stream&&) = delete;
    fail_stream& operator=(fail_stream const&) = delete;

    template<class... Args>
    explicit
    fail_stream(std::size_t n, Args&&... args)
        : fc_(n)
        , pfc_(&*fc_)
        , next_layer_(std::forward<Args>(args)...)
    {
    }

    template<class... Args>
    explicit
    fail_stream(fail_counter& fc, Args&&... args)
        : pfc_(&fc)
        , next_layer_(std::forward<Args>(args)...)
    {
    }

    next_layer_type&
    next_layer()
    {
        return next_layer_;
    }

    lowest_layer_type&
    lowest_layer()
    {
        return next_layer_.lowest_layer();
    }

    lowest_layer_type const&
    lowest_layer() const
    {
        return next_layer_.lowest_layer();
    }

    boost::asio::io_service&
    get_io_service()
    {
        return next_layer_.get_io_service();
    }

    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers)
    {
        pfc_->fail();
        return next_layer_.read_some(buffers);
    }

    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers, error_code& ec)
    {
        if(pfc_->fail(ec))
            return 0;
        return next_layer_.read_some(buffers, ec);
    }

    template<class MutableBufferSequence, class ReadHandler>
    async_return_type<
        ReadHandler, void(error_code, std::size_t)>
    async_read_some(MutableBufferSequence const& buffers,
        ReadHandler&& handler)
    {
        error_code ec;
        if(pfc_->fail(ec))
        {
            async_completion<ReadHandler,
                void(error_code, std::size_t)> init{handler};
            next_layer_.get_io_service().post(
                bind_handler(init.completion_handler, ec, 0));
            return init.result.get();
        }
        return next_layer_.async_read_some(buffers,
            std::forward<ReadHandler>(handler));
    }

    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers)
    {
        pfc_->fail();
        return next_layer_.write_some(buffers);
    }

    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers, error_code& ec)
    {
        if(pfc_->fail(ec))
            return 0;
        return next_layer_.write_some(buffers, ec);
    }

    template<class ConstBufferSequence, class WriteHandler>
    async_return_type<
        WriteHandler, void(error_code, std::size_t)>
    async_write_some(ConstBufferSequence const& buffers,
        WriteHandler&& handler)
    {
        error_code ec;
        if(pfc_->fail(ec))
        {
            async_completion<WriteHandler,
                void(error_code, std::size_t)> init{handler};
            next_layer_.get_io_service().post(
                bind_handler(init.completion_handler, ec, 0));
            return init.result.get();
        }
        return next_layer_.async_write_some(buffers,
            std::forward<WriteHandler>(handler));
    }

    friend
    void
    teardown(websocket::teardown_tag,
        fail_stream<NextLayer>& stream,
            boost::system::error_code& ec)
    {
        if(stream.pfc_->fail(ec))
            return;
        beast::websocket_helpers::call_teardown(stream.next_layer(), ec);
    }

    template<class TeardownHandler>
    friend
    void
    async_teardown(websocket::teardown_tag,
        fail_stream<NextLayer>& stream,
            TeardownHandler&& handler)
    {
        error_code ec;
        if(stream.pfc_->fail(ec))
        {
            stream.get_io_service().post(
                bind_handler(std::move(handler), ec));
            return;
        }
        beast::websocket_helpers::call_async_teardown(
            stream.next_layer(), std::forward<TeardownHandler>(handler));
    }
};

} //测试
} //野兽

#endif
