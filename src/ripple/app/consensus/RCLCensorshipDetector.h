
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
    版权所有（c）2018 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_CONSENSUS_RCLCENSORSHIPDETECTOR_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_RCLCENSORSHIPDETECTOR_H_INCLUDED

#include <ripple/shamap/SHAMap.h>
#include <algorithm>
#include <map>

namespace ripple {

template <class TxID, class Sequence>
class RCLCensorshipDetector
{
private:
    std::map<TxID, Sequence> tracker_;

    /*从跟踪器中删除满足特定条件的所有元素

        @param pred一个谓词，用于跟踪
                    应该移除。谓词必须可调用为：
                        bool pred（txid const&，sequence）
                    对于应删除的条目，它必须返回true。

        @注意：当它成为部件时，可以用std：：erase_替换它。
              标准的。例如：

                修剪（[]（txid const&id，sequence seq）
                    {
                        返回id.iszero（）seq==314159；
                    （}）；

              将成为：

                std:：erase_if（tracker_.begin（），tracker_.end（），
                    []（自动施工与维护）
                    {
                        返回e.first.iszero（）e.second==314159；
                    }
    **/

    template <class Predicate>
    void prune(Predicate&& pred)
    {
        auto t = tracker_.begin();

        while (t != tracker_.end())
        {
            if (pred(t->first, t->second))
                t = tracker_.erase(t);
            else
                t = std::next(t);
        }
    }

public:
    RCLCensorshipDetector() = default;

    /*添加为当前共识轮提议的交易。

        @param seq正在生成的分类帐的序列号。
        @param提出了我们最初提出的一组交易
                        为了这一轮。
    **/

    void propose(
        Sequence seq,
        std::vector<TxID> proposed)
    {
        std::sort (proposed.begin(), proposed.end());

//我们想删除我们在上一轮中提出的任何条目
//如果我们不再向他们求婚，这还没有实现。
        prune ([&proposed](TxID const& id, Sequence seq)
            {
                return !std::binary_search(proposed.begin(), proposed.end(), id);
            });

//跟踪我们在本轮提议的条目。
        for (auto const& p : proposed)
tracker_.emplace(p, seq); //FixMe C++ 17：使用TryEySePress
    }

    /*确定哪些事务进行了检查，并执行检查检测。

        当服务器提出并达成一致意见时，调用此函数
        它参加的回合结束了。

        @param接受了网络同意的一组事务
                        应包括在正在构建的分类帐中。
        @param pred为我们建议的每个事务调用的谓词
                        但这还没有实现。谓词必须是
                        可赎回为：
                            bool pred（txid const&，sequence）
                        对于应删除的条目，它必须返回true。
    **/

    template <class Predicate>
    void check(
        std::vector<TxID> accepted,
        Predicate&& pred)
    {
        std::sort (accepted.begin(), accepted.end());

//我们要删除所有跟踪条目
//接受以及那些与谓词匹配的。
        prune ([&pred, &accepted](TxID const& id, Sequence seq)
            {
                if (std::binary_search(accepted.begin(), accepted.end(), id))
                    return true;

                return pred(id, seq);
            });
    }

    /*从跟踪器中删除所有元素

        通常，在我们重新连接到
        网络中断后，或在我们开始跟踪网络之后。
    **/

    void reset()
    {
        tracker_.clear();
    }
};

}

#endif
