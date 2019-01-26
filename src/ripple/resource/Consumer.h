
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

#ifndef RIPPLE_RESOURCE_CONSUMER_H_INCLUDED
#define RIPPLE_RESOURCE_CONSUMER_H_INCLUDED

#include <ripple/resource/Charge.h>
#include <ripple/resource/Disposition.h>

namespace ripple {
namespace Resource {

struct Entry;
class Logic;

/*消耗资源的终结点。*/
class Consumer
{
private:
    friend class Logic;
    Consumer (Logic& logic, Entry& entry);

public:
    Consumer ();
    ~Consumer ();
    Consumer (Consumer const& other);
    Consumer& operator= (Consumer const& other);

    /*返回唯一标识此使用者的可读字符串。*/
    std::string to_string () const;

    /*如果这是特权终结点，则返回“true”。*/
    bool isUnlimited () const;

    /*将使用者的特权级别提升到命名端点。
        将释放对原始端点描述符的引用。
    **/

    void elevate (std::string const& name);

    /*返回此使用者的当前处置。
        创建时应检查此项，以确定消费者是否
        应立即断开。
    **/

    Disposition disposition() const;

    /*向消费者收取负载费用。*/
    Disposition charge (Charge const& fee);

    /*如果应警告消费者，则返回“true”。
        这将消耗警告。
    **/

    bool warn();

    /*如果使用者应断开连接，则返回“true”。*/
    bool disconnect();

    /*返回表示消费的贷方余额。*/
    int balance();

//private：检索与使用者关联的条目
    Entry& entry();

private:
    Logic* m_logic;
    Entry* m_entry;
};

std::ostream& operator<< (std::ostream& os, Consumer const& v);

}
}

#endif
