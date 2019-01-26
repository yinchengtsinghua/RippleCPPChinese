
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

#include <ripple/protocol/STBase.h>
#include <boost/checked_delete.hpp>
#include <cassert>
#include <memory>

namespace ripple {

STBase::STBase()
    : fName(&sfGeneric)
{
}

STBase::STBase (SField const& n)
    : fName(&n)
{
    assert(fName);
}

STBase&
STBase::operator= (const STBase& t)
{
    if ((t.fName != fName) && fName->isUseful() && t.fName->isUseful())
    {
//vfalco我们不应该记录在这么低的A级别
        /*
        写入日志（（t.getStype（）==sti_amount）？lstrace:lswarning，stbase）//这对于金额很常见
                <“小心：”<<t.fname->getname（）<“不替换”<<fname->getname（）；
        **/

    }
    if (!fName->isUseful())
        fName = t.fName;
    return *this;
}

bool
STBase::operator== (const STBase& t) const
{
    return (getSType () == t.getSType ()) && isEquivalent (t);
}

bool
STBase::operator!= (const STBase& t) const
{
    return (getSType () != t.getSType ()) || !isEquivalent (t);
}

SerializedTypeID
STBase::getSType() const
{
    return STI_NOTPRESENT;
}

std::string
STBase::getFullText() const
{
    std::string ret;

    if (getSType () != STI_NOTPRESENT)
    {
        if (fName->hasName ())
        {
            ret = fName->fieldName;
            ret += " = ";
        }

        ret += getText ();
    }

    return ret;
}

std::string
STBase::getText() const
{
    return std::string();
}

Json::Value
/*ase:：getjson（int/*选项*/）const
{
    返回getText（）；
}

无效
stbase：：添加（序列化程序&s）常量
{
    //不应调用
    断言（假）；
}

布尔
stbase：：等量（const stbase&t）const
{
    断言（getstype（）==sti不存在）；
    如果（t.getstype（）==sti_不存在）
        回归真实；
    //vfalc我们不应该记录在这么低的A级别
    //写入日志（lsdebug，stbase）<“notequiv”<<getfulltext（）<“not sti_notpresent”；
    返回错误；
}

布尔
stbase:：isdefault（）常量
{
    回归真实；
}

无效
stbase:：setfname（sfield const&n）
{
    FNEY＝& N；
    断言（FNEX）；
}

斯菲尔德常数
stbase:：getfname（）常量
{
    返回* FNEX；
}

无效
stbase:：addFieldID（序列化程序&S）常量
{
    断言（fname->isbinary（））；
    s.addfieldid（fname->fieldtype，fname->fieldvalue）；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

STD：：
运算符<（std:：ostream&out，const stbase&t）
{
    返回<<t.getfulltext（）；
}

} /纹波
