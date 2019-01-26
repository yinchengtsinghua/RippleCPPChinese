
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
    版权所有（c）2017 Ripple Labs Inc.

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

#ifndef TEST_UNIT_TEST_MULTI_RUNNER_H
#define TEST_UNIT_TEST_MULTI_RUNNER_H

#include <boost/beast/core/static_string.hpp>
#include <beast/unit_test/global_suites.hpp>
#include <beast/unit_test/runner.hpp>

#include <boost/container/static_vector.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include <atomic>
#include <chrono>
#include <numeric>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>

namespace ripple {
namespace test {

namespace detail {

using clock_type = std::chrono::steady_clock;

struct case_results
{
    std::string name;
    std::size_t total = 0;
    std::size_t failed = 0;

    explicit
    case_results(std::string name_ = "")
        : name(std::move(name_))
    {
    }
};

struct suite_results
{
    std::string name;
    std::size_t cases = 0;
    std::size_t total = 0;
    std::size_t failed = 0;
    typename clock_type::time_point start = clock_type::now();

    explicit
    suite_results(std::string name_ = "")
        : name(std::move(name_))
    {
    }

    void
    add(case_results const& r);
};

struct results
{
    using static_string = boost::beast::static_string<256>;
//结果可以存储在共享内存中。使用“static_string”确保
//来自不同内存空间的指针不混合
    using run_time = std::pair<static_string, typename clock_type::duration>;

    enum { max_top = 10 };

    std::size_t suites = 0;
    std::size_t cases = 0;
    std::size_t total = 0;
    std::size_t failed = 0;
    boost::container::static_vector<run_time, max_top> top;
    typename clock_type::time_point start = clock_type::now();

    void
    add(suite_results const& r);

    void
    merge(results const& r);

    template <class S>
    void
    print(S& s);
};

template <bool IsParent>
class multi_runner_base
{
//“inner”将在共享内存中创建。这是一种方式
//多跑者父对象和多跑者子对象通信。其他的
//它们的通信方式是通过消息队列。
    struct inner
    {
        std::atomic<std::size_t> job_index_{0};
        std::atomic<std::size_t> test_index_{0};
        std::atomic<bool> any_failed_{false};
//父进程将定期增加“keep-live”。孩子
//进程将检查“保持活动”是否正在增加。如果是
//如果在足够长的时间内不递增，则子级将假定
//父进程已停止。
        std::atomic<std::size_t> keep_alive_{0};

        mutable boost::interprocess::interprocess_mutex m_;
        detail::results results_;

        std::size_t
        checkout_job_index();

        std::size_t
        checkout_test_index();

        bool
        any_failed() const;

        void
        any_failed(bool v);

        void
        inc_keep_alive_count();

        std::size_t
        get_keep_alive_count();

        void
        add(results const& r);

        template <class S>
        void
        print_results(S& s);
    };

    static constexpr const char* shared_mem_name_ = "RippledUnitTestSharedMem";
//多运行程序子级将用来与之通信的消息队列的名称
//多跑步者家长
    static constexpr const char* message_queue_name_ = "RippledUnitTestMessageQueue";

//“内部”将在共享内存中创建
    inner* inner_;
//用于“inner”成员的共享内存
    boost::interprocess::shared_memory_object shared_mem_;
    boost::interprocess::mapped_region region_;

protected:
    std::unique_ptr<boost::interprocess::message_queue> message_queue_;

    enum class MessageType : std::uint8_t {test_start, test_end, log};
    void message_queue_send(MessageType mt, std::string const& s);

public:
    multi_runner_base();
    ~multi_runner_base();

    std::size_t
    checkout_test_index();

    std::size_t
    checkout_job_index();

    void
    any_failed(bool v);

    void
    add(results const& r);

    void
    inc_keep_alive_count();

    std::size_t
    get_keep_alive_count();

    template <class S>
    void
    print_results(S& s);

    bool
    any_failed() const;
};

}  //细节

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*运行单元测试的儿童经理
 **/

/*ss multi_runner_parent:private detail:：multi_runner_base</*isparent*/true>
{
私人：
    //消息队列用于从子级收集日志消息
    标准：：Ostream&os_uu；
    std:：atomic<bool>continue_message_queue_true_
    std：：线程消息_queue_thread_u；
    //跟踪正在运行的套件，这样如果子程序崩溃，则可以标记罪犯
    std:：set<std:：string>正在运行的\u suites_uu；
公众：
    multi_runner_parent（multi_runner_parent const&）=删除；
    多跑步者家长和
    operator=（multi_runner_parent const&）=删除；

    multi_runner_parent（）；
    ~multi_runner_parent（）；

    布尔
    任何_failed（）常量；
}；

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/**用于运行单元测试子集的类
 **/

class multi_runner_child : public beast::unit_test::runner,
                           /*vate detail:：multi_runner_base</*isparent*/false>
{
私人：
    std：：大小“作业”索引；
    详细信息：结果结果\；
    详细信息：：suite_results suite_results_uuu；
    详细信息：案例结果案例结果
    标准：：大小作业数0
    布尔安静假
    bool print_log_true_

    std:：atomic<bool>continue_keep_live_true_
    std：：线程保持\u活动\u线程\；

公众：
    multi_runner_child（multi_runner_child const&）=删除；
    多跑者的孩子和
    operator=（multi_runner_child const&）=删除；

    multi_runner_child（std:：size_t num_jobs，bool quiet，bool print_log）；
    ~multi_runner_child（）；

    模板<class pred>
    布尔
    运行multi（pred pred）；

私人：
    虚虚空
    在“Suite”开始时（Beast：：Unit\测试：：Suite\信息常量和信息）覆盖；

    虚虚空
    在\u suite \u end（）覆盖上；

    虚虚空
    在“用例”开始时（std:：string const&name）重写；

    虚虚空
    在\u case \u end（）重写时；

    虚虚空
    onou pass（）覆盖；

    虚虚空
    当“失败”（std:：string const&reason）覆盖时；

    虚虚空
    在日志（std:：string const&s）上重写；
}；

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

模板<class pred>
布尔
multi_runner_child:：run_multi（pred pred）
{
    auto const&suite=Beast:：Unit_test:：Global_suites（）；
    auto const num_tests=suite.size（）；
    bool failed=false；

    自动获取_test=[&]（）->Beast:：Unit_test:：Suite_info const*
        auto const cur_test_index=checkout_test_index（）；
        如果（cur_test_index>=num_tests）
            返回null pTR；
        auto iter=suite.begin（）；
        std:：advance（iter，cur_test_index）；
        退货和退货；
    }；
    while（auto t=get_test（））
    {
        如果（！）PRD（*T）
            继续；
        尝试
        {
            失败=运行（*t）失败；
        }
        接住（…）
        {
            如果（num_jobs_uu<=1）
                throw；//一个进程可能会死亡

            //通知家长
            std：：字符串流s；
            s<<job_index<<“>未能在测试中处理异常。\n”；
            message_queue_send（messagetype:：log，s.str（））；
            失败=真；
        }
    }
    任何故障（失败）；
    返回失败；
}


} / / UNITY测试
} /兽

第二节
