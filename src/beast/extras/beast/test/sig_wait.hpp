
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

#ifndef BEAST_TEST_SIG_WAIT_HPP
#define BEAST_TEST_SIG_WAIT_HPP

#include <boost/asio.hpp>

namespace beast {
namespace test {

///block直到收到sigint或sigterm。
inline
void
sig_wait()
{
    boost::asio::io_service ios;
    boost::asio::signal_set signals(
        ios, SIGINT, SIGTERM);
    signals.async_wait(
        [&](boost::system::error_code const&, int)
        {
        });
    ios.run();
}

} //测试
} //野兽

#endif
