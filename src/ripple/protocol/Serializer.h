
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

#ifndef RIPPLE_PROTOCOL_SERIALIZER_H_INCLUDED
#define RIPPLE_PROTOCOL_SERIALIZER_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Buffer.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/basics/Slice.h>
#include <ripple/beast/crypto/secure_erase.h>
#include <ripple/protocol/SField.h>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <type_traits>

namespace ripple {

class CKey; //远期申报

class Serializer
{
private:
//贬低
    Blob mData;

public:
    explicit
    Serializer (int n = 256)
    {
        mData.reserve (n);
    }

    Serializer (void const* data, std::size_t size)
    {
        mData.resize(size);

        if (size)
        {
            assert(data != nullptr);
            std::memcpy(mData.data(), data, size);
        }
    }

    Slice slice() const noexcept
    {
        return Slice(mData.data(), mData.size());
    }

    std::size_t
    size() const noexcept
    {
        return mData.size();
    }

    void const*
    data() const noexcept
    {
        return mData.data();
    }

//汇编函数
    int add8 (unsigned char byte);
    int add16 (std::uint16_t);
int add32 (std::uint32_t);      //分类帐索引、科目序列、时间戳
int add64 (std::uint64_t);      //本国货币金额
int add128 (const uint128&);    //私钥生成器
int add256 (uint256 const& );       //交易和分类帐哈希

    template <typename Integer>
    int addInteger(Integer);

    template <int Bits, class Tag>
    int addBitString(base_uint<Bits, Tag> const& v) {
        int ret = mData.size ();
        mData.insert (mData.end (), v.begin (), v.end ());
        return ret;
    }

//TODO（汤姆）：与add128和add256合并。
    template <class Tag>
    int add160 (base_uint<160, Tag> const& i)
    {
        return addBitString<160, Tag>(i);
    }

    int addRaw (Blob const& vector);
    int addRaw (const void* ptr, int len);
    int addRaw (const Serializer& s);
    int addZeros (size_t uBytes);

    int addVL (Blob const& vector);
    int addVL (Slice const& slice);
    template<class Iter>
    int addVL (Iter begin, Iter end, int len);
    int addVL (const void* ptr, int len);

//反汇编函数
    bool get8 (int&, int offset) const;
    bool get256 (uint256&, int offset) const;

    template <typename Integer>
    bool getInteger(Integer& number, int offset) {
        static const auto bytes = sizeof(Integer);
        if ((offset + bytes) > mData.size ())
            return false;
        number = 0;

        auto ptr = &mData[offset];
        for (auto i = 0; i < bytes; ++i)
        {
            if (i)
                number <<= 8;
            number |= *ptr++;
        }
        return true;
    }

    template <int Bits, typename Tag = void>
    bool getBitString(base_uint<Bits, Tag>& data, int offset) const {
        auto success = (offset + (Bits / 8)) <= mData.size ();
        if (success)
            memcpy (data.begin (), & (mData.front ()) + offset, (Bits / 8));
        return success;
    }

//TODO（Tom）：与get128和get256合并。
    template <class Tag>
    bool get160 (base_uint<160, Tag>& o, int offset) const
    {
        return getBitString<160, Tag>(o, offset);
    }

    bool getRaw (Blob&, int offset, int length) const;
    Blob getRaw (int offset, int length) const;

    bool getVL (Blob& objectVL, int offset, int& length) const;
    bool getVLLength (int& length, int offset) const;

    int addFieldID (int type, int name);
    int addFieldID (SerializedTypeID type, int name)
    {
        return addFieldID (safe_cast<int> (type), name);
    }

//贬低
    uint256 getSHA512Half() const;

//总体功能
    Blob const& peekData () const
    {
        return mData;
    }
    Blob getData () const
    {
        return mData;
    }
    Blob& modData ()
    {
        return mData;
    }

