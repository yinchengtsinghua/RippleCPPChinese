
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

#ifndef RIPPLE_APP_LEDGER_BUILD_LEDGER_H_INCLUDED
#define RIPPLE_APP_LEDGER_BUILD_LEDGER_H_INCLUDED

#include <ripple/ledger/ApplyView.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/utility/Journal.h>
#include <chrono>
#include <memory>

namespace ripple {

class Application;
class CanonicalTXSet;
class Ledger;
class LedgerReplay;
class SHAMap;


/*通过应用共识交易建立新的分类帐

    通过应用作为
    共识。

    @param parent要应用交易的分类帐
    @param closetime分类帐关闭的时间
    @param closetimecorrect是否一致同意关闭时间
    @param close resolution resolution用于确定共识关闭时间
    应用实例的@param app句柄
    @param txs on entry，要应用的事务；exit，必须
               在下一轮中重试。
    @param failedtxs填充了在这一轮中失败的事务
    用于日志记录的@param j日志
    @归还新建账本
 **/

std::shared_ptr<Ledger>
buildLedger(
    std::shared_ptr<Ledger const> const& parent,
    NetClock::time_point closeTime,
    const bool closeTimeCorrect,
    NetClock::duration closeResolution,
    Application& app,
    CanonicalTXSet& txns,
    std::set<TxID>& failedTxs,
    beast::Journal j);

/*通过重放交易记录创建新分类帐

    通过将接受的交易重放到上一个分类帐中来创建新分类帐。

    @param replay data要重播的分类帐数据
    应用事务时要使用的@param applyFlags标志
    应用实例的@param app句柄
    用于日志记录的@param j日志
    @归还新建账本
 **/

std::shared_ptr<Ledger>
buildLedger(
    LedgerReplay const& replayData,
    ApplyFlags applyFlags,
    Application& app,
    beast::Journal j);

}  //命名空间波纹
#endif
