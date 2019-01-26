
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

#ifndef RIPPLE_APP_PATHS_NODEDIRECTORY_H_INCLUDED
#define RIPPLE_APP_PATHS_NODEDIRECTORY_H_INCLUDED

#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/ledger/ApplyView.h>

namespace ripple {

//vfalc todo de inline这些函数定义

class NodeDirectory
{
public:
    explicit NodeDirectory() = default;

//当前目录-最后64位是质量。
    uint256 current;

//下一个订单的开始-一个超过了可能的最差质量
//用于当前订单簿。
    uint256 next;

//todo（tom）：directory.current和directory.next的类型应为
//目录。

bool advanceNeeded;  //需要前进目录。
bool restartNeeded;  //需要重新启动目录。

    SLE::pointer ledgerEntry;

    void restart (bool multiQuality)
    {
        if (multiQuality)
current = 0;             //重新开始图书搜索。
        else
restartNeeded  = true;   //以相同的质量重新启动。
    }

    bool initialize (Book const& book, ApplyView& view)
    {
        if (current != beast::zero)
            return false;

        current.copyFrom (getBookBase (book));
        next.copyFrom (getQualityNext (current));

//托多（汤姆）：任何实际报价都不可能
//质量==0可能发生-我们应该禁止它们，并清除
//directory.ledgerEntry，下一行不调用数据库。
        ledgerEntry = view.peek (keylet::page(current));

//如果找不到的话，提前。正常不能查找
//第一个目录。甚至可以跳过这个查找。
        advanceNeeded  = ! ledgerEntry;
        restartNeeded  = false;

//关联的var是脏的，如果找到的话。
        return bool(ledgerEntry);
    }

    enum Advance
    {
        NO_ADVANCE,
        NEW_QUALITY,
        END_ADVANCE
    };

    /*前进到订单簿中的下一个质量目录。*/
//vfalc考虑将其重命名为“nextquality”或其他
    Advance
    advance (ApplyView& view)
    {
        if (!(advanceNeeded || restartNeeded))
            return NO_ADVANCE;

//获得下一个质量。
//默克尔基树是按键排序的，所以我们可以转到下一个
//O（1）中的质量。
        if (advanceNeeded)
        {
            auto const opt =
                view.succ (current, next);
            current = opt ? *opt : uint256{};
        }
        advanceNeeded  = false;
        restartNeeded  = false;

        if (current == beast::zero)
            return END_ADVANCE;

        ledgerEntry = view.peek (keylet::page(current));
        return NEW_QUALITY;
    }
};

} //涟漪

#endif
