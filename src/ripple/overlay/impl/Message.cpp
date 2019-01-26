
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
#include <ripple/overlay/Message.h>
#include <ripple/overlay/impl/TrafficCount.h>
#include <cstdint>

namespace ripple {

Message::Message (::google::protobuf::Message const& message, int type)
{
    unsigned const messageBytes = message.ByteSize ();

    assert (messageBytes != 0);

    mBuffer.resize (kHeaderBytes + messageBytes);

    encodeHeader (messageBytes, type);

    if (messageBytes != 0)
    {
        message.SerializeToArray (&mBuffer [Message::kHeaderBytes], messageBytes);
    }

    mCategory = safe_cast<int>(TrafficCount::categorize
        (message, type, false));
}

bool Message::operator== (Message const& other) const
{
    return mBuffer == other.mBuffer;
}

unsigned Message::getLength (std::vector <uint8_t> const& buf)
{
    unsigned result;

    if (buf.size () >= Message::kHeaderBytes)
    {
        result = buf [0];
        result <<= 8;
        result |= buf [1];
        result <<= 8;
        result |= buf [2];
        result <<= 8;
        result |= buf [3];
    }
    else
    {
        result = 0;
    }

    return result;
}

int Message::getType (std::vector<uint8_t> const& buf)
{
    if (buf.size () < Message::kHeaderBytes)
        return 0;

    int ret = buf[4];
    ret <<= 8;
    ret |= buf[5];
    return ret;
}

void Message::encodeHeader (unsigned size, int type)
{
    assert (mBuffer.size () >= Message::kHeaderBytes);
    mBuffer[0] = static_cast<std::uint8_t> ((size >> 24) & 0xFF);
    mBuffer[1] = static_cast<std::uint8_t> ((size >> 16) & 0xFF);
    mBuffer[2] = static_cast<std::uint8_t> ((size >> 8) & 0xFF);
    mBuffer[3] = static_cast<std::uint8_t> (size & 0xFF);
    mBuffer[4] = static_cast<std::uint8_t> ((type >> 8) & 0xFF);
    mBuffer[5] = static_cast<std::uint8_t> (type & 0xFF);
}

}
