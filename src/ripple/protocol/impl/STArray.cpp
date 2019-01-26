
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
#include <ripple/basics/Log.h>
#include <ripple/protocol/STBase.h>
#include <ripple/protocol/STArray.h>

namespace ripple {

STArray::STArray()
{
//vvalco注：我们需要确定
//做正确的事，并考虑
//使其成为可选的。
//v.储备（reservesize）；
}

STArray::STArray (STArray&& other)
    : STBase(other.getFName())
    , v_(std::move(other.v_))
{
}

STArray& STArray::operator= (STArray&& other)
{
    setFName(other.getFName());
    v_ = std::move(other.v_);
    return *this;
}

STArray::STArray (int n)
{
    v_.reserve(n);
}

STArray::STArray (SField const& f)
    : STBase (f)
{
    v_.reserve(reserveSize);
}

STArray::STArray (SField const& f, int n)
    : STBase (f)
{
    v_.reserve(n);
}

STArray::STArray (SerialIter& sit, SField const& f, int depth)
    : STBase(f)
{
    while (!sit.empty ())
    {
        int type, field;
        sit.getFieldID (type, field);

        if ((type == STI_ARRAY) && (field == 1))
            break;

        if ((type == STI_OBJECT) && (field == 1))
        {
            JLOG (debugLog().error()) <<
                "Encountered array with end of object marker";
            Throw<std::runtime_error> ("Illegal terminator in array");
        }

        auto const& fn = SField::getField (type, field);

        if (fn.isInvalid ())
        {
            JLOG (debugLog().error()) <<
                "Unknown field: " << type << "/" << field;
            Throw<std::runtime_error> ("Unknown field");
        }

        if (fn.fieldType != STI_OBJECT)
        {
            JLOG (debugLog().error()) <<
                "Array contains non-object";
            Throw<std::runtime_error> ("Non-object in array");
        }

        v_.emplace_back(sit, fn, depth+1);

v_.back().applyTemplateFromSField (fn);  //可能扔
    }
}

std::string STArray::getFullText () const
{
    std::string r = "[";

    bool first = true;
    for (auto const& obj : v_)
    {
        if (!first)
            r += ",";

        r += obj.getFullText ();
        first = false;
    }

    r += "]";
    return r;
}

std::string STArray::getText () const
{
    std::string r = "[";

    bool first = true;
    for (STObject const& o : v_)
    {
        if (!first)
            r += ",";

        r += o.getText ();
        first = false;
    }

    r += "]";
    return r;
}

Json::Value STArray::getJson (int p) const
{
    Json::Value v = Json::arrayValue;
    int index = 1;
    for (auto const& object: v_)
    {
        if (object.getSType () != STI_NOTPRESENT)
        {
            Json::Value& inner = v.append (Json::objectValue);
            auto const& fname = object.getFName ();
            auto k = fname.hasName () ? fname.fieldName : std::to_string(index);
            inner[k] = object.getJson (p);
            index++;
        }
    }
    return v;
}

void STArray::add (Serializer& s) const
{
    for (STObject const& object : v_)
    {
        object.addFieldID (s);
        object.add (s);
        s.addFieldID (STI_OBJECT, 1);
    }
}

bool STArray::isEquivalent (const STBase& t) const
{
    auto v = dynamic_cast<const STArray*> (&t);
    return v != nullptr && v_ == v->v_;
}

void STArray::sort (bool (*compare) (const STObject&, const STObject&))
{
    std::sort(v_.begin(), v_.end(), compare);
}

} //涟漪
