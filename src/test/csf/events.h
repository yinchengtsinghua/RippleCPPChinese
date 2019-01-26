
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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
#ifndef RIPPLE_TEST_CSF_EVENTS_H_INCLUDED
#define RIPPLE_TEST_CSF_EVENTS_H_INCLUDED

#include <test/csf/Tx.h>
#include <test/csf/Validation.h>
#include <test/csf/ledgers.h>
#include <test/csf/Proposal.h>
#include <chrono>


namespace ripple {
namespace test {
namespace csf {

//在模拟过程中，事件由对等方在不同的点发出。
//每个事件都是由一个特定时间的particlar对等机发出的。收集器
//处理这些事件，可能是计算统计信息或将事件存储到
//用于后处理的日志。
//
//事件类型可以是任意的，但应该是可复制的和轻量级的。
//
//示例收集器可以在collectors.h中找到，但是
//接口：
//
//@代码
//模板<class t>
//结构收集器
//{
//模板<类事件>
//无效
//开（peerid who，simtime when，event e）；
//}；
//@端码
//
//collectorRef.f为任意收集器定义了一个类型擦除的保持架。如果
//任何新事件都会被添加，那里的接口需要更新。



/*从该对等机开始向所有其他对等机发送的值。
 **/

template <class V>
struct Share
{
//！共享的事件
    V val;
};

/*作为泛洪的一部分中继到另一个对等机的值
 **/

template <class V>
struct Relay
{
//！对等中继到
    PeerID to;

//！要中继的值
    V val;
};

/*作为泛洪的一部分从另一个对等机接收的值。
 **/

template <class V>
struct Receive
{
//！发送值的对等机
    PeerID from;

//！收到的价值
    V val;
};

/*提交给对等方的事务*/
struct SubmitTx
{
//！提交的交易
    Tx tx;
};

/*同行开始新一轮共识
 **/

struct StartRound
{
//！达成共识的首选分类账
    Ledger::ID bestLedger;

//！现有的上一个分类帐
    Ledger prevLedger;
};

/*同行关闭了打开的分类帐
 **/

struct CloseLedger
{
//分类帐结束于
    Ledger prevLedger;

//包含在分类帐中的初始TXS
    TxSetType txs;
};

//！同行接受的共识结果
struct AcceptLedger
{
//新创建的分类帐
    Ledger ledger;

//上一个分类帐（如果prior.id（），这是一个跳转！=分类帐.parentID（））
    Ledger prior;
};

//！在协商一致期间，对等方检测到错误的先前分类帐
struct WrongPrevLedger
{
//我们有错误的分类帐ID
    Ledger::ID wrong;
//我们认为正确的分类帐的ID
    Ledger::ID right;
};

//！对等完全验证了新的分类帐
struct FullyValidateLedger
{
//！新的完全有效的分类帐
    Ledger ledger;

//！先前完全验证的分类帐
//！这是一个跳转，如果在.id（）之前！=分类帐.parentID（））
    Ledger prior;
};


}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹

#endif
