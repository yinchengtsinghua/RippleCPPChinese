
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

#ifndef BEAST_UNIT_TEST_H_INCLUDED
#define BEAST_UNIT_TEST_H_INCLUDED

#include <beast/unit_test/amount.hpp>
#include <beast/unit_test/global_suites.hpp>
#include <beast/unit_test/match.hpp>
#include <beast/unit_test/recorder.hpp>
#include <beast/unit_test/reporter.hpp>
#include <beast/unit_test/results.hpp>
#include <beast/unit_test/runner.hpp>
#include <beast/unit_test/suite.hpp>
#include <beast/unit_test/suite_info.hpp>
#include <beast/unit_test/suite_list.hpp>

#ifndef BEAST_EXPECT
#define BEAST_EXPECT_S1(x) #x
#define BEAST_EXPECT_S2(x) BEAST_EXPECT_S1(x)
//define beast_expect（cond）expect（cond，uu file_uuu“：”uuu line_uuuuuuuuuu）；while（false）
#define BEAST_EXPECT(cond) expect(cond, __FILE__ ":" BEAST_EXPECT_S2(__LINE__))
#endif

#endif
