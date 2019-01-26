
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

#ifndef RIPPLE_RESOURCE_CHARGE_H_INCLUDED
#define RIPPLE_RESOURCE_CHARGE_H_INCLUDED

#include <ios>
#include <string>

namespace ripple {
namespace Resource {

/*消费费用。*/
class Charge
{
public:
    /*用于保存消费费用的类型。*/
    using value_type = int;

//默认构造电荷无法获得标签。删除
    Charge () = delete;

    /*使用指定的成本和名称创建费用。*/
    Charge (value_type cost, std::string const& label = std::string());

    /*返回与电荷相关的可读标签。*/
    std::string const& label() const;

    /*以资源：：经理单位返回费用的成本。*/
    value_type cost () const;

    /*将此电荷转换为人类可读的字符串。*/
    std::string to_string () const;

    bool operator== (Charge const&) const;
    bool operator!= (Charge const&) const;

private:
    value_type m_cost;
    std::string m_label;
};

std::ostream& operator<< (std::ostream& os, Charge const& v);

}
}

#endif
