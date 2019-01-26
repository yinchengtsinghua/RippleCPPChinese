
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

#include <ripple/app/tx/impl/Taker.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/core/LexicalCast.h>
#include <type_traits>

namespace ripple {

class Taker_test : public beast::unit_test::suite
{
    static bool const Buy = false;
    static bool const Sell = true;

    class TestTaker
        : public BasicTaker
    {
        STAmount funds_;
        STAmount cross_funds;

    public:
        TestTaker (
                CrossType cross_type,
                Amounts const& amount,
                Quality const& quality,
                STAmount const& funds,
                std::uint32_t flags,
                Rate const& rate_in,
                Rate const& rate_out)
            : BasicTaker (
                cross_type,
                AccountID(0x4701),
                amount,
                quality,
                flags,
                rate_in,
                rate_out)
            , funds_ (funds)
        {
        }

        void
        set_funds (STAmount const& funds)
        {
            cross_funds = funds;
        }

        STAmount
        get_funds (AccountID const& owner, STAmount const& funds) const override
        {
            if (owner == account ())
                return funds_;

            return cross_funds;
        }

        Amounts
        cross (Amounts offer, Quality quality)
        {
            if (reject (quality))
                return Amounts (offer.in.zeroed (), offer.out.zeroed ());

//我们需要模仿“无资金提供”的行为
            if (get_funds (AccountID (0x4702), offer.out) == beast::zero)
                return Amounts (offer.in.zeroed (), offer.out.zeroed ());

            if (done ())
                return Amounts (offer.in.zeroed (), offer.out.zeroed ());

            auto result = do_cross (offer, quality, AccountID (0x4702));

            funds_ -= result.order.in;

            return result.order;
        }

        std::pair<Amounts, Amounts>
        cross (Amounts offer1, Quality quality1, Amounts offer2, Quality quality2)
        {
            /*检查组合质量是否应被拒绝*/
            Quality const quality (composed_quality (
                quality1, quality2));

            if (reject (quality))
                return std::make_pair(
                    Amounts { offer1.in.zeroed (), offer1.out.zeroed () },
                    Amounts { offer2.in.zeroed (), offer2.out.zeroed () });

            if (done ())
                return std::make_pair(
                    Amounts { offer1.in.zeroed (), offer1.out.zeroed () },
                    Amounts { offer2.in.zeroed (), offer2.out.zeroed () });

            auto result = do_cross (
                offer1, quality1, AccountID (0x4703),
                offer2, quality2, AccountID (0x4704));

            return std::make_pair (result.first.order, result.second.order);
        }
    };

private:
    Issue const& usd () const
    {
        static Issue const issue (
            Currency (0x5553440000000000), AccountID (0x4985601));
        return issue;
    }

    Issue const& eur () const
    {
        static Issue const issue (
            Currency (0x4555520000000000), AccountID (0x4985602));
        return issue;
    }

    Issue const& xrp () const
    {
        static Issue const issue (
            xrpCurrency (), xrpAccount ());
        return issue;
    }

    STAmount parse_amount (std::string const& amount, Issue const& issue)
    {
        return amountFromString (issue, amount);
    }

    Amounts parse_amounts (
        std::string const& amount_in, Issue const& issue_in,
        std::string const& amount_out, Issue const& issue_out)
    {
        STAmount const in (parse_amount (amount_in, issue_in));
        STAmount const out (parse_amount (amount_out, issue_out));

        return { in, out };
    }

    struct cross_attempt_offer
    {
        cross_attempt_offer (std::string const& in_,
            std::string const &out_)
            : in (in_)
            , out (out_)
        {
        }

        std::string in;
        std::string out;
    };

private:
    std::string
    format_amount (STAmount const& amount)
    {
        std::string txt = amount.getText ();
        txt += "/";
        txt += to_string (amount.issue().currency);
        return txt;
    }

