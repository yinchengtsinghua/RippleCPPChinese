
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

#include <ripple/protocol/STObject.h>
#include <ripple/protocol/InnerObjectFormats.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/STArray.h>
#include <ripple/protocol/STBlob.h>
#include <ripple/basics/Log.h>

namespace ripple {

STObject::~STObject()
{
#if 0
//打开此项以获取退出时的柱状图
    static Log log;
    log(v_.size());
#endif
}

STObject::STObject(STObject&& other)
    : STBase(other.getFName())
    , v_(std::move(other.v_))
    , mType(other.mType)
{
}

STObject::STObject (SField const& name)
    : STBase (name)
    , mType (nullptr)
{
//看看这是不是正确的做法
//v.储备（reservesize）；
}

STObject::STObject (SOTemplate const& type,
        SField const& name)
    : STBase (name)
{
    set (type);
}

STObject::STObject (SOTemplate const& type,
        SerialIter & sit, SField const& name) noexcept (false)
    : STBase (name)
{
    v_.reserve(type.size());
    set (sit);
applyTemplate (type);  //可能扔
}

STObject::STObject (
    SerialIter& sit, SField const& name, int depth) noexcept (false)
    : STBase(name)
    , mType(nullptr)
{
    if (depth > 10)
        Throw<std::runtime_error> ("Maximum nesting depth of STObject exceeded");
    set(sit, depth);
}

STObject&
STObject::operator= (STObject&& other)
{
    setFName(other.getFName());
    mType = other.mType;
    v_ = std::move(other.v_);
    return *this;
}

void STObject::set (const SOTemplate& type)
{
    v_.clear();
    v_.reserve(type.size());
    mType = &type;

    for (auto const& elem : type.all())
    {
        if (elem->flags != SOE_REQUIRED)
            v_.emplace_back(detail::nonPresentObject, elem->e_field);
        else
            v_.emplace_back(detail::defaultObject, elem->e_field);
    }
}

void STObject::applyTemplate (const SOTemplate& type) noexcept (false)
{
    auto throwFieldErr = [] (std::string const& field, char const* description)
    {
        std::stringstream ss;
        ss << "Field '" << field << "' " << description;
        std::string text {ss.str()};
        JLOG (debugLog().error()) << "STObject::applyTemplate failed: " << text;
        Throw<FieldErr> (text);
    };

    mType = &type;
    decltype(v_) v;
    v.reserve(type.size());
    for (auto const& e : type.all())
    {
        auto const iter = std::find_if(
            v_.begin(), v_.end(), [&](detail::STVar const& b)
                { return b.get().getFName() == e->e_field; });
        if (iter != v_.end())
        {
            if ((e->flags == SOE_DEFAULT) && iter->get().isDefault())
            {
                throwFieldErr (e->e_field.fieldName,
                    "may not be explicitly set to default.");
            }
            v.emplace_back(std::move(*iter));
            v_.erase(iter);
        }
        else
        {
            if (e->flags == SOE_REQUIRED)
            {
                throwFieldErr (e->e_field.fieldName,
                    "is required but missing.");
            }
            v.emplace_back(detail::nonPresentObject, e->e_field);
        }
    }
    for (auto const& e : v_)
    {
//任何遗留在对象中的东西都必须是可丢弃的
        if (! e->getFName().isDiscardable())
        {
            throwFieldErr (e->getFName().getName(),
                "found in disallowed location.");
        }
    }
//将匹配数据的模板替换为旧数据，
//清除所有剩余的垃圾
    v_.swap(v);
}

void STObject::applyTemplateFromSField (SField const& sField) noexcept (false)
{
    SOTemplate const* elements =
        InnerObjectFormats::getInstance ().findSOTemplateBySField (sField);
    if (elements)
applyTemplate (*elements);  //可能扔
}

//返回真=以对象结尾终止
bool STObject::set (SerialIter& sit, int depth) noexcept (false)
{
    bool reachedEndOfObject = false;

    v_.clear();

//消耗管道中的数据，直到用完或到达终点
    while (!sit.empty ())
    {
        int type;
        int field;

//获取下一个字段的元数据
        sit.getFieldID(type, field);

//已找到对象终止标记和终止
//标记已被使用。反序列化完成。
        if (type == STI_OBJECT && field == 1)
        {
            reachedEndOfObject = true;
            break;
        }

        if (type == STI_ARRAY && field == 1)
        {
            JLOG (debugLog().error())
                << "Encountered object with embedded end-of-array marker";
            Throw<std::runtime_error>("Illegal end-of-array marker in object");
        }

        auto const& fn = SField::getField(type, field);

        if (fn.isInvalid ())
        {
            JLOG (debugLog().error())
                << "Unknown field: field_type=" << type
                << ", field_name=" << field;
            Throw<std::runtime_error> ("Unknown field");
        }

//把这块地拆开
        v_.emplace_back(sit, fn, depth+1);

//如果对象类型具有已知的sotemplate，则设置它。
        if (auto const obj = dynamic_cast<STObject*>(&(v_.back().get())))
obj->applyTemplateFromSField (fn);  //可能扔
    }

//我们希望确保反序列化对象不包含任何
//重复字段。这是一个键不变量：
    auto const sf = getSortedFields(*this, withAllFields);

    auto const dup = std::adjacent_find (sf.cbegin(), sf.cend(),
        [] (STBase const* lhs, STBase const* rhs)
        { return lhs->getFName() == rhs->getFName(); });

    if (dup != sf.cend())
        Throw<std::runtime_error> ("Duplicate field detected");

    return reachedEndOfObject;
}

bool STObject::hasMatchingEntry (const STBase& t)
{
    const STBase* o = peekAtPField (t.getFName ());

    if (!o)
        return false;

    return t == *o;
}

std::string STObject::getFullText () const
{
    std::string ret;
    bool first = true;

    if (fName->hasName ())
    {
        ret = fName->getName ();
        ret += " = {";
    }
    else ret = "{";

    for (auto const& elem : v_)
    {
        if (elem->getSType () != STI_NOTPRESENT)
        {
            if (!first)
                ret += ", ";
            else
                first = false;

            ret += elem->getFullText ();
        }
    }

    ret += "}";
    return ret;
}

std::string STObject::getText () const
{
    std::string ret = "{";
    bool first = false;
    for (auto const& elem : v_)
    {
        if (! first)
        {
            ret += ", ";
            first = false;
        }

        ret += elem->getText ();
    }
    ret += "}";
    return ret;
}

bool STObject::isEquivalent (const STBase& t) const
{
    const STObject* v = dynamic_cast<const STObject*> (&t);

    if (!v)
        return false;

    if (mType != nullptr && v->mType == mType)
    {
        return std::equal (begin(), end(), v->begin(), v->end(),
            [](STBase const& st1, STBase const& st2)
            {
                return (st1.getSType() == st2.getSType()) && st1.isEquivalent(st2);
            });
    }

    auto const sf1 = getSortedFields(*this, withAllFields);
    auto const sf2 = getSortedFields(*v, withAllFields);

    return std::equal (sf1.begin (), sf1.end (), sf2.begin (), sf2.end (),
        [] (STBase const* st1, STBase const* st2)
        {
            return (st1->getSType() == st2->getSType()) && st1->isEquivalent (*st2);
        });
}

uint256 STObject::getHash (std::uint32_t prefix) const
{
    Serializer s;
    s.add32 (prefix);
    add (s, withAllFields);
    return s.getSHA512Half ();
}

uint256 STObject::getSigningHash (std::uint32_t prefix) const
{
    Serializer s;
    s.add32 (prefix);
    add (s, omitSigningFields);
    return s.getSHA512Half ();
}

int STObject::getFieldIndex (SField const& field) const
{
    if (mType != nullptr)
        return mType->getIndex (field);

    int i = 0;
    for (auto const& elem : v_)
    {
        if (elem->getFName () == field)
            return i;
        ++i;
    }
    return -1;
}

const STBase& STObject::peekAtField (SField const& field) const
{
    int index = getFieldIndex (field);

    if (index == -1)
        Throw<std::runtime_error> ("Field not found");

    return peekAtIndex (index);
}

STBase& STObject::getField (SField const& field)
{
    int index = getFieldIndex (field);

    if (index == -1)
        Throw<std::runtime_error> ("Field not found");

    return getIndex (index);
}

SField const&
STObject::getFieldSType (int index) const
{
    return v_[index]->getFName ();
}

const STBase* STObject::peekAtPField (SField const& field) const
{
    int index = getFieldIndex (field);

    if (index == -1)
        return nullptr;

    return peekAtPIndex (index);
}

STBase* STObject::getPField (SField const& field, bool createOkay)
{
    int index = getFieldIndex (field);

    if (index == -1)
    {
        if (createOkay && isFree ())
            return getPIndex(emplace_back(detail::defaultObject, field));

        return nullptr;
    }

    return getPIndex (index);
}

bool STObject::isFieldPresent (SField const& field) const
{
    int index = getFieldIndex (field);

    if (index == -1)
        return false;

    return peekAtIndex (index).getSType () != STI_NOTPRESENT;
}

STObject& STObject::peekFieldObject (SField const& field)
{
    return peekField<STObject> (field);
}

STArray& STObject::peekFieldArray (SField const& field)
{
    return peekField<STArray> (field);
}

bool STObject::setFlag (std::uint32_t f)
{
    STUInt32* t = dynamic_cast<STUInt32*> (getPField (sfFlags, true));

    if (!t)
        return false;

    t->setValue (t->value () | f);
    return true;
}

bool STObject::clearFlag (std::uint32_t f)
{
    STUInt32* t = dynamic_cast<STUInt32*> (getPField (sfFlags));

    if (!t)
        return false;

    t->setValue (t->value () & ~f);
    return true;
}

bool STObject::isFlag (std::uint32_t f) const
{
    return (getFlags () & f) == f;
}

std::uint32_t STObject::getFlags (void) const
{
    const STUInt32* t = dynamic_cast<const STUInt32*> (peekAtPField (sfFlags));

    if (!t)
        return 0;

    return t->value ();
}

STBase* STObject::makeFieldPresent (SField const& field)
{
    int index = getFieldIndex (field);

    if (index == -1)
    {
        if (!isFree ())
            Throw<std::runtime_error> ("Field not found");

        return getPIndex (emplace_back(detail::nonPresentObject, field));
    }

    STBase* f = getPIndex (index);

    if (f->getSType () != STI_NOTPRESENT)
        return f;

    v_[index] = detail::STVar(
        detail::defaultObject, f->getFName());
    return getPIndex (index);
}

void STObject::makeFieldAbsent (SField const& field)
{
    int index = getFieldIndex (field);

    if (index == -1)
        Throw<std::runtime_error> ("Field not found");

    const STBase& f = peekAtIndex (index);

    if (f.getSType () == STI_NOTPRESENT)
        return;
    v_[index] = detail::STVar(
        detail::nonPresentObject, f.getFName());
}

bool STObject::delField (SField const& field)
{
    int index = getFieldIndex (field);

    if (index == -1)
        return false;

    delField (index);
    return true;
}

void STObject::delField (int index)
{
    v_.erase (v_.begin () + index);
}

unsigned char STObject::getFieldU8 (SField const& field) const
{
    return getFieldByValue <STUInt8> (field);
}

std::uint16_t STObject::getFieldU16 (SField const& field) const
{
    return getFieldByValue <STUInt16> (field);
}

std::uint32_t STObject::getFieldU32 (SField const& field) const
{
    return getFieldByValue <STUInt32> (field);
}

std::uint64_t STObject::getFieldU64 (SField const& field) const
{
    return getFieldByValue <STUInt64> (field);
}

uint128 STObject::getFieldH128 (SField const& field) const
{
    return getFieldByValue <STHash128> (field);
}

uint160 STObject::getFieldH160 (SField const& field) const
{
    return getFieldByValue <STHash160> (field);
}

uint256 STObject::getFieldH256 (SField const& field) const
{
    return getFieldByValue <STHash256> (field);
}

AccountID STObject::getAccountID (SField const& field) const
{
    return getFieldByValue <STAccount> (field);
}

Blob STObject::getFieldVL (SField const& field) const
{
    STBlob empty;
    STBlob const& b = getFieldByConstRef <STBlob> (field, empty);
    return Blob (b.data (), b.data () + b.size ());
}

STAmount const& STObject::getFieldAmount (SField const& field) const
{
    static STAmount const empty{};
    return getFieldByConstRef <STAmount> (field, empty);
}

STPathSet const& STObject::getFieldPathSet (SField const& field) const
{
    static STPathSet const empty{};
    return getFieldByConstRef <STPathSet> (field, empty);
}

const STVector256& STObject::getFieldV256 (SField const& field) const
{
    static STVector256 const empty{};
    return getFieldByConstRef <STVector256> (field, empty);
}

const STArray& STObject::getFieldArray (SField const& field) const
{
    static STArray const empty{};
    return getFieldByConstRef <STArray> (field, empty);
}

void
STObject::set (std::unique_ptr<STBase> v)
{
    auto const i =
        getFieldIndex(v->getFName());
    if (i != -1)
    {
        v_[i] = std::move(*v);
    }
    else
    {
        if (! isFree())
            Throw<std::runtime_error> (
                "missing field in templated STObject");
        v_.emplace_back(std::move(*v));
    }
}

void STObject::setFieldU8 (SField const& field, unsigned char v)
{
    setFieldUsingSetValue <STUInt8> (field, v);
}

void STObject::setFieldU16 (SField const& field, std::uint16_t v)
{
    setFieldUsingSetValue <STUInt16> (field, v);
}

void STObject::setFieldU32 (SField const& field, std::uint32_t v)
{
    setFieldUsingSetValue <STUInt32> (field, v);
}

void STObject::setFieldU64 (SField const& field, std::uint64_t v)
{
    setFieldUsingSetValue <STUInt64> (field, v);
}

void STObject::setFieldH128 (SField const& field, uint128 const& v)
{
    setFieldUsingSetValue <STHash128> (field, v);
}

void STObject::setFieldH256 (SField const& field, uint256 const& v)
{
    setFieldUsingSetValue <STHash256> (field, v);
}

void STObject::setFieldV256 (SField const& field, STVector256 const& v)
{
    setFieldUsingSetValue <STVector256> (field, v);
}

void STObject::setAccountID (SField const& field, AccountID const& v)
{
    setFieldUsingSetValue <STAccount> (field, v);
}

void STObject::setFieldVL (SField const& field, Blob const& v)
{
    setFieldUsingSetValue <STBlob> (field, Buffer(v.data (), v.size ()));
}

void STObject::setFieldVL (SField const& field, Slice const& s)
{
    setFieldUsingSetValue <STBlob>
            (field, Buffer(s.data(), s.size()));
}

void STObject::setFieldAmount (SField const& field, STAmount const& v)
{
    setFieldUsingAssignment (field, v);
}

void STObject::setFieldPathSet (SField const& field, STPathSet const& v)
{
    setFieldUsingAssignment (field, v);
}

void STObject::setFieldArray (SField const& field, STArray const& v)
{
    setFieldUsingAssignment (field, v);
}

Json::Value STObject::getJson (int options) const
{
    Json::Value ret (Json::objectValue);

//TODO（汤姆）：这个变量永远不会改变……？
    int index = 1;
    for (auto const& elem : v_)
    {
        if (elem->getSType () != STI_NOTPRESENT)
        {
            auto const& n = elem->getFName ();
            auto key = n.hasName () ? std::string(n.getJsonName ()) :
                    std::to_string (index);
            ret[key] = elem->getJson (options);
        }
    }
    return ret;
}

bool STObject::operator== (const STObject& obj) const
{
//这不是特别有效，只比较数据元素
//用二进制表示
    int matches = 0;
    for (auto const& t1 : v_)
    {
        if ((t1->getSType () != STI_NOTPRESENT) && t1->getFName ().isBinary ())
        {
//每个现有字段必须有一个匹配的字段
            bool match = false;
            for (auto const& t2 : obj.v_)
            {
                if (t1->getFName () == t2->getFName ())
                {
                    if (t2 != t1)
                        return false;

                    match = true;
                    ++matches;
                    break;
                }
            }

            if (!match)
                return false;
        }
    }

    int fields = 0;
    for (auto const& t2 : obj.v_)
    {
        if ((t2->getSType () != STI_NOTPRESENT) && t2->getFName ().isBinary ())
            ++fields;
    }

    if (fields != matches)
        return false;

    return true;
}

void STObject::add (Serializer& s, WhichFields whichFields) const
{
//根据字段的类型，签名字段可以序列化，也可以
//不是。然后字段被添加到按字段代码排序的序列化程序中。
    std::vector<STBase const*> const
        fields {getSortedFields (*this, whichFields)};

//插入排序
    for (STBase const* const field : fields)
    {
//当我们在另一个对象中序列化一个对象时，
//按规则与此字段名关联的类型
//必须是对象，否则无法反序列化对象
        SerializedTypeID const sType {field->getSType()};
        assert ((sType != STI_OBJECT) ||
            (field->getFName().fieldType == STI_OBJECT));
        field->addFieldID (s);
        field->add (s);
        if (sType == STI_ARRAY || sType == STI_OBJECT)
            s.addFieldID (sType, 1);
    }
}

std::vector<STBase const*>
STObject::getSortedFields (
    STObject const& objToSort, WhichFields whichFields)
{
    std::vector<STBase const*> sf;
    sf.reserve (objToSort.getCount());

//选择需要排序的字段。
    for (detail::STVar const& elem : objToSort.v_)
    {
        STBase const& base = elem.get();
        if ((base.getSType() != STI_NOTPRESENT) &&
            base.getFName().shouldInclude (whichFields))
        {
            sf.push_back (&base);
        }
    }

//按字段代码对字段排序。
    std::sort (sf.begin(), sf.end(),
        [] (STBase const* lhs, STBase const* rhs)
        {
            return lhs->getFName().fieldCode < rhs->getFName().fieldCode;
        });

    return sf;
}

} //涟漪
