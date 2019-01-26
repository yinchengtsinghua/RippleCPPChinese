
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

#include <ripple/core/Config.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/FileUtilities.h>
#include <ripple/basics/Log.h>
#include <ripple/json/json_reader.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/SystemParameters.h>
#include <ripple/net/HTTPClient.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/beast/core/string.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>
#include <fstream>
#include <iostream>
#include <iterator>

namespace ripple {

//
//TODO:使用配置文件之前，请检查其权限。
//

#define SECTION_DEFAULT_NAME    ""

IniFileSections
parseIniFile (std::string const& strInput, const bool bTrim)
{
    std::string strData (strInput);
    std::vector<std::string> vLines;
    IniFileSections secResult;

//将DOS格式转换为Unix。
    boost::algorithm::replace_all (strData, "\r\n", "\n");

//将MacOS格式转换为Unix。
    boost::algorithm::replace_all (strData, "\r", "\n");

    boost::algorithm::split (vLines, strData,
        boost::algorithm::is_any_of ("\n"));

//设置默认节名称。
    std::string strSection  = SECTION_DEFAULT_NAME;

//初始化默认节。
    secResult[strSection]   = IniFileSections::mapped_type ();

//分析每一行。
    for (auto& strValue : vLines)
    {
        if (strValue.empty () || strValue[0] == '#')
        {
//空行或注释，不执行任何操作。
        }
        else if (strValue[0] == '[' && strValue[strValue.length () - 1] == ']')
        {
//新章节。
            strSection              = strValue.substr (1, strValue.length () - 2);
            secResult.emplace(strSection, IniFileSections::mapped_type{});
        }
        else
        {
//另一行用于段落。
            if (bTrim)
                boost::algorithm::trim (strValue);

            if (!strValue.empty ())
                secResult[strSection].push_back (strValue);
        }
    }

    return secResult;
}

IniFileSections::mapped_type*
getIniFileSection (IniFileSections& secSource, std::string const& strSection)
{
    IniFileSections::iterator it;
    IniFileSections::mapped_type* smtResult;
    it  = secSource.find (strSection);
    if (it == secSource.end ())
        smtResult   = 0;
    else
        smtResult   = & (it->second);
    return smtResult;
}

bool getSingleSection (IniFileSections& secSource,
    std::string const& strSection, std::string& strValue, beast::Journal j)
{
    IniFileSections::mapped_type* pmtEntries =
        getIniFileSection (secSource, strSection);
    bool bSingle = pmtEntries && 1 == pmtEntries->size ();

    if (bSingle)
    {
        strValue    = (*pmtEntries)[0];
    }
    else if (pmtEntries)
    {
        JLOG (j.warn()) << boost::str (
            boost::format ("Section [%s]: requires 1 line not %d lines.") %
            strSection % pmtEntries->size ());
    }

    return bSingle;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//配置（已弃用）
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

char const* const Config::configFileName = "rippled.cfg";
char const* const Config::databaseDirName = "db";
char const* const Config::validatorsFileName = "validators.txt";

static
std::string
getEnvVar (char const* name)
{
    std::string value;

    auto const v = getenv (name);

    if (v != nullptr)
        value = v;

    return value;
}

void Config::setupControl(bool bQuiet,
    bool bSilent, bool bStandalone)
{
    QUIET = bQuiet || bSilent;
    SILENT = bSilent;
    RUN_STANDALONE = bStandalone;
}

void Config::setup (std::string const& strConf, bool bQuiet,
    bool bSilent, bool bStandalone)
{
    boost::filesystem::path dataDir;
    std::string strDbPath, strConfFile;

//确定配置和数据目录。
//如果在当前工作中找到配置文件
//目录，使用当前工作目录作为
//配置目录和以“db”为数据的目录
//目录。

    setupControl(bQuiet, bSilent, bStandalone);

    strDbPath = databaseDirName;

    if (!strConf.empty())
        strConfFile = strConf;
    else
        strConfFile = configFileName;

    if (!strConf.empty ())
    {
//--conf=<path>：所有内容都与该文件相关。
        CONFIG_FILE             = strConfFile;
        CONFIG_DIR              = boost::filesystem::absolute (CONFIG_FILE);
        CONFIG_DIR.remove_filename ();
        dataDir                 = CONFIG_DIR / strDbPath;
    }
    else
    {
        CONFIG_DIR              = boost::filesystem::current_path ();
        CONFIG_FILE             = CONFIG_DIR / strConfFile;
        dataDir                 = CONFIG_DIR / strDbPath;

//构造xdg配置和数据主页。
//http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
        std::string strHome          = getEnvVar ("HOME");
        std::string strXdgConfigHome = getEnvVar ("XDG_CONFIG_HOME");
        std::string strXdgDataHome   = getEnvVar ("XDG_DATA_HOME");

        if (boost::filesystem::exists (CONFIG_FILE)
//我们能找出xdg dirs吗？
                || (strHome.empty () && (strXdgConfigHome.empty () || strXdgDataHome.empty ())))
        {
//当前工作目录很好，将DBS放在一个子目录中。
        }
        else
        {
            if (strXdgConfigHome.empty ())
            {
//$xdg_config_home未设置，请使用基于$home的默认值。
                strXdgConfigHome    = strHome + "/.config";
            }

            if (strXdgDataHome.empty ())
            {
//$xdg_data_home未设置，请使用基于$home的默认值。
                strXdgDataHome  = strHome + "/.local/share";
            }

            CONFIG_DIR  = strXdgConfigHome + "/" + systemName ();
            CONFIG_FILE = CONFIG_DIR / strConfFile;
            dataDir    = strXdgDataHome + "/" + systemName ();

            if (!boost::filesystem::exists (CONFIG_FILE))
            {
                CONFIG_DIR  = "/etc/opt/" + systemName ();
                CONFIG_FILE = CONFIG_DIR / strConfFile;
                dataDir = "/var/opt/" + systemName();
            }
        }
    }

//更新默认值
    load ();
    {
//load（）可能为datadir设置了一个新值
        std::string const dbPath (legacy ("database_path"));
        if (!dbPath.empty ())
            dataDir = boost::filesystem::path (dbPath);
        else if (RUN_STANDALONE)
            dataDir.clear();
    }

    if (!dataDir.empty())
    {
        boost::system::error_code ec;
        boost::filesystem::create_directories(dataDir, ec);

        if (ec)
            Throw<std::runtime_error>(
                boost::str(boost::format("Can not create %s") % dataDir));

        legacy("database_path", boost::filesystem::absolute(dataDir).string());
    }

    HTTPClient::initializeSSLContext(*this, j_);

    if (RUN_STANDALONE)
        LEDGER_HISTORY = 0;
}

void Config::load ()
{
//注意：这将写入CERR，因为我们希望不能保留
//用于编写JSON响应（以便stdout可以是
//例如，管道）
    if (!QUIET)
        std::cerr << "Loading: " << CONFIG_FILE << "\n";

    boost::system::error_code ec;
    auto const fileContents = getFileContents(ec, CONFIG_FILE);

    if (ec)
    {
        std::cerr << "Failed to read '" << CONFIG_FILE << "'." <<
            ec.value() << ": " << ec.message() << std::endl;
        return;
    }

    loadFromString (fileContents);
}

void Config::loadFromString (std::string const& fileContents)
{
    IniFileSections secConfig = parseIniFile (fileContents, true);

    build (secConfig);

    if (auto s = getIniFileSection (secConfig, SECTION_IPS))
        IPS = *s;

    if (auto s = getIniFileSection (secConfig, SECTION_IPS_FIXED))
        IPS_FIXED = *s;

    if (auto s = getIniFileSection (secConfig, SECTION_SNTP))
        SNTP_SERVERS = *s;

    {
        std::string dbPath;
        if (getSingleSection (secConfig, "database_path", dbPath, j_))
        {
            boost::filesystem::path p(dbPath);
            legacy("database_path",
                   boost::filesystem::absolute (p).string ());
        }
    }

    std::string strTemp;

    if (getSingleSection (secConfig, SECTION_PEER_PRIVATE, strTemp, j_))
        PEER_PRIVATE = beast::lexicalCastThrow <bool> (strTemp);

    if (getSingleSection (secConfig, SECTION_PEERS_MAX, strTemp, j_))
        PEERS_MAX = std::max (0, beast::lexicalCastThrow <int> (strTemp));

    if (getSingleSection (secConfig, SECTION_NODE_SIZE, strTemp, j_))
    {
        if (boost::beast::detail::iequals(strTemp, "tiny"))
            NODE_SIZE = 0;
        else if (boost::beast::detail::iequals(strTemp, "small"))
            NODE_SIZE = 1;
        else if (boost::beast::detail::iequals(strTemp, "medium"))
            NODE_SIZE = 2;
        else if (boost::beast::detail::iequals(strTemp, "large"))
            NODE_SIZE = 3;
        else if (boost::beast::detail::iequals(strTemp, "huge"))
            NODE_SIZE = 4;
        else
        {
            NODE_SIZE = beast::lexicalCastThrow <int> (strTemp);

            if (NODE_SIZE < 0)
                NODE_SIZE = 0;
            else if (NODE_SIZE > 4)
                NODE_SIZE = 4;
        }
    }

    if (getSingleSection (secConfig, SECTION_SIGNING_SUPPORT, strTemp, j_))
        signingEnabled_     = beast::lexicalCastThrow <bool> (strTemp);

    if (getSingleSection (secConfig, SECTION_ELB_SUPPORT, strTemp, j_))
        ELB_SUPPORT         = beast::lexicalCastThrow <bool> (strTemp);

    if (getSingleSection (secConfig, SECTION_WEBSOCKET_PING_FREQ, strTemp, j_))
        WEBSOCKET_PING_FREQ = std::chrono::seconds{beast::lexicalCastThrow <int>(strTemp)};

    getSingleSection (secConfig, SECTION_SSL_VERIFY_FILE, SSL_VERIFY_FILE, j_);
    getSingleSection (secConfig, SECTION_SSL_VERIFY_DIR, SSL_VERIFY_DIR, j_);

    if (getSingleSection (secConfig, SECTION_SSL_VERIFY, strTemp, j_))
        SSL_VERIFY          = beast::lexicalCastThrow <bool> (strTemp);

    if (exists(SECTION_VALIDATION_SEED) && exists(SECTION_VALIDATOR_TOKEN))
        Throw<std::runtime_error> (
            "Cannot have both [" SECTION_VALIDATION_SEED "] "
            "and [" SECTION_VALIDATOR_TOKEN "] config sections");

    if (getSingleSection (secConfig, SECTION_NETWORK_QUORUM, strTemp, j_))
        NETWORK_QUORUM      = beast::lexicalCastThrow <std::size_t> (strTemp);

    if (getSingleSection (secConfig, SECTION_FEE_ACCOUNT_RESERVE, strTemp, j_))
        FEE_ACCOUNT_RESERVE = beast::lexicalCastThrow <std::uint64_t> (strTemp);

    if (getSingleSection (secConfig, SECTION_FEE_OWNER_RESERVE, strTemp, j_))
        FEE_OWNER_RESERVE   = beast::lexicalCastThrow <std::uint64_t> (strTemp);

    if (getSingleSection (secConfig, SECTION_FEE_OFFER, strTemp, j_))
        FEE_OFFER           = beast::lexicalCastThrow <int> (strTemp);

    if (getSingleSection (secConfig, SECTION_FEE_DEFAULT, strTemp, j_))
        FEE_DEFAULT         = beast::lexicalCastThrow <int> (strTemp);

    if (getSingleSection (secConfig, SECTION_LEDGER_HISTORY, strTemp, j_))
    {
        if (boost::beast::detail::iequals(strTemp, "full"))
            LEDGER_HISTORY = 1000000000u;
        else if (boost::beast::detail::iequals(strTemp, "none"))
            LEDGER_HISTORY = 0;
        else
            LEDGER_HISTORY = beast::lexicalCastThrow <std::uint32_t> (strTemp);
    }

    if (getSingleSection (secConfig, SECTION_FETCH_DEPTH, strTemp, j_))
    {
        if (boost::beast::detail::iequals(strTemp, "none"))
            FETCH_DEPTH = 0;
        else if (boost::beast::detail::iequals(strTemp, "full"))
            FETCH_DEPTH = 1000000000u;
        else
            FETCH_DEPTH = beast::lexicalCastThrow <std::uint32_t> (strTemp);

        if (FETCH_DEPTH < 10)
            FETCH_DEPTH = 10;
    }

    if (getSingleSection (secConfig, SECTION_PATH_SEARCH_OLD, strTemp, j_))
        PATH_SEARCH_OLD     = beast::lexicalCastThrow <int> (strTemp);
    if (getSingleSection (secConfig, SECTION_PATH_SEARCH, strTemp, j_))
        PATH_SEARCH         = beast::lexicalCastThrow <int> (strTemp);
    if (getSingleSection (secConfig, SECTION_PATH_SEARCH_FAST, strTemp, j_))
        PATH_SEARCH_FAST    = beast::lexicalCastThrow <int> (strTemp);
    if (getSingleSection (secConfig, SECTION_PATH_SEARCH_MAX, strTemp, j_))
        PATH_SEARCH_MAX     = beast::lexicalCastThrow <int> (strTemp);

    if (getSingleSection (secConfig, SECTION_DEBUG_LOGFILE, strTemp, j_))
        DEBUG_LOGFILE       = strTemp;

    if (getSingleSection (secConfig, SECTION_WORKERS, strTemp, j_))
        WORKERS      = beast::lexicalCastThrow <std::size_t> (strTemp);

//不为独立模式加载受信任的验证程序配置
    if (! RUN_STANDALONE)
    {
//如果显式指定了文件，则如果
//路径格式不正确，或者文件不存在，或者
//不是文件。
//如果指定的文件不是绝对路径，则查找
//与配置文件位于同一目录中。
//如果没有指定路径，则查找validators.txt
//与配置文件在同一目录中，但不要抱怨
//如果我们找不到。
        boost::filesystem::path validatorsFile;

        if (getSingleSection (secConfig, SECTION_VALIDATORS_FILE, strTemp, j_))
        {
            validatorsFile = strTemp;

            if (validatorsFile.empty ())
                Throw<std::runtime_error> (
                    "Invalid path specified in [" SECTION_VALIDATORS_FILE "]");

            if (!validatorsFile.is_absolute() && !CONFIG_DIR.empty())
                validatorsFile = CONFIG_DIR / validatorsFile;

            if (!boost::filesystem::exists (validatorsFile))
                Throw<std::runtime_error> (
                    "The file specified in [" SECTION_VALIDATORS_FILE "] "
                    "does not exist: " + validatorsFile.string());

            else if (!boost::filesystem::is_regular_file (validatorsFile) &&
                    !boost::filesystem::is_symlink (validatorsFile))
                Throw<std::runtime_error> (
                    "Invalid file specified in [" SECTION_VALIDATORS_FILE "]: " +
                    validatorsFile.string());
        }
        else if (!CONFIG_DIR.empty())
        {
            validatorsFile = CONFIG_DIR / validatorsFileName;

            if (!validatorsFile.empty ())
            {
                if(!boost::filesystem::exists (validatorsFile))
                    validatorsFile.clear();
                else if (!boost::filesystem::is_regular_file (validatorsFile) &&
                        !boost::filesystem::is_symlink (validatorsFile))
                    validatorsFile.clear();
            }
        }

        if (!validatorsFile.empty () &&
                boost::filesystem::exists (validatorsFile) &&
                (boost::filesystem::is_regular_file (validatorsFile) ||
                boost::filesystem::is_symlink (validatorsFile)))
        {
            boost::system::error_code ec;
            auto const data = getFileContents(ec, validatorsFile);
            if (ec)
            {
                Throw<std::runtime_error>("Failed to read '" +
                    validatorsFile.string() + "'." +
                    std::to_string(ec.value()) + ": " + ec.message());
            }

            auto iniFile = parseIniFile (data, true);

            auto entries = getIniFileSection (
                iniFile,
                SECTION_VALIDATORS);

            if (entries)
                section (SECTION_VALIDATORS).append (*entries);

            auto valKeyEntries = getIniFileSection(
                iniFile,
                SECTION_VALIDATOR_KEYS);

            if (valKeyEntries)
                section (SECTION_VALIDATOR_KEYS).append (*valKeyEntries);

            auto valSiteEntries = getIniFileSection(
                iniFile,
                SECTION_VALIDATOR_LIST_SITES);

            if (valSiteEntries)
                section (SECTION_VALIDATOR_LIST_SITES).append (*valSiteEntries);

            auto valListKeys = getIniFileSection(
                iniFile,
                SECTION_VALIDATOR_LIST_KEYS);

            if (valListKeys)
                section (SECTION_VALIDATOR_LIST_KEYS).append (*valListKeys);

            if (!entries && !valKeyEntries && !valListKeys)
                Throw<std::runtime_error> (
                    "The file specified in [" SECTION_VALIDATORS_FILE "] "
                    "does not contain a [" SECTION_VALIDATORS "], "
                    "[" SECTION_VALIDATOR_KEYS "] or "
                    "[" SECTION_VALIDATOR_LIST_KEYS "]"
                    " section: " +
                    validatorsFile.string());
        }

//合并[验证程序密钥]和[验证程序]
        section (SECTION_VALIDATORS).append (
            section (SECTION_VALIDATOR_KEYS).lines ());

        if (! section (SECTION_VALIDATOR_LIST_SITES).lines().empty() &&
            section (SECTION_VALIDATOR_LIST_KEYS).lines().empty())
        {
            Throw<std::runtime_error> (
                "[" + std::string(SECTION_VALIDATOR_LIST_KEYS) +
                "] config section is missing");
        }
    }

    {
        auto const part = section("features");
        for(auto const& s : part.values())
        {
            if (auto const f = getRegisteredFeature(s))
                features.insert(*f);
            else
                Throw<std::runtime_error>(
                    "Unknown feature: " + s + "  in config file.");
        }
    }
}

int Config::getSize (SizedItemName item) const
{
SizedItem sizeTable[] =   //小-中-大-大
    {

        { siSweepInterval,      {   10,     30,     60,     90,         120     } },

        { siLedgerFetch,        {   2,      3,      5,      5,          8       } },

        { siNodeCacheSize,      {   16384,  32768,  131072, 262144,     524288  } },
        { siNodeCacheAge,       {   60,     90,     120,    900,        1800    } },

        { siTreeCacheSize,      {   128000, 256000, 512000, 768000,     2048000 } },
        { siTreeCacheAge,       {   30,     60,     90,     120,        900     } },

        { siSLECacheSize,       {   4096,   8192,   16384,  65536,      131072  } },
        { siSLECacheAge,        {   30,     60,     90,     120,        300     } },

        { siLedgerSize,         {   32,     128,    256,    384,        768     } },
        { siLedgerAge,          {   30,     90,     180,    240,        900     } },

        { siHashNodeDBCache,    {   4,      12,     24,     64,         128     } },
        { siTxnDBCache,         {   4,      12,     24,     64,         128     } },
        { siLgrDBCache,         {   4,      8,      16,     32,         128     } },
    };

    for (int i = 0; i < (sizeof (sizeTable) / sizeof (SizedItem)); ++i)
    {
        if (sizeTable[i].item == item)
            return sizeTable[i].sizes[NODE_SIZE];
    }

    assert (false);
    return -1;
}

boost::filesystem::path Config::getDebugLogFile () const
{
    auto log_file = DEBUG_LOGFILE;

    if (!log_file.empty () && !log_file.is_absolute ())
    {
//除非指定了日志文件的绝对路径，否则
//路径是相对于配置文件目录的。
        log_file = boost::filesystem::absolute (
            log_file, CONFIG_DIR);
    }

    if (!log_file.empty ())
    {
        auto log_dir = log_file.parent_path ();

        if (!boost::filesystem::is_directory (log_dir))
        {
            boost::system::error_code ec;
            boost::filesystem::create_directories (log_dir, ec);

//如果失败，我们会发出警告，但继续，以便调用代码可以
//决定如何处理这种情况。
            if (ec)
            {
                std::cerr <<
                    "Unable to create log file path " << log_dir <<
                    ": " << ec.message() << '\n';
            }
        }
    }

    return log_file;
}

} //涟漪
