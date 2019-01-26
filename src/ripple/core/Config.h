
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

#ifndef RIPPLE_CORE_CONFIG_H_INCLUDED
#define RIPPLE_CORE_CONFIG_H_INCLUDED

#include <ripple/basics/BasicConfig.h>
#include <ripple/basics/base_uint.h>
#include <ripple/protocol/SystemParameters.h> //vfalc破坏水平化
#include <ripple/beast/net/IPEndpoint.h>
#include <boost/beast/core/string.hpp>
#include <ripple/beast/utility/Journal.h>
#include <boost/filesystem.hpp> //vfalc修复：此包含不应在此处
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace ripple {

class Rules;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

enum SizedItemName
{
    siSweepInterval,
    siNodeCacheSize,
    siNodeCacheAge,
    siTreeCacheSize,
    siTreeCacheAge,
    siSLECacheSize,
    siSLECacheAge,
    siLedgerSize,
    siLedgerAge,
    siLedgerFetch,
    siHashNodeDBCache,
    siTxnDBCache,
    siLgrDBCache,
};

struct SizedItem
{
    SizedItemName   item;
    int             sizes[5];
};

//整个派生类已弃用。
//对于新的配置信息，请使用暗示的样式
//在基类中。有关现有配置信息
//尝试重构代码以使用新样式。
//
class Config : public BasicConfig
{
public:
//与配置文件位置和目录相关的设置
    static char const* const configFileName;
    static char const* const databaseDirName;
    static char const* const validatorsFileName;

    /*返回调试日志文件的完整路径和文件名。*/
    boost::filesystem::path getDebugLogFile () const;

private:
    boost::filesystem::path CONFIG_FILE;
public:
    boost::filesystem::path CONFIG_DIR;
private:
    boost::filesystem::path DEBUG_LOGFILE;

    void load ();
    beast::Journal j_;

bool QUIET = false;          //最小化日志的冗长性。
bool SILENT = false;         //启动后没有输出到控制台。
    /*在独立模式下操作。

        在独立模式下：

        -未尝试或接受对等连接
        -分类帐不是自动提前的。
        -如果未加载分类帐，则为具有根目录的默认分类帐
          帐户已创建。
    **/

    bool                        RUN_STANDALONE = false;

    /*确定在给定帐户的机密种子的情况下，服务器是否要签署Tx。

        在过去，这是允许的，但此功能可以具有安全性
        启示。新的默认设置是不允许此功能，但是
        包括一个配置选项来启用此功能。
    **/

    bool signingEnabled_ = false;

public:
    bool doImport = false;
    bool nodeToShard = false;
    bool validateShards = false;
    bool ELB_SUPPORT = false;

std::vector<std::string>    IPS;                    //来自rippled.cfg的对等IP。
std::vector<std::string>    IPS_FIXED;              //固定来自rippled.cfg的对等IP。
std::vector<std::string>    SNTP_SERVERS;           //来自rippled.cfg的sntp服务器。

    enum StartUpType
    {
        FRESH,
        NORMAL,
        LOAD,
        LOAD_FILE,
        REPLAY,
        NETWORK
    };
    StartUpType                 START_UP = NORMAL;

    bool                        START_VALID = false;

    std::string                 START_LEDGER;

//网络参数
int const                   TRANSACTION_FEE_BASE = 10;   //参考交易成本的费用单位数

//注意：以下参数与UNL或信任根本不相关
std::size_t                 NETWORK_QUORUM = 0;         //考虑网络存在的最小节点数

//对等网络参数
bool                        PEER_PRIVATE = false;           //要求对等方不要中继当前IP。
    int                         PEERS_MAX = 0;

    std::chrono::seconds        WEBSOCKET_PING_FREQ = std::chrono::minutes {5};

//路径搜索
    int                         PATH_SEARCH_OLD = 7;
    int                         PATH_SEARCH = 7;
    int                         PATH_SEARCH_FAST = 2;
    int                         PATH_SEARCH_MAX = 10;

//验证
boost::optional<std::size_t> VALIDATION_QUORUM;     //验证以考虑分类帐权威性

    std::uint64_t                      FEE_DEFAULT = 10;
    std::uint64_t                      FEE_ACCOUNT_RESERVE = 200*SYSTEM_CURRENCY_PARTS;
    std::uint64_t                      FEE_OWNER_RESERVE = 50*SYSTEM_CURRENCY_PARTS;
    std::uint64_t                      FEE_OFFER = 10;

//节点存储配置
    std::uint32_t                      LEDGER_HISTORY = 256;
    std::uint32_t                      FETCH_DEPTH = 1000000000;
    int                         NODE_SIZE = 0;

    bool                        SSL_VERIFY = true;
    std::string                 SSL_VERIFY_FILE;
    std::string                 SSL_VERIFY_DIR;

//线程池配置
    std::size_t                 WORKERS = 0;

//这些将覆盖命令行客户端设置
    boost::optional<beast::IP::Endpoint> rpc_ip;

    std::unordered_set<uint256, beast::uhash<>> features;

public:
    Config()
    : j_ {beast::Journal::getNullSink()}
    { }

    int getSize (SizedItemName) const;
    /*小心确保这些bool参数
        顺序正确。*/

    void setup (std::string const& strConf, bool bQuiet,
        bool bSilent, bool bStandalone);
    void setupControl (bool bQuiet,
        bool bSilent, bool bStandalone);

    /*
     *从字符串的内容加载配置。
     *
     *@param filecontents表示配置内容的字符串。
     **/

    void loadFromString (std::string const& fileContents);

    bool quiet() const { return QUIET; }
    bool silent() const { return SILENT; }
    bool standalone() const { return RUN_STANDALONE; }

    bool canSign() const { return signingEnabled_; }
};

} //涟漪

#endif
