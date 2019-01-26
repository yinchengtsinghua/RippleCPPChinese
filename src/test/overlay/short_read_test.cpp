
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

    版权所有2014 Ripple Labs Inc.
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

#include <ripple/basics/make_SSLContext.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <ripple/beast/unit_test.h>
#include <test/jtx/envconfig.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <memory>
#include <thread>
#include <utility>

namespace ripple {
/*

试验结果：

如果远程主机调用异步关闭，则本地主机的
异步读取将使用EOF完成。

如果两个主机都调用async_shutdown，则调用async_shutdown
将完成EOF。

**/


class short_read_test : public beast::unit_test::suite
{
private:
    using io_service_type = boost::asio::io_service;
    using strand_type = io_service_type::strand;
    using timer_type = boost::asio::basic_waitable_timer<
        std::chrono::steady_clock>;
    using acceptor_type = boost::asio::ip::tcp::acceptor;
    using socket_type = boost::asio::ip::tcp::socket;
    using stream_type = boost::asio::ssl::stream<socket_type&>;
    using error_code = boost::system::error_code;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using address_type = boost::asio::ip::address;

    io_service_type io_service_;
    boost::optional<io_service_type::work> work_;
    std::thread thread_;
    std::shared_ptr<boost::asio::ssl::context> context_;

    template <class Streambuf>
    static
    void
    write(Streambuf& sb, std::string const& s)
    {
        using boost::asio::buffer;
        using boost::asio::buffer_copy;
        using boost::asio::buffer_size;
        boost::asio::const_buffers_1 buf(s.data(), s.size());
        sb.commit(buffer_copy(sb.prepare(buffer_size(buf)), buf));
    }

//——————————————————————————————————————————————————————————————

    class Base
    {
    protected:
        class Child
        {
        private:
            Base& base_;

        public:
            explicit Child(Base& base)
                : base_(base)
            {
            }

            virtual ~Child()
            {
                base_.remove(this);
            }

            virtual void close() = 0;
        };

    private:
        std::mutex mutex_;
        std::condition_variable cond_;
        std::map<Child*, std::weak_ptr<Child>> list_;
        bool closed_ = false;

    public:
        ~Base()
        {
//派生类必须在析构函数中调用wait（）。
            assert(list_.empty());
        }

        void
        add (std::shared_ptr<Child> const& child)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            list_.emplace(child.get(), child);
        }

        void
        remove (Child* child)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            list_.erase(child);
            if (list_.empty())
                cond_.notify_one();
        }

        void
        close()
        {
            std::vector<std::shared_ptr<Child>> v;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                v.reserve(list_.size());
                if (closed_)
                    return;
                closed_ = true;
                for(auto const& c : list_)
                {
                    if(auto p = c.second.lock())
                    {
                        p->close();
//必须在
//锁，否则死锁
//托管对象的析构函数。
                        v.emplace_back(std::move(p));
                    }
                }
            }
        }

        void
        wait()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while(! list_.empty())
                cond_.wait(lock);
        }
    };

