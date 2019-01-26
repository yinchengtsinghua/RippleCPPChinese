
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

#ifndef RIPPLE_OVERLAY_TUNING_H_INCLUDED
#define RIPPLE_OVERLAY_TUNING_H_INCLUDED

namespace ripple {

namespace Tuning
{

enum
{
    /*用于从套接字读取的缓冲区大小。*/
    readBufferBytes     = 4096,

    /*在我们
        断开连接（如果出站）*/

    maxInsaneTime       =   60,

    /*在我们之前，服务器可以保持未知状态多长时间
        断开连接（如果是出站）*/

    maxUnknownTime      =  300,

    /*一台服务器上可以有多少个分类账，我们将
        仍然认为它是健全的*/

    saneLedgerLimit     =   24,

    /*一台服务器上有多少个账本
        认为这很疯狂*/

    insaneLedgerLimit   =  128,

    /*单个分类帐分录的最大数目
        回复*/

    maxReplyNodes       = 8192,

    /*考虑高延迟的毫秒数
        在对等连接上*/

    peerHighLatency     =  300,

    /*我们多久检查一次连接（秒）*/
    checkSeconds        =   32,

    /*我们多长时间延迟/sendq探测连接一次*/
    timerSeconds        =    8,

    /*在我们断开连接之前，一个sendq必须保持多大的计时器间隔*/
    sendqIntervals      =    4,

    /*如果没有ping回复，我们可以有多少个计时器间隔*/
    noPing              =   10,

    /*在拒绝查询之前，发送队列中有多少消息*/
    dropSendQueue       =   192,

    /*在发送队列中我们认为合理的持续消息数*/
    targetSendQueue     =   128,

    /*记录发送队列大小的频率*/
    sendQueueLogFreq    =    64,
};

} //调谐

} //涟漪

#endif
