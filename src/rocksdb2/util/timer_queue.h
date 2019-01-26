
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//部分版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//
//借来
//http://www.crazygaze.com/blog/2016/03/24/portable-c-timer-queue/
//计时器队列
//
//许可证
//
//本文中的源代码是在CC0许可下获得许可的，所以请感觉
//可以自由复制、修改、共享，随心所欲。
//不需要归因，但如果你这样做，我会很高兴。
//cc0许可证

//将工作与本契约关联的人已将工作奉献给
//通过放弃他或她在世界范围内的所有工作权利而获得的公共领域
//根据版权法，包括与
//法律允许的范围。您可以复制、修改、分发和执行
//工作，甚至是商业目的，都没有得到许可。

#pragma once

#include "port/port.h"

#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

//允许以后在指定时间执行处理程序
//保证：
//-所有处理程序都执行一次，即使取消（中止的参数将
//被设置为真）
//-如果TimerQueue被销毁，它将取消所有处理程序。
//-处理程序始终在计时器队列工作线程中执行。
//-不保证处理程序执行顺序
//
//////////////////////////////////////////////////
//借来
//http://www.crazygaze.com/blog/2016/03/24/portable-c-timer-queue/
class TimerQueue {
 public:
  TimerQueue() : m_th(&TimerQueue::run, this) {}

  ~TimerQueue() {
    cancelAll();
//滥用计时器队列触发关闭。
    add(0, [this](bool) {
      m_finish = true;
      return std::make_pair(false, 0);
    });
    m_th.join();
  }

//添加新计时器
//返回
//返回新计时器的ID。您可以使用此ID取消
//计时器
  uint64_t add(int64_t milliseconds,
               std::function<std::pair<bool, int64_t>(bool)> handler) {
    WorkItem item;
    Clock::time_point tp = Clock::now();
    item.end = tp + std::chrono::milliseconds(milliseconds);
    item.period = milliseconds;
    item.handler = std::move(handler);

    std::unique_lock<std::mutex> lk(m_mtx);
    uint64_t id = ++m_idcounter;
    item.id = id;
    m_items.push(std::move(item));

//有什么变化，所以唤醒计时器线程
    m_checkWork.notify_one();
    return id;
  }

//取消指定的计时器
//返回
//1如果计时器被取消。
//0，如果您晚了无法取消（或计时器ID对
//开始）
  size_t cancel(uint64_t id) {
//而不是从容器中取出物品（这样会破坏
//堆完整性），我们将该项设置为没有处理程序，然后
//位于顶部的新项上的用于立即执行的处理程序
//然后，计时器线程将忽略原始项，因为它没有
//处理程序。
    std::unique_lock<std::mutex> lk(m_mtx);
    for (auto&& item : m_items.getContainer()) {
      if (item.id == id && item.handler) {
        WorkItem newItem;
//零时间，所以它保持在顶部以便立即执行
        newItem.end = Clock::time_point();
newItem.id = 0;  //意味着这是一个取消的项目
//将处理程序从项移动到newitem（从而清除项）
        newItem.handler = std::move(item.handler);
        m_items.push(std::move(newItem));

//有什么变化，所以唤醒计时器线程
        m_checkWork.notify_one();
        return 1;
      }
    }
    return 0;
  }

//取消所有计时器
//返回
//已取消的计时器数
  size_t cancelAll() {
//将所有“结束”设置为0（立即执行）是正常的，
//因为它保持了堆的完整性
    std::unique_lock<std::mutex> lk(m_mtx);
    m_cancel = true;
    for (auto&& item : m_items.getContainer()) {
      if (item.id && item.handler) {
        item.end = Clock::time_point();
        item.id = 0;
      }
    }
    auto ret = m_items.size();

    m_checkWork.notify_one();
    return ret;
  }

 private:
  using Clock = std::chrono::steady_clock;
  TimerQueue(const TimerQueue&) = delete;
  TimerQueue& operator=(const TimerQueue&) = delete;

  void run() {
    std::unique_lock<std::mutex> lk(m_mtx);
    while (!m_finish) {
      auto end = calcWaitTime_lock();
      if (end.first) {
//计时器已找到，请等待它过期（或其他
//变化）
        m_checkWork.wait_until(lk, end.second);
      } else {
//不存在计时器，所以永远等待，直到发生变化。
        m_checkWork.wait(lk);
      }

//检查并执行尽可能多的工作，例如所有过期的工作
//计时器
      checkWork(&lk);
    }

//如果我们要关闭，我们不应该留下任何东西，
//因为关机会取消所有项目
    assert(m_items.size() == 0);
  }

  std::pair<bool, Clock::time_point> calcWaitTime_lock() {
    while (m_items.size()) {
      if (m_items.top().handler) {
//存在项，因此返回新的等待时间
        return std::make_pair(true, m_items.top().end);
      } else {
//丢弃空处理程序（它们已被取消）
        m_items.pop();
      }
    }

//找不到项目，因此返回无等待时间（导致线程等待
//无限期地）
    return std::make_pair(false, Clock::time_point());
  }

  void checkWork(std::unique_lock<std::mutex>* lk) {
    while (m_items.size() && m_items.top().end <= Clock::now()) {
      WorkItem item(m_items.top());
      m_items.pop();

      if (item.handler) {
        (*lk).unlock();
        auto reschedule_pair = item.handler(item.id == 0);
        (*lk).lock();
        if (!m_cancel && reschedule_pair.first) {
          int64_t new_period = (reschedule_pair.second == -1)
                                   ? item.period
                                   : reschedule_pair.second;

          item.period = new_period;
          item.end = Clock::now() + std::chrono::milliseconds(new_period);
          m_items.push(std::move(item));
        }
      }
    }
  }

  bool m_finish = false;
  bool m_cancel = false;
  uint64_t m_idcounter = 0;
  std::condition_variable m_checkWork;

  struct WorkItem {
    Clock::time_point end;
    int64_t period;
uint64_t id;  //id==0表示已取消
    std::function<std::pair<bool, int64_t>(bool)> handler;
    bool operator>(const WorkItem& other) const { return end > other.end; }
  };

  std::mutex m_mtx;
//从优先级队列继承，这样我们可以访问内部容器
  class Queue : public std::priority_queue<WorkItem, std::vector<WorkItem>,
                                           std::greater<WorkItem>> {
   public:
    std::vector<WorkItem>& getContainer() { return this->c; }
  } m_items;
  rocksdb::port::Thread m_th;
};
