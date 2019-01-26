
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

#ifndef RIPPLE_RPC_HANDLERS_HANDLERS_H_INCLUDED
#define RIPPLE_RPC_HANDLERS_HANDLERS_H_INCLUDED

#include <ripple/rpc/handlers/LedgerHandler.h>

namespace ripple {

Json::Value doAccountCurrencies     (RPC::Context&);
Json::Value doAccountInfo           (RPC::Context&);
Json::Value doAccountLines          (RPC::Context&);
Json::Value doAccountChannels       (RPC::Context&);
Json::Value doAccountObjects        (RPC::Context&);
Json::Value doAccountOffers         (RPC::Context&);
Json::Value doAccountTx             (RPC::Context&);
Json::Value doAccountTxSwitch       (RPC::Context&);
Json::Value doAccountTxOld          (RPC::Context&);
Json::Value doBookOffers            (RPC::Context&);
Json::Value doBlackList             (RPC::Context&);
Json::Value doCanDelete             (RPC::Context&);
Json::Value doChannelAuthorize      (RPC::Context&);
Json::Value doChannelVerify         (RPC::Context&);
Json::Value doConnect               (RPC::Context&);
Json::Value doConsensusInfo         (RPC::Context&);
Json::Value doDepositAuthorized     (RPC::Context&);
Json::Value doDownloadShard         (RPC::Context&);
Json::Value doFeature               (RPC::Context&);
Json::Value doFee                   (RPC::Context&);
Json::Value doFetchInfo             (RPC::Context&);
Json::Value doGatewayBalances       (RPC::Context&);
Json::Value doGetCounts             (RPC::Context&);
Json::Value doLedgerAccept          (RPC::Context&);
Json::Value doLedgerCleaner         (RPC::Context&);
Json::Value doLedgerClosed          (RPC::Context&);
Json::Value doLedgerCurrent         (RPC::Context&);
Json::Value doLedgerData            (RPC::Context&);
Json::Value doLedgerEntry           (RPC::Context&);
Json::Value doLedgerHeader          (RPC::Context&);
Json::Value doLedgerRequest         (RPC::Context&);
Json::Value doLogLevel              (RPC::Context&);
Json::Value doLogRotate             (RPC::Context&);
Json::Value doNoRippleCheck         (RPC::Context&);
Json::Value doOwnerInfo             (RPC::Context&);
Json::Value doPathFind              (RPC::Context&);
Json::Value doPeers                 (RPC::Context&);
Json::Value doPing                  (RPC::Context&);
Json::Value doPrint                 (RPC::Context&);
Json::Value doRandom                (RPC::Context&);
Json::Value doRipplePathFind        (RPC::Context&);
Json::Value doServerInfo            (RPC::Context&); //为人类
Json::Value doServerState           (RPC::Context&); //对于机器
Json::Value doSign                  (RPC::Context&);
Json::Value doSignFor               (RPC::Context&);
Json::Value doCrawlShards           (RPC::Context&);
Json::Value doStop                  (RPC::Context&);
Json::Value doSubmit                (RPC::Context&);
Json::Value doSubmitMultiSigned     (RPC::Context&);
Json::Value doSubscribe             (RPC::Context&);
Json::Value doTransactionEntry      (RPC::Context&);
Json::Value doTx                    (RPC::Context&);
Json::Value doTxHistory             (RPC::Context&);
Json::Value doUnlList               (RPC::Context&);
Json::Value doUnsubscribe           (RPC::Context&);
Json::Value doValidationCreate      (RPC::Context&);
Json::Value doWalletPropose         (RPC::Context&);
Json::Value doValidators            (RPC::Context&);
Json::Value doValidatorListSites    (RPC::Context&);
} //涟漪

#endif
