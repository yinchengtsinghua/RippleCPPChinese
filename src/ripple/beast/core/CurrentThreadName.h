
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

    这个文件的一部分来自JUCE。
    版权所有（c）2013-原材料软件有限公司
    请访问http://www.juce.com

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

#ifndef BEAST_CORE_CURRENT_THREAD_NAME_H_INCLUDED
#define BEAST_CORE_CURRENT_THREAD_NAME_H_INCLUDED

#include <boost/optional.hpp>
#include <string>

namespace beast {

/*更改调用线程的名称。
    不同的操作系统可能对此名称设置不同的长度或内容限制。
**/

void setCurrentThreadName (std::string newThreadName);

/*返回调用线程的名称。

    返回的名称是通过调用setCurrentThreadName（）设置的名称。
    如果线程名称由外力设置，则该名称将更改
    不会被报告。如果从来没有设置过任何名称，那么boost：：none
    返回。
**/

boost::optional<std::string> getCurrentThreadName ();

}

#endif
