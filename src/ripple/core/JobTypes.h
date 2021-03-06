
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

#ifndef RIPPLE_CORE_JOBTYPES_H_INCLUDED
#define RIPPLE_CORE_JOBTYPES_H_INCLUDED

#include <ripple/core/Job.h>
#include <ripple/core/JobTypeInfo.h>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace ripple
{

class JobTypes
{
public:
    using Map = std::map <JobType, JobTypeInfo>;
    using const_iterator = Map::const_iterator;

private:
    JobTypes ()
        : m_unknown (jtINVALID, "invalid", 0, true, std::chrono::milliseconds{0},
                                                    std::chrono::milliseconds{0})
    {
        using namespace std::chrono_literals;
        int maxLimit = std::numeric_limits <int>::max ();

add(    jtPACK,          "makeFetchPack",           1,        false, 0ms,     0ms);
add(    jtPUBOLDLEDGER,  "publishAcqLedger",        2,        false, 10000ms, 15000ms);
add(    jtVALIDATION_ut, "untrustedValidation",     maxLimit, false, 2000ms,  5000ms);
add(    jtTRANSACTION_l, "localTransaction",        maxLimit, false, 100ms,   500ms);
add(    jtLEDGER_REQ,    "ledgerRequest",           2,        false, 0ms,     0ms);
add(    jtPROPOSAL_ut,   "untrustedProposal",       maxLimit, false, 500ms,   1250ms);
add(    jtLEDGER_DATA,   "ledgerData",              2,        false, 0ms,     0ms);
add(    jtCLIENT,        "clientCommand",           maxLimit, false, 2000ms,  5000ms);
add(    jtRPC,           "RPC",                     maxLimit, false, 0ms,     0ms);
add(    jtUPDATE_PF,     "updatePaths",             maxLimit, false, 0ms,     0ms);
add(    jtTRANSACTION,   "transaction",             maxLimit, false, 250ms,   1000ms);
add(    jtBATCH,         "batch",                   maxLimit, false, 250ms,   1000ms);
add(    jtADVANCE,       "advanceLedger",           maxLimit, false, 0ms,     0ms);
add(    jtPUBLEDGER,     "publishNewLedger",        maxLimit, false, 3000ms,  4500ms);
add(    jtTXN_DATA,      "fetchTxnData",            1,        false, 0ms,     0ms);
add(    jtWAL,           "writeAhead",              maxLimit, false, 1000ms,  2500ms);
add(    jtVALIDATION_t,  "trustedValidation",       maxLimit, false, 500ms,   1500ms);
add(    jtWRITE,         "writeObjects",            maxLimit, false, 1750ms,  2500ms);
add(    jtACCEPT,        "acceptLedger",            maxLimit, false, 0ms,     0ms);
add(    jtPROPOSAL_t,    "trustedProposal",         maxLimit, false, 100ms,   500ms);
add(    jtSWEEP,         "sweep",                   maxLimit, false, 0ms,     0ms);
add(    jtNETOP_CLUSTER, "clusterReport",           1,        false, 9999ms,  9999ms);
add(    jtNETOP_TIMER,   "heartbeat",               1,        false, 999ms,   999ms);
add(    jtADMIN,         "administration",          maxLimit, false, 0ms,     0ms);

add(    jtPEER,          "peerCommand",             0,        true,  200ms,   2500ms);
add(    jtDISK,          "diskAccess",              0,        true,  500ms,   1000ms);
add(    jtTXN_PROC,      "processTransaction",      0,        true,  0ms,     0ms);
add(    jtOB_SETUP,      "orderBookSetup",          0,        true,  0ms,     0ms);
add(    jtPATH_FIND,     "pathFind",                0,        true,  0ms,     0ms);
add(    jtHO_READ,       "nodeRead",                0,        true,  0ms,     0ms);
add(    jtHO_WRITE,      "nodeWrite",               0,        true,  0ms,     0ms);
add(    jtGENERIC,       "generic",                 0,        true,  0ms,     0ms);
add(    jtNS_SYNC_READ,  "SyncReadNode",            0,        true,  0ms,     0ms);
add(    jtNS_ASYNC_READ, "AsyncReadNode",           0,        true,  0ms,     0ms);
add(    jtNS_WRITE,      "WriteNode",               0,        true,  0ms,     0ms);

    }

public:
    static JobTypes const& instance()
    {
        static JobTypes const types;
        return types;
    }

    JobTypeInfo const& get (JobType jt) const
    {
        Map::const_iterator const iter (m_map.find (jt));
        assert (iter != m_map.end ());

        if (iter != m_map.end())
            return iter->second;

        return m_unknown;
    }

    JobTypeInfo const& getInvalid () const
    {
        return m_unknown;
    }

    Map::size_type size () const
    {
        return m_map.size();
    }

    const_iterator begin () const
    {
        return m_map.cbegin ();
    }

    const_iterator cbegin () const
    {
        return m_map.cbegin ();
    }

    const_iterator end () const
    {
        return m_map.cend ();
    }

    const_iterator cend () const
    {
        return m_map.cend ();
    }

private:
    void add(JobType jt, std::string name, int limit,
        bool special, std::chrono::milliseconds avgLatency,
        std::chrono::milliseconds peakLatency)
    {
        assert (m_map.find (jt) == m_map.end ());

        std::pair<Map::iterator,bool> result (m_map.emplace (
            std::piecewise_construct,
            std::forward_as_tuple (jt),
            std::forward_as_tuple (jt, name, limit, special,
                avgLatency, peakLatency)));

        assert (result.second == true);
        (void) result.second;
    }

    JobTypeInfo m_unknown;
    Map m_map;
};

}

#endif
