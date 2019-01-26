
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

#ifndef RIPPLE_APP_LEDGER_BOOKLISTENERS_H_INCLUDED
#define RIPPLE_APP_LEDGER_BOOKLISTENERS_H_INCLUDED

#include <ripple/net/InfoSub.h>
#include <memory>
#include <mutex>

namespace ripple {

/*听一本书的公开/订阅信息。*/
class BookListeners
{
public:
    using pointer = std::shared_ptr<BookListeners>;

    BookListeners()
    {
    }

    /*为这本书添加新订阅
    **/

    void
    addSubscriber(InfoSub::ref sub);

    /*停止发布到订阅服务器
    **/

    void
    removeSubscriber(std::uint64_t sub);

    /*向订阅服务器发布事务

        向订阅了此书更改的客户端发布事务。
        使用HavePublished阻止向客户端发送重复的事务
        订阅了多本书。

        @param jvobj json要发布的事务数据
        @param已经发布了已经发布的信息子序列号
                             已发布此事务。

    **/

    void
    publish(Json::Value const& jvObj, hash_set<std::uint64_t>& havePublished);

private:
    std::recursive_mutex mLock;

    hash_map<std::uint64_t, InfoSub::wptr> mListeners;
};

}  //命名空间波纹

#endif
