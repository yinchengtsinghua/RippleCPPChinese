
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

#include <ripple/basics/contract.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/STArray.h>
#include <ripple/protocol/STBase.h>
#include <ripple/protocol/STBitString.h>
#include <ripple/protocol/STBlob.h>
#include <ripple/protocol/STInteger.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/STPathSet.h>
#include <ripple/protocol/STVector256.h>
#include <ripple/protocol/impl/STVar.h>

namespace ripple {
namespace detail {

defaultObject_t defaultObject;
nonPresentObject_t nonPresentObject;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

STVar::~STVar()
{
    destroy();
}

STVar::STVar (STVar const& other)
{
    if (other.p_ != nullptr)
        p_ = other.p_->copy(max_size, &d_);
}

STVar::STVar (STVar&& other)
{
    if (other.on_heap())
    {
        p_ = other.p_;
        other.p_ = nullptr;
    }
    else
    {
        p_ = other.p_->move(max_size, &d_);
    }
}

STVar&
STVar::operator= (STVar const& rhs)
{
    if (&rhs != this)
    {
        destroy();
        if (rhs.p_)
            p_ = rhs.p_->copy(max_size, &d_);
        else
            p_ = nullptr;
    }

    return *this;
}

STVar&
STVar::operator= (STVar&& rhs)
{
    if (&rhs != this)
    {
        destroy();
        if (rhs.on_heap())
        {
            p_ = rhs.p_;
            rhs.p_ = nullptr;
        }
        else
        {
            p_ = rhs.p_->move(max_size, &d_);
        }
    }

    return *this;
}

STVar::STVar (defaultObject_t, SField const& name)
    : STVar(name.fieldType, name)
{
}

STVar::STVar (nonPresentObject_t, SField const& name)
    : STVar(STI_NOTPRESENT, name)
{
}

STVar::STVar (SerialIter& sit, SField const& name, int depth)
{
    if (depth > 10)
        Throw<std::runtime_error> ("Maximum nesting depth of STVar exceeded");
    switch (name.fieldType)
    {
    case STI_NOTPRESENT:    construct<STBase>(name); return;
    case STI_UINT8:         construct<STUInt8>(sit, name); return;
    case STI_UINT16:        construct<STUInt16>(sit, name); return;
    case STI_UINT32:        construct<STUInt32>(sit, name); return;
    case STI_UINT64:        construct<STUInt64>(sit, name); return;
    case STI_AMOUNT:        construct<STAmount>(sit, name); return;
    case STI_HASH128:       construct<STHash128>(sit, name); return;
    case STI_HASH160:       construct<STHash160>(sit, name); return;
    case STI_HASH256:       construct<STHash256>(sit, name); return;
    case STI_VECTOR256:     construct<STVector256>(sit, name); return;
    case STI_VL:            construct<STBlob>(sit, name); return;
    case STI_ACCOUNT:       construct<STAccount>(sit, name); return;
    case STI_PATHSET:       construct<STPathSet>(sit, name); return;
    case STI_OBJECT:        construct<STObject>(sit, name, depth); return;
    case STI_ARRAY:         construct<STArray>(sit, name, depth); return;
    default:
        Throw<std::runtime_error> ("Unknown object type");
    }
}

STVar::STVar (SerializedTypeID id, SField const& name)
{
    assert ((id == STI_NOTPRESENT) || (id == name.fieldType));
    switch (id)
    {
    case STI_NOTPRESENT:    construct<STBase>(name); return;
    case STI_UINT8:         construct<STUInt8>(name); return;
    case STI_UINT16:        construct<STUInt16>(name); return;
    case STI_UINT32:        construct<STUInt32>(name); return;
    case STI_UINT64:        construct<STUInt64>(name); return;
    case STI_AMOUNT:        construct<STAmount>(name); return;
    case STI_HASH128:       construct<STHash128>(name); return;
    case STI_HASH160:       construct<STHash160>(name); return;
    case STI_HASH256:       construct<STHash256>(name); return;
    case STI_VECTOR256:     construct<STVector256>(name); return;
    case STI_VL:            construct<STBlob>(name); return;
    case STI_ACCOUNT:       construct<STAccount>(name); return;
    case STI_PATHSET:       construct<STPathSet>(name); return;
    case STI_OBJECT:        construct<STObject>(name); return;
    case STI_ARRAY:         construct<STArray>(name); return;
    default:
        Throw<std::runtime_error> ("Unknown object type");
    }
}

void
STVar::destroy()
{
    if (on_heap())
        delete p_;
    else
        p_->~STBase();

    p_ = nullptr;
}

} //细节
} //涟漪
