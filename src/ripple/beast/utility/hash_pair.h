
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_UTILITY_HASH_PAIR_H_INCLUDED
#define BEAST_UTILITY_HASH_PAIR_H_INCLUDED

#include <functional>
#include <utility>

#include <boost/functional/hash.hpp>
#include <boost/utility/base_from_member.hpp>

namespace std {

/*任何std:：pair类型的std:：hash的专门化。*/
template <class First, class Second>
struct hash <std::pair <First, Second>>
    : private boost::base_from_member <std::hash <First>, 0>
    , private boost::base_from_member <std::hash <Second>, 1>
{
private:
    using first_hash = boost::base_from_member <std::hash <First>, 0>;
    using second_hash = boost::base_from_member <std::hash <Second>, 1>;

public:
    hash ()
    {
    }

    hash (std::hash <First> const& first_hash_,
          std::hash <Second> const& second_hash_)
          : first_hash (first_hash_)
          , second_hash (second_hash_)
    {
    }

    std::size_t operator() (std::pair <First, Second> const& value)
    {
        std::size_t result (first_hash::member (value.first));
        boost::hash_combine (result, second_hash::member (value.second));
        return result;
    }

    std::size_t operator() (std::pair <First, Second> const& value) const
    {
        std::size_t result (first_hash::member (value.first));
        boost::hash_combine (result, second_hash::member (value.second));
        return result;
    }
};

}

#endif
