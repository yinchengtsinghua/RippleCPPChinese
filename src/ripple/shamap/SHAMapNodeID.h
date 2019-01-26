
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

#ifndef RIPPLE_SHAMAP_SHAMAPNODEID_H_INCLUDED
#define RIPPLE_SHAMAP_SHAMAPNODEID_H_INCLUDED

#include <ripple/protocol/Serializer.h>
#include <ripple/basics/base_uint.h>
#include <ripple/beast/utility/Journal.h>
#include <ostream>
#include <string>
#include <tuple>

namespace ripple {

//标识半sha512（256位）哈希图中的节点
class SHAMapNodeID
{
private:
    uint256 mNodeID;
    int mDepth;

public:
    SHAMapNodeID ();
    SHAMapNodeID (int depth, uint256 const& hash);
    SHAMapNodeID (void const* ptr, int len);

    bool isValid () const;
    bool isRoot () const;

//转换为/来自线格式（256位nodeid，1字节深度）
    void addIDRaw (Serializer& s) const;
    std::string getRawString () const;

    bool operator== (const SHAMapNodeID& n) const;
    bool operator!= (const SHAMapNodeID& n) const;

    bool operator< (const SHAMapNodeID& n) const;
    bool operator> (const SHAMapNodeID& n) const;
    bool operator<= (const SHAMapNodeID& n) const;
    bool operator>= (const SHAMapNodeID& n) const;

    std::string getString () const;
    void dump (beast::Journal journal) const;

//仅由shamap和shamaptreenode使用

    uint256 const& getNodeID ()  const;
    SHAMapNodeID getChildNodeID (int m) const;
    int selectBranch (uint256 const& hash) const;
    int getDepth () const;
    bool has_common_prefix(SHAMapNodeID const& other) const;

private:
    static uint256 const& Masks (int depth);

    friend std::ostream& operator<< (std::ostream& out, SHAMapNodeID const& node);

private:  //当前未使用
    SHAMapNodeID getParentNodeID () const;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline
SHAMapNodeID::SHAMapNodeID ()
    : mDepth (0)
{
}

inline
int
SHAMapNodeID::getDepth () const
{
    return mDepth;
}

inline
uint256 const&
SHAMapNodeID::getNodeID ()  const
{
    return mNodeID;
}

inline
bool
SHAMapNodeID::isValid () const
{
    return (mDepth >= 0) && (mDepth <= 64);
}

inline
bool
SHAMapNodeID::isRoot () const
{
    return mDepth == 0;
}

inline
SHAMapNodeID
SHAMapNodeID::getParentNodeID () const
{
    assert (mDepth);
    return SHAMapNodeID (mDepth - 1,
        mNodeID & Masks (mDepth - 1));
}

inline
bool
SHAMapNodeID::operator< (const SHAMapNodeID& n) const
{
    return std::tie(mDepth, mNodeID) < std::tie(n.mDepth, n.mNodeID);
}

inline
bool
SHAMapNodeID::operator> (const SHAMapNodeID& n) const
{
    return n < *this;
}

inline
bool
SHAMapNodeID::operator<= (const SHAMapNodeID& n) const
{
    return !(n < *this);
}

inline
bool
SHAMapNodeID::operator>= (const SHAMapNodeID& n) const
{
    return !(*this < n);
}

inline
bool
SHAMapNodeID::operator== (const SHAMapNodeID& n) const
{
    return (mDepth == n.mDepth) && (mNodeID == n.mNodeID);
}

inline
bool
SHAMapNodeID::operator!= (const SHAMapNodeID& n) const
{
    return !(*this == n);
}

inline std::ostream& operator<< (std::ostream& out, SHAMapNodeID const& node)
{
    return out << node.getString ();
}

} //涟漪

#endif
