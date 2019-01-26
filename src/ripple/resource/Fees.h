
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

#ifndef RIPPLE_RESOURCE_FEES_H_INCLUDED
#define RIPPLE_RESOURCE_FEES_H_INCLUDED

#include <ripple/resource/Charge.h>

namespace ripple {
namespace Resource {

/*对服务器施加负载的收费计划。*/
/*@ {*/
extern Charge const feeInvalidRequest;        //我们可以立即知道的请求无效
extern Charge const feeRequestNoReply;        //我们不能满足的要求
extern Charge const feeInvalidSignature;      //必须检查其签名但失败的对象
extern Charge const feeUnwantedData;          //我们没有用的数据
extern Charge const feeBadData;               //在拒绝之前我们必须验证的数据

//RPC负载
extern Charge const feeInvalidRPC;            //我们可以立即判断的RPC请求无效。
extern Charge const feeReferenceRPC;          //默认“引用”未指定的加载
extern Charge const feeExceptionRPC;          //导致异常的RPC加载
extern Charge const feeLightRPC;              //普通的rpc命令
extern Charge const feeLowBurdenRPC;          //稍微繁重的RPC负载
extern Charge const feeMediumBurdenRPC;       //有点繁重的RPC负载
extern Charge const feeHighBurdenRPC;         //非常繁重的RPC负载
extern Charge const feePathFindUpdate;        //对现有pf请求的更新

//对等负载
extern Charge const feeLightPeer;             //不需要答复
extern Charge const feeLowBurdenPeer;         //快速/便宜，回答轻微
extern Charge const feeMediumBurdenPeer;      //需要一些工作
extern Charge const feeHighBurdenPeer;        //广泛的工作

//好东西
extern Charge const feeNewTrustedNote;        //我们信任的新交易/验证/建议
extern Charge const feeNewValidTx;            //新的有效交易
extern Charge const feeSatisfiedRequest;      //我们要求的数据

//行政的
extern Charge const feeWarning;               //收到警告的费用
extern Charge const feeDrop;                  //因超载而降低的成本
/*@ }*/

}
}

#endif
