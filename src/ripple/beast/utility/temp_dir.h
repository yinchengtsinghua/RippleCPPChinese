
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

#ifndef BEAST_UTILITY_TEMP_DIR_H_INCLUDED
#define BEAST_UTILITY_TEMP_DIR_H_INCLUDED

#include <boost/filesystem.hpp>
#include <string>

namespace beast {

/*RAII临时目录。

    当
    “temp_dir”的实例已销毁。
**/

class temp_dir
{
    boost::filesystem::path path_;

public:
#if ! GENERATING_DOCS
    temp_dir(const temp_dir&) = delete;
    temp_dir& operator=(const temp_dir&) = delete;
#endif

///构造临时目录。
    temp_dir()
    {
        auto const dir =
            boost::filesystem::temp_directory_path();
        do
        {
            path_ =
                dir / boost::filesystem::unique_path();
        }
        while(boost::filesystem::exists(path_));
        boost::filesystem::create_directory (path_);
    }

///销毁临时目录。
    ~temp_dir()
    {
//在析构函数中使用非引发调用
        boost::system::error_code ec;
        boost::filesystem::remove_all(path_, ec);
//TODO:警告/通知是否设置了EC？
    }

///get临时目录的本机路径
    std::string
    path() const
    {
        return path_.string();
    }

    /*获取A文件的本机路径。

        文件不需要存在。
    **/

    std::string
    file(std::string const& name) const
    {
        return (path_ / name).string();
    }
};

} //野兽

#endif
