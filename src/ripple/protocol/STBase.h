
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

#ifndef RIPPLE_PROTOCOL_STBASE_H_INCLUDED
#define RIPPLE_PROTOCOL_STBASE_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/Serializer.h>
#include <ostream>
#include <memory>
#include <string>
#include <typeinfo>
#include <utility>
#include <type_traits>
namespace ripple {

//vfalc要修复此副本分配限制。
//
//注意：不要创建任何派生对象的向量（或类似容器）
//从斯巴基。使用Boost ptr_x容器。复制分配运算符
//stbase的语义将导致包含的类型更改
//当对象被删除时，由于复制分配被用于
//“向下滑动”其余类型，这不会复制字段
//姓名。更改“复制分配”运算符以复制字段名将中断
//使用复制分配仅复制事务中使用的值
//发动机代码。

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*可以导出为众所周知的二进制格式的类型。

    STBase：
        -始终是字段
        -始终可以进入合格的封闭式stbase
            （如斯塔雷）
        -具有字段名

    像JSON一样，序列化对象是一个有规则的篮子。
    它能容纳什么。

    @note“st”代表“序列化类型”。
**/

class STBase
{
public:
    STBase();

    explicit
    STBase (SField const& n);

    virtual ~STBase() = default;

    STBase(const STBase& t) = default;
    STBase& operator= (const STBase& t);

    bool operator== (const STBase& t) const;
    bool operator!= (const STBase& t) const;

    virtual
    STBase*
    copy (std::size_t n, void* buf) const
    {
        return emplace(n, buf, *this);
    }

    virtual
    STBase*
    move (std::size_t n, void* buf)
    {
        return emplace(n, buf, std::move(*this));
    }

    template <class D>
    D&
    downcast()
    {
        D* ptr = dynamic_cast<D*> (this);
        if (ptr == nullptr)
            Throw<std::bad_cast> ();
        return *ptr;
    }

    template <class D>
    D const&
    downcast() const
    {
        D const * ptr = dynamic_cast<D const*> (this);
        if (ptr == nullptr)
            Throw<std::bad_cast> ();
        return *ptr;
    }

    virtual
    SerializedTypeID
    getSType() const;

    virtual
    std::string
    getFullText() const;

    virtual
    std::string
    getText() const;

    virtual
    Json::Value
    /*json（int/*选项*/）常量；

    事实上的
    无效
    添加（序列化程序&S）常量；

    事实上的
    布尔
    等量（stbase const&t）const；

    事实上的
    布尔
    isdefault（）常量；

    /**stbase是一个字段。
        这将设置名称。
    **/

    void
    setFName (SField const& n);

    SField const&
    getFName() const;

    void
    addFieldID (Serializer& s) const;

protected:
    SField const* fName;

    template <class T>
    static
    STBase*
    emplace(std::size_t n, void* buf, T&& val)
    {
        using U = std::decay_t<T>;
        if (sizeof(U) > n)
            return new U(std::forward<T>(val));
        return new(buf) U(std::forward<T>(val));
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::ostream& operator<< (std::ostream& out, const STBase& t);

} //涟漪

#endif
