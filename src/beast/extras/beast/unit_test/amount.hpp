
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

#ifndef BEAST_UNIT_TEST_AMOUNT_HPP
#define BEAST_UNIT_TEST_AMOUNT_HPP

#include <cstddef>
#include <ostream>
#include <string>

namespace beast {
namespace unit_test {

/*用于产生单位数量的完美组合输出的实用程序。*/
class amount
{
private:
    std::size_t n_;
    std::string const& what_;

public:
    amount(amount const&) = default;
    amount& operator=(amount const&) = delete;

    template<class = void>
    amount(std::size_t n, std::string const& what);

    friend
    std::ostream&
    operator<<(std::ostream& s, amount const& t);
};

template<class>
amount::amount(std::size_t n, std::string const& what)
    : n_(n)
    , what_(what)
{
}

inline
std::ostream&
operator<<(std::ostream& s, amount const& t)
{
    s << t.n_ << " " << t.what_ <<((t.n_ != 1) ? "s" : "");
    return s;
}

} //单位试验
} //野兽

#endif
