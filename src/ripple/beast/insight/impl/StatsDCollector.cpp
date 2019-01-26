
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#include <ripple/beast/net/IPAddressConversion.h>
#include <ripple/beast/insight/HookImpl.h>
#include <ripple/beast/insight/CounterImpl.h>
#include <ripple/beast/insight/EventImpl.h>
#include <ripple/beast/insight/GaugeImpl.h>
#include <ripple/beast/insight/MeterImpl.h>
#include <ripple/beast/insight/StatsDCollector.h>
#include <ripple/beast/core/List.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/optional.hpp>
#include <cassert>
#include <climits>
#include <deque>
#include <functional>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

#ifndef BEAST_STATSDCOLLECTOR_TRACING_ENABLED
#define BEAST_STATSDCOLLECTOR_TRACING_ENABLED 0
#endif

namespace beast {
namespace insight {

namespace detail {

class StatsDCollectorImp;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class StatsDMetricBase : public List <StatsDMetricBase>::Node
{
public:
    virtual void do_process () = 0;
    virtual ~StatsDMetricBase() = default;
    StatsDMetricBase() = default;
    StatsDMetricBase(StatsDMetricBase const&) = delete;
    StatsDMetricBase& operator=(StatsDMetricBase const&) = delete;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class StatsDHookImpl
    : public HookImpl
    , public StatsDMetricBase
{
public:
    StatsDHookImpl (
        HandlerType const& handler,
            std::shared_ptr <StatsDCollectorImp> const& impl);

    ~StatsDHookImpl () override;

    void do_process () override;

private:
    StatsDHookImpl& operator= (StatsDHookImpl const&);

    std::shared_ptr <StatsDCollectorImp> m_impl;
    HandlerType m_handler;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class StatsDCounterImpl
    : public CounterImpl
    , public StatsDMetricBase
{
public:
    StatsDCounterImpl (std::string const& name,
        std::shared_ptr <StatsDCollectorImp> const& impl);

    ~StatsDCounterImpl () override;

    void increment (CounterImpl::value_type amount) override;

    void flush ();
    void do_increment (CounterImpl::value_type amount);
    void do_process () override;

private:
    StatsDCounterImpl& operator= (StatsDCounterImpl const&);

    std::shared_ptr <StatsDCollectorImp> m_impl;
    std::string m_name;
    CounterImpl::value_type m_value;
    bool m_dirty;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class StatsDEventImpl
    : public EventImpl
{
public:
    StatsDEventImpl (std::string const& name,
        std::shared_ptr <StatsDCollectorImp> const& impl);

    ~StatsDEventImpl () = default;

    void notify (EventImpl::value_type const& value) override;

    void do_notify (EventImpl::value_type const& value);
    void do_process ();

private:
    StatsDEventImpl& operator= (StatsDEventImpl const&);

    std::shared_ptr <StatsDCollectorImp> m_impl;
    std::string m_name;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class StatsDGaugeImpl
    : public GaugeImpl
    , public StatsDMetricBase
{
public:
    StatsDGaugeImpl (std::string const& name,
        std::shared_ptr <StatsDCollectorImp> const& impl);

    ~StatsDGaugeImpl () override;

    void set (GaugeImpl::value_type value) override;
    void increment (GaugeImpl::difference_type amount) override;

    void flush ();
    void do_set (GaugeImpl::value_type value);
    void do_increment (GaugeImpl::difference_type amount);
    void do_process () override;

private:
    StatsDGaugeImpl& operator= (StatsDGaugeImpl const&);

    std::shared_ptr <StatsDCollectorImp> m_impl;
    std::string m_name;
    GaugeImpl::value_type m_last_value;
    GaugeImpl::value_type m_value;
    bool m_dirty;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class StatsDMeterImpl
    : public MeterImpl
    , public StatsDMetricBase
{
public:
    explicit StatsDMeterImpl (std::string const& name,
        std::shared_ptr <StatsDCollectorImp> const& impl);

    ~StatsDMeterImpl () override;

    void increment (MeterImpl::value_type amount) override;

    void flush ();
    void do_increment (MeterImpl::value_type amount);
    void do_process () override;

private:
    StatsDMeterImpl& operator= (StatsDMeterImpl const&);

    std::shared_ptr <StatsDCollectorImp> m_impl;
    std::string m_name;
    MeterImpl::value_type m_value;
    bool m_dirty;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class StatsDCollectorImp
    : public StatsDCollector
    , public std::enable_shared_from_this <StatsDCollectorImp>
{
private:
    enum
    {
//最大数据包大小=484
        max_packet_size = 1472
    };

    Journal m_journal;
    IP::Endpoint m_address;
    std::string m_prefix;
    boost::asio::io_service m_io_service;
    boost::optional <boost::asio::io_service::work> m_work;
    boost::asio::io_service::strand m_strand;
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> m_timer;
    boost::asio::ip::udp::socket m_socket;
    std::deque <std::string> m_data;
    std::recursive_mutex metricsLock_;
    List <StatsDMetricBase> metrics_;

//必须排在最后才能执行init命令
    std::thread m_thread;

    static boost::asio::ip::udp::endpoint to_endpoint (
        IP::Endpoint const &ep)
    {
        return boost::asio::ip::udp::endpoint (ep.address(), ep.port());
    }

public:
    StatsDCollectorImp (
        IP::Endpoint const& address,
        std::string const& prefix,
        Journal journal)
        : m_journal (journal)
        , m_address (address)
        , m_prefix (prefix)
        , m_work (std::ref (m_io_service))
        , m_strand (m_io_service)
        , m_timer (m_io_service)
        , m_socket (m_io_service)
        , m_thread (&StatsDCollectorImp::run, this)
    {
    }

    ~StatsDCollectorImp () override
    {
        boost::system::error_code ec;
        m_timer.cancel (ec);

        m_work = boost::none;
        m_thread.join ();
    }

    Hook make_hook (HookImpl::HandlerType const& handler) override
    {
        return Hook (std::make_shared <detail::StatsDHookImpl> (
            handler, shared_from_this ()));
    }

    Counter make_counter (std::string const& name) override
    {
        return Counter (std::make_shared <detail::StatsDCounterImpl> (
            name, shared_from_this ()));
    }

    Event make_event (std::string const& name) override
    {
        return Event (std::make_shared <detail::StatsDEventImpl> (
            name, shared_from_this ()));
    }

    Gauge make_gauge (std::string const& name) override
    {
        return Gauge (std::make_shared <detail::StatsDGaugeImpl> (
            name, shared_from_this ()));
    }

    Meter make_meter (std::string const& name) override
    {
        return Meter (std::make_shared <detail::StatsDMeterImpl> (
            name, shared_from_this ()));
    }

//——————————————————————————————————————————————————————————————

    void add (StatsDMetricBase& metric)
    {
        std::lock_guard<std::recursive_mutex> _(metricsLock_);
        metrics_.push_back (metric);
    }

    void remove (StatsDMetricBase& metric)
    {
        std::lock_guard<std::recursive_mutex> _(metricsLock_);
        metrics_.erase (metrics_.iterator_to (metric));
    }

//——————————————————————————————————————————————————————————————

    boost::asio::io_service& get_io_service ()
    {
        return m_io_service;
    }

    std::string const& prefix () const
    {
        return m_prefix;
    }

    void do_post_buffer (std::string const& buffer)
    {
        m_data.emplace_back (buffer);
    }

    void post_buffer (std::string&& buffer)
    {
        m_io_service.dispatch (m_strand.wrap (std::bind (
            &StatsDCollectorImp::do_post_buffer, this,
                std::move (buffer))));
    }

//keepalive参数确保将缓冲区发送到
//boost：：asio：：async_send在调用完成之前不离开
    /*d on_send（std:：shared_ptr<std:：deque<std:：string>>/*keepalive*/，
                  boost:：system:：error_code ec，std:：size_t）
    {
        if（ec==boost:：asio:：error:：operation_已中止）
            返回；

        如果（EC）
        {
            if（auto-stream=m_journal.error（））
                流<“异步发送失败：”<<ec.message（）；
            返回；
        }
    }

    无效日志（std:：vector<boost:：asio:：const_buffer>const&buffers）
    {
        （空隙）缓冲器；
如果启用了Beast_StatsdCollector_Tracing_
        for（自动构造和缓冲区：缓冲区）
        {
            std：：string const s（boost：：asio：：buffer_cast<char const*>（buffer）），
                boost:：asio:：buffer_大小（buffer））；
            STR:：
        }
        std：：cerr<<'\n'；
第二节
    }

    //发送我们拥有的
    无效发送缓冲区（）
    {
        if（m_data.empty（））
            返回；

        //将字符串数组分解为块
        //每个都适合一个UDP包。
        / /
        std:：vector<boost:：asio:：const_buffer>缓冲区；
        buffers.reserve（m_data.size（））；
        标准：尺寸（0）；

        自动保留=
            std:：make_shared<std:：deque<std:：string>>（std:：move（m_data））；
        M_data.clear（）；

        for（自动构造和s:*keepalive）
        {
            std:：size_t const length（s.size（））；
            断言（！）S.空（））；
            如果（！）buffers.empty（）&&（大小+长度）>最大数据包大小）
            {
                日志（缓冲器）；
                m_socket.async_send（缓冲区，std:：bind（
                    &statsdcollectorimp：：发送时，此，保留，
                        标准：：占位符：：1，
                            标准：：占位符：：_2））；
                buffers.clear（）；
                尺寸＝0；
            }

            buffers.emplace_back（&s[0]，长度）；
            长度＝长度；
        }

        如果（！）buffers.empty（））
        {
            日志（缓冲器）；
            m_socket.async_send（缓冲区，std:：bind（
                &statsdcollectorimp：：发送时，此，保留，
                    标准：：占位符：：1，
                        标准：：占位符：：_2））；
        }
    }

    无效设置\计时器（）
    {
        使用名称空间std:：chrono-literals；
        m_timer.expires_from_now（1秒）；
        m_timer.async_wait（标准：：绑定（
            &statsdcollectorimp:：在计时器上，这，
                标准：：占位符：：_1））；
    }

    定时器无效（boost:：system:：error_code ec）
    {
        if（ec==boost:：asio:：error:：operation_已中止）
            返回；

        如果（EC）
        {
            if（auto-stream=m_journal.error（））
                流<“on_timer failed：”<<ec.message（）；
            返回；
        }

        std:：lock_guard<std:：recursive_mutex>（metricslock_uu）；

        for（auto&m：度量值）
            DO进程（）；

        发送缓冲区（）；

        StIt计时器（）；
    }

    空隙运行（）
    {
        boost：：系统：：错误代码EC；

        if（M_socket.connect（到_endpoint（M_address），EC））。
        {
            if（auto-stream=m_journal.error（））
                stream<“connect failed：”<<ec.message（）；
            返回；
        }

        StIt计时器（）；

        m_io_service.run（）；

        M插座关闭（
            boost:：asio:：ip:：udp:：socket:：shutdown_send，ec）；

        M_socket.close（）；

        m_io_service.poll（）；
    }
}；

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

statsdhookimpl:：statsdhookimpl（handlerType常量和handler，
    std:：shared&ptr<statsdcollectorimp>const&impl）
    MIIMPL（IMPL）
    ，M_处理程序（处理程序）
{
    m_impl->add（*this）；
}

Statsdhookimpl:：~Statsdhookimpl（）。
{
    m_impl->remove（*this）；
}

void statsdhookimpl:：do_process（）。
{
    M.汉德勒（）；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

statsdcounterimpl:：statsdcounterimpl（std:：string const&name，
    std:：shared&ptr<statsdcollectorimp>const&impl）
    MIIMPL（IMPL）
    ，Mname（名称）
    ，MyValk（0）
    ，M_脏（假）
{
    m_impl->add（*this）；
}

statsdcounterimpl:：~ statsdcounterimpl（）。
{
    m_impl->remove（*this）；
}

void statsdcounterimpl:：increment（counterImpl:：value_type amount）
{
    m_impl->get_io_service（）.dispatch（std:：bind（
        &statsdcounterimpl:：do_increment，，
            std:：static_pointer_cast<statsdcounterimpl>
                分享_来自_此（），金额））；
}

void statsdcounterimpl:：flush（）。
{
    如果（My脏）
    {
        m_dirty=假；
        std：：字符串流ss；
        SS <
            m_impl->prefix（）<“<<
            m_name<<“：”<<
            m_值<“c”<<
            “\n”；
        My值＝0；
        m_impl->post_buffer（ss.str（））；
    }
}

void statsdcounterimpl:：do_increment（counterImpl:：value_type amount）
{
    m_value+=金额；
    肮脏=真实；
}

void statsdcounterimpl:：do_process（）。
{
    冲洗（）；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

statsdeventimpl:：statsdeventimpl（std:：string const&name，
    std:：shared&ptr<statsdcollectorimp>const&impl）
    MIIMPL（IMPL）
    ，Mname（名称）
{
}

void statsdeventimpl:：notify（eventimpl:：value_type const&value）
{
    m_impl->get_io_service（）.dispatch（std:：bind（
        &statsdeventimpl：：不通知，
            std：：静态指针强制转换
                分享了“来自”这个（），值））；
}

void statsdeventimpl:：do_notify（eventimpl:：value_type const&value）
{
    std：：字符串流ss；
    SS <
        m_impl->prefix（）<“<<
        m_name<<“：”<<
        value.count（）<“ms”<<
        “\n”；
    m_impl->post_buffer（ss.str（））；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

statsdgaugeimpl:：statsdgaugeimpl（std:：string const&name，
    std:：shared&ptr<statsdcollectorimp>const&impl）
    MIIMPL（IMPL）
    ，Mname（名称）
    ，m_Last_值（0）
    ，MyValk（0）
    ，M_脏（假）
{
    m_impl->add（*this）；
}

statsdgaugeimpl:：~ statsdgaugeimpl（）。
{
    m_impl->remove（*this）；
}

void statsdgaugeimpl:：set（gaugeimpl:：value_type value）
{
    m_impl->get_io_service（）.dispatch（std:：bind（
        &statsdgaugeimpl：：进行设置，
            std：：静态指针强制转换
                分享了“来自”这个（），值））；
}

void statsdgaugeimpl:：increment（gaugeimpl:：difference_type amount）
{
    m_impl->get_io_service（）.dispatch（std:：bind（
        &statsdgaugeimpl：：做增量，
            std：：静态指针强制转换
                分享_来自_此（），金额））；
}

void statsdgaugeimpl:：flush（）。
{
    如果（My脏）
    {
        m_dirty=假；
        std：：字符串流ss；
        SS <
            m_impl->prefix（）<“<<
            m_name<<“：”<<
            m_值<“c”<<
            “\n”；
        m_impl->post_buffer（ss.str（））；
    }
}

void statsdgaugeimpl:：do_set（gaugeimpl:：value_type value）
{
    m_值=值；

    如果（My值）！= Ma-拉斯特值
    {
        m_last_value=m_value；
        肮脏=真实；
    }
}

void statsdgaugeimpl:：do_increment（gaugeimpl:：difference_type amount）
{
    gaugeimpl：：值_型值（m_值）；

    如果（数量＞0）
    {
        gaugeimpl:：value_type const d（
            static_cast<gaugeimpl:：value_type>（amount））；
        值+=
            （d>=std:：numeric_limits<gaugeimpl:：value_type>：：max（）-m_value）
            ？std:：numeric_limits<gaugeimpl:：value_type>：：max（）-m_value
            ：D；
    }
    否则，如果（金额<0）
    {
        gaugeimpl:：value_type const d（
            static_cast<gaugeimpl:：value_type>（-amount））；
        值=（d>=值）？0：价值D；
    }

    DoSET（值）；
}

void statsdgaugeimpl:：do_process（）。
{
    冲洗（）；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

statsdmeterImpl:：statsdmeterImpl（std:：string const&name，
    std:：shared&ptr<statsdcollectorimp>const&impl）
    MIIMPL（IMPL）
    ，Mname（名称）
    ，MyValk（0）
    ，M_脏（假）
{
    m_impl->add（*this）；
}

StatsdmeterImpl:：~ StatsdmeterImpl（）。
{
    m_impl->remove（*this）；
}

void statsdmeterimpl:：increment（meterimpl:：value_type amount）
{
    m_impl->get_io_service（）.dispatch（std:：bind（
        &statsdmeterImpl:：do_increment，，
            std：：静态指针强制转换
                分享_来自_此（），金额））；
}

void statsdmeterImpl:：flush（）。
{
    如果（My脏）
    {
        m_dirty=假；
        std：：字符串流ss；
        SS <
            m_impl->prefix（）<“<<
            m_name<<“：”<<
            m_值<“m”<<
            “\n”；
        My值＝0；
        m_impl->post_buffer（ss.str（））；
    }
}

void statsdmeterImpl:：do_increment（meterimpl:：value_type amount）
{
    m_value+=金额；
    肮脏=真实；
}

void statsdmeterImpl:：do_process（）。
{
    冲洗（）；
}

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std:：shared&ptr<statsdcollector>statsdcollector:：new（
    IP:：endpoint const&address，std:：string const&prefix，日记账）
{
    返回std:：make_shared<detail:：statsdcollectorimp>（）
        地址、前缀、日记）；
}

}
}
