
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

#include <ripple/protocol/HashPrefix.h>

namespace ripple {

//前缀代码是Ripple协议的一部分。
//现有的代码不能随意更改。

HashPrefix const HashPrefix::transactionID               ('T', 'X', 'N');
HashPrefix const HashPrefix::txNode                      ('S', 'N', 'D');
HashPrefix const HashPrefix::leafNode                    ('M', 'L', 'N');
HashPrefix const HashPrefix::innerNode                   ('M', 'I', 'N');
HashPrefix const HashPrefix::innerNodeV2                 ('I', 'N', 'R');
HashPrefix const HashPrefix::ledgerMaster                ('L', 'W', 'R');
HashPrefix const HashPrefix::txSign                      ('S', 'T', 'X');
HashPrefix const HashPrefix::txMultiSign                 ('S', 'M', 'T');
HashPrefix const HashPrefix::validation                  ('V', 'A', 'L');
HashPrefix const HashPrefix::proposal                    ('P', 'R', 'P');
HashPrefix const HashPrefix::manifest                    ('M', 'A', 'N');
HashPrefix const HashPrefix::paymentChannelClaim         ('C', 'L', 'M');

} //涟漪
