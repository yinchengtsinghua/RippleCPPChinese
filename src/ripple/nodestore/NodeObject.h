
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

#ifndef RIPPLE_NODESTORE_NODEOBJECT_H_INCLUDED
#define RIPPLE_NODESTORE_NODEOBJECT_H_INCLUDED

#include <ripple/basics/CountedObject.h>
#include <ripple/protocol/Protocol.h>

//vfalc说明故意不在nodestore命名空间中

namespace ripple {

/*节点对象的类型。*/
enum NodeObjectType
    : std::uint32_t
{
    hotUNKNOWN = 0,
    hotLEDGER = 1,
//HotTransaction=2//未使用
    hotACCOUNT_NODE = 3,
    hotTRANSACTION_NODE = 4
};

/*分类帐用于存储条目的简单对象。
    nodeObjects由一个类型、一个哈希、一个分类索引和一个blob组成。
    它们可以通过散列进行唯一标识，散列是
    斑点。blob是一个长度可变的序列化数据块。这个
    类型标识blob包含的内容。

    @注意：不执行任何检查以确保哈希与数据匹配。
    “见SHAMap”
**/

class NodeObject : public CountedObject <NodeObject>
{
public:
    static char const* getCountedObjectName () { return "NodeObject"; }

    enum
    {
        /*固定键的大小，以字节为单位。

            我们对密钥使用256位哈希。

            参见NoDeObjor
        **/

        keyBytes = 32,
    };

private:
//此黑客程序用于使构造函数有效地私有化。
//除非我们在通话中使用它来共享。
//没有可移植的方法可以让你成为一个朋友。
    struct PrivateAccess
    {
        explicit PrivateAccess() = default;
    };
public:
//此构造函数是私有的，请改用CreateObject。
    NodeObject (NodeObjectType type,
                Blob&& data,
                uint256 const& hash,
                PrivateAccess);

    /*从字段创建对象。

        调用方的变量在此调用期间被修改。这个
        blob的底层存储被nodeObject接管。

        @param键入对象类型。
        @param ledgerindex显示此对象的分类帐。
        @param data包含有效负载的缓冲区。调用方的变量
                    被覆盖。
        @param hash有效载荷数据的256位散列。
    **/

    static
    std::shared_ptr<NodeObject>
    createObject (NodeObjectType type,
        Blob&& data, uint256 const& hash);

    /*返回此对象的类型。*/
    NodeObjectType getType () const;

    /*返回数据的哈希值。*/
    uint256 const& getHash () const;

    /*返回基础数据。*/
    Blob const& getData () const;

private:
    NodeObjectType mType;
    uint256 mHash;
    Blob mData;
};

}

#endif