    int getDataLength () const
    {
        return mData.size ();
    }
    const void* getDataPtr () const
    {
        return mData.data();
    }
    void* getDataPtr ()
    {
        return mData.data();
    }
    int getLength () const
    {
        return mData.size ();
    }
    std::string getString () const
    {
        return std::string (static_cast<const char*> (getDataPtr ()), size ());
    }
    void secureErase ()
    {
        beast::secure_erase(mData.data(), mData.size());
        mData.clear ();
    }
    void erase ()
    {
        mData.clear ();
    }
    bool chop (int num);

//类向量函数
    Blob ::iterator begin ()
    {
        return mData.begin ();
    }
    Blob ::iterator end ()
    {
        return mData.end ();
    }
    Blob ::const_iterator begin () const
    {
        return mData.begin ();
    }
    Blob ::const_iterator end () const
    {
        return mData.end ();
    }
    void reserve (size_t n)
    {
        mData.reserve (n);
    }
    void resize (size_t n)
    {
        mData.resize (n);
    }
    size_t capacity () const
    {
        return mData.capacity ();
    }

    bool operator== (Blob const& v)
    {
        return v == mData;
    }
    bool operator!= (Blob const& v)
    {
        return v != mData;
    }
    bool operator== (const Serializer& v)
    {
        return v.mData == mData;
    }
    bool operator!= (const Serializer& v)
    {
        return v.mData != mData;
    }

    std::string getHex () const
    {
        std::stringstream h;

        for (unsigned char const& element : mData)
        {
            h <<
                std::setw (2) <<
                std::hex <<
                std::setfill ('0') <<
                safe_cast<unsigned int>(element);
        }
        return h.str ();
    }

    static int decodeLengthLength (int b1);
    static int decodeVLLength (int b1);
    static int decodeVLLength (int b1, int b2);
    static int decodeVLLength (int b1, int b2, int b3);
private:
    static int lengthVL (int length)
    {
        return length + encodeLengthLength (length);
    }
static int encodeLengthLength (int length); //编码长度的长度
    int addEncoded (int length);
};

template<class Iter>
int Serializer::addVL(Iter begin, Iter end, int len)
{
    int ret = addEncoded(len);
    for (; begin != end; ++begin)
    {
        addRaw(begin->data(), begin->size());
#ifndef NDEBUG
        len -= begin->size();
#endif
    }
    assert(len == 0);
    return ret;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//贬低
//新序列化接口的过渡适配器
class SerialIter
{
private:
    std::uint8_t const* p_;
    std::size_t remain_;
    std::size_t used_ = 0;

public:
    SerialIter (void const* data,
            std::size_t size) noexcept;

    SerialIter (Slice const& slice)
        : SerialIter(slice.data(), slice.size())
    {
    }

//根据传递的数组的大小推断数据的大小。
    template<int N>
    explicit SerialIter (std::uint8_t const (&data)[N])
        : SerialIter(&data[0], N)
    {
        static_assert (N > 0, "");
    }

    std::size_t
    empty() const noexcept
    {
        return remain_ == 0;
    }

    void
    reset() noexcept;

    int
    getBytesLeft() const noexcept
    {
        return static_cast<int>(remain_);
    }

//获取函数时出错
    unsigned char
    get8();

    std::uint16_t
    get16();

    std::uint32_t
    get32();

    std::uint64_t
    get64();

    template <int Bits, class Tag = void>
    base_uint<Bits, Tag>
    getBitString();

    uint128
    get128()
    {
        return getBitString<128>();
    }

    uint160
    get160()
    {
        return getBitString<160>();
    }

    uint256
    get256()
    {
        return getBitString<256>();
    }

    void
    getFieldID (int& type, int& name);

//返回VL的大小，如果
//下一个对象是VL。推进迭代器
//到VL的开始。
    int
    getVLDataLength ();

    Slice
    getSlice (std::size_t bytes);

//vfalc deprecated返回副本
    Blob
    getRaw (int size);

//vfalc deprecated返回副本
    Blob
    getVL();

    void
    skip (int num);

    Buffer
    getVLBuffer();

    template<class T>
    T getRawHelper (int size);
};

template <int Bits, class Tag>
base_uint<Bits, Tag>
SerialIter::getBitString()
{
    base_uint<Bits, Tag> u;
    auto const n = Bits/8;
    if (remain_ < n)
        Throw<std::runtime_error> (
            "invalid SerialIter getBitString");
    std::memcpy (u.begin(), p_, n);
    p_ += n;
    used_ += n;
    remain_ -= n;
    return u;
}

} //涟漪

#endif
