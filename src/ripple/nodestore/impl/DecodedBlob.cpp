
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

#include <ripple/basics/safe_cast.h>
#include <ripple/nodestore/impl/DecodedBlob.h>
#include <algorithm>
#include <cassert>

namespace ripple {
namespace NodeStore {

DecodedBlob::DecodedBlob (void const* key, void const* value, int valueBytes)
{
    /*数据格式：

        字节

        0…3 LedgerIndex 32位大尾数整数
        4…7未使用？未使用的Ledgerindex副本
        8个字符nodeObjectType之一
        
    **/


    m_success = false;
    m_key = key;
//vvalco票据分类帐索引应该从1开始
    m_objectType = hotUNKNOWN;
    m_objectData = nullptr;
    m_dataBytes = std::max (0, valueBytes - 9);

//vfalc注意到字节4到7包括什么？

    if (valueBytes > 8)
    {
        unsigned char const* byte = static_cast <unsigned char const*> (value);
        m_objectType = safe_cast <NodeObjectType> (byte [8]);
    }

    if (valueBytes > 9)
    {
        m_objectData = static_cast <unsigned char const*> (value) + 9;

        switch (m_objectType)
        {
        default:
            break;

        case hotUNKNOWN:
        case hotLEDGER:
        case hotACCOUNT_NODE:
        case hotTRANSACTION_NODE:
            m_success = true;
            break;
        }
    }
}

std::shared_ptr<NodeObject> DecodedBlob::createObject ()
{
    assert (m_success);

    std::shared_ptr<NodeObject> object;

    if (m_success)
    {
        Blob data(m_objectData, m_objectData + m_dataBytes);

        object = NodeObject::createObject (
            m_objectType, std::move(data), uint256::fromVoid(m_key));
    }

    return object;
}

}
}
