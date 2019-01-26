
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//
//版权所有（c）2013-2017 Vinnie Falco（Gmail.com上的Vinnie Dot Falco）
//
//在Boost软件许可证1.0版下分发。（见附件
//文件许可证_1_0.txt或复制到http://www.boost.org/license_1_0.txt）
//

#ifndef BEAST_UNIT_TEST_DETAIL_CONST_CONTAINER_HPP
#define BEAST_UNIT_TEST_DETAIL_CONST_CONTAINER_HPP

namespace beast {
namespace unit_test {
namespace detail {

/*用于约束容器接口的适配器。
    接口允许有限的只读操作。派生类
    提供其他行为。
**/

template<class Container>
class const_container
{
private:
    using cont_type = Container;

    cont_type m_cont;

protected:
    cont_type& cont()
    {
        return m_cont;
    }

    cont_type const& cont() const
    {
        return m_cont;
    }

public:
    using value_type = typename cont_type::value_type;
    using size_type = typename cont_type::size_type;
    using difference_type = typename cont_type::difference_type;
    using iterator = typename cont_type::const_iterator;
    using const_iterator = typename cont_type::const_iterator;

    /*如果容器为空，则返回“true”。*/
    bool
    empty() const
    {
        return m_cont.empty();
    }

    /*返回容器中的项数。*/
    size_type
    size() const
    {
        return m_cont.size();
    }

    /*返回用于遍历的前向迭代器。*/
    /*@ {*/
    const_iterator
    begin() const
    {
        return m_cont.cbegin();
    }

    const_iterator
    cbegin() const
    {
        return m_cont.cbegin();
    }

    const_iterator
    end() const
    {
        return m_cont.cend();
    }

    const_iterator
    cend() const
    {
        return m_cont.cend();
    }
    /*@ }*/
};

} //细节
} //单位试验
} //野兽

#endif
