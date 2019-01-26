
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

#ifndef RIPPLE_APP_MISC_ORDERBOOK_H_INCLUDED
#define RIPPLE_APP_MISC_ORDERBOOK_H_INCLUDED

namespace ripple {

/*描述订单簿的序列化分类帐条目。*/
class OrderBook
{
public:
    using pointer = std::shared_ptr <OrderBook>;
    using ref = std::shared_ptr <OrderBook> const&;
    using List = std::vector<pointer>;

    /*从货币规范构造。

        PARAM索引？？？？
        @param book in and out currency/issuer对。
    **/

//vfalc注意索引参数的含义是什么？
    OrderBook (uint256 const& base, Book const& book)
            : mBookBase(base), mBook(book)
    {
    }

    uint256 const& getBookBase () const
    {
        return mBookBase;
    }

    Book const& book() const
    {
        return mBook;
    }

    Currency const& getCurrencyIn () const
    {
        return mBook.in.currency;
    }

    Currency const& getCurrencyOut () const
    {
        return mBook.out.currency;
    }

    AccountID const& getIssuerIn () const
    {
        return mBook.in.account;
    }

    AccountID const& getIssuerOut () const
    {
        return mBook.out.account;
    }

private:
    uint256 const mBookBase;
    Book const mBook;
};

} //涟漪

#endif
