
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

#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/net/HTTPClient.h>
#include <ripple/net/AutoSocket.h>
#include <ripple/net/RegisterSSLCerts.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/regex.hpp>
#include <boost/optional.hpp>

namespace ripple {

//
//通过HTTP或HTTPS获取网页。
//

class HTTPClientSSLContext
{
public:
    explicit
    HTTPClientSSLContext (Config const& config, beast::Journal j)
        : m_context (boost::asio::ssl::context::sslv23)
        , verify_ (config.SSL_VERIFY)
    {
        boost::system::error_code ec;

        if (config.SSL_VERIFY_FILE.empty ())
        {
            registerSSLCerts(m_context, ec, j);

            if (ec && config.SSL_VERIFY_DIR.empty ())
                Throw<std::runtime_error> (
                    boost::str (boost::format (
                        "Failed to set_default_verify_paths: %s") %
                            ec.message ()));
        }
        else
        {
            m_context.load_verify_file (config.SSL_VERIFY_FILE);
        }

        if (! config.SSL_VERIFY_DIR.empty ())
        {
            m_context.add_verify_path (config.SSL_VERIFY_DIR, ec);

            if (ec)
                Throw<std::runtime_error> (
                    boost::str (boost::format (
                        "Failed to add verify path: %s") % ec.message ()));
        }
    }

    boost::asio::ssl::context& context()
    {
        return m_context;
    }

