
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
    版权所有（c）2012-2015 Ripple Labs Inc.

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

#ifndef RIPPLE_SOCIDB_H_INCLUDED
#define RIPPLE_SOCIDB_H_INCLUDED

/*具有直观、类型安全的界面的嵌入式数据库包装器。

    这个类集合允许您访问嵌入的sqlite数据库
    使用C++语法，非常类似于常规SQL。

    此模块需要@ref beast_sqlite外部模块。
**/


#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
#endif

#include <ripple/basics/Log.h>
#include <ripple/core/JobQueue.h>
#define SOCI_USE_BOOST
#include <soci/soci.h>
#include <string>
#include <cstdint>
#include <vector>

namespace sqlite_api {
    struct sqlite3;
}

namespace ripple {

template <class T, class C>
T rangeCheckedCast (C c)
{
    if ((c > std::numeric_limits<T>::max ()) ||
        (!std::numeric_limits<T>::is_signed && c < 0) ||
        (std::numeric_limits<T>::is_signed &&
         std::numeric_limits<C>::is_signed &&
         c < std::numeric_limits<T>::lowest ()))
    {
        JLOG (debugLog().error()) << "rangeCheckedCast domain error:"
          << " value = " << c
          << " min = " << std::numeric_limits<T>::lowest ()
          << " max: " << std::numeric_limits<T>::max ();
    }

    return static_cast<T>(c);
}

class BasicConfig;

/*
   当客户端希望延迟在以下时间后打开soci:：session时，使用sociconfig
   正在分析配置参数。如果客户端要打开会话
   立即使用下面的自由功能“打开”。
 **/

class SociConfig
{
    std::string connectionString_;
    soci::backend_factory const& backendFactory_;
    SociConfig(std::pair<std::string, soci::backend_factory const&> init);

public:
    SociConfig(BasicConfig const& config,
               std::string const& dbName);
    std::string connectionString () const;
    void open (soci::session& s) const;
};

/*
   打开一个Soci会话。

   @param s会话打开。

   @param config参数选择soci后端以及如何连接到该后端
                 后端。

   @param dbname数据库的名称。这对不同的人有不同的意义
                 后端。有时它是文件名（sqlite3）的一部分，
                 有时它是一个数据库名（postgresql）。
**/

void open (soci::session& s,
           BasicConfig const& config,
           std::string const& dbName);

/*
 *打开Soci会话。
 *
 要打开的*@param s会话。
 *@param bename后端名称。
 *@param connection string要转发到soci:：open的连接字符串。
 *有关如何使用它的信息，请参阅soci:：open文档。
 *
 **/

void open (soci::session& s,
           std::string const& beName,
           std::string const& connectionString);

size_t getKBUsedAll (soci::session& s);
size_t getKBUsedDB (soci::session& s);

void convert (soci::blob& from, std::vector<std::uint8_t>& to);
void convert (soci::blob& from, std::string& to);
void convert (std::vector<std::uint8_t> const& from, soci::blob& to);
void convert (std::string const& from, soci::blob& to);

class Checkpointer
{
  public:
    virtual ~Checkpointer() = default;
};

/*返回一个新的检查点，该检查点使
    soci database every checkpointpagecount页，使用作业队列中的作业。

    检查点包含对会话和作业队列的引用
    他们两个都得活下去。
 **/

std::unique_ptr <Checkpointer> makeCheckpointer (soci::session&, JobQueue&, Logs&);

} //涟漪

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
