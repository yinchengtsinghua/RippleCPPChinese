
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

#ifndef RIPPLE_APP_PATHS_FLOW_H_INCLUDED
#define RIPPLE_APP_PATHS_FLOW_H_INCLUDED

#include <ripple/app/paths/impl/Steps.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/protocol/Quality.h>

namespace ripple
{

namespace path {
namespace detail{
struct FlowDebugInfo;
}
}

/*
  从SRC帐户向DST帐户付款

  @param查看信任行和余额
  @param传递要传递到DST帐户的金额
  @param src为付款提供输入资金的帐户
  @param dst账户收到付款
  @param paths流动性探索路径集
  @param defaultpaths在路径集中包含defaultpaths
  @param partialpayment如果付款不能交付整个
           请求数量，根据限制条件，尽可能多地交付
  @param ownerpaystrasferfee如果为真，则所有者而不是发送者支付费用
  @param offerrocking如果为true，则流正在执行offerrocking，而不是payments
  @param limitquality不使用低于此质量阈值的流动性
  @param sendmax的花费不超过这个数量
  @param j journal将日志消息写入
  @param flowdebuginfo如果非空，则为调试提供指向flowdebuginfo的指针。
  @返回实际进出金额和结果代码
**/

path::RippleCalc::Output
flow (PaymentSandbox& view,
    STAmount const& deliver,
    AccountID const& src,
    AccountID const& dst,
    STPathSet const& paths,
    bool defaultPaths,
    bool partialPayment,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    boost::optional<Quality> const& limitQuality,
    boost::optional<STAmount> const& sendMax,
    beast::Journal j,
    path::detail::FlowDebugInfo* flowDebugInfo=nullptr);

}  //涟漪

#endif
