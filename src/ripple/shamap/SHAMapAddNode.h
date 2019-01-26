
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

#ifndef RIPPLE_SHAMAP_SHAMAPADDNODE_H_INCLUDED
#define RIPPLE_SHAMAP_SHAMAPADDNODE_H_INCLUDED

#include <string>

namespace ripple {

//添加节点的结果
class SHAMapAddNode
{
private:
    int mGood;
    int mBad;
    int mDuplicate;

public:
    SHAMapAddNode ();
    void incInvalid ();
    void incUseful ();
    void incDuplicate ();
    void reset ();
    int getGood () const;
    bool isGood () const;
    bool isInvalid () const;
    bool isUseful () const;
    std::string get () const;

    SHAMapAddNode& operator+= (SHAMapAddNode const& n);

    static SHAMapAddNode duplicate ();
    static SHAMapAddNode useful ();
    static SHAMapAddNode invalid ();

private:
    SHAMapAddNode (int good, int bad, int duplicate);
};

inline
SHAMapAddNode::SHAMapAddNode ()
    : mGood (0)
    , mBad (0)
    , mDuplicate (0)
{
}

inline
SHAMapAddNode::SHAMapAddNode (int good, int bad, int duplicate)
    : mGood (good)
    , mBad (bad)
    , mDuplicate (duplicate)
{
}

inline
void
SHAMapAddNode::incInvalid ()
{
    ++mBad;
}

inline
void
SHAMapAddNode::incUseful ()
{
    ++mGood;
}

inline
void
SHAMapAddNode::incDuplicate ()
{
    ++mDuplicate;
}

inline
void
SHAMapAddNode::reset ()
{
    mGood = mBad = mDuplicate = 0;
}

inline
int
SHAMapAddNode::getGood () const
{
    return mGood;
}

inline
bool
SHAMapAddNode::isInvalid () const
{
    return mBad > 0;
}

inline
bool
SHAMapAddNode::isUseful () const
{
    return mGood > 0;
}

inline
SHAMapAddNode&
SHAMapAddNode::operator+= (SHAMapAddNode const& n)
{
    mGood += n.mGood;
    mBad += n.mBad;
    mDuplicate += n.mDuplicate;

    return *this;
}

inline
bool
SHAMapAddNode::isGood () const
{
    return (mGood + mDuplicate) > mBad;
}

inline
SHAMapAddNode
SHAMapAddNode::duplicate ()
{
    return SHAMapAddNode (0, 0, 1);
}

inline
SHAMapAddNode
SHAMapAddNode::useful ()
{
    return SHAMapAddNode (1, 0, 0);
}

inline
SHAMapAddNode
SHAMapAddNode::invalid ()
{
    return SHAMapAddNode (0, 1, 0);
}

inline
std::string
SHAMapAddNode::get () const
{
    std::string ret;
    if (mGood > 0)
    {
        ret.append("good:");
        ret.append(std::to_string(mGood));
    }
    if (mBad > 0)
    {
        if (!ret.empty())
            ret.append(" ");
         ret.append("bad:");
         ret.append(std::to_string(mBad));
    }
    if (mDuplicate > 0)
    {
        if (!ret.empty())
            ret.append(" ");
         ret.append("dupe:");
         ret.append(std::to_string(mDuplicate));
    }
    if (ret.empty ())
        ret = "no nodes processed";
    return ret;
}

}

#endif