    void
    attempt (
        bool sell,
        std::string name,
        Quality taker_quality,
        cross_attempt_offer const offer,
        std::string const funds,
        Quality cross_quality,
        cross_attempt_offer const cross,
        std::string const cross_funds,
        cross_attempt_offer const flow,
        Issue const& issue_in,
        Issue const& issue_out,
        Rate rate_in = parityRate,
        Rate rate_out = parityRate)
    {
        Amounts taker_offer (parse_amounts (
            offer.in, issue_in,
            offer.out, issue_out));

        Amounts cross_offer (parse_amounts (
            cross.in, issue_in,
            cross.out, issue_out));

        CrossType cross_type;

        if (isXRP (issue_out))
            cross_type = CrossType::IouToXrp;
        else if (isXRP (issue_in))
            cross_type = CrossType::XrpToIou;
        else
            cross_type = CrossType::IouToIou;

//Fixme：我们总是向借据接受者调用借据。我们应该选择
//动态地选择正确的类型。
        TestTaker taker (cross_type, taker_offer, taker_quality,
            parse_amount (funds, issue_in), sell ? tfSell : 0,
            rate_in, rate_out);

        taker.set_funds (parse_amount (cross_funds, issue_out));

        auto result = taker.cross (cross_offer, cross_quality);

        Amounts const expected (parse_amounts (
            flow.in, issue_in,
            flow.out, issue_out));

        BEAST_EXPECT(expected == result);

        if (expected != result)
        {
            log <<
                "Expected: " << format_amount (expected.in) <<
                " : " << format_amount (expected.out) << '\n' <<
                "  Actual: " << format_amount (result.in) <<
                " : " << format_amount (result.out) << std::endl;
        }
    }

    Quality get_quality(std::string in, std::string out)
    {
        return Quality (parse_amounts (in, xrp(), out, xrp ()));
    }

public:
//夹具方案说明符号：
//
//输入：输出（列表中的最后一个为限制因素）
//没有任何东西
//T=接受方报价余额
//A=账户余额
//B=业主账户余额
//
//（S）=sell语义：接受者想要无限的输出
//（b）=buy语义：接受者想要一个有限的数量

//Nikb Todo：增加测试者，以便指定货币和利率。
//一次不需要重复。
    void
    test_xrp_to_iou ()
    {
        testcase ("XRP Quantization: input");

        Quality q1 = get_quality ("1", "1");

//承购人所有者
//均等报价基金
//XRP美元
        attempt (Sell, "N:N",   q1, { "2", "2" },  "2",  q1, { "2", "2" }, "2",   { "2", "2" },   xrp(), usd());
        attempt (Sell, "N:B",   q1, { "2", "2" },  "2",  q1, { "2", "2" }, "1.8", { "1", "1.8" }, xrp(), usd());
        attempt (Buy,  "N:T",   q1, { "1", "1" },  "2",  q1, { "2", "2" }, "2",   { "1", "1" },   xrp(), usd());
        attempt (Buy,  "N:BT",  q1, { "1", "1" },  "2",  q1, { "2", "2" }, "1.8", { "1", "1" },   xrp(), usd());
        attempt (Buy,  "N:TB",  q1, { "1", "1" },  "2",  q1, { "2", "2" }, "0.8", { "0", "0.8" }, xrp(), usd());

        attempt (Sell, "T:N",   q1, { "1", "1" },  "2",  q1, { "2", "2" }, "2",   { "1", "1" },   xrp(), usd());
        attempt (Sell, "T:B",   q1, { "1", "1" },  "2",  q1, { "2", "2" }, "1.8", { "1", "1.8" }, xrp(), usd());
        attempt (Buy,  "T:T",   q1, { "1", "1" },  "2",  q1, { "2", "2" }, "2",   { "1", "1" },   xrp(), usd());
        attempt (Buy,  "T:BT",  q1, { "1", "1" },  "2",  q1, { "2", "2" }, "1.8", { "1", "1" },   xrp(), usd());
        attempt (Buy,  "T:TB",  q1, { "1", "1" },  "2",  q1, { "2", "2" }, "0.8", { "0", "0.8" }, xrp(), usd());

        attempt (Sell, "A:N",   q1, { "2", "2" },  "1",  q1, { "2", "2" }, "2",   { "1", "1" },   xrp(), usd());
        attempt (Sell, "A:B",   q1, { "2", "2" },  "1",  q1, { "2", "2" }, "1.8", { "1", "1.8" }, xrp(), usd());
        attempt (Buy,  "A:T",   q1, { "2", "2" },  "1",  q1, { "3", "3" }, "3",   { "1", "1" },   xrp(), usd());
        attempt (Buy,  "A:BT",  q1, { "2", "2" },  "1",  q1, { "3", "3" }, "2.4", { "1", "1" },   xrp(), usd());
        attempt (Buy,  "A:TB",  q1, { "2", "2" },  "1",  q1, { "3", "3" }, "0.8", { "0", "0.8" }, xrp(), usd());

        attempt (Sell, "TA:N",  q1, { "2", "2" },  "1",  q1, { "2", "2" }, "2",   { "1", "1" },   xrp(), usd());
        attempt (Sell, "TA:B",  q1, { "2", "2" },  "1",  q1, { "3", "3" }, "1.8", { "1", "1.8" }, xrp(), usd());
        attempt (Buy,  "TA:T",  q1, { "2", "2" },  "1",  q1, { "3", "3" }, "3",   { "1", "1" },   xrp(), usd());
        attempt (Buy,  "TA:BT", q1, { "2", "2" },  "1",  q1, { "3", "3" }, "1.8", { "1", "1.8" }, xrp(), usd());
        attempt (Buy,  "TA:TB", q1, { "2", "2" },  "1",  q1, { "3", "3" }, "1.8", { "1", "1.8" }, xrp(), usd());

        attempt (Sell, "AT:N",  q1, { "2", "2" },  "1",  q1, { "3", "3" }, "3",   { "1", "1" },   xrp(), usd());
        attempt (Sell, "AT:B",  q1, { "2", "2" },  "1",  q1, { "3", "3" }, "1.8", { "1", "1.8" }, xrp(), usd());
        attempt (Buy,  "AT:T",  q1, { "2", "2" },  "1",  q1, { "3", "3" }, "3",   { "1", "1" },   xrp(), usd());
        attempt (Buy,  "AT:BT", q1, { "2", "2" },  "1",  q1, { "3", "3" }, "1.8", { "1", "1.8" }, xrp(), usd());
        attempt (Buy,  "AT:TB", q1, { "2", "2" },  "1",  q1, { "3", "3" }, "0.8", { "0", "0.8" }, xrp(), usd());
    }

