
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

#ifndef RIPPLE_PROTOCOL_TXFORMATS_H_INCLUDED
#define RIPPLE_PROTOCOL_TXFORMATS_H_INCLUDED

#include <ripple/protocol/KnownFormats.h>

namespace ripple {

/*事务类型标识符。

    这些是二进制消息格式的一部分。

    @ingroup协议
**/

enum TxType
{
    ttINVALID           = -1,

    ttPAYMENT           = 0,
    ttESCROW_CREATE     = 1,
    ttESCROW_FINISH     = 2,
    ttACCOUNT_SET       = 3,
    ttESCROW_CANCEL     = 4,
    ttREGULAR_KEY_SET   = 5,
ttNICKNAME_SET      = 6, //打开
    ttOFFER_CREATE      = 7,
    ttOFFER_CANCEL      = 8,
    no_longer_used      = 9,
    ttTICKET_CREATE     = 10,
    ttTICKET_CANCEL     = 11,
    ttSIGNER_LIST_SET   = 12,
    ttPAYCHAN_CREATE    = 13,
    ttPAYCHAN_FUND      = 14,
    ttPAYCHAN_CLAIM     = 15,
    ttCHECK_CREATE      = 16,
    ttCHECK_CASH        = 17,
    ttCHECK_CANCEL      = 18,
    ttDEPOSIT_PREAUTH   = 19,
    ttTRUST_SET         = 20,

    ttAMENDMENT         = 100,
    ttFEE               = 101,
};

/*管理已知事务格式的列表。
**/

class TxFormats : public KnownFormats <TxType>
{
private:
    void addCommonFields (Item& item) override;

public:
    /*创建对象。
        这将加载对象的所有已知事务格式。
    **/

    TxFormats ();

    static TxFormats const& getInstance ();
};

} //涟漪

#endif
