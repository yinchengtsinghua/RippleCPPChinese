
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

#ifndef RIPPLE_PEERFINDER_STORESQDB_H_INCLUDED
#define RIPPLE_PEERFINDER_STORESQDB_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/core/SociDB.h>
#include <boost/optional.hpp>

namespace ripple {
namespace PeerFinder {

/*使用sqlite的peerfinder的数据库持久性*/
class StoreSqdb
    : public Store
{
private:
    beast::Journal m_journal;
    soci::session m_session;
public:
    enum
    {
//这将确定数据的数据库格式
        currentSchemaVersion = 4
    };

    explicit StoreSqdb (beast::Journal journal =
            beast::Journal {beast::Journal::getNullSink()})
        : m_journal (journal)
    {
    }

    ~StoreSqdb ()
    {
    }

    void open (SociConfig const& sociConfig)
    {
        sociConfig.open (m_session);

        JLOG(m_journal.info()) <<
            "Opening database at '" << sociConfig.connectionString () << "'";

        init ();
        update ();
    }

//加载引导缓存，为每个条目调用回调
//
    std::size_t load (load_callback const& cb) override
    {
        std::size_t n (0);
        std::string s;
        int valence;
        soci::statement st = (m_session.prepare <<
            "SELECT "
            " address, "
            " valence "
            "FROM PeerFinder_BootstrapCache;"
            , soci::into (s)
            , soci::into (valence)
            );

        st.execute ();
        while (st.fetch ())
        {
            beast::IP::Endpoint const endpoint (
                beast::IP::Endpoint::from_string (s));

            if (!is_unspecified (endpoint))
            {
                cb (endpoint, valence);
                ++n;
            }
            else
            {
                JLOG(m_journal.error()) <<
                    "Bad address string '" << s << "' in Bootcache table";
            }
        }
        return n;
    }

//用指定的数组覆盖存储的引导缓存。
//
    void save (std::vector <Entry> const& v) override
    {
        soci::transaction tr (m_session);
        m_session <<
            "DELETE FROM PeerFinder_BootstrapCache;";

        if (!v.empty ())
        {
            std::vector<std::string> s;
            std::vector<int> valence;
            s.reserve (v.size ());
            valence.reserve (v.size ());

            for (auto const& e : v)
            {
                s.emplace_back (to_string (e.endpoint));
                valence.emplace_back (e.valence);
            }

            m_session <<
                    "INSERT INTO PeerFinder_BootstrapCache ( "
                    "  address, "
                    "  valence "
                    ") VALUES ( "
                    "  :s, :valence "
                    ");"
                    , soci::use (s)
                    , soci::use (valence);
        }

        tr.commit ();
    }

//将旧架构中的任何现有条目转换为
//当前的，如果合适的话。
    void update ()
    {
        soci::transaction tr (m_session);
//获取版本
        int version (0);
        {
            boost::optional<int> vO;
            m_session <<
                "SELECT "
                "  version "
                "FROM SchemaVersion WHERE "
                "  name = 'PeerFinder';"
                , soci::into (vO)
                ;

            version = vO.value_or (0);

            JLOG(m_journal.info()) <<
                "Opened version " << version << " database";
        }

        {
            if (version < currentSchemaVersion)
            {
                JLOG(m_journal.info()) <<
                    "Updating database to version " << currentSchemaVersion;
            }
            else if (version > currentSchemaVersion)
            {
                Throw<std::runtime_error> (
                    "The PeerFinder database version is higher than expected");
            }
        }

        if (version < 4)
        {
//
//从引导表中删除“正常运行时间”列
//

            m_session <<
                "CREATE TABLE IF NOT EXISTS PeerFinder_BootstrapCache_Next ( "
                "  id       INTEGER PRIMARY KEY AUTOINCREMENT, "
                "  address  TEXT UNIQUE NOT NULL, "
                "  valence  INTEGER"
                ");"
                ;

            m_session <<
                "CREATE INDEX IF NOT EXISTS "
                "  PeerFinder_BootstrapCache_Next_Index ON "
                "    PeerFinder_BootstrapCache_Next "
                "  ( address ); "
                ;

            std::size_t count;
            m_session <<
                "SELECT COUNT(*) FROM PeerFinder_BootstrapCache;"
                , soci::into (count)
                ;

            std::vector <Store::Entry> list;

            {
                list.reserve (count);
                std::string s;
                int valence;
                soci::statement st = (m_session.prepare <<
                    "SELECT "
                    " address, "
                    " valence "
                    "FROM PeerFinder_BootstrapCache;"
                    , soci::into (s)
                    , soci::into (valence)
                    );

                st.execute ();
                while (st.fetch ())
                {
                    Store::Entry entry;
                    entry.endpoint = beast::IP::Endpoint::from_string (s);
                    if (!is_unspecified (entry.endpoint))
                    {
                        entry.valence = valence;
                        list.push_back (entry);
                    }
                    else
                    {
                        JLOG(m_journal.error()) <<
                            "Bad address string '" << s << "' in Bootcache table";
                    }
                }
            }

            if (!list.empty ())
            {
                std::vector<std::string> s;
                std::vector<int> valence;
                s.reserve (list.size ());
                valence.reserve (list.size ());

                for (auto iter (list.cbegin ());
                     iter != list.cend (); ++iter)
                {
                    s.emplace_back (to_string (iter->endpoint));
                    valence.emplace_back (iter->valence);
                }

                m_session <<
                    "INSERT INTO PeerFinder_BootstrapCache_Next ( "
                    "  address, "
                    "  valence "
                    ") VALUES ( "
                    "  :s, :valence"
                    ");"
                    , soci::use (s)
                    , soci::use (valence)
                    ;

            }

            m_session <<
                "DROP TABLE IF EXISTS PeerFinder_BootstrapCache;";

            m_session <<
                "DROP INDEX IF EXISTS PeerFinder_BootstrapCache_Index;";

            m_session <<
                "ALTER TABLE PeerFinder_BootstrapCache_Next "
                "  RENAME TO PeerFinder_BootstrapCache;";

            m_session <<
                "CREATE INDEX IF NOT EXISTS "
                "  PeerFinder_BootstrapCache_Index ON "
                "PeerFinder_BootstrapCache "
                "  (  "
                "    address "
                "  ); "
                ;
        }

        if (version < 3)
        {
//
//从架构中删除旧终结点
//

            m_session <<
                "DROP TABLE IF EXISTS LegacyEndpoints;";

            m_session <<
                "DROP TABLE IF EXISTS PeerFinderLegacyEndpoints;";

            m_session <<
                "DROP TABLE IF EXISTS PeerFinder_LegacyEndpoints;";

            m_session <<
                "DROP TABLE IF EXISTS PeerFinder_LegacyEndpoints_Index;";
        }

        {
            int const version (currentSchemaVersion);
            m_session <<
                "INSERT OR REPLACE INTO SchemaVersion ("
                "   name "
                "  ,version "
                ") VALUES ( "
                "  'PeerFinder', :version "
                ");"
                , soci::use (version);
        }

        tr.commit ();
    }

private:
    void init ()
    {
        soci::transaction tr (m_session);
        m_session << "PRAGMA encoding=\"UTF-8\";";

        m_session <<
            "CREATE TABLE IF NOT EXISTS SchemaVersion ( "
            "  name             TEXT PRIMARY KEY, "
            "  version          INTEGER"
            ");"
            ;

        m_session <<
            "CREATE TABLE IF NOT EXISTS PeerFinder_BootstrapCache ( "
            "  id       INTEGER PRIMARY KEY AUTOINCREMENT, "
            "  address  TEXT UNIQUE NOT NULL, "
            "  valence  INTEGER"
            ");"
            ;

        m_session <<
            "CREATE INDEX IF NOT EXISTS "
            "  PeerFinder_BootstrapCache_Index ON "
            "PeerFinder_BootstrapCache "
            "  (  "
            "    address "
            "  ); "
            ;

        tr.commit ();
    }
};

}
}

#endif