//——————————————————————————————————————————————————————————————

    class Server : public Base
    {
    private:
        short_read_test& test_;
        endpoint_type endpoint_;

        struct Acceptor
            : Child, std::enable_shared_from_this<Acceptor>
        {
            Server& server_;
            short_read_test& test_;
            acceptor_type acceptor_;
            socket_type socket_;
            strand_type strand_;

            explicit Acceptor(Server& server)
                : Child(server)
                , server_(server)
                , test_(server_.test_)
                , acceptor_(test_.io_service_,
                    endpoint_type(beast::IP::Address::from_string(
                        test::getEnvLocalhostAddr()), 0))
                , socket_(test_.io_service_)
                , strand_(socket_.get_io_service())
            {
                acceptor_.listen();
                server_.endpoint_ = acceptor_.local_endpoint();
                test_.log << "[server] up on port: " <<
                    server_.endpoint_.port() << std::endl;
            }

            void
            close() override
            {
                if(! strand_.running_in_this_thread())
                    return strand_.post(std::bind(&Acceptor::close,
                        shared_from_this()));
                acceptor_.close();
            }

            void
            run()
            {
                acceptor_.async_accept(socket_, strand_.wrap(std::bind(
                    &Acceptor::on_accept, shared_from_this(),
                        std::placeholders::_1)));
            }

            void
            fail (std::string const& what, error_code ec)
            {
                if (acceptor_.is_open())
                {
                    if (ec != boost::asio::error::operation_aborted)
                        test_.log << what <<
                            ": " << ec.message() << std::endl;
                    acceptor_.close();
                }
            }

            void
            on_accept(error_code ec)
            {
                if (ec)
                    return fail ("accept", ec);
                auto const p = std::make_shared<Connection>(
                    server_, std::move(socket_));
                server_.add(p);
                p->run();
                acceptor_.async_accept(socket_, strand_.wrap(std::bind(
                    &Acceptor::on_accept, shared_from_this(),
                        std::placeholders::_1)));
            }
        };

        struct Connection
            : Child, std::enable_shared_from_this<Connection>
        {
            Server& server_;
            short_read_test& test_;
            socket_type socket_;
            stream_type stream_;
            strand_type strand_;
            timer_type timer_;
            boost::asio::streambuf buf_;

            Connection (Server& server, socket_type&& socket)
                : Child(server)
                , server_(server)
                , test_(server_.test_)
                , socket_(std::move(socket))
                , stream_(socket_, *test_.context_)
                , strand_(socket_.get_io_service())
                , timer_(socket_.get_io_service())
            {
            }

            void
            close() override
            {
                if(! strand_.running_in_this_thread())
                    return strand_.post(std::bind(&Connection::close,
                        shared_from_this()));
                if (socket_.is_open())
                {
                    socket_.close();
                    timer_.cancel();
                }
            }

            void
            run()
            {
                timer_.expires_from_now(std::chrono::seconds(3));
                timer_.async_wait(strand_.wrap(std::bind(&Connection::on_timer,
                    shared_from_this(), std::placeholders::_1)));
                stream_.async_handshake(stream_type::server, strand_.wrap(
                    std::bind(&Connection::on_handshake, shared_from_this(),
                        std::placeholders::_1)));
            }

            void
            fail (std::string const& what, error_code ec)
            {
                if (socket_.is_open())
                {
                    if (ec != boost::asio::error::operation_aborted)
                        test_.log << "[server] " << what <<
                            ": " << ec.message() << std::endl;
                    socket_.close();
                    timer_.cancel();
                }
            }

            void
            on_timer(error_code ec)
            {
                if (ec == boost::asio::error::operation_aborted)
                    return;
                if (ec)
                    return fail("timer", ec);
                test_.log << "[server] timeout" << std::endl;
                socket_.close();
            }

            void
            on_handshake(error_code ec)
            {
                if (ec)
                    return fail("handshake", ec);
#if 1
                boost::asio::async_read_until(stream_, buf_, "\n", strand_.wrap(
                    std::bind(&Connection::on_read, shared_from_this(),
                        std::placeholders::_1,
                            std::placeholders::_2)));
#else
                close();
#endif
            }

            void
            on_read(error_code ec, std::size_t bytes_transferred)
            {
                if (ec == boost::asio::error::eof)
                {
                    server_.test_.log << "[server] read: EOF" << std::endl;
                    return stream_.async_shutdown(strand_.wrap(std::bind(
                        &Connection::on_shutdown, shared_from_this(),
                            std::placeholders::_1)));
                }
                if (ec)
                    return fail("read", ec);

                buf_.commit(bytes_transferred);
                buf_.consume(bytes_transferred);
                write(buf_, "BYE\n");
                boost::asio::async_write(stream_, buf_.data(), strand_.wrap(
                    std::bind(&Connection::on_write, shared_from_this(),
                        std::placeholders::_1,
                            std::placeholders::_2)));
            }

            void
            on_write(error_code ec, std::size_t bytes_transferred)
            {
                buf_.consume(bytes_transferred);
                if (ec)
                    return fail("write", ec);
                stream_.async_shutdown(strand_.wrap(std::bind(
                    &Connection::on_shutdown, shared_from_this(),
                        std::placeholders::_1)));
            }

            void
            on_shutdown(error_code ec)
            {
                if (ec)
                    return fail("shutdown", ec);
                socket_.close();
                timer_.cancel();
            }
        };

    public:
        explicit Server(short_read_test& test)
            : test_(test)
        {
            auto const p = std::make_shared<Acceptor>(*this);
            add(p);
            p->run();
        }

        ~Server()
        {
            close();
            wait();
        }

        endpoint_type const&
        endpoint () const
        {
            return endpoint_;
        }
    };

