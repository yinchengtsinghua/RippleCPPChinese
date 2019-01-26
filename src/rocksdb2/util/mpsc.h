
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
//此文件的大部分是从下面的公共域代码中借用的。
//来自https://github.com/mstump/queues

//Dmitry Vyukov非侵入式C++实现
//锁定可用未绑定MPSC队列
//http://www.1024cores.net/home/
//无锁算法/队列/非侵入式基于MPSC节点的队列

//来自MSTUMP/队列的许可证
//这是发布到公共领域的免费和无负担的软件。
//
//任何人都可以自由复制、修改、发布、使用、编译、销售或
//以源代码形式或编译的形式分发此软件
//二进制，用于任何目的，商业或非商业，以及由任何
//手段。
//
//在承认版权法的司法管辖区内，作者或作者
//本软件的任何和所有版权权益
//公共领域的软件。我们为利益而奉献
//对公众和我们的继承人不利，以及
//接班人。我们打算把这种奉献作为
//永久放弃目前和将来对此的所有权利
//版权法下的软件。
//
//本软件按“原样”提供，不作任何形式的保证。
//明示或暗示，包括但不限于
//适销性、特定用途的适用性和非侵权性。
//在任何情况下，作者对任何索赔、损害或
//其他责任，无论是在合同诉讼、侵权诉讼或其他诉讼中，
//因软件或使用而产生、产生或与之相关的，或
//软件的其他交易。
//
//有关详细信息，请参阅<http://unlicense.org>

//来自http://www.1024cores.net/home/
//无锁算法/队列/非侵入式基于MPSC节点的队列
//版权所有（c）2010-2011 Dmitry Vyukov。版权所有。
//以源和二进制形式重新分配和使用，带有或
//不作任何修改，但前提是：
//满足条件：
//1。源代码的再分配必须保留上述版权声明，
//此条件列表和以下免责声明。
//
//2。二进制形式的再分配必须复制上述版权声明，
//此条件列表和文档中的以下免责声明
//和/或其他随分发提供的材料。
//
//本软件由Dmitry Vyukov“原样”和任何明示或
//默示保证，包括但不限于默示保证
//不承认适销性和特定用途的适用性。在没有
//事件应由Dmitry Vyukov或贡献者负责任何直接、间接的，
//附带、特殊、惩戒性或后果性损害（包括，
//但不限于替代货物或服务的采购；损失
//使用、数据或利润；或业务中断），无论何种原因
//责任理论，无论是合同责任、严格责任还是侵权责任。
//（包括疏忽或其他）因使用不当而引起的
//即使已告知此类损坏的可能性。
//
//软件和文档中包含的视图和结论
//是作者的，不应解释为代表
//Dmitry Vyukov的官方政策，无论明示或暗示。
//

#ifndef UTIL_MPSC_H_
#define UTIL_MPSC_H_

#include <atomic>
#include <cassert>
#include <type_traits>

/*
 *多生产商单用户无锁Q
 **/

template <typename T>
class mpsc_queue_t {
 public:
  struct buffer_node_t {
    T data;
    std::atomic<buffer_node_t*> next;
  };

  mpsc_queue_t() {
    buffer_node_aligned_t* al_st = new buffer_node_aligned_t;
    buffer_node_t* node = new (al_st) buffer_node_t();
    _head.store(node);
    _tail.store(node);

    node->next.store(nullptr, std::memory_order_relaxed);
  }

  ~mpsc_queue_t() {
    T output;
    while (this->dequeue(&output)) {
    }
    buffer_node_t* front = _head.load(std::memory_order_relaxed);
    front->~buffer_node_t();

    ::operator delete(front);
  }

  void enqueue(const T& input) {
    buffer_node_aligned_t* al_st = new buffer_node_aligned_t;
    buffer_node_t* node = new (al_st) buffer_node_t();

    node->data = input;
    node->next.store(nullptr, std::memory_order_relaxed);

    buffer_node_t* prev_head = _head.exchange(node, std::memory_order_acq_rel);
    prev_head->next.store(node, std::memory_order_release);
  }

  bool dequeue(T* output) {
    buffer_node_t* tail = _tail.load(std::memory_order_relaxed);
    buffer_node_t* next = tail->next.load(std::memory_order_acquire);

    if (next == nullptr) {
      return false;
    }

    *output = next->data;
    _tail.store(next, std::memory_order_release);

    tail->~buffer_node_t();

    ::operator delete(tail);
    return true;
  }

//如果队列是spsc，则只能使用pop_all
  buffer_node_t* pop_all() {
//其他人都不能移动尾部指针。
    buffer_node_t* tptr = _tail.load(std::memory_order_relaxed);
    buffer_node_t* next =
        tptr->next.exchange(nullptr, std::memory_order_acquire);
    _head.exchange(tptr, std::memory_order_acquire);

//这里有比赛条件
    return next;
  }

 private:
  typedef typename std::aligned_storage<
      sizeof(buffer_node_t), std::alignment_of<buffer_node_t>::value>::type
      buffer_node_aligned_t;

  std::atomic<buffer_node_t*> _head;
  std::atomic<buffer_node_t*> _tail;

  mpsc_queue_t(const mpsc_queue_t&) = delete;
  mpsc_queue_t& operator=(const mpsc_queue_t&) = delete;
};

#endif  //应用程序
