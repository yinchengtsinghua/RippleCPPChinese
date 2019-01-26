
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

#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/impl/Tuning.h>

namespace ripple {
namespace PeerFinder {

Config::Config()
    : maxPeers (Tuning::defaultMaxPeers)
    , outPeers (calcOutPeers ())
    , wantIncoming (true)
    , autoConnect (true)
    , listeningPort (0)
    , ipLimit (0)
{
}

double Config::calcOutPeers () const
{
    return std::max (
        maxPeers * Tuning::outPercent * 0.01,
            double (Tuning::minOutCount));
}

void Config::applyTuning ()
{
    if (maxPeers < Tuning::minOutCount)
        maxPeers = Tuning::minOutCount;
    outPeers = calcOutPeers ();

    auto const inPeers = maxPeers - outPeers;

    if (ipLimit == 0)
    {
//除非明确设置限制，否则我们允许
//2和5个非RFC-1918“专用”连接
//IP地址。
        ipLimit = 2;

        if (inPeers > Tuning::defaultMaxPeers)
            ipLimit += std::min(5,
                static_cast<int>(inPeers / Tuning::defaultMaxPeers));
    }

//我们不允许一个IP占用所有进入的插槽，
//除非我们只有一个输入槽可用。
    ipLimit = std::max(1,
        std::min(ipLimit, static_cast<int>(inPeers / 2)));
}

void Config::onWrite (beast::PropertyStream::Map &map)
{
    map ["max_peers"]       = maxPeers;
    map ["out_peers"]       = outPeers;
    map ["want_incoming"]   = wantIncoming;
    map ["auto_connect"]    = autoConnect;
    map ["port"]            = listeningPort;
    map ["features"]        = features;
    map ["ip_limit"]        = ipLimit;
}

}
}
