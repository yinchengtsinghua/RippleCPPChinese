
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

#ifndef RIPPLE_PEERFINDER_SOURCE_H_INCLUDED
#define RIPPLE_PEERFINDER_SOURCE_H_INCLUDED

#include <ripple/peerfinder/PeerfinderManager.h>
#include <boost/system/error_code.hpp>

namespace ripple {
namespace PeerFinder {

/*对等地址的静态或动态源。
    当我们正在引导并且没有
    一个本地缓存，或者当我们的地址都不起作用时。典型地
    源将表示配置文件中的静态文本等内容，
    使用地址或远程https URL分隔本地文件
    自动更新。另一个解决方案是使用自定义DNS服务器
    在执行名称查找时分发对等IP地址。
**/

class Source
{
public:
    /*获取的结果。*/
    struct Results
    {
        explicit Results() = default;

//故障时的错误代码
        boost::system::error_code error;

//获取的终结点列表
        IPAddresses addresses;
    };

    virtual ~Source () { }
    virtual std::string const& name () = 0;
    virtual void cancel () { }
    virtual void fetch (Results& results, beast::Journal journal) = 0;
};

}
}

#endif
