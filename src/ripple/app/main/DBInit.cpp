
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

#include <ripple/app/main/DBInit.h>
#include <type_traits>

namespace ripple {

//事务数据库保存事务和公钥
const char* TxnDBName = "transaction.db";
const char* TxnDBInit[] =
{
    "PRAGMA page_size=4096;",
    "PRAGMA synchronous=NORMAL;",
    "PRAGMA journal_mode=WAL;",
    "PRAGMA journal_size_limit=1582080;",
    "PRAGMA max_page_count=2147483646;",

#if (ULONG_MAX > UINT_MAX) && !defined (NO_SQLITE_MMAP)
    "PRAGMA mmap_size=17179869184;",
#endif

    "BEGIN TRANSACTION;",

    "CREATE TABLE IF NOT EXISTS Transactions (                \
        TransID     CHARACTER(64) PRIMARY KEY,  \
        TransType   CHARACTER(24),              \
        FromAcct    CHARACTER(35),              \
        FromSeq     BIGINT UNSIGNED,            \
        LedgerSeq   BIGINT UNSIGNED,            \
        Status      CHARACTER(1),               \
        RawTxn      BLOB,                       \
        TxnMeta     BLOB                        \
    );",
    "CREATE INDEX IF NOT EXISTS TxLgrIndex ON                 \
        Transactions(LedgerSeq);",

    "CREATE TABLE IF NOT EXISTS AccountTransactions (         \
        TransID     CHARACTER(64),              \
        Account     CHARACTER(64),              \
        LedgerSeq   BIGINT UNSIGNED,            \
        TxnSeq      INTEGER                     \
    );",
    "CREATE INDEX IF NOT EXISTS AcctTxIDIndex ON              \
        AccountTransactions(TransID);",
    "CREATE INDEX IF NOT EXISTS AcctTxIndex ON                \
        AccountTransactions(Account, LedgerSeq, TxnSeq, TransID);",
    "CREATE INDEX IF NOT EXISTS AcctLgrIndex ON               \
        AccountTransactions(LedgerSeq, Account, TransID);",

    "END TRANSACTION;"
};

int TxnDBCount = std::extent<decltype(TxnDBInit)>::value;

//分类帐数据库保存分类帐和分类帐确认
const char* LedgerDBInit[] =
{
    "PRAGMA synchronous=NORMAL;",
    "PRAGMA journal_mode=WAL;",
    "PRAGMA journal_size_limit=1582080;",

    "BEGIN TRANSACTION;",

    "CREATE TABLE IF NOT EXISTS Ledgers (                         \
        LedgerHash      CHARACTER(64) PRIMARY KEY,  \
        LedgerSeq       BIGINT UNSIGNED,            \
        PrevHash        CHARACTER(64),              \
        TotalCoins      BIGINT UNSIGNED,            \
        ClosingTime     BIGINT UNSIGNED,            \
        PrevClosingTime BIGINT UNSIGNED,            \
        CloseTimeRes    BIGINT UNSIGNED,            \
        CloseFlags      BIGINT UNSIGNED,            \
        AccountSetHash  CHARACTER(64),              \
        TransSetHash    CHARACTER(64)               \
    );",
    "CREATE INDEX IF NOT EXISTS SeqLedger ON Ledgers(LedgerSeq);",

//当行
//插入。仅在联机删除时相关
    "CREATE TABLE IF NOT EXISTS Validations   (                   \
        LedgerSeq   BIGINT UNSIGNED,                \
        InitialSeq  BIGINT UNSIGNED,                \
        LedgerHash  CHARACTER(64),                  \
        NodePubKey  CHARACTER(56),                  \
        SignTime    BIGINT UNSIGNED,                \
        RawData     BLOB                            \
    );",
    "CREATE INDEX IF NOT EXISTS ValidationsByHash ON              \
        Validations(LedgerHash);",
    "CREATE INDEX IF NOT EXISTS ValidationsBySeq ON              \
        Validations(LedgerSeq);",
    "CREATE INDEX IF NOT EXISTS ValidationsByInitialSeq ON              \
        Validations(InitialSeq, LedgerSeq);",
    "CREATE INDEX IF NOT EXISTS ValidationsByTime ON              \
        Validations(SignTime);",

    "END TRANSACTION;"
};

int LedgerDBCount = std::extent<decltype(LedgerDBInit)>::value;

const char* WalletDBInit[] =
{
    "BEGIN TRANSACTION;",

//必须持久化节点的标识，包括
//用于集群目的。这张桌子有一个
//条目：服务器的唯一标识，但
//通过指定节点可以重写值
//使用[节点种子]在配置文件中标识
//条目。
    "CREATE TABLE IF NOT EXISTS NodeIdentity (      \
        PublicKey       CHARACTER(53),              \
        PrivateKey      CHARACTER(52)               \
    );",

//验证程序清单
    "CREATE TABLE IF NOT EXISTS ValidatorManifests ( \
        RawData          BLOB NOT NULL               \
    );",

    "CREATE TABLE IF NOT EXISTS PublisherManifests ( \
        RawData          BLOB NOT NULL               \
    );",

//wallet.db中存在的旧表和我们
//不再需要或使用。
    "DROP INDEX IF EXISTS SeedNodeNext;",
    "DROP INDEX IF EXISTS SeedDomainNext;",
    "DROP TABLE IF EXISTS Features;",
    "DROP TABLE IF EXISTS TrustedNodes;",
    "DROP TABLE IF EXISTS ValidatorReferrals;",
    "DROP TABLE IF EXISTS IpReferrals;",
    "DROP TABLE IF EXISTS SeedNodes;",
    "DROP TABLE IF EXISTS SeedDomains;",
    "DROP TABLE IF EXISTS Misc;",

    "END TRANSACTION;"
};

int WalletDBCount = std::extent<decltype(WalletDBInit)>::value;

} //涟漪
