
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

#include <ripple/beast/unit_test.h>
#include <ripple/app/consensus/RCLCensorshipDetector.h>
#include <algorithm>
#include <vector>

namespace ripple {
namespace test {

class RCLCensorshipDetector_test : public beast::unit_test::suite
{
    void test(
        RCLCensorshipDetector<int, int>& cdet, int round,
        std::vector<int> proposed, std::vector<int> accepted,
        std::vector<int> remain, std::vector<int> remove)
    {
//开始跟踪我们这一轮的提议
        cdet.propose(round, std::move(proposed));

//通过处理我们接受的内容，最终确定回合；然后
//删除任何需要删除的内容，并确保
//剩下的是正确的。
        cdet.check(std::move(accepted),
            [&remove, &remain](auto id, auto seq)
            {
//如果该项目应该从审查中删除
//手动检测内部跟踪器，现在就做：
                if (std::find(remove.begin(), remove.end(), id) != remove.end())
                    return true;

//如果该项目仍在审查中
//探测器内部跟踪器；将其从矢量中移除。
                auto it = std::find(remain.begin(), remain.end(), id);
                if (it != remain.end())
                    remain.erase(it);
                return false;
            });

//输入时，此集合包含所有应跟踪的元素
//在我们处理完这一轮之后，通过探测器。我们把所有的东西都拿走了
//它实际上在追踪器中，所以现在应该是空的：
        BEAST_EXPECT(remain.empty());
    }

public:
    void
    run() override
    {
        testcase ("Censorship Detector");

        RCLCensorshipDetector<int, int> cdet;
        int round = 0;

        test(cdet, ++round,     { },                { },        { },            { });
        test(cdet, ++round,     { 10, 11, 12, 13 }, { 11, 2 },  { 10, 13 },     { });
        test(cdet, ++round,     { 10, 13, 14, 15 }, { 14 },     { 10, 13, 15 }, { });
        test(cdet, ++round,     { 10, 13, 15, 16 }, { 15, 16 }, { 10, 13 },     { });
        test(cdet, ++round,     { 10, 13 },         { 17, 18 }, { 10, 13 },     { });
        test(cdet, ++round,     { 10, 19 },         { },        { 10, 19 },     { });
        test(cdet, ++round,     { 10, 19, 20 },     { 20 },     { 10 },         { 19 });
        test(cdet, ++round,     { 21 },             { 21 },     { },            { });
        test(cdet, ++round,     { },                { 22 },     { },            { });
        test(cdet, ++round,     { 23, 24, 25, 26 }, { 25, 27 }, { 23, 26 },     { 24 });
        test(cdet, ++round,     { 23, 26, 28 },     { 26, 28 }, { 23 },         { });

        for (int i = 0; i != 10; ++i)
            test(cdet, ++round, { 23 },             { },        { 23 },         { });

        test(cdet, ++round,     { 23, 29 },         { 29 },     { 23 },         { });
        test(cdet, ++round,     { 30, 31 },         { 31 },     { 30 },         { });
        test(cdet, ++round,     { 30 },             { 30 },     { },            { });
        test(cdet, ++round,     { },                { },        { },            { });
    }
};

BEAST_DEFINE_TESTSUITE(RCLCensorshipDetector, app, ripple);
}  //命名空间测试
}  //命名空间波纹
