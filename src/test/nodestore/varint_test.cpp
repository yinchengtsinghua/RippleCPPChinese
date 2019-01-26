
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
    版权所有2014，vinnie falco<vinnie.falco@gmail.com>

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

#include <ripple/beast/unit_test.h>
#include <ripple/nodestore/impl/varint.h>
#include <array>
#include <vector>

namespace ripple {
namespace NodeStore {
namespace tests {

class varint_test : public beast::unit_test::suite
{
public:
    void
    test_varints (std::vector<std::size_t> vv)
    {
        testcase("encode, decode");
        for (auto const v : vv)
        {
            std::array<std::uint8_t, varint_traits<std::size_t>::max> vi;
            auto const n0 = write_varint(vi.data(), v);
            expect (n0 > 0, "write error");
            expect(n0 == size_varint(v), "size error");
            std::size_t v1;
            auto const n1 = read_varint(vi.data(), n0, v1);
            expect(n1 == n0, "read error");
            expect(v == v1, "wrong value");
        }
    }

    void
    run() override
    {
        test_varints({
                0,     1,     2,
              126,   127,   128,
              253,   254,   255,
            16127, 16128, 16129,
            0xff,
            0xffff,
            0xffffffff,
            0xffffffffffffUL,
            0xffffffffffffffffUL});
    }
};

BEAST_DEFINE_TESTSUITE(varint,NodeStore,ripple);

} //测试
} //节点存储
} //涟漪