    bool sslVerify() const
    {
        return verify_;
    }

private:
    boost::asio::ssl::context m_context;
    bool verify_;
};

boost::optional<HTTPClientSSLContext> httpClientSSLContext;

void HTTPClient::initializeSSLContext (Config const& config, beast::Journal j)
{
    httpClientSSLContext.emplace (config, j);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class HTTPClientImp
    : public std::enable_shared_from_this <HTTPClientImp>
    , public HTTPClient
{
public:
    HTTPClientImp (boost::asio::io_service& io_service,
        const unsigned short port,
        std::size_t responseSize,
        beast::Journal& j)
        : mSocket (io_service, httpClientSSLContext->context ())
        , mResolver (io_service)
        , mHeader (maxClientHeaderBytes)
        , mPort (port)
        , mResponseSize (responseSize)
        , mDeadline (io_service)
        , j_ (j)
    {
        if (!httpClientSSLContext->sslVerify())
            mSocket.SSLSocket ().set_verify_mode (boost::asio::ssl::verify_none);
    }

//——————————————————————————————————————————————————————————————

    void makeGet (std::string const& strPath, boost::asio::streambuf& sb,
        std::string const& strHost)
    {
        std::ostream    osRequest (&sb);

        osRequest <<
                  "GET " << strPath << " HTTP/1.0\r\n"
                  "Host: " << strHost << "\r\n"
                  /*cept:*/*\r\n“//yyy是否需要此行？
                  “连接：关闭\r\n\r\n”；
    }

    //————————————————————————————————————————————————————————————————————————————————————————————————————————————————

    无效请求
        BOL BSSL
        std:：deque<std:：string>deqsites，
        std:：function<void（boost:：asio:：streambuf&sb，std:：string const&strhost）>build，
        std:：chrono:：seconds超时，
        std:：function<bool（const boost:：system:：error_code&ecresult，
            int-istatus，std:：string const&strdata）>完成）
    {
        mssl=bssl；
        mdeqsites=deqsites；
        mbuild=构建；
        mcomplete=完成；
        mtimeout=超时；

        HTTPSNEXT（）；
    }

    //————————————————————————————————————————————————————————————————————————————————————————————————————————————————

    空隙获取
        BOL BSSL
        std:：deque<std:：string>deqsites，
        std：：字符串const&strpath，
        std:：chrono:：seconds超时，
        std:：function<bool（const boost:：system:：error_code&ecresult，int istatus，
            std:：string const&strdata）>完成）
    {

        mcomplete=完成；
        mtimeout=超时；

        请求（
            BSSL
            DEQSPITE，
            std：：bind（&httpclientimp：：makeget，shared_from_this（），strpath，
                       标准：：占位符：：1，标准：：占位符：：2，
            超时，
            完成）；
    }

    //————————————————————————————————————————————————————————————————————————————————————————————————————————————————

    无效httpsnext（）
    {
        jLog（j_.trace（））<“fetch：”<<mdeqsites[0]；

        auto query=std:：make_shared<boost:：asio:：ip:：tcp:：resolver:：query>（）
                MDEQSPITE〔0〕；
                beast:：lexicalcast<std:：string>（mport），
                boost:：asio:：ip:：resolver_query_base:：numeric_service）；
        mquery=查询；

        mdeadline.expires_from_now（mtimeout，mshutdown）；

        jLog（j_.trace（））<“expires_from_now：”<<mshutdown.message（）；

        如果（！）M关机）
        {
            mdeadline.async_等待（
                STD：：绑定
                    &httpclientimp:：handleedeadline，
                    从这个（）共享了\u，
                    标准：：占位符：：_1））；
        }

        如果（！）M关机）
        {
            jLog（j_.trace（））<“解析：”<<mdeqsites[0]；

            mresolver.async_resolve（*mquery，
                                     STD：：绑定
                                         &httpclientimp：：handleresolve，
                                         从这个（）共享了\u，
                                         标准：：占位符：：1，
                                         标准：：占位符：：_2））；
        }

        如果（关闭）
            invokecomplete（mshutdown）；
    }

    void handleedeadline（const boost:：system:：error_code&ecresult）
    {
        if（ecresult==boost:：asio:：error:：operation_已中止）
        {
            //计时器已取消，因为不再需要最后期限。
            jLog（j_.trace（））<“Deadline Cancelled.”。

            //中止操作完成。
        }
        否则，如果（ecresult）
        {
            jLog（j_.trace（））<“截止时间错误：”<<mdeqsites[0]<“：”<<ecresult.message（）；

            //听不到任何声音。
            异常中止（）；
        }
        其他的
        {
            jLog（j_.trace（））<“截止日期已到。”；

            //将我们标记为关闭。
            //XXX使用我们自己的错误代码。
            mshutdown=boost:：system:：error_code（boost:：system:：errc:：bad_address，boost:：system:：system_category（））；

            //取消任何解析。
            mresolver.cancel（）；

            //停止事务。
            msocket.async_shutdown（std:：bind（
                                        &httpclientimp：：handleShutdown，
                                        从这个（）共享了\u，
                                        标准：：占位符：：_1））；

        }
    }

    空手柄输出（
        const boost：：系统：：错误代码和ECresult
    ）
    {
        如果（EcREST）
        {
            jLog（j_.trace（））<“关机错误：”<<mdeqsites[0]<“：”<<ecresult.message（）；
        }
    }

    无效的处理程序版本（
        const boost：：系统：：错误代码和ECresult，
        boost:：asio:：ip:：tcp:：resolver:：iterator itrendpoint
    ）
    {
        如果（！）M关机）
            mshutdown=ecresult；

        如果（关闭）
        {
            jLog（j_.trace（））<“解决错误：”<<mdeqsites[0]<“：”<<mshutdown.message（）；

            invokecomplete（mshutdown）；
        }
        其他的
        {
            jLog（j_.trace（））<“解决完毕。”；

            //如果我们要验证SSL连接，则需要
            //设置服务器名称指示*之前*的默认域
            / /连接
            if（httpclientsslcontext->sslverify（））
                msocket.settlshostname（mdeqsites[0]）；

            boost：：asio：：异步连接（
                msocket.lowest_层（），
                逆端点
                STD：：绑定
                    &httpclientimp：：handleconnect，
                    从这个（）共享了\u，
                    标准：：占位符：：_1））；
        }
    }

    无效的handleconnect（const boost:：system:：error_code&ecresult）
    {
        如果（！）M关机）
            mshutdown=ecresult；

        如果（关闭）
        {
            jLog（j_.trace（））<“连接错误：”<<mshutdown.message（）；
        }

        如果（！）M关机）
        {
            jLog（j_.trace（））<“Connected.”。

            if（httpclientsslcontext->sslverify（））
            {
                mshutdown=msocket.verify（mdeqsites[0]）；

                如果（关闭）
                {
                    jLog（j_.trace（））<“set_verify_callback：”<<mdeqsites[0]<<“：”<<mshutdown.message（）；
                }
            }
        }

        如果（关闭）
        {
            invokecomplete（mshutdown）；
        }
        否则如果（MSSL）
        {
            msocket.async_握手（
                自动锁定：：ssl_socket:：client，
                STD：：绑定
                    &httpclientimp：：handlerequest，
                    从这个（）共享了\u，
                    标准：：占位符：：_1））；
        }
        其他的
        {
            处理请求（ecresult）；
        }
    }

    void handlerequest（const boost:：system:：error_code&ecresult）
    {
        如果（！）M关机）
            mshutdown=ecresult；

        如果（关闭）
        {
            jLog（j_.trace（））<“握手错误：”<<mshutdown.message（）；

            invokecomplete（mshutdown）；
        }
        其他的
        {
            jLog（j_.trace（））<“会话已启动。”；

            mbuild（mrequest，mdeqsites[0]）；

            msocket.async_写入（
                请求，
                std：：bind（&httpclientimp：：handleWrite，
                             从这个（）共享了\u，
                             标准：：占位符：：1，
                             标准：：占位符：：_2））；
        }
    }

    void handlewrite（const boost:：system:：error_code&ecresult，std:：size_t bytes_transferred）
    {
        如果（！）M关机）
            mshutdown=ecresult；

        如果（关闭）
        {
            jLog（j_.trace（））<“写入错误：”<<mshutdown.message（）；

            invokecomplete（mshutdown）；
        }
        其他的
        {
            jLog（j_.trace（））<“written.”。

            msocket.async_read_until（
                报头器，
                “\r\n\r\n”，
                标准：：绑定（&httpclientimp:：handleheader，
                             从这个（）共享了\u，
                             标准：：占位符：：1，
                             标准：：占位符：：_2））；
        }
    }

    void handleheader（const boost:：system:：error_code&ecresult，std:：size_t bytes_transferred）
    {
        std:：string strheader（（std:：istreambuf_iterator<char>（&mheader）），std:：istreambuf_iterator<char>（））；
        jLog（j_.trace（））<“header:”“<<strheader<<”“\”“；

        static boost：：regex repressus（“\\`http/1\\s+（\\d 3）.*\\'”）；//http/1.1 200好的
        static boost：：regex resize（“\\`.*\\r\\n内容长度：\\s+（[0-9]+）.
        静态增强：：regex rebody（“\\`.*\\r\\n\\r\\n（.*）\\'”）；

        boost：：smatch smmatch；

        bool bmatch=boost:：regex_match（strheader，smmatch，repressus）；//匹配状态代码。

        如果（！）比赛）
        {
            //XXX使用我们自己的错误代码。
            jLog（j_.trace（））<“no status code”；
            invokeComplete（boost:：system:：error_code（boost:：system:：errc:：bad_address，boost:：system:：system_category（））；
            返回；
        }

        mstatus=beast:：lexicalcaststrow<int>（std:：string（smmatch[1]）；

        if（boost:：regex_match（strheader，smmatch，rebody））//我们有了一些身体
            mbody=smmatch[1]；

        if（boost：：regex_match（strheader，smmatch，resize））。
            mresponsesize=beast:：lexicalcaststrow<int>（std:：string（smmatch[1]））；

        如果（mresponsesize==0）
        {
            //没有需要或可用的尸体
            invokeComplete（ecresult，mstatus）；
        }
        else if（mbody.size（）>=mresponsesize）
        {
            我们得到了全部
            invokeComplete（ecresult、mstatus、mbody）；
        }
        其他的
        {
            msocket.async读取（
                mresponse.prepare（mresponse size-mbody.size（）），
                boost:：asio:：transfer_all（），
                标准：：绑定（&httpclientimp:：handledata，
                             从这个（）共享了\u，
                             标准：：占位符：：1，
                             标准：：占位符：：_2））；
        }
    }

    void handledata（const boost:：system:：error_code&ecresult，std:：size_t bytes_transferred）
    {
        如果（！）M关机）
            mshutdown=ecresult；

        如果（关闭和关闭！=boost：：asio：：错误：：eof）
        {
            jLog（j_.trace（））<“读取错误：”<<mshutdown.message（）；

            invokecomplete（mshutdown）；
        }
        其他的
        {
            如果（关闭）
            {
                jLog（j_.trace（））<“完成。”；
            }
            其他的
            {
                mresponse.commit（传输的字节数）；
                std:：string strbody（（std:：istreambuf_iterator<char>（&mresponse）），std:：istreambuf_iterator<char>（））；
                invokeComplete（ecresult，mstatus，mbody+strbody）；
            }
        }
    }

    //调用取消截止时间计时器并调用完成例程。
    void invokeComplete（const boost:：system:：error_code&ecresult，int istatus=0，std:：string const&strdata=）
    {
        boost:：system：：错误代码eccancel；

        （void）mdeadline.cancel（取消）；

        如果（EC取消）
        {
            jLog（j_.trace（））<“InvokeComplete:Deadline Cancel错误：”<<eccancel.message（）；
        }

        jLog（j_.debug（））<“invokeComplete:截止日期弹出：”<<mdeqsites.size（）；

        如果（！）mdeqsites.empty（））
        {
            mdeqsites.pop_front（）；
        }

        bool bagain=真；

        如果（mdeqsites.empty（）！EcREST）
        {
            //结果：0=有错误，最后一个条目
            //istatus:结果，如果没有错误
            //strdata:data，如果没有错误
            bagain=mcomplete和mcomplete（ecresult？ecresult：eccancel、istatus、strdata）；
        }

        如果（！）mdeqsites.empty（）&&bagain）
        {
            HTTPSNEXT（）；
        }
    }

私人：
    使用pointer=std:：shared_ptr<httpclient>；

    胸部MSSL；
    自动锁定msocket；
    boost:：asio:：ip:：tcp:：resolver mresolver；
    std:：shared_ptr<boost:：asio:：ip:：tcp:：resolver:：query>mquery；
    boost：：asio：：streambuf mrequest；
    boost：：asio：：streambuf mheader；
    boost：：asio：：streambuf mresponse；
    std：：字符串mbody；
    常量无符号短导入；
    内景：设计；
    内姆斯塔图斯；
    std:：function<void（boost:：asio:：streambuf&sb，std:：string const&strhost）>mbuild；
    std:：function<bool（const boost:：system:：error_code&ecresult，int istatus，std:：string const&strdata）>mcomplete；

    boost：：asio：：basic_waitable_timer<std：：chrono：：steady_clock>mdeadline；

    //如果不成功，我们将关闭。
    Boost：：System：：错误代码MShutdown；

    std:：deque<std:：string>mdeqsites；
    std：：chrono：：秒mtimeout；
    野兽杂志
}；

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void httpclient:：get（
    BOL BSSL
    boost:：asio:：io_服务和io_服务，
    std:：deque<std:：string>deqsites，
    const无符号短端口，
    std：：字符串const&strpath，
    标准：尺寸响应最大值，
    std:：chrono:：seconds超时，
    std:：function<bool（const boost:：system:：error_code&ecresult，int istatus，
        std:：string const&strdata）>完成，
    野兽：日记&J）
{
    auto-client=std:：make_shared<httpclientimp>（）
        IO服务，端口，响应最大值，j）；
    client->get（bssl，deqsites，strpath，timeout，complete）；
}

void httpclient:：get（
    BOL BSSL
    boost:：asio:：io_服务和io_服务，
    std：：字符串strsite，
    const无符号短端口，
    std：：字符串const&strpath，
    标准：尺寸响应最大值，
    std:：chrono:：seconds超时，
    std:：function<bool（const boost:：system:：error_code&ecresult，int istatus，
        std:：string const&strdata）>完成，
    野兽：日记&J）
{
    std:：deque<std:：string>deqsites（1，strsite）；

    auto-client=std:：make_shared<httpclientimp>（）
        IO服务，端口，响应最大值，j）；
    client->get（bssl，deqsites，strpath，timeout，complete）；
}

void httpclient:：请求（
    BOL BSSL
    boost:：asio:：io_服务和io_服务，
    std：：字符串strsite，
    const无符号短端口，
    std:：function<void（boost:：asio:：streambuf&sb，std:：string const&strhost）>setrequest，
    标准：尺寸响应最大值，
    std:：chrono:：seconds超时，
    std:：function<bool（const boost:：system:：error_code&ecresult，int istatus，
        std:：string const&strdata）>完成，
    野兽：日记&J）
{
    std:：deque<std:：string>deqsites（1，strsite）；

    auto-client=std:：make_shared<httpclientimp>（）
        IO服务，端口，响应最大值，j）；
    客户机->请求（bssl，deqsites，setrequest，timeout，complete）；
}

} /纹波
