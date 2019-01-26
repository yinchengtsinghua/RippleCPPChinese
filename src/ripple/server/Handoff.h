
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

#ifndef RIPPLE_SERVER_HANDOFF_H_INCLUDED
#define RIPPLE_SERVER_HANDOFF_H_INCLUDED

#include <ripple/server/Writer.h>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <memory>

namespace ripple {

using http_request_type =
    boost::beast::http::request<boost::beast::http::dynamic_body>;

using http_response_type =
    boost::beast::http::response<boost::beast::http::dynamic_body>;

/*用于指示服务器连接切换的结果。*/
struct Handoff
{
//当“true”时，会话将关闭套接字。这个
//处理程序可以选择使用std:：move获取套接字所有权。
    bool moved = false;

//如果设置了响应，这将确定保持活动状态
    bool keep_alive = false;

//设置后，此邮件将被发送回
    std::shared_ptr<Writer> response;

    bool handled() const
    {
        return moved || response;
    }
};

} //涟漪

#endif
