
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

#include <ripple/basics/ResolverAsio.h>
#include <ripple/basics/Log.h>
#include <ripple/beast/net/IPAddressConversion.h>
#include <ripple/beast/net/IPEndpoint.h>
#include <boost/asio.hpp>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <deque>
#include <locale>
#include <memory>
#include <mutex>

namespace ripple {

/*当所有挂起的I/O完成时，混合跟踪。
    派生类必须可以使用此签名调用：
        void AsyncHandlersComplete（））
**/

template <class Derived>
class AsyncObject
{
protected:
    AsyncObject ()
        : m_pending (0)
    { }

public:
    ~AsyncObject ()
    {
//正在销毁I/O挂起的对象？出口不干净！
        assert (m_pending.load () == 0);
    }

    /*保存待处理I/O计数的RAII容器。
        将它绑定到每个传递的处理程序的参数列表中
        启动功能。
    **/

    class CompletionCounter
    {
    public:
        explicit CompletionCounter (Derived* owner)
            : m_owner (owner)
        {
            ++m_owner->m_pending;
        }

        CompletionCounter (CompletionCounter const& other)
            : m_owner (other.m_owner)
        {
            ++m_owner->m_pending;
        }

        ~CompletionCounter ()
        {
            if (--m_owner->m_pending == 0)
                m_owner->asyncHandlersComplete ();
        }

        CompletionCounter& operator= (CompletionCounter const&) = delete;

    private:
        Derived* m_owner;
    };

    void addReference ()
    {
        ++m_pending;
    }

