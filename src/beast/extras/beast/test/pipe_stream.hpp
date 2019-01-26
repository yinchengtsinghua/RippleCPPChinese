
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

#ifndef BEAST_TEST_PIPE_STREAM_HPP
#define BEAST_TEST_PIPE_STREAM_HPP

#include <beast/core/async_result.hpp>
#include <beast/core/bind_handler.hpp>
#include <beast/core/flat_buffer.hpp>
#include <beast/core/string.hpp>
#include <beast/core/type_traits.hpp>
#include <beast/websocket/teardown.hpp>
#include <beast/test/fail_counter.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <utility>

namespace beast {
namespace test {

/*双向内存通信信道

    此类的实例提供客户端和服务器
    自动相互连接的端点
    类似于连接的插座。

    测试管用于方便编写单元测试
    运输行为受到严格控制的地方
    帮助照亮所有代码路径（用于代码覆盖）
**/

class pipe
{
public:
    using buffer_type = flat_buffer;

private:
    struct read_op
    {
        virtual ~read_op() = default;
        virtual void operator()() = 0;
    };

    struct state
    {
        std::mutex m;
        buffer_type b;
        std::condition_variable cv;
        std::unique_ptr<read_op> op;
        bool eof = false;
    };

    state s_[2];

public:
    /*表示终结点。

        每个管道都有一个客户机流和一个服务器流。
    **/

    class stream
    {
        friend class pipe;

        template<class Handler, class Buffers>
        class read_op_impl;

        state& in_;
        state& out_;
        boost::asio::io_service& ios_;
        fail_counter* fc_ = nullptr;
        std::size_t read_max_ =
            (std::numeric_limits<std::size_t>::max)();
        std::size_t write_max_ =
            (std::numeric_limits<std::size_t>::max)();

        stream(state& in, state& out,
                boost::asio::io_service& ios)
            : in_(in)
            , out_(out)
            , ios_(ios)
            , buffer(in_.b)
        {
        }

    public:
        using buffer_type = pipe::buffer_type;

///直接访问基础缓冲区
        buffer_type& buffer;

///counts读取调用的数目
        std::size_t nread = 0;

///统计写入调用的数目
        std::size_t nwrite = 0;

        ~stream() = default;
        stream(stream&&) = default;

///设置对象的失败计数器
        void
        fail(fail_counter& fc)
        {
            fc_ = &fc;
        }

///返回与对象关联的“IO服务”
        boost::asio::io_service&
        get_io_service()
        {
            return ios_;
        }

///设置读操作返回的最大字节数
        void
        read_size(std::size_t n)
        {
            read_max_ = n;
        }

///设置写入返回的最大字节数\u一些
        void
        write_size(std::size_t n)
        {
            write_max_ = n;
        }

///返回表示挂起输入数据的字符串
        string_view
        str() const
        {
            using boost::asio::buffer_cast;
            using boost::asio::buffer_size;
            return {
                buffer_cast<char const*>(*in_.b.data().begin()),
                buffer_size(*in_.b.data().begin())};
        }

///清除保存输入数据的缓冲区
        void
        clear()
        {
            in_.b.consume((std::numeric_limits<
                std::size_t>::max)());
        }

        /*关闭流。

            管道的另一端将看到
            读时为'boost:：asio:：error:：eof'。
        **/

        template<class = void>
        void
        close();

        template<class MutableBufferSequence>
        std::size_t
        read_some(MutableBufferSequence const& buffers);

        template<class MutableBufferSequence>
        std::size_t
        read_some(MutableBufferSequence const& buffers,
            error_code& ec);

        template<class MutableBufferSequence, class ReadHandler>
        async_return_type<
            ReadHandler, void(error_code, std::size_t)>
        async_read_some(MutableBufferSequence const& buffers,
            ReadHandler&& handler);

        template<class ConstBufferSequence>
        std::size_t
        write_some(ConstBufferSequence const& buffers);

        template<class ConstBufferSequence>
        std::size_t
        write_some(
            ConstBufferSequence const& buffers, error_code&);

        template<class ConstBufferSequence, class WriteHandler>
        async_return_type<
            WriteHandler, void(error_code, std::size_t)>
        async_write_some(ConstBufferSequence const& buffers,
            WriteHandler&& handler);

        friend
        void
        teardown(websocket::teardown_tag,
            stream&, boost::system::error_code& ec)
        {
            ec.assign(0, ec.category());
        }

