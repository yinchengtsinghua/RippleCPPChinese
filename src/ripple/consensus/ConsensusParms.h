
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

#ifndef RIPPLE_CONSENSUS_CONSENSUS_PARMS_H_INCLUDED
#define RIPPLE_CONSENSUS_CONSENSUS_PARMS_H_INCLUDED

#include <chrono>
#include <cstddef>

namespace ripple {

/*共识算法参数

    控制共识算法的参数。这不是
    意味着任意改变。
**/

struct ConsensusParms
{
    explicit ConsensusParms() = default;

//————————————————————————————————————————————————————————————————
//验证和建议持续时间与网络时钟时间相关，因此使用
//第二决议
    /*验证的持续时间在其分类帐之后保持为当前状态
       关闭时间。

        这是一种安全措施，可以防止非常古老的验证和时间
        需要调整关闭时间精度窗口。
    **/

    std::chrono::seconds validationVALID_WALL = std::chrono::minutes {5};

    /*持续时间首次观察后，验证仍为当前状态。

       验证的持续时间在我们
       首先看到它。这在极少数情况下提供了更快的恢复，
       网络生成的验证数低于正常值
    **/

    std::chrono::seconds validationVALID_LOCAL = std::chrono::minutes {3};

    /*可接受验证的预关闭时间。

        我们考虑验证的关闭时间之前的秒数
        可接受的。这可以防止极端的时钟错误
    **/

    std::chrono::seconds validationVALID_EARLY = std::chrono::minutes {3};


//！我们考虑一个新提议多长时间
    std::chrono::seconds proposeFRESHNESS = std::chrono::seconds {20};

//！我们多久强制提出一个新建议来保持我们的新鲜感
    std::chrono::seconds proposeINTERVAL = std::chrono::seconds {12};


//————————————————————————————————————————————————————————————————
//共识持续时间与内部Consenus时钟和使用有关。
//毫秒分辨率。

//！我们可以宣布达成共识的百分比阈值。
    std::size_t minCONSENSUS_PCT = 80;

//！分类帐在关闭前可能保持空闲的持续时间
    std::chrono::milliseconds ledgerIDLE_INTERVAL = std::chrono::seconds {15};

//！我们等待最少的秒数以确保参与
    std::chrono::milliseconds ledgerMIN_CONSENSUS =
        std::chrono::milliseconds {1950};

//！等待以确保其他人已计算LCL的最小秒数
    std::chrono::milliseconds ledgerMIN_CLOSE = std::chrono::seconds {2};

//！我们多久检查一次状态或改变位置
    std::chrono::milliseconds ledgerGRANULARITY = std::chrono::seconds {1};

    /*考虑上一轮的最短时间
        拿走了。

        考虑上一轮的最短时间
        拿走了。这样可以确保有机会
        对于每个雪崩临界点的一轮，即使
        先前的共识非常迅速。这至少应该是
        两倍建议间隔（0.7秒）除以
        中晚期共识之间的间隔（[85-50]/100）。
    **/

    std::chrono::milliseconds avMIN_CONSENSUS_TIME = std::chrono::seconds {5};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//雪崩调谐
//作为百分比的函数，这一轮的持续时间是前一轮的，
//我们增加了YesVots的阈值，以便向我们的
//位置。

//！必须投票通过的UNL上节点的百分比
    std::size_t avINIT_CONSENSUS_PCT = 50;

//！在我们前进之前，上一轮持续时间的百分比
    std::size_t avMID_CONSENSUS_TIME = 50;

//！前进后大多数选择“是”的节点的百分比
    std::size_t avMID_CONSENSUS_PCT = 65;

//！在我们前进之前，上一轮持续时间的百分比
    std::size_t avLATE_CONSENSUS_TIME = 85;

//！前进后大多数选择“是”的节点的百分比
    std::size_t avLATE_CONSENSUS_PCT = 70;

//！在我们陷入困境之前，上一轮持续时间的百分比
    std::size_t avSTUCK_CONSENSUS_TIME = 200;

//！在我们陷入困境后必须投赞成票的节点的百分比
    std::size_t avSTUCK_CONSENSUS_PCT = 95;

//！在分类帐关闭时间达成协议所需节点的百分比
    std::size_t avCT_CONSENSUS_PCT = 75;

//——————————————————————————————————————————————————————————————

    /*是否使用RoundCloseTime或EffCloseTime来达到关闭时间
        共识。
        这是为了在上从efffclosetime迁移到roundclosetime而添加的。
        现场网络。所需的行为（由默认值给出）是
        在协商一致投票期间使用RoundCloseTime，然后使用EffCloseTime
        接受共识分类账时。
    **/

    bool useRoundedCloseTime = true;
};

}  //涟漪
#endif
