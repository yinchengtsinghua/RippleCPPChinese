
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

#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>
#include <tuple>

namespace ripple {
namespace path {

TER PathCursor::liquidity () const
{
    TER resultCode = tecPATH_DRY;
    PathCursor pc = *this;

    pathState_.resetView (rippleCalc_.view);

    for (pc.nodeIndex_ = pc.nodeSize(); pc.nodeIndex_--; )
    {
        JLOG (j_.trace())
            << "reverseLiquidity>"
            << " nodeIndex=" << pc.nodeIndex_
            << ".issue_.account=" << to_string (pc.node().issue_.account);

        resultCode = pc.reverseLiquidity();

        if (!pc.node().transferRate_)
            return tefINTERNAL;

        JLOG (j_.trace())
            << "reverseLiquidity< "
            << "nodeIndex=" << pc.nodeIndex_
            << " resultCode=" << transToken (resultCode)
            << " transferRate_=" << *pc.node().transferRate_
            << ": " << resultCode;

        if (resultCode != tesSUCCESS)
            break;
    }

//vfalc-fixme这会产生错误
//jLog（j_.trace（））
//<“nextincrement:path after reverse：”<<pathstate_.getjson（）；

    if (resultCode != tesSUCCESS)
        return resultCode;

    pathState_.resetView (rippleCalc_.view);

    for (pc.nodeIndex_ = 0; pc.nodeIndex_ < pc.nodeSize(); ++pc.nodeIndex_)
    {
        JLOG (j_.trace())
            << "forwardLiquidity> nodeIndex=" << nodeIndex_;

        resultCode = pc.forwardLiquidity();
        if (resultCode != tesSUCCESS)
            return resultCode;

        JLOG (j_.trace())
            << "forwardLiquidity<"
            << " nodeIndex:" << pc.nodeIndex_
            << " resultCode:" << resultCode;

        if (pathState_.isDry())
            resultCode = tecPATH_DRY;
    }
    return resultCode;
}

} //路径
} //涟漪
