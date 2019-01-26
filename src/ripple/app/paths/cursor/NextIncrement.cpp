
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
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>

namespace ripple {
namespace path {

//计算路径的下一个增量。
//
//增量可以满足部分或全部请求的输出
//最好的质量。
//
//<--pathstate.uquality
//
//这是用于恢复分类帐检查点版本的包装器，因此
//可以写得到处都是，没有任何后果。

void PathCursor::nextIncrement () const
{
//下一个状态是按优先顺序可用的状态。
//这是在引用帐户更改时计算的。

    auto status = liquidity();

    if (status == tesSUCCESS)
    {
        if (pathState_.isDry())
        {
            JLOG (j_.debug())
                << "nextIncrement: success on dry path:"
                << " outPass=" << pathState_.outPass()
                << " inPass=" << pathState_.inPass();
            Throw<std::runtime_error> ("Made no progress.");
        }

//计算相对质量。
        pathState_.setQuality(getRate (
            pathState_.outPass(), pathState_.inPass()));
    }
    else
    {
        pathState_.setQuality(0);
    }
    pathState_.setStatus (status);
}

} //路径
} //涟漪