//——————————————————————————————————————————————————————————————
    class Client : public Base

    {
    private:
        short_read_test& test_;

        struct Connection
            : Child, std::enable_shared_from_this<Connection>
        {
            Client& client_;
            short_read_test& test_;
            socket_type socket_;
            stream_type stream_;
            strand_type strand_;
            timer_type timer_;
            boost::asio::streambuf buf_;
            endpoint_type const& ep_;

            Connection (Client& client, endpoint_type const& ep)
                : Child(client)
                , client_(client)
                , test_(client_.test_)
                , socket_(test_.io_service_)
                , stream_(socket_, *test_.context_)
                , strand_(socket_.get_io_service())
                , timer_(socket_.get_io_service())
                , ep_(ep)
            {
            }

            void
            close() override
            {
                if(! strand_.running_in_this_thread())
                    return strand_.post(std::bind(&Connection::close,
                        shared_from_this()));
                if (socket_.is_open())
                {
                    socket_.close();
                    timer_.cancel();
                }
            }

            void
            run(endpoint_type const& ep)
            {
                timer_.expires_from_now(std::chrono::seconds(3));
                timer_.async_wait(strand_.wrap(std::bind(&Connection::on_timer,
                    shared_from_this(), std::placeholders::_1)));
                socket_.async_connect(ep, strand_.wrap(std::bind(
                    &Connection::on_connect, shared_from_this(),
                        std::placeholders::_1)));
            }

            void
            fail (std::string const& what, error_code ec)
            {
                if (socket_.is_open())
                {
                    if (ec != boost::asio::error::operation_aborted)
                        test_.log << "[client] " << what <<
                            ": " << ec.message() << std::endl;
                    socket_.close();
                    timer_.cancel();
                }
            }

            void
            on_timer(error_code ec)
            {
                if (ec == boost::asio::error::operation_aborted)
                    return;
                if (ec)
                    return fail("timer", ec);
                test_.log << "[client] timeout";
                socket_.close();
            }

            void
            on_connect(error_code ec)
            {
                if (ec)
                    return fail("connect", ec);
                stream_.async_handshake(stream_type::client, strand_.wrap(
                    std::bind(&Connection::on_handshake, shared_from_this(),
                        std::placeholders::_1)));
            }

            void
            on_handshake(error_code ec)
            {
                if (ec)
                    return fail("handshake", ec);
                write(buf_, "HELLO\n");

#if 1
                boost::asio::async_write(stream_, buf_.data(), strand_.wrap(
                    std::bind(&Connection::on_write, shared_from_this(),
                        std::placeholders::_1,
                            std::placeholders::_2)));
#else
                stream_.async_shutdown(strand_.wrap(std::bind(
                    &Connection::on_shutdown, shared_from_this(),
                        std::placeholders::_1)));
#endif
            }

            void
            on_write(error_code ec, std::size_t bytes_transferred)
            {
                buf_.consume(bytes_transferred);
                if (ec)
                    return fail("write", ec);
#if 1
                boost::asio::async_read_until(stream_, buf_, "\n", strand_.wrap(
                    std::bind(&Connection::on_read, shared_from_this(),
                        std::placeholders::_1,
                            std::placeholders::_2)));
#else
                stream_.async_shutdown(strand_.wrap(std::bind(
                    &Connection::on_shutdown, shared_from_this(),
                        std::placeholders::_1)));
#endif
            }

            void
            on_read(error_code ec, std::size_t bytes_transferred)
            {
                if (ec)
                    return fail("read", ec);
                buf_.commit(bytes_transferred);
                stream_.async_shutdown(strand_.wrap(std::bind(
                    &Connection::on_shutdown, shared_from_this(),
                        std::placeholders::_1)));
            }

            void
            on_shutdown(error_code ec)
            {

                if (ec)
                    return fail("shutdown", ec);
                socket_.close();
                timer_.cancel();
            }
        };

    public:
        Client(short_read_test& test, endpoint_type const& ep)
            : test_(test)
        {
            auto const p = std::make_shared<Connection>(*this, ep);
            add(p);
            p->run(ep);
        }

        ~Client()
        {
            close();
            wait();
        }
    };

public:
    short_read_test()
        : work_(boost::in_place(std::ref(io_service_)))
        , thread_(std::thread([this]()
            {
                beast::setCurrentThreadName("io_service");
                this->io_service_.run();
            }))
        , context_(make_SSLContext(""))
    {
    }

    ~short_read_test()
    {
        work_ = boost::none;
        thread_.join();
    }

    void run() override
    {
        Server s(*this);
        Client c(*this, s.endpoint());
        c.wait();
        pass();
    }
};

BEAST_DEFINE_TESTSUITE(short_read,overlay,ripple);

}
