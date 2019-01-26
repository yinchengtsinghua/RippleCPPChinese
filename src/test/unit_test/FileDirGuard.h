
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

#ifndef TEST_UNIT_TEST_DIRGUARD_H
#define TEST_UNIT_TEST_DIRGUARD_H

#include <ripple/basics/contract.h>
#include <test/jtx/TestSuite.h>
#include <boost/filesystem.hpp>

namespace ripple {
namespace test {
namespace detail {

/*
    创建一个目录，完成后将其删除
**/

class DirGuard
{
protected:
    using path = boost::filesystem::path;

private:
    path subDir_;
    bool rmSubDir_{false};

protected:
    beast::unit_test::suite& test_;

    auto rmDir (path const& toRm)
    {
        if (is_directory (toRm) && is_empty (toRm))
            remove (toRm);
        else
            test_.log << "Expected " << toRm.string ()
            << " to be an empty existing directory." << std::endl;
    }

public:
    DirGuard (beast::unit_test::suite& test, path subDir,
        bool useCounter = true)
        : subDir_ (std::move (subDir))
        , test_ (test)
    {
        using namespace boost::filesystem;

        static auto subDirCounter = 0;
        if (useCounter)
            subDir_ += std::to_string(++subDirCounter);
        if (!exists (subDir_))
        {
            create_directory (subDir_);
            rmSubDir_ = true;
        }
        else if (is_directory (subDir_))
            rmSubDir_ = false;
        else
        {
//无法运行测试。有人在我们想要的地方创建了一个文件
//把我们的目录
            Throw<std::runtime_error> (
                "Cannot create directory: " + subDir_.string ());
        }
    }

    ~DirGuard ()
    {
        try
        {
            using namespace boost::filesystem;

            if (rmSubDir_)
                rmDir (subDir_);
            else
                test_.log << "Skipping rm dir: "
                << subDir_.string () << std::endl;
        }
        catch (std::exception& e)
        {
//如果我们扔到这里，就让它死吧。
            test_.log << "Error in ~DirGuard: " << e.what () << std::endl;
        };
    }

    path const& subdir() const
    {
        return subDir_;
    }
};

/*
    在目录中写入文件，完成后删除
**/

class FileDirGuard : public DirGuard
{
protected:
    path const file_;

public:
    FileDirGuard(beast::unit_test::suite& test,
        path subDir, path file, std::string const& contents,
        bool useCounter = true)
        : DirGuard(test, subDir, useCounter)
        , file_(file.is_absolute()  ? file : subdir() / file)
    {
        if (!exists (file_))
        {
            std::ofstream o (file_.string ());
            o << contents;
        }
        else
        {
            Throw<std::runtime_error> (
                "Refusing to overwrite existing file: " +
                file_.string ());
        }
    }

    ~FileDirGuard ()
    {
        try
        {
            using namespace boost::filesystem;
            if (!exists (file_))
                test_.log << "Expected " << file_.string ()
                << " to be an existing file." << std::endl;
            else
                remove (file_);
        }
        catch (std::exception& e)
        {
//如果我们扔到这里，就让它死吧。
            test_.log << "Error in ~FileGuard: "
                << e.what () << std::endl;
        };
    }

    path const& file() const
    {
        return file_;
    }

    bool fileExists () const
    {
        return boost::filesystem::exists (file_);
    }
};

}
}
}

#endif //测试单元测试dirguard
