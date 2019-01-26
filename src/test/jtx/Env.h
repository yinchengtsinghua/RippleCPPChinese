
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

#ifndef RIPPLE_TEST_JTX_ENV_H_INCLUDED
#define RIPPLE_TEST_JTX_ENV_H_INCLUDED

#include <test/jtx/Account.h>
#include <test/jtx/amount.h>
#include <test/jtx/envconfig.h>
#include <test/jtx/JTx.h>
#include <test/jtx/require.h>
#include <test/jtx/tags.h>
#include <test/jtx/AbstractClient.h>
#include <test/jtx/ManualTimeKeeper.h>
#include <test/unit_test/SuiteJournal.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/paths/Pathfinder.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/json/json_value.h>
#include <ripple/json/to_string.h>
#include <ripple/ledger/CachedSLEs.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/Issue.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/STTx.h>
#include <boost/beast/core/detail/type_traits.hpp>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <unordered_map>
#include <vector>


namespace ripple {
namespace test {
namespace jtx {

/*在env:：fund中将帐户指定为“无波动”*/
template <class... Args>
std::array<Account, 1 + sizeof...(Args)>
noripple (Account const& account, Args const&... args)
{
    return {{account, args...}};
}

inline
FeatureBitset
supported_amendments()
{
    static const FeatureBitset ids = []{
        auto const& sa = ripple::detail::supportedAmendments();
        std::vector<uint256> feats;
        feats.reserve(sa.size());
        for (auto const& s : sa)
        {
            if (auto const f = getRegisteredFeature(s))
                feats.push_back(*f);
            else
                Throw<std::runtime_error> ("Unknown feature: " + s + "  in supportedAmendments.");
        }
        return FeatureBitset(feats);
    }();
    return ids;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class SuiteLogs : public Logs
{
    beast::unit_test::suite& suite_;

public:
    explicit
    SuiteLogs(beast::unit_test::suite& suite)
        : Logs (beast::severities::kError)
        , suite_(suite)
    {
    }

    ~SuiteLogs() override = default;

    std::unique_ptr<beast::Journal::Sink>
    makeSink(std::string const& partition,
        beast::severities::Severity threshold) override
    {
        return std::make_unique<SuiteJournalSink>(
            partition, threshold, suite_);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*事务测试环境。*/
class Env
{
public:
    beast::unit_test::suite& test;

    Account const& master = Account::master;

private:
    struct AppBundle
    {
        Application* app;
        std::unique_ptr<Application> owned;
        ManualTimeKeeper* timeKeeper;
        std::thread thread;
        std::unique_ptr<AbstractClient> client;

        AppBundle (beast::unit_test::suite& suite,
            std::unique_ptr<Config> config,
            std::unique_ptr<Logs> logs);
        ~AppBundle();
    };

    AppBundle bundle_;

public:
    beast::Journal const journal;

    Env() = delete;
    Env& operator= (Env const&) = delete;
    Env (Env const&) = delete;

    /*
     *@brief使用suite、config指针和显式功能创建env。
     *
     *此构造函数将创建具有指定配置的env
     *并获得所传递的配置指针的所有权。将启用功能
     *根据下面描述的规则（见下一个构造函数）。
     *
     *@param suite_u当前单元测试：：suite
     *@param config移动将获得所需的配置所有权
     *指针。有关常见的配置调整，请参见envconfig和相关函数。
     *@param args with_only_features（）to显式启用或
     *支持_features_except（）以启用和禁用所有特定功能。
     **/

//vfalc可以在这里将suite:：log包装在日志中
    Env (beast::unit_test::suite& suite_,
            std::unique_ptr<Config> config,
            FeatureBitset features,
            std::unique_ptr<Logs> logs = nullptr)
        : test (suite_)
        , bundle_ (
            suite_,
            std::move(config),
            logs ? std::move(logs) : std::make_unique<SuiteLogs>(suite_))
        , journal {bundle_.app->journal ("Env")}
    {
        memoize(Account::master);
        Pathfinder::initPathTable();
        foreachFeature(
            features, [& appFeats = app().config().features](uint256 const& f) {
                appFeats.insert(f);
            });
    }

    /*
     *@brief用默认配置创建env并指定
     *特征。
     *
     *此构造函数将使用标准env配置创建一个env
     （来自envconfig（））和显式指定的功能。使用
     *使用“仅限您的功能”（…）或支持的“功能”（…）创建
     *适合在此传递的功能集合。
     *
     *@param suite_u当前单元测试：：suite
     *@param args功能集合
     *
     **/

    Env (beast::unit_test::suite& suite_,
            FeatureBitset features)
        : Env(suite_, envconfig(), features)
    {
    }

    /*
     *@brief使用suite和config指针创建env。
     *
     *此构造函数将创建具有指定配置的env
     *并获得所传递的配置指针的所有权。所有支持的修订
     *由此版本的构造函数启用。
     *
     *@param suite_u当前单元测试：：suite
     *@param config移动将获得所需的配置所有权
     *指针。有关常见的配置调整，请参见envconfig和相关函数。
     **/

    Env (beast::unit_test::suite& suite_,
        std::unique_ptr<Config> config,
        std::unique_ptr<Logs> logs = nullptr)
        : Env(suite_, std::move(config),
            supported_amendments(), std::move(logs))
    {
    }

    /*
     *@brief只使用当前测试套件创建env
     *
     *此构造函数将使用标准创建一个env
     *测试env配置（来自envconfig（））和所有支持的
     *已启用修订。
     *
     *@param suite_u当前单元测试：：suite
     **/

    Env (beast::unit_test::suite& suite_)
        : Env(suite_, envconfig())
    {
    }

    virtual ~Env() = default;

    Application&
    app()
    {
        return *bundle_.app;
    }

    Application const&
    app() const
    {
        return *bundle_.app;
    }

    ManualTimeKeeper&
    timeKeeper()
    {
        return *bundle_.timeKeeper;
    }

    /*返回当前Ripple网络时间

        @注意：当分类帐
              关闭或由呼叫者关闭。
    **/

    NetClock::time_point
    now()
    {
        return timeKeeper().now();
    }

    /*返回连接的客户端。*/
    AbstractClient&
    client()
    {
        return *bundle_.client;
    }

    /*执行rpc命令。

        该命令被检查并用于生成
        根据参数设置正确的JSON。
    **/

    template<class... Args>
    Json::Value
    rpc(std::string const& cmd, Args&&... args);

    /*返回当前分类帐。

        这是不可修改的
        呼叫时打开分类帐。
        调用open（）后应用的事务
        将不可见。

    **/

    std::shared_ptr<OpenView const>
    current() const
    {
        return app().openLedger().current();
    }

    /*返回上次关闭的分类帐。

        打开的分类帐建立在
        上次关闭的分类帐。当未结分类帐
        已关闭，它将成为新的已关闭分类帐
        一个新的未结分类账取代了它。
    **/

    std::shared_ptr<ReadView const>
    closed();

    /*关闭并推进分类帐。

        结果关闭时间将不同，并且
        大于上一个关闭时间，并且在或
        经过了很近的时间。

        影响：

            从上一个分类帐创建新的已关闭分类帐
            封闭分类帐

            所有使其进入打开状态的交易
            分类帐应用于已关闭的分类帐。

            应用程序网络时间设置为
            结果分类帐的结束时间。
    **/

    void
    close (NetClock::time_point closeTime,
        boost::optional<std::chrono::milliseconds> consensusDelay = boost::none);

    /*关闭并推进分类帐。

        时间计算为
        上一个分类帐结算时间。
    **/

    template <class Rep, class Period>
    void
    close (std::chrono::duration<
        Rep, Period> const& elapsed)
    {
//这是正确的时间吗？
        close (now() + elapsed);
    }

    /*关闭并推进分类帐。

        时间计算为5秒，从
        上一个分类帐结算时间。
    **/

    void
    close()
    {
//这是正确的时间吗？
        close (std::chrono::seconds(5));
    }

    /*打开JSON跟踪。
        不带参数，全部跟踪
    **/

    void
    trace (int howMany = -1)
    {
        trace_ = howMany;
    }

    /*关闭JSON跟踪。*/
    void
    notrace ()
    {
        trace_ = 0;
    }

    /*关闭签名检查。*/
    void
    disable_sigs()
    {
        app().checkSigs(false);
    }

    /*将accountid与account关联。*/
    void
    memoize (Account const& account);

    /*返回给定accountID的帐户。*/
    /*@ {*/
    Account const&
    lookup (AccountID const& id) const;

    Account const&
    lookup (std::string const& base58ID) const;
    /*@ }*/

    /*返回帐户的xrp余额。
        如果帐户不存在，则返回0。
    **/

    PrettyAmount
    balance (Account const& account) const;

    /*返回帐户的下一个序列号。
        例外情况：
            如果帐户不存在则引发
    **/

    std::uint32_t
    seq (Account const& account) const;

    /*把帐户上的余额退回。
        如果信任行不存在，则返回0。
    **/

//vfalc注：这应返回单位减去金额
    PrettyAmount
    balance (Account const& account,
        Issue const& issue) const;

    /*返回帐户根。
        @如果帐户不存在，返回空值。
    **/

    std::shared_ptr<SLE const>
    le (Account const& account) const;

    /*返回分类帐条目。
        @如果分类账分录不存在，返回空值
    **/

    std::shared_ptr<SLE const>
    le (Keylet const& k) const;

    /*从参数创建JTX。*/
    template <class JsonValue,
        class... FN>
    JTx
    jt (JsonValue&& jv, FN const&... fN)
    {
        JTx jt(std::forward<JsonValue>(jv));
        invoke(jt, fN...);
        autofill(jt);
        jt.stx = st(jt);
        return jt;
    }

    /*从参数创建JSON。
        这将应用函数和自动填充。
    **/

    template <class JsonValue,
        class... FN>
    Json::Value
    json (JsonValue&&jv, FN const&... fN)
    {
        auto tj = jt(
            std::forward<JsonValue>(jv),
                fN...);
        return std::move(tj.jv);
    }

    /*检查一组要求。

        形成了要求
        从条件函数。
    **/

    template <class... Args>
    void
    require (Args const&... args)
    {
        jtx::required(args...)(*this);
    }

    /*从rpc json result对象获取ter结果和'didapply'标志。
    **/

    static
    std::pair<TER, bool>
    parseResult(Json::Value const& jr);

    /*提交现有的JTX。
        这称为后置条件。
    **/

    virtual
    void
    submit (JTx const& jt);

    /*将submit rpc命令与提供的jtx对象一起使用。
        这称为后置条件。
    **/

    void
    sign_and_submit(JTx const& jt, Json::Value params = Json::nullValue);

    /*检查预期的后置条件
        JTX提交。
    **/

    void
    postconditions(JTx const& jt, TER ter, bool didApply);

    /*应用函数并提交。*/
    /*@ {*/
    template <class JsonValue, class... FN>
    void
    apply (JsonValue&& jv, FN const&... fN)
    {
        submit(jt(std::forward<
            JsonValue>(jv), fN...));
    }

    template <class JsonValue,
        class... FN>
    void
    operator()(JsonValue&& jv, FN const&... fN)
    {
        apply(std::forward<
            JsonValue>(jv), fN...);
    }
    /*@ }*/

    /*返回最后一个JTX的TER。*/
    TER
    ter() const
    {
        return ter_;
    }

    /*返回最后一个JTX的元数据。

        影响：

            打开的分类帐被关闭，如同通过呼叫
            关闭（）。最后一个元数据
            返回事务ID（如果有）。
    **/

    std::shared_ptr<STObject const>
    meta();

    /*返回最后一个JTX的Tx数据。

        影响：

            上一个事务的Tx数据
            如果有ID，则返回。无边
            影响。

        @注：仅需提交JTX
            通过签字和提交方式。
    **/

    std::shared_ptr<STTx const>
    tx() const;

    void
    enableFeature(uint256 const feature);

private:
    void
    fund (bool setDefaultRipple,
        STAmount const& amount,
            Account const& account);

//如果你在这里出错，这意味着
//你在打没有账户的基金
    inline
    void
    fund (STAmount const&)
    {
    }

    void
    fund_arg (STAmount const& amount,
        Account const& account)
    {
        fund (true, amount, account);
    }

    template <std::size_t N>
    void
    fund_arg (STAmount const& amount,
        std::array<Account, N> const& list)
    {
        for (auto const& account : list)
            fund (false, amount, account);
    }
public:

    /*用一些xrp创建一个新帐户。

        这些便利功能便于设置
        在环境方面，他们绕过了费用、序列和SIG
        设置。XRP从主机传输
        帐户。

        先决条件：
            帐户不能已经存在

        影响：
            已设置帐户上的asfDefaultRipple，
            序列号递增，除非
            帐户已通过呼叫Noripple进行包装。

            帐户的xrp余额设置为amount。

            生成设置平衡的测试。

        @param amount要传输到的xrp数量
                      每个帐户。

        @param args要资助的帐户的异类列表
                    或者打电话给诺里普尔
                    资助。
    **/

    template<class Arg, class... Args>
    void
    fund (STAmount const& amount,
        Arg const& arg, Args const&... args)
    {
        fund_arg (amount, arg);
        fund (amount, args...);
    }

    /*建立信任关系。

        这些便利功能便于设置
        在环境方面，他们绕过了费用、序列和SIG
        设置。

        先决条件：
            帐户必须已经存在

        影响：
            为帐户添加信任行。
            帐户的序列号递增。
            账户已退还交易费
                设置信任线。

        退款来自主账户。
    **/

    /*@ {*/
    void
    trust (STAmount const& amount,
        Account const& account);

    template<class... Accounts>
    void
    trust (STAmount const& amount, Account const& to0,
        Account const& to1, Accounts const&... toN)
    {
        trust(amount, to0);
        trust(amount, to1, toN...);
    }
    /*@ }*/

protected:
    int trace_ = 0;
    TestStopwatch stopwatch_;
    uint256 txid_;
    TER ter_ = tesSUCCESS;

    Json::Value
    do_rpc(std::vector<std::string> const& args);

    void
    autofill_sig (JTx& jt);

    virtual
    void
    autofill (JTx& jt);

    /*从JTX创建STTX
        框架要求JSON有效。
        在分析错误时，将记录JSON并
        引发异常。
        投掷：
            副错误
    **/

    std::shared_ptr<STTx const>
    st (JTx const& jt);

    inline
    void
    invoke (STTx& stx)
    {
    }

    template <class F>
    inline
    void
    maybe_invoke (STTx& stx, F const& f,
        std::false_type)
    {
    }

    template <class F>
    void
    maybe_invoke (STTx& stx, F const& f,
        std::true_type)
    {
        f(*this, stx);
    }

//在stx上调用函数
//注：STTX不能修改
    template <class F, class... FN>
    void
    invoke (STTx& stx, F const& f,
        FN const&... fN)
    {
        maybe_invoke(stx, f,
            boost::beast::detail::is_invocable<F,
                void(Env&, STTx const&)>());
        invoke(stx, fN...);
    }

    inline
    void
    invoke (JTx&)
    {
    }

    template <class F>
    inline
    void
    maybe_invoke (JTx& jt, F const& f,
        std::false_type)
    {
    }

    template <class F>
    void
    maybe_invoke (JTx& jt, F const& f,
        std::true_type)
    {
        f(*this, jt);
    }

//在jt上调用函数
    template <class F, class... FN>
    void
    invoke (JTx& jt, F const& f,
        FN const&... fN)
    {
        maybe_invoke(jt, f,
            boost::beast::detail::is_invocable<F,
                void(Env&, JTx&)>());
        invoke(jt, fN...);
    }

//帐户ID到帐户的映射
    std::unordered_map<
        AccountID, Account> map_;
};

template<class... Args>
Json::Value
Env::rpc(std::string const& cmd, Args&&... args)
{
    std::vector<std::string> vs{cmd,
        std::forward<Args>(args)...};
    return do_rpc(vs);
}

} //JTX
} //测试
} //涟漪

#endif
