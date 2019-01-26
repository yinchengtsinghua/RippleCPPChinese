
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

#include <ripple/shamap/SHAMapNodeID.h>
#include <ripple/crypto/csprng.h>
#include <ripple/basics/Log.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/format.hpp>
#include <cassert>
#include <cstring>

namespace ripple {

uint256 const&
SHAMapNodeID::Masks (int depth)
{
    enum
    {
        mask_size = 65
    };

    struct masks_t
    {
        uint256 entry [mask_size];

        masks_t()
        {
            uint256 selector;
            for (int i = 0; i < mask_size-1; i += 2)
            {
                entry[i] = selector;
                * (selector.begin () + (i / 2)) = 0xF0;
                entry[i + 1] = selector;
                * (selector.begin () + (i / 2)) = 0xFF;
            }
            entry[mask_size-1] = selector;
        }
    };
    static masks_t const masks;
    return masks.entry[depth];
}

//为此深度将哈希规范化为节点ID
SHAMapNodeID::SHAMapNodeID (int depth, uint256 const& hash)
    : mNodeID (hash), mDepth (depth)
{
    assert ((depth >= 0) && (depth < 65));
    assert (mNodeID == (mNodeID & Masks(depth)));
}

SHAMapNodeID::SHAMapNodeID (void const* ptr, int len)
{
    if (len < 33)
        mDepth = -1;
    else
    {
        std::memcpy (mNodeID.begin (), ptr, 32);
        mDepth = * (static_cast<unsigned char const*> (ptr) + 32);
    }
}

std::string SHAMapNodeID::getString () const
{
    if ((mDepth == 0) && (mNodeID.isZero ()))
        return "NodeID(root)";

    return "NodeID(" + std::to_string (mDepth) +
        "," + to_string (mNodeID) + ")";
}

void SHAMapNodeID::addIDRaw (Serializer& s) const
{
    s.add256 (mNodeID);
    s.add8 (mDepth);
}

std::string SHAMapNodeID::getRawString () const
{
    Serializer s (33);
    addIDRaw (s);
    return s.getString ();
}

//这可以优化以避免<<如果需要
SHAMapNodeID SHAMapNodeID::getChildNodeID (int m) const
{
    assert ((m >= 0) && (m < 16));
    assert (mDepth < 64);

    uint256 child (mNodeID);
    child.begin ()[mDepth / 2] |= (mDepth & 1) ? m : (m << 4);

    return SHAMapNodeID (mDepth + 1, child);
}

//哪个分支将包含指定的哈希
int SHAMapNodeID::selectBranch (uint256 const& hash) const
{
#if RIPPLE_VERIFY_NODEOBJECT_KEYS

    if (mDepth >= 64)
    {
        assert (false);
        return -1;
    }

    if ((hash & Masks(mDepth)) != mNodeID)
    {
        std::cerr << "selectBranch(" << getString () << std::endl;
        std::cerr << "  " << hash << " off branch" << std::endl;
        assert (false);
return -1;  //不在此节点下
    }

#endif

    int branch = * (hash.begin () + (mDepth / 2));

    if (mDepth & 1)
        branch &= 0xf;
    else
        branch >>= 4;

    assert ((branch >= 0) && (branch < 16));

    return branch;
}

bool
SHAMapNodeID::has_common_prefix(SHAMapNodeID const& other) const
{
    assert(mDepth <= other.mDepth);
    auto x = mNodeID.begin();
    auto y = other.mNodeID.begin();
    for (unsigned i = 0; i < mDepth/2; ++i, ++x, ++y)
    {
        if (*x != *y)
            return false;
    }
    if (mDepth & 1)
    {
        auto i = mDepth/2;
        return (*(mNodeID.begin() + i) & 0xF0) == (*(other.mNodeID.begin() + i) & 0xF0);
    }
    return true;
}

void SHAMapNodeID::dump (beast::Journal journal) const
{
    JLOG(journal.debug()) <<
        getString ();
}

} //涟漪