    void removeReference ()
    {
        if (--m_pending == 0)
            (static_cast<Derived*>(this))->asyncHandlersComplete();
    }

private:
//挂起的处理程序数。
    std::atomic <int> m_pending;
};

class ResolverAsioImpl
    : public ResolverAsio
    , public AsyncObject <ResolverAsioImpl>
{
public:
    using HostAndPort = std::pair <std::string, std::string>;

    beast::Journal m_journal;

    boost::asio::io_service& m_io_service;
    boost::asio::io_service::strand m_strand;
    boost::asio::ip::tcp::resolver m_resolver;

    std::condition_variable m_cv;
    std::mutex              m_mut;
    bool m_asyncHandlersCompleted;

    std::atomic <bool> m_stop_called;
    std::atomic <bool> m_stopped;

//表示解析程序要执行的工作单元
    struct Work
    {
        std::vector <std::string> names;
        HandlerType handler;

        template <class StringSequence>
        Work (StringSequence const& names_, HandlerType const& handler_)
            : handler (handler_)
        {
            names.reserve(names_.size ());

            std::reverse_copy (names_.begin (), names_.end (),
                std::back_inserter (names));
        }
    };

    std::deque <Work> m_work;

    ResolverAsioImpl (boost::asio::io_service& io_service,
        beast::Journal journal)
            : m_journal (journal)
            , m_io_service (io_service)
            , m_strand (io_service)
            , m_resolver (io_service)
            , m_asyncHandlersCompleted (true)
            , m_stop_called (false)
            , m_stopped (true)
    {

    }

    ~ResolverAsioImpl () override
    {
        assert (m_work.empty ());
        assert (m_stopped);
    }

//————————————————————————————————————————————————————————————————
//异步对象
    void asyncHandlersComplete()
    {
        std::unique_lock<std::mutex> lk{m_mut};
        m_asyncHandlersCompleted = true;
        m_cv.notify_all();
    }

//——————————————————————————————————————————————————————————————
//
//旋转变压器
//
//——————————————————————————————————————————————————————————————

    void start () override
    {
        assert (m_stopped == true);
        assert (m_stop_called == false);

        if (m_stopped.exchange (false) == true)
        {
            {
                std::lock_guard<std::mutex> lk{m_mut};
                m_asyncHandlersCompleted = false;
            }
            addReference ();
        }
    }

    void stop_async () override
    {
        if (m_stop_called.exchange (true) == false)
        {
            m_io_service.dispatch (m_strand.wrap (std::bind (
                &ResolverAsioImpl::do_stop,
                    this, CompletionCounter (this))));

            JLOG(m_journal.debug()) << "Queued a stop request";
        }
    }

    void stop () override
    {
        stop_async ();

        JLOG(m_journal.debug()) << "Waiting to stop";
        std::unique_lock<std::mutex> lk{m_mut};
        m_cv.wait(lk, [this]{return m_asyncHandlersCompleted;});
        lk.unlock();
        JLOG(m_journal.debug()) << "Stopped";
    }

    void resolve (
        std::vector <std::string> const& names,
        HandlerType const& handler) override
    {
        assert (m_stop_called == false);
        assert (m_stopped == true);
        assert (!names.empty());

//todo nikb使用右值引用来构造和移动
//降低成本。
        m_io_service.dispatch (m_strand.wrap (std::bind (
            &ResolverAsioImpl::do_resolve, this,
                names, handler, CompletionCounter (this))));
    }

//————————————————————————————————————————————————————————————————
//旋转变压器
    void do_stop (CompletionCounter)
    {
        assert (m_stop_called == true);

        if (m_stopped.exchange (true) == false)
        {
            m_work.clear ();
            m_resolver.cancel ();

            removeReference ();
        }
    }

    void do_finish (
        std::string name,
        boost::system::error_code const& ec,
        HandlerType handler,
        boost::asio::ip::tcp::resolver::iterator iter,
        CompletionCounter)
    {
        if (ec == boost::asio::error::operation_aborted)
            return;

        std::vector <beast::IP::Endpoint> addresses;

//如果我们收到错误信息，我们不会返回任何
//我们可能得到的结果。
        if (!ec)
        {
            while (iter != boost::asio::ip::tcp::resolver::iterator())
            {
                addresses.push_back (beast::IPAddressConversion::from_asio (*iter));
                ++iter;
            }
        }

        handler (name, addresses);

        m_io_service.post (m_strand.wrap (std::bind (
            &ResolverAsioImpl::do_work, this,
                CompletionCounter (this))));
    }

    HostAndPort parseName(std::string const& str)
    {
//第一次尝试解析为端点（IP地址+端口）。
//如果不成功，返回到通用名称+端口分析

        auto result {beast::IP::Endpoint::from_string_checked (str)};
        if (result.second)
        {
            return make_pair (
                result.first.address().to_string(),
                std::to_string(result.first.port()));
        }

//通用名称/端口分析，不适用于
//尤其是IPv6地址，因为它考虑冒号
//端口分隔符

//尝试查找第一个和最后一个非空白
        auto const find_whitespace = std::bind (
            &std::isspace <std::string::value_type>,
            std::placeholders::_1,
            std::locale ());

        auto host_first = std::find_if_not (
            str.begin (), str.end (), find_whitespace);

        auto port_last = std::find_if_not (
            str.rbegin (), str.rend(), find_whitespace).base();

//这只应发生在所有空白字符串中
        if (host_first >= port_last)
            return std::make_pair(std::string (), std::string ());

//尝试查找第一个和最后一个有效的端口分隔符
        auto const find_port_separator = [](char const c) -> bool
        {
            if (std::isspace (static_cast<unsigned char>(c)))
                return true;

            if (c == ':')
                return true;

            return false;
        };

        auto host_last = std::find_if (
            host_first, port_last, find_port_separator);

        auto port_first = std::find_if_not (
            host_last, port_last, find_port_separator);

        return make_pair (
            std::string (host_first, host_last),
            std::string (port_first, port_last));
    }

    void do_work (CompletionCounter)
    {
        if (m_stop_called == true)
            return;

//我们现在没有工作要做
        if (m_work.empty ())
            return;

        std::string const name (m_work.front ().names.back());
        HandlerType handler (m_work.front ().handler);

        m_work.front ().names.pop_back ();

        if (m_work.front ().names.empty ())
            m_work.pop_front();

        HostAndPort const hp (parseName (name));

        if (hp.first.empty ())
        {
            JLOG(m_journal.error()) <<
                "Unable to parse '" << name << "'";

            m_io_service.post (m_strand.wrap (std::bind (
                &ResolverAsioImpl::do_work, this,
                CompletionCounter (this))));

            return;
        }

        boost::asio::ip::tcp::resolver::query query (
            hp.first, hp.second);

        m_resolver.async_resolve (query, std::bind (
            &ResolverAsioImpl::do_finish, this, name,
                std::placeholders::_1, handler,
                    std::placeholders::_2,
                        CompletionCounter (this)));
    }

    void do_resolve (std::vector <std::string> const& names,
        HandlerType const& handler, CompletionCounter)
    {
        assert (! names.empty());

        if (m_stop_called == false)
        {
            m_work.emplace_back (names, handler);

            JLOG(m_journal.debug()) <<
                "Queued new job with " << names.size() <<
                " tasks. " << m_work.size() << " jobs outstanding.";

            if (m_work.size() > 0)
            {
                m_io_service.post (m_strand.wrap (std::bind (
                    &ResolverAsioImpl::do_work, this,
                        CompletionCounter (this))));
            }
        }
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::unique_ptr<ResolverAsio> ResolverAsio::New (
    boost::asio::io_service& io_service,
    beast::Journal journal)
{
    return std::make_unique<ResolverAsioImpl> (io_service, journal);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
Resolver::~Resolver() = default;
}
