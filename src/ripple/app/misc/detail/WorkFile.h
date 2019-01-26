
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
    版权所有（c）2018 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_MISC_DETAIL_WORKFILE_H_INCLUDED
#define RIPPLE_APP_MISC_DETAIL_WORKFILE_H_INCLUDED

#include <ripple/app/misc/detail/Work.h>
#include <ripple/basics/ByteUtilities.h>
#include <ripple/basics/FileUtilities.h>
#include <cassert>
#include <cerrno>

namespace ripple {

namespace detail {

//与文件一起工作
class WorkFile: public Work
    , public std::enable_shared_from_this<WorkFile>
{
protected:
    using error_code = boost::system::error_code;
//重写work.h中的定义
    using response_type = std::string;

public:
    using callback_type =
        std::function<void(error_code const&, response_type const&)>;
public:
    WorkFile(
        std::string const& path,
        boost::asio::io_service& ios, callback_type cb);
    ~WorkFile();

    void run() override;

    void cancel() override;

private:
    std::string path_;
    callback_type cb_;
    boost::asio::io_service& ios_;
    boost::asio::io_service::strand strand_;

};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

WorkFile::WorkFile(
    std::string const& path,
    boost::asio::io_service& ios, callback_type cb)
    : path_(path)
    , cb_(std::move(cb))
    , ios_(ios)
    , strand_(ios)
{
}

WorkFile::~WorkFile()
{
    if (cb_)
        cb_ (make_error_code(boost::system::errc::interrupted), {});
}

void
WorkFile::run()
{
    if (! strand_.running_in_this_thread())
        return ios_.post(strand_.wrap (std::bind(
            &WorkFile::run, shared_from_this())));

    error_code ec;
    auto const fileContents = getFileContents(ec, path_, megabytes(1));

    assert(cb_);
    cb_(ec, fileContents);
    cb_ = nullptr;
}

void
WorkFile::cancel()
{
//无事可做。要么是跑完了，要么就是没开始。
}


} //细节

} //涟漪

#endif
