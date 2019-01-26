
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

#include <ripple/core/DatabaseCon.h>
#include <ripple/core/SociDB.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <memory>

namespace ripple {

DatabaseCon::DatabaseCon (
    Setup const& setup,
    std::string const& strName,
    const char* initStrings[],
    int initCount)
{
auto const useTempFiles  //使用临时文件还是常规数据库文件？
        = setup.standAlone &&
          setup.startUp != Config::LOAD &&
          setup.startUp != Config::LOAD_FILE &&
          setup.startUp != Config::REPLAY;
    boost::filesystem::path pPath = useTempFiles
        ? "" : (setup.dataDir / strName);

    open (session_, "sqlite", pPath.string());

    for (int i = 0; i < initCount; ++i)
    {
        try
        {
            soci::statement st = session_.prepare <<
                initStrings[i];
            st.execute(true);
        }
        catch (soci::soci_error&)
        {
//忽略错误
        }
    }
}

DatabaseCon::Setup setup_DatabaseCon (Config const& c)
{
    DatabaseCon::Setup setup;

    setup.startUp = c.START_UP;
    setup.standAlone = c.standalone();
    setup.dataDir = c.legacy ("database_path");
    if (!setup.standAlone && setup.dataDir.empty())
    {
        Throw<std::runtime_error>(
            "database_path must be set.");
    }

    return setup;
}

void DatabaseCon::setupCheckpointing (JobQueue* q, Logs& l)
{
    if (! q)
        Throw<std::logic_error> ("No JobQueue");
    checkpointer_ = makeCheckpointer (session_, *q, l);
}

} //涟漪
