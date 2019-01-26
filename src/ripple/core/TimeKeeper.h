
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

#ifndef RIPPLE_CORE_TIMEKEEPER_H_INCLUDED
#define RIPPLE_CORE_TIMEKEEPER_H_INCLUDED

#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/basics/chrono.h>
#include <string>
#include <vector>

namespace ripple {

/*管理服务器使用的各种时间。*/
class TimeKeeper
    : public beast::abstract_clock<NetClock>
{
public:
    virtual ~TimeKeeper() = default;

    /*启动内部线程。

        内部线程同步本地网络时间
        使用提供的sntp服务器列表。
    **/

    virtual
    void
    run (std::vector<std::string> const& servers) = 0;

    /*返回墙时间的估计值（以网络时间为单位）。

        网络时间是根据纹波调整的壁面时间。
        纪元，2000年1月1日开始。每个服务器
        可以计算不同的网络时间值。其他
        网络时间的服务器值不能直接观察到，
        但是通过查看验证器可以做出很好的猜测
        关闭时间的位置。

        服务器通过调整本地墙计算网络时间
        时钟使用sntp，然后调整为时代。
    **/

    virtual
    time_point
    now() const override = 0;

    /*返回关闭时间（网络时间）。

        Close Time是网络同意
        如果分类帐现在已关闭，则为已关闭的分类帐。

        关闭时间代表概念上的“中心”
        网络的。每个服务器假定其时钟
        是正确的，并试图将结束时间拉向
        它对网络时间的度量。
    **/

    virtual
    time_point
    closeTime() const = 0;

    /*调整关闭时间。

        这是为了响应收到的验证而调用的。
    **/

    virtual
    void
    adjustCloseTime (std::chrono::duration<std::int32_t> amount) = 0;

//这可能返回负值
    virtual
    std::chrono::duration<std::int32_t>
    nowOffset() const = 0;

//这可能返回负值
    virtual
    std::chrono::duration<std::int32_t>
    closeOffset() const = 0;
};

extern
std::unique_ptr<TimeKeeper>
make_TimeKeeper(beast::Journal j);

} //涟漪

#endif
