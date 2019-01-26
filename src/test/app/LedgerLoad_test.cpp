
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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

#include <ripple/beast/unit_test.h>
#include <test/jtx.h>
#include <test/jtx/Env.h>
#include <ripple/beast/utility/temp_dir.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/SField.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

namespace ripple {

class LedgerLoad_test : public beast::unit_test::suite
{
    auto static ledgerConfig(
        std::unique_ptr<Config> cfg,
        std::string const& dbPath,
        std::string const& ledger,
        Config::StartUpType type)
    {
        cfg->START_LEDGER = ledger;
        cfg->START_UP = type;
        assert(! dbPath.empty());
        cfg->legacy("database_path", dbPath);
        return cfg;
    }

//测试用例设置
    struct SetupData
    {
        std::string const dbPath;
        std::string ledgerFile;
        Json::Value ledger;
        Json::Value hashes;
    };

    SetupData
    setupLedger(beast::temp_dir const& td)
    {
        using namespace test::jtx;
        SetupData retval = {td.path()};

        retval.ledgerFile = td.file("ledgerdata.json");

        Env env {*this};
        Account prev;

        for(auto i = 0; i < 20; ++i)
        {
            Account acct {"A" + std::to_string(i)};
            env.fund(XRP(10000), acct);
            env.close();
            if(i > 0)
            {
                env.trust(acct["USD"](1000), prev);
                env(pay(acct, prev, acct["USD"](5)));
            }
            env(offer(acct, XRP(100), acct["USD"](1)));
            env.close();
            prev = std::move(acct);
        }

        retval.ledger = env.rpc ("ledger", "current", "full") [jss::result];
        BEAST_EXPECT(retval.ledger[jss::ledger][jss::accountState].size() == 101);

        retval.hashes = [&] {
            for(auto const& it : retval.ledger[jss::ledger][jss::accountState])
            {
                if(it[sfLedgerEntryType.fieldName] == "LedgerHashes")
                    return it[sfHashes.fieldName];
            }
            return Json::Value {};
        }();

        BEAST_EXPECT(retval.hashes.size() == 41);

//将此分类帐数据写入文件。
        std::ofstream o (retval.ledgerFile, std::ios::out | std::ios::trunc);
        o << to_string(retval.ledger);
        o.close();
        return retval;
    }

    void
    testLoad (SetupData const& sd)
    {
        testcase ("Load a saved ledger");
        using namespace test::jtx;

//使用为启动指定的分类帐文件创建新env
        Env env(*this,
            envconfig( ledgerConfig,
                sd.dbPath, sd.ledgerFile, Config::LOAD_FILE));
        auto jrb = env.rpc ( "ledger", "current", "full") [jss::result];
        BEAST_EXPECT(
            sd.ledger[jss::ledger][jss::accountState].size() ==
            jrb[jss::ledger][jss::accountState].size());
    }

    void
    testBadFiles (SetupData const& sd)
    {
        testcase ("Load ledger: Bad Files");
        using namespace test::jtx;
        using namespace boost::filesystem;

//空路径
        except ([&]
        {
            Env env(*this,
                envconfig( ledgerConfig,
                    sd.dbPath, "", Config::LOAD_FILE));
        });

//文件不存在
        except ([&]
        {
            Env env(*this,
                envconfig( ledgerConfig,
                    sd.dbPath, "badfile.json", Config::LOAD_FILE));
        });

//生成损坏的分类帐文件版本（最后10个字节已删除）。
        boost::system::error_code ec;
        auto ledgerFileCorrupt =
            boost::filesystem::path{sd.dbPath} / "ledgerdata_bad.json";
        copy_file(
            sd.ledgerFile,
            ledgerFileCorrupt,
            copy_option::overwrite_if_exists,
            ec);
        if(! BEAST_EXPECTS(!ec, ec.message()))
            return;
        auto filesize = file_size(ledgerFileCorrupt, ec);
        if(! BEAST_EXPECTS(!ec, ec.message()))
            return;
        resize_file(ledgerFileCorrupt, filesize - 10, ec);
        if(! BEAST_EXPECTS(!ec, ec.message()))
            return;

        except ([&]
        {
            Env env(*this,
                envconfig( ledgerConfig,
                    sd.dbPath, ledgerFileCorrupt.string(), Config::LOAD_FILE));
        });
    }

    void
    testLoadByHash (SetupData const& sd)
    {
        testcase ("Load by hash");
        using namespace test::jtx;

//使用为启动指定的分类帐哈希创建新env
        auto ledgerHash = to_string(sd.hashes[sd.hashes.size()-1]);
        boost::erase_all(ledgerHash, "\"");
        Env env(*this,
            envconfig( ledgerConfig,
                sd.dbPath, ledgerHash, Config::LOAD));
        auto jrb = env.rpc ( "ledger", "current", "full") [jss::result];
        BEAST_EXPECT(jrb[jss::ledger][jss::accountState].size() == 97);
        BEAST_EXPECT(
            jrb[jss::ledger][jss::accountState].size() <=
            sd.ledger[jss::ledger][jss::accountState].size());
    }

    void
    testLoadLatest (SetupData const& sd)
    {
        testcase ("Load by keyword");
        using namespace test::jtx;

//使用为启动指定的分类帐“最新”创建新env
        Env env(*this,
            envconfig( ledgerConfig,
                sd.dbPath, "latest", Config::LOAD));
        auto jrb = env.rpc ( "ledger", "current", "full") [jss::result];
        BEAST_EXPECT(
            sd.ledger[jss::ledger][jss::accountState].size() ==
            jrb[jss::ledger][jss::accountState].size());
    }

    void
    testLoadIndex (SetupData const& sd)
    {
        testcase ("Load by index");
        using namespace test::jtx;

//在启动时使用特定分类帐索引创建新env
        Env env(*this,
            envconfig( ledgerConfig,
                sd.dbPath, "43", Config::LOAD));
        auto jrb = env.rpc ( "ledger", "current", "full") [jss::result];
        BEAST_EXPECT(
            sd.ledger[jss::ledger][jss::accountState].size() ==
            jrb[jss::ledger][jss::accountState].size());
    }

public:
    void run () override
    {
        beast::temp_dir td;
        auto sd = setupLedger(td);

//测试用例
        testLoad (sd);
        testBadFiles (sd);
        testLoadByHash (sd);
        testLoadLatest (sd);
        testLoadIndex (sd);
    }
};

BEAST_DEFINE_TESTSUITE (LedgerLoad, app, ripple);

}  //涟漪