    void
    test_iou_to_xrp ()
    {
        testcase ("XRP Quantization: output");

        Quality q1 = get_quality ("1", "1");

//承购人所有者
//均等报价基金
//美元xrp
        attempt (Sell, "N:N",   q1, { "3", "3" }, "3",   q1, { "3", "3" }, "3",  { "3",   "3" }, usd(), xrp());
        attempt (Sell, "N:B",   q1, { "3", "3" }, "3",   q1, { "3", "3" }, "2",  { "2",   "2" }, usd(), xrp());
        attempt (Buy,  "N:T",   q1, { "3", "3" }, "2.5", q1, { "5", "5" }, "5",  { "2.5", "2" }, usd(), xrp());
        attempt (Buy,  "N:BT",  q1, { "3", "3" }, "1.5", q1, { "5", "5" }, "4",  { "1.5", "1" }, usd(), xrp());
        attempt (Buy,  "N:TB",  q1, { "3", "3" }, "2.2", q1, { "5", "5" }, "1",  { "1",   "1" }, usd(), xrp());

        attempt (Sell, "T:N",   q1, { "1", "1" }, "2",   q1, { "2", "2" }, "2",  { "1",   "1" }, usd(), xrp());
        attempt (Sell, "T:B",   q1, { "2", "2" }, "2",   q1, { "3", "3" }, "1",  { "1",   "1" }, usd(), xrp());
        attempt (Buy,  "T:T",   q1, { "1", "1" }, "2",   q1, { "2", "2" }, "2",  { "1",   "1" }, usd(), xrp());
        attempt (Buy,  "T:BT",  q1, { "1", "1" }, "2",   q1, { "3", "3" }, "2",  { "1",   "1" }, usd(), xrp());
        attempt (Buy,  "T:TB",  q1, { "2", "2" }, "2",   q1, { "3", "3" }, "1",  { "1",   "1" }, usd(), xrp());

        attempt (Sell, "A:N",   q1, { "2", "2" }, "1.5", q1, { "2", "2" }, "2",  { "1.5", "1" }, usd(), xrp());
        attempt (Sell, "A:B",   q1, { "2", "2" }, "1.8", q1, { "3", "3" }, "2",  { "1.8", "1" }, usd(), xrp());
        attempt (Buy,  "A:T",   q1, { "2", "2" }, "1.2", q1, { "3", "3" }, "3",  { "1.2", "1" }, usd(), xrp());
        attempt (Buy,  "A:BT",  q1, { "2", "2" }, "1.5", q1, { "4", "4" }, "3",  { "1.5", "1" }, usd(), xrp());
        attempt (Buy,  "A:TB",  q1, { "2", "2" }, "1.5", q1, { "4", "4" }, "1",  { "1",   "1" }, usd(), xrp());

        attempt (Sell, "TA:N",  q1, { "2", "2" }, "1.5", q1, { "2", "2" }, "2",  { "1.5", "1" }, usd(), xrp());
        attempt (Sell, "TA:B",  q1, { "2", "2" }, "1.5", q1, { "3", "3" }, "1",  { "1",   "1" }, usd(), xrp());
        attempt (Buy,  "TA:T",  q1, { "2", "2" }, "1.5", q1, { "3", "3" }, "3",  { "1.5", "1" }, usd(), xrp());
        attempt (Buy,  "TA:BT", q1, { "2", "2" }, "1.8", q1, { "4", "4" }, "3",  { "1.8", "1" }, usd(), xrp());
        attempt (Buy,  "TA:TB", q1, { "2", "2" }, "1.2", q1, { "3", "3" }, "1",  { "1",   "1" }, usd(), xrp());

        attempt (Sell, "AT:N",  q1, { "2", "2" }, "2.5", q1, { "4", "4" }, "4",  { "2",   "2" }, usd(), xrp());
        attempt (Sell, "AT:B",  q1, { "2", "2" }, "2.5", q1, { "3", "3" }, "1",  { "1",   "1" }, usd(), xrp());
        attempt (Buy,  "AT:T",  q1, { "2", "2" }, "2.5", q1, { "3", "3" }, "3",  { "2",   "2" }, usd(), xrp());
        attempt (Buy,  "AT:BT", q1, { "2", "2" }, "2.5", q1, { "4", "4" }, "3",  { "2",   "2" }, usd(), xrp());
        attempt (Buy,  "AT:TB", q1, { "2", "2" }, "2.5", q1, { "3", "3" }, "1",  { "1",   "1" }, usd(), xrp());
    }

