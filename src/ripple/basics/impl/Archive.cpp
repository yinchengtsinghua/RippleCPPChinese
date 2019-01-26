
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
    版权所有（c）2012，2018 Ripple Labs Inc.

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

#include <ripple/basics/Archive.h>
#include <ripple/basics/contract.h>

#include <archive.h>
#include <archive_entry.h>

namespace ripple {

void
extractTarLz4(
    boost::filesystem::path const& src,
    boost::filesystem::path const& dst)
{
    if (!is_regular_file(src))
        Throw<std::runtime_error>("Invalid source file");

    using archive_ptr =
        std::unique_ptr<struct archive, void(*)(struct archive*)>;
    archive_ptr ar {archive_read_new(),
        [](struct archive* ar)
        {
            archive_read_free(ar);
        }};
    if (!ar)
        Throw<std::runtime_error>("Failed to allocate archive");

    if (archive_read_support_format_tar(ar.get()) < ARCHIVE_OK)
        Throw<std::runtime_error>(archive_error_string(ar.get()));

    if (archive_read_support_filter_lz4(ar.get()) < ARCHIVE_OK)
        Throw<std::runtime_error>(archive_error_string(ar.get()));

//示例建议此块大小
    if (archive_read_open_filename(
        ar.get(), src.string().c_str(), 10240) < ARCHIVE_OK)
    {
        Throw<std::runtime_error>(archive_error_string(ar.get()));
    }

    archive_ptr aw {archive_write_disk_new(),
        [](struct archive* aw)
        {
            archive_write_free(aw);
        }};
    if (!aw)
        Throw<std::runtime_error>("Failed to allocate archive");

    if (archive_write_disk_set_options(
        aw.get(),
        ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
        ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS) < ARCHIVE_OK)
    {
        Throw<std::runtime_error>(archive_error_string(aw.get()));
    }

    if(archive_write_disk_set_standard_lookup(aw.get()) < ARCHIVE_OK)
        Throw<std::runtime_error>(archive_error_string(aw.get()));

    int result;
    struct archive_entry* entry;
    while(true)
    {
        result = archive_read_next_header(ar.get(), &entry);
        if (result == ARCHIVE_EOF)
            break;
        if (result < ARCHIVE_OK)
            Throw<std::runtime_error>(archive_error_string(ar.get()));

        archive_entry_set_pathname(
            entry, (dst / archive_entry_pathname(entry)).string().c_str());
        if (archive_write_header(aw.get(), entry) < ARCHIVE_OK)
            Throw<std::runtime_error>(archive_error_string(aw.get()));

        if (archive_entry_size(entry) > 0)
        {
            const void *buf;
            size_t sz;
            la_int64_t offset;
            while (true)
            {
                result = archive_read_data_block(ar.get(), &buf, &sz, &offset);
                if (result == ARCHIVE_EOF)
                    break;
                if (result < ARCHIVE_OK)
                    Throw<std::runtime_error>(archive_error_string(ar.get()));

                if (archive_write_data_block(
                    aw.get(), buf, sz, offset) < ARCHIVE_OK)
                {
                    Throw<std::runtime_error>(archive_error_string(aw.get()));
                }
            }
        }

        if (archive_write_finish_entry(aw.get()) < ARCHIVE_OK)
            Throw<std::runtime_error>(archive_error_string(aw.get()));
    }
}

} //涟漪
