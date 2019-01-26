
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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
#include <test/csf/Histogram.h>


namespace ripple {
namespace test {

class Histogram_test : public beast::unit_test::suite
{
public:
    void
    run() override
    {
        using namespace csf;
        Histogram<int> hist;

        BEAST_EXPECT(hist.size() == 0);
        BEAST_EXPECT(hist.numBins() == 0);
        BEAST_EXPECT(hist.minValue() == 0);
        BEAST_EXPECT(hist.maxValue() == 0);
        BEAST_EXPECT(hist.avg() == 0);
        BEAST_EXPECT(hist.percentile(0.0f) == hist.minValue());
        BEAST_EXPECT(hist.percentile(0.5f) == 0);
        BEAST_EXPECT(hist.percentile(0.9f) == 0);
        BEAST_EXPECT(hist.percentile(1.0f) == hist.maxValue());

        hist.insert(1);

        BEAST_EXPECT(hist.size() == 1);
        BEAST_EXPECT(hist.numBins() == 1);
        BEAST_EXPECT(hist.minValue() == 1);
        BEAST_EXPECT(hist.maxValue() == 1);
        BEAST_EXPECT(hist.avg() == 1);
        BEAST_EXPECT(hist.percentile(0.0f) == hist.minValue());
        BEAST_EXPECT(hist.percentile(0.5f) == 1);
        BEAST_EXPECT(hist.percentile(0.9f) == 1);
        BEAST_EXPECT(hist.percentile(1.0f) == hist.maxValue());

        hist.insert(9);

        BEAST_EXPECT(hist.size() == 2);
        BEAST_EXPECT(hist.numBins() == 2);
        BEAST_EXPECT(hist.minValue() == 1);
        BEAST_EXPECT(hist.maxValue() == 9);
        BEAST_EXPECT(hist.avg() == 5);
        BEAST_EXPECT(hist.percentile(0.0f) == hist.minValue());
        BEAST_EXPECT(hist.percentile(0.5f) == 1);
        BEAST_EXPECT(hist.percentile(0.9f) == 9);
        BEAST_EXPECT(hist.percentile(1.0f) == hist.maxValue());

        hist.insert(1);

        BEAST_EXPECT(hist.size() == 3);
        BEAST_EXPECT(hist.numBins() == 2);
        BEAST_EXPECT(hist.minValue() == 1);
        BEAST_EXPECT(hist.maxValue() == 9);
        BEAST_EXPECT(hist.avg() == 11/3);
        BEAST_EXPECT(hist.percentile(0.0f) == hist.minValue());
        BEAST_EXPECT(hist.percentile(0.5f) == 1);
        BEAST_EXPECT(hist.percentile(0.9f) == 9);
        BEAST_EXPECT(hist.percentile(1.0f) == hist.maxValue());
    }
};

BEAST_DEFINE_TESTSUITE(Histogram, test, ripple);

}  //测试
}  //涟漪
