
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

#ifndef RIPPLE_PROTOCOL_LEDGERFORMATS_H_INCLUDED
#define RIPPLE_PROTOCOL_LEDGERFORMATS_H_INCLUDED

#include <ripple/protocol/KnownFormats.h>

namespace ripple {

/*分类帐分录类型。

    它们存储在序列化数据中。

    @注意，更改这些值会导致一个硬分叉。

    @ingroup协议
**/

//用作交易记录类型或分类账分录类型。
enum LedgerEntryType
{
    /*特殊类型，任何类型
        当keylet中的类型未知时使用，
        例如在构建元数据时。
    **/

    ltANY = -3,

    /*特殊类型，任何不是目录的类型
        当keylet中的类型未知时使用，
        例如在迭代时
    **/

    ltCHILD             = -2,

    ltINVALID           = -1,

//————————————————————————————————————————————————————————————————————

    ltACCOUNT_ROOT      = 'a',

    /*目录节点。

        目录是一个向量256位值。通常它们代表
        分类帐中其他对象的哈希。

        以仅追加的方式使用。

        （还有更多信息，请参阅模板）
    **/

    ltDIR_NODE          = 'd',

    ltRIPPLE_STATE      = 'r',

    ltTICKET            = 'T',

    ltSIGNER_LIST       = 'S',

    ltOFFER             = 'o',

    ltLEDGER_HASHES     = 'h',

    ltAMENDMENTS        = 'f',

    ltFEE_SETTINGS      = 's',

    ltESCROW            = 'u',

//简单单向xrp通道
    ltPAYCHAN           = 'x',

    ltCHECK             = 'C',

    ltDEPOSIT_PREAUTH   = 'p',

//不再使用或支持。留在这里以防意外
//重新分配分类帐类型。
    ltNICKNAME          = 'n',

    ltNotUsed01         = 'c',
};

/*
    @ingroup协议
**/

//用作计算分类帐索引（键）的前缀。
enum LedgerNameSpace
{
    spaceAccount        = 'a',
    spaceDirNode        = 'd',
    spaceGenerator      = 'g',
    spaceRipple         = 'r',
spaceOffer          = 'o',  //报价条目。
spaceOwnerDir       = 'O',  //帐户拥有的物品目录。
spaceBookDir        = 'B',  //订购书目录。
    spaceContract       = 'c',
    spaceSkipList       = 's',
    spaceEscrow         = 'u',
    spaceAmendment      = 'f',
    spaceFee            = 'e',
    spaceTicket         = 'T',
    spaceSignerList     = 'S',
    spaceXRPUChannel    = 'x',
    spaceCheck          = 'C',
    spaceDepositPreauth = 'p',

//不再使用或支持。留下来保留空间
//避免空间的意外重用。
    spaceNickname       = 'n',
};

/*
    @ingroup协议
**/

enum LedgerSpecificFlags
{
//长根
lsfPasswordSpent    = 0x00010000,   //如果使用密码设置费，则为真。
lsfRequireDestTag   = 0x00020000,   //是的，需要付款的destinationtag。
lsfRequireAuth      = 0x00040000,   //是的，要求授权持有借据。
lsfDisallowXRP      = 0x00080000,   //是的，不允许发送xrp。
lsfDisableMaster    = 0x00100000,   //是，强制常规键
lsfNoFreeze         = 0x00200000,   //是的，不能冻结涟漪状态
lsfGlobalFreeze     = 0x00400000,   //是的，所有资产都冻结了
lsfDefaultRipple    = 0x00800000,   //是的，默认情况下信任线允许波动
lsfDepositAuth      = 0x01000000,   //是的，所有存款都需要授权

//洛福特
    lsfPassive          = 0x00010000,
lsfSell             = 0x00020000,   //是的，出价是作为出售。

//拉普拉斯状态
lsfLowReserve       = 0x00010000,   //是的，如果条目计入保留。
    lsfHighReserve      = 0x00020000,
    lsfLowAuth          = 0x00040000,
    lsfHighAuth         = 0x00080000,
    lsfLowNoRipple      = 0x00100000,
    lsfHighNoRipple     = 0x00200000,
lsfLowFreeze        = 0x00400000,   //是，低端已设置冻结标志
lsfHighFreeze       = 0x00800000,   //是的，高端已设置冻结标志

//LTSwitter列表
lsfOneOwnerCount    = 0x00010000,   //是的，只使用一个所有者计数
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*保存已知分类帐分录格式的列表。
**/

class LedgerFormats : public KnownFormats <LedgerEntryType>
{
private:
    LedgerFormats ();

public:
    static LedgerFormats const& getInstance ();

private:
    void addCommonFields (Item& item) override;
};

} //涟漪

#endif
