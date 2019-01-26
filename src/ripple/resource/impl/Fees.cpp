
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

#include <ripple/resource/Fees.h>

namespace ripple {
namespace Resource {

Charge const feeInvalidRequest    (  100, "malformed request"      );
Charge const feeRequestNoReply    (   10, "unsatisfiable request"  );
Charge const feeInvalidSignature  ( 1000, "invalid signature"      );
Charge const feeUnwantedData      (  150, "useless data"           );
Charge const feeBadData           (  200, "invalid data"           );

Charge const feeInvalidRPC        (  100, "malformed RPC"          );
Charge const feeReferenceRPC      (   20, "reference RPC"          );
Charge const feeExceptionRPC      (  100, "exceptioned RPC"        );
Charge const feeMediumBurdenRPC   (  400, "medium RPC"             );
Charge const feeHighBurdenRPC     ( 3000, "heavy RPC"              );

Charge const feeLightPeer         (    1, "trivial peer request"   );
Charge const feeMediumBurdenPeer  (  250, "moderate peer request"  );
Charge const feeHighBurdenPeer    ( 2000, "heavy peer request"     );

Charge const feeWarning           ( 2000, "received warning"       );
Charge const feeDrop              ( 3000, "dropped"                );

}
}