    void
    test_iou_to_iou ()
    {
        testcase ("IOU to IOU");

        Quality q1 = get_quality ("1", "1");

//输入和输出的高度夸大的50%传输率：
        Rate const rate { parityRate.value + (parityRate.value / 2) };

//承购人所有者
//均等报价基金
//欧元/美元
        attempt (Sell, "N:N",   q1, { "2", "2" },  "10",  q1, { "2", "2" }, "10",   { "2",                  "2" },                  eur(), usd(), rate, rate);
        attempt (Sell, "N:B",   q1, { "4", "4" },  "10",  q1, { "4", "4" },  "4",   { "2.666666666666666",  "2.666666666666666" },  eur(), usd(), rate, rate);
        attempt (Buy,  "N:T",   q1, { "1", "1" },  "10",  q1, { "2", "2" }, "10",   { "1",                  "1" },                  eur(), usd(), rate, rate);
        attempt (Buy,  "N:BT",  q1, { "2", "2" },  "10",  q1, { "6", "6" },  "5",   { "2",                  "2" },                  eur(), usd(), rate, rate);
        attempt (Buy,  "N:TB",  q1, { "2", "2" },   "2",  q1, { "6", "6" },  "1",   { "0.6666666666666667", "0.6666666666666667" }, eur(), usd(), rate, rate);
        attempt (Sell, "A:N",   q1, { "2", "2" },  "2.5", q1, { "2", "2" }, "10",   { "1.666666666666666",  "1.666666666666666" },  eur(), usd(), rate, rate);
    }

    void
    run() override
    {
        test_xrp_to_iou ();
        test_iou_to_xrp ();
        test_iou_to_iou ();
    }
};

BEAST_DEFINE_TESTSUITE(Taker,tx,ripple);

}