        template<class TeardownHandler>
        friend
        void
        async_teardown(websocket::teardown_tag,
            stream& s, TeardownHandler&& handler)
        {
            s.get_io_service().post(
                bind_handler(std::move(handler),
                    error_code{}));
        }
    };

    /*构造函数。

        客户端和服务器终结点将使用相同的“IO”服务。
    **/

    explicit
    pipe(boost::asio::io_service& ios)
        : client(s_[0], s_[1], ios)
        , server(s_[1], s_[0], ios)
    {
    }

    /*构造函数。

        客户端和服务器终结点将不同的“IO服务”对象。
    **/

    explicit
    pipe(boost::asio::io_service& ios1,
            boost::asio::io_service& ios2)
        : client(s_[0], s_[1], ios1)
        , server(s_[1], s_[0], ios2)
    {
    }

///表示客户端终结点
    stream client;

///表示服务器终结点
    stream server;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Handler, class Buffers>
class pipe::stream::read_op_impl :
    public pipe::read_op
{
    stream& s_;
    Buffers b_;
    Handler h_;

public:
    read_op_impl(stream& s,
            Buffers const& b, Handler&& h)
        : s_(s)
        , b_(b)
        , h_(std::move(h))
    {
    }

    read_op_impl(stream& s,
            Buffers const& b, Handler const& h)
        : s_(s)
        , b_(b)
        , h_(h)
    {
    }

    void
    operator()() override;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Handler, class Buffers>
void
pipe::stream::
read_op_impl<Handler, Buffers>::operator()()
{
    using boost::asio::buffer_copy;
    using boost::asio::buffer_size;
    s_.ios_.post(
        [&]()
        {
            BOOST_ASSERT(s_.in_.op);
            std::unique_lock<std::mutex> lock{s_.in_.m};
            if(s_.in_.b.size() > 0)
            {
                auto const bytes_transferred = buffer_copy(
                    b_, s_.in_.b.data(), s_.read_max_);
                s_.in_.b.consume(bytes_transferred);
                auto& s = s_;
                Handler h{std::move(h_)};
                lock.unlock();
                s.in_.op.reset(nullptr);
                ++s.nread;
                s.ios_.post(bind_handler(std::move(h),
                    error_code{}, bytes_transferred));
            }
            else
            {
                BOOST_ASSERT(s_.in_.eof);
                auto& s = s_;
                Handler h{std::move(h_)};
                lock.unlock();
                s.in_.op.reset(nullptr);
                ++s.nread;
                s.ios_.post(bind_handler(std::move(h),
                    boost::asio::error::eof, 0));
            }
        });
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class>
void
pipe::stream::
close()
{
    std::lock_guard<std::mutex> lock{out_.m};
    out_.eof = true;
    if(out_.op)
        out_.op.get()->operator()();
    else
        out_.cv.notify_all();
}


template<class MutableBufferSequence>
std::size_t
pipe::stream::
read_some(MutableBufferSequence const& buffers)
{
    static_assert(is_mutable_buffer_sequence<
            MutableBufferSequence>::value,
        "MutableBufferSequence requirements not met");
    error_code ec;
    auto const n = read_some(buffers, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return n;
}

template<class MutableBufferSequence>
std::size_t
pipe::stream::
read_some(MutableBufferSequence const& buffers,
    error_code& ec)
{
    static_assert(is_mutable_buffer_sequence<
            MutableBufferSequence>::value,
        "MutableBufferSequence requirements not met");
    using boost::asio::buffer_copy;
    using boost::asio::buffer_size;
    BOOST_ASSERT(! in_.op);
    BOOST_ASSERT(buffer_size(buffers) > 0);
    if(fc_ && fc_->fail(ec))
        return 0;
    std::unique_lock<std::mutex> lock{in_.m};
    in_.cv.wait(lock,
        [&]()
        {
            return in_.b.size() > 0 || in_.eof;
        });
    std::size_t bytes_transferred;
    if(in_.b.size() > 0)
    {   
        ec.assign(0, ec.category());
        bytes_transferred = buffer_copy(
            buffers, in_.b.data(), read_max_);
        in_.b.consume(bytes_transferred);
    }
    else
    {
        BOOST_ASSERT(in_.eof);
        bytes_transferred = 0;
        ec = boost::asio::error::eof;
    }
    ++nread;
    return bytes_transferred;
}

template<class MutableBufferSequence, class ReadHandler>
async_return_type<
    ReadHandler, void(error_code, std::size_t)>
pipe::stream::
async_read_some(MutableBufferSequence const& buffers,
    ReadHandler&& handler)
{
    static_assert(is_mutable_buffer_sequence<
            MutableBufferSequence>::value,
        "MutableBufferSequence requirements not met");
    using boost::asio::buffer_copy;
    using boost::asio::buffer_size;
    BOOST_ASSERT(! in_.op);
    BOOST_ASSERT(buffer_size(buffers) > 0);
    async_completion<ReadHandler,
        void(error_code, std::size_t)> init{handler};
    if(fc_)
    {
        error_code ec;
        if(fc_->fail(ec))
            return ios_.post(bind_handler(
                init.completion_handler, ec, 0));
    }
    {
        std::unique_lock<std::mutex> lock{in_.m};
        if(in_.eof)
        {
            lock.unlock();
            ++nread;
            ios_.post(bind_handler(init.completion_handler,
                boost::asio::error::eof, 0));
        }
        else if(buffer_size(buffers) == 0 ||
            buffer_size(in_.b.data()) > 0)
        {
            auto const bytes_transferred = buffer_copy(
                buffers, in_.b.data(), read_max_);
            in_.b.consume(bytes_transferred);
            lock.unlock();
            ++nread;
            ios_.post(bind_handler(init.completion_handler,
                error_code{}, bytes_transferred));
        }
        else
        {
            in_.op.reset(new read_op_impl<handler_type<
                ReadHandler, void(error_code, std::size_t)>,
                    MutableBufferSequence>{*this, buffers,
                        init.completion_handler});
        }
    }
    return init.result.get();
}

template<class ConstBufferSequence>
std::size_t
pipe::stream::
write_some(ConstBufferSequence const& buffers)
{
    static_assert(is_const_buffer_sequence<
            ConstBufferSequence>::value,
        "ConstBufferSequence requirements not met");
    BOOST_ASSERT(! out_.eof);
    error_code ec;
    auto const bytes_transferred =
        write_some(buffers, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return bytes_transferred;
}

template<class ConstBufferSequence>
std::size_t
pipe::stream::
write_some(
    ConstBufferSequence const& buffers, error_code& ec)
{
    static_assert(is_const_buffer_sequence<
            ConstBufferSequence>::value,
        "ConstBufferSequence requirements not met");
    using boost::asio::buffer_copy;
    using boost::asio::buffer_size;
    BOOST_ASSERT(! out_.eof);
    if(fc_ && fc_->fail(ec))
        return 0;
    auto const n = (std::min)(
        buffer_size(buffers), write_max_);
    std::unique_lock<std::mutex> lock{out_.m};
    auto const bytes_transferred =
        buffer_copy(out_.b.prepare(n), buffers);
    out_.b.commit(bytes_transferred);
    lock.unlock();
    if(out_.op)
        out_.op.get()->operator()();
    else
        out_.cv.notify_all();
    ++nwrite;
    ec.assign(0, ec.category());
    return bytes_transferred;
}

template<class ConstBufferSequence, class WriteHandler>
async_return_type<
    WriteHandler, void(error_code, std::size_t)>
pipe::stream::
async_write_some(ConstBufferSequence const& buffers,
    WriteHandler&& handler)
{
    static_assert(is_const_buffer_sequence<
            ConstBufferSequence>::value,
        "ConstBufferSequence requirements not met");
    using boost::asio::buffer_copy;
    using boost::asio::buffer_size;
    BOOST_ASSERT(! out_.eof);
    async_completion<WriteHandler,
        void(error_code, std::size_t)> init{handler};
    if(fc_)
    {
        error_code ec;
        if(fc_->fail(ec))
            return ios_.post(bind_handler(
                init.completion_handler, ec, 0));
    }
    auto const n =
        (std::min)(buffer_size(buffers), write_max_);
    std::unique_lock<std::mutex> lock{out_.m};
    auto const bytes_transferred =
        buffer_copy(out_.b.prepare(n), buffers);
    out_.b.commit(bytes_transferred);
    lock.unlock();
    if(out_.op)
        out_.op.get()->operator()();
    else
        out_.cv.notify_all();
    ++nwrite;
    ios_.post(bind_handler(init.completion_handler,
        error_code{}, bytes_transferred));
    return init.result.get();
}

} //测试
} //野兽

#endif
