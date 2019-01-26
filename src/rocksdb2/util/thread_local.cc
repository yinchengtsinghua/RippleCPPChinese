
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include "util/thread_local.h"
#include "util/mutexlock.h"
#include "port/likely.h"
#include <stdlib.h>

namespace rocksdb {

struct Entry {
  Entry() : ptr(nullptr) {}
  Entry(const Entry& e) : ptr(e.ptr.load(std::memory_order_relaxed)) {}
  std::atomic<void*> ptr;
};

class StaticMeta;

//这是声明为“线程本地”存储的结构。
//矢量保留“当前”的所有实例的原子指针列表
//线程。矢量由进程中唯一的ID索引，并且
//与一个ThreadLocalPtr实例关联。ID由
//全局StaticMeta单例。所以如果我们实例化了3个threadlocalptr
//实例中，每个线程都将有一个向量大小为3的threaddata：
//———————————————————————————————————————
//实例1实例2_instnace 3_
//———————————————————————————————————————
//螺纹1_void*void*void*<-threaddata
//———————————————————————————————————————
//螺纹2_void*void*void*<-threaddata
//———————————————————————————————————————
//螺纹3_void*void*void*<-threaddata
//———————————————————————————————————————
struct ThreadData {
  explicit ThreadData(ThreadLocalPtr::StaticMeta* _inst) : entries(), inst(_inst) {}
  std::vector<Entry> entries;
  ThreadData* next;
  ThreadData* prev;
  ThreadLocalPtr::StaticMeta* inst;
};

class ThreadLocalPtr::StaticMeta {
public:
  StaticMeta();

//返回下一个可用ID
  uint32_t GetId();
//返回下一个可用的ID而不声明它
  uint32_t PeekId() const;
//将给定的ID返回到空闲池。这也会触发
//所有线程的关联指针值（如果不是空值）的UnrefHandler。
  void ReclaimId(uint32_t id);

//返回当前线程的给定ID的指针值。
  void* Get(uint32_t id) const;
//为当前线程的给定ID重置指针值。
  void Reset(uint32_t id, void* ptr);
//自动交换提供的ptr并返回上一个值
  void* Swap(uint32_t id, void* ptr);
//仅当提供的值等于时才进行原子比较和交换
//到预期值。
  bool CompareAndSwap(uint32_t id, void* ptr, void*& expected);
//将所有线程本地数据重置为替换，并返回非nullptr
//所有现有线程的数据
  void Scrape(uint32_t id, autovector<void*>* ptrs, void* const replacement);
//通过对每个线程的本地值应用func来更新res。拿着一把锁
//阻止UNREF处理程序在此调用期间运行，但客户端必须
//仍然提供外部同步，因为拥有的线程可以
//在不进行内部锁定的情况下访问值，例如，通过get（）和reset（）。
  void Fold(uint32_t id, FoldFunc func, void* res);

//为ID注册Unrefhandler
  void SetHandler(uint32_t id, UnrefHandler handler);

//保护inst，下一个实例，自由实例，头，
//threaddata.entries条
//
//注意，这里我们更喜欢函数静态变量而不是通常的
//全局静态变量。原因是C++销毁顺序
//静态变量的构造顺序与静态变量的构造顺序相反。
//但是，C++在全局时不能保证任何构造顺序。
//静态变量在不同的文件中定义，而函数
//静态变量在首次调用函数时初始化。
//因此，函数静态变量的构造顺序
//可以通过在中正确调用其第一个函数调用来控制
//正确的顺序。
//
//例如，以下函数包含一个静态函数
//变量。我们在里面放置了一个虚拟的函数调用
//env:：default（）以确保构造的构造顺序
//秩序。
  static port::Mutex* Mutex();

//返回当前staticmeta的成员互斥体。一般来说，
//应该使用mutex（）而不是这个。但是，如果
//instance（）内的静态变量超出范围，membermutex（）。
//应该使用。一个例子是OnThreadExit（）函数。
  port::Mutex* MemberMutex() { return &mutex_; }

private:
//通过获取mutex获取id的unrefhandler
//要求：互斥锁
  UnrefHandler GetHandler(uint32_t id);

//线程终止前触发
  static void OnThreadExit(void* ptr);

//将当前线程的threaddata添加到全局链
//要求：互斥锁
  void AddThreadData(ThreadData* d);

//从全局链中删除当前线程的threaddata
//要求：互斥锁
  void RemoveThreadData(ThreadData* d);

  static ThreadData* GetThreadLocal();

  uint32_t next_instance_id_;
//用于在threadlocalptr被实例化和销毁时回收ID
//经常。这也可以防止它炸毁向量空间。
  autovector<uint32_t> free_instance_ids_;
//将所有螺纹局部结构连接在一起。这是必要的，因为
//当一个threadlocalptr被破坏时，我们需要循环
//与该实例对应的线程指针版本，以及
//为此，请致电Unrefhandler。
  ThreadData head_;

  std::unordered_map<uint32_t, UnrefHandler> handler_map_;

//专用互斥。开发人员应该始终使用mutex（），而不是
//直接使用这个变量。
  port::Mutex mutex_;
#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL
//线程本地存储
  static __thread ThreadData* tls_;
#endif

//用于使线程退出触发器在以下情况下成为可能！已定义（OS_Macosx）。
//否则，用于检索线程数据。
  pthread_key_t pthread_key_;
};


#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL
__thread ThreadData* ThreadLocalPtr::StaticMeta::tls_ = nullptr;
#endif

//Windows不支持每个线程的析构函数
//TLS原语。因此，我们通过插入
//在每个线程的出口上调用的函数。
//请参阅http://www.codeproject.com/articles/8113/thread-local-storage-the-c-way
//和http://www.nynaeve.net/？P＝183
//
//真的吗，我们这样做是为了有一个明确的良心，因为在线程池中使用TLS
//是不是
//尽管在请求中可以。否则，线程在其
//现代用途。

//这只在从系统加载器调用的Windows上运行
#ifdef OS_WIN

//从具有不同
//签名，这样我们就不能直接连接原来的OnThreadExit，即
//私人成员
//所以我们让staticmeta类和我们共享函数的地址，所以
//我们可以调用它。
namespace wintlscleanup {

//在StaticMeta Singleton构造函数中将此设置为OnThreadExit
UnrefHandler thread_local_inclass_routine = nullptr;
pthread_key_t thread_local_key = -1;

//用于在每个线程终止时调用的静态回调函数。
void NTAPI WinOnThreadExit(PVOID module, DWORD reason, PVOID reserved) {
//我们决定把钱花在退出进程上。
  if (DLL_THREAD_DETACH == reason) {
    if (thread_local_key != pthread_key_t(-1) && thread_local_inclass_routine != nullptr) {
      void* tls = pthread_getspecific(thread_local_key);
      if (tls != nullptr) {
        thread_local_inclass_routine(tls);
      }
    }
  }
}

}  //温特斯利兰普

//外部“C”禁止C++名称的修改，因此我们知道
//链接器/包含：上面的符号pragma。
extern "C" {

#ifdef _MSC_VER
//链接器不能在退出时丢弃线程回调。（我们强制引用
//使用linker/include:symbol pragma确保。）如果
//此变量被丢弃，将永远不会调用OnThreadExit函数。
#ifdef _WIN64

//.crt节与x64上的.rdata合并，因此它必须是常量数据。
#pragma const_seg(".CRT$XLB")
//定义常量变量时，它必须具有外部链接，以确保
//链接器不会丢弃它。
extern const PIMAGE_TLS_CALLBACK p_thread_callback_on_exit;
const PIMAGE_TLS_CALLBACK p_thread_callback_on_exit =
    wintlscleanup::WinOnThreadExit;
//重置默认节。
#pragma const_seg()

#pragma comment(linker, "/include:_tls_used")
#pragma comment(linker, "/include:p_thread_callback_on_exit")

#else  //Win 64

#pragma data_seg(".CRT$XLB")
PIMAGE_TLS_CALLBACK p_thread_callback_on_exit = wintlscleanup::WinOnThreadExit;
//重置默认节。
#pragma data_seg()

#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_p_thread_callback_on_exit")

#endif  //Win 64

#else
//https://github.com/couchbase/gperftools/blob/master/src/windows/port.cc
BOOL WINAPI DllMain(HINSTANCE h, DWORD dwReason, PVOID pv) {
  if (dwReason == DLL_THREAD_DETACH)
    wintlscleanup::WinOnThreadExit(h, dwReason, pv);
  return TRUE;
}
#endif
}  //外部“C”

#endif  //奥斯温

void ThreadLocalPtr::InitSingletons() { ThreadLocalPtr::Instance(); }

ThreadLocalPtr::StaticMeta* ThreadLocalPtr::Instance() {
//这里我们更喜欢函数静态变量而不是全局变量
//静态变量作为函数初始化静态变量
//当函数是第一个调用时。因此，我们可以
//通过适当的准备来控制他们的施工顺序
//第一个函数调用。
//
//注意，这里我们决定使“inst”成为一个不删除的静态指针。
//它位于末尾，而不是静态变量。这是为了避免以下情况
//当使用threadlocalptr的子线程发生破坏顺序灾难
//在主线程死后死亡：当子线程碰巧使用时
//threadlocalptr，它将尝试删除其线程上的本地数据
//当子线程死亡时，OnThreadExit。但是，OnThreadExit取决于
//在以下变量上。因此，如果主螺纹在
//子线程碰巧使用了threadlocalptr，然后销毁
//以下变量将先转到readexit，然后转到readexit，因此导致
//无效访问。
//
//以上问题可以通过使用线程本地存储tls来解决。
//使用\线。螺纹“局部”和“螺纹”的主要区别
//是线程本地支持动态构造和破坏
//非基元类型变量。因此，我们可以保证
//销毁顺序，即使主线程在任何子线程之前死亡。
//但是，在接受STD＝C++ 11的编译器中，TyRekLoad并不支持。
//（例如，xcode<8的mac。xcode 8+支持线程_本地）。
  static ThreadLocalPtr::StaticMeta* inst = new ThreadLocalPtr::StaticMeta();
  return inst;
}

port::Mutex* ThreadLocalPtr::StaticMeta::Mutex() { return &Instance()->mutex_; }

void ThreadLocalPtr::StaticMeta::OnThreadExit(void* ptr) {
  auto* tls = static_cast<ThreadData*>(ptr);
  assert(tls != nullptr);

//使用缓存的StaticMeta:：Instance（）而不是直接调用
//StaticMeta:：Instance（）中的变量可能已从
//这里的作用域，以防在主线程之后调用此OnThreadExit
//模具。
  auto* inst = tls->inst;
  pthread_setspecific(inst->pthread_key_, nullptr);

  MutexLock l(inst->MemberMutex());
  inst->RemoveThreadData(tls);
//从所有实例中释放当前线程的存储指针
  uint32_t id = 0;
  for (auto& e : tls->entries) {
    void* raw = e.ptr.load();
    if (raw != nullptr) {
      auto unref = inst->GetHandler(id);
      if (unref != nullptr) {
        unref(raw);
      }
    }
    ++id;
  }
//删除线程本地结构，无论它是Mac平台
  delete tls;
}

ThreadLocalPtr::StaticMeta::StaticMeta() : next_instance_id_(0), head_(this) {
  if (pthread_key_create(&pthread_key_, &OnThreadExit) != 0) {
    abort();
  }

//没有在主线程上调用OnThreadExit。
//通过静态析构函数机制调用以避免内存泄漏。
//
//警告：~a（）将在全局的staticMeta之后调用
//singleton（析构函数按构造函数的相反顺序调用
//_completion_uu）；后者不得改变内部成员。这个
//清除机制本质上依赖于释放
//StaticMeta，对于编译器特定的处理是脆弱的
//内存备份的静态作用域对象被破坏。也许
//在ATEXIT（3）注册将更加强大。
//
//这在Windows上不是必需的。
#if !defined(OS_WIN)
  static struct A {
    ~A() {
#ifndef ROCKSDB_SUPPORT_THREAD_LOCAL
      ThreadData* tls_ =
        static_cast<ThreadData*>(pthread_getspecific(Instance()->pthread_key_));
#endif
      if (tls_) {
        OnThreadExit(tls_);
      }
    }
  } a;
#endif  //！定义（OsWin）

  head_.next = &head_;
  head_.prev = &head_;

#ifdef OS_WIN
//与Windows共享其清理例程和密钥
  wintlscleanup::thread_local_inclass_routine = OnThreadExit;
  wintlscleanup::thread_local_key = pthread_key_;
#endif
}

void ThreadLocalPtr::StaticMeta::AddThreadData(ThreadData* d) {
  Mutex()->AssertHeld();
  d->next = &head_;
  d->prev = head_.prev;
  head_.prev->next = d;
  head_.prev = d;
}

void ThreadLocalPtr::StaticMeta::RemoveThreadData(
    ThreadData* d) {
  Mutex()->AssertHeld();
  d->next->prev = d->prev;
  d->prev->next = d->next;
  d->next = d->prev = d;
}

ThreadData* ThreadLocalPtr::StaticMeta::GetThreadLocal() {
#ifndef ROCKSDB_SUPPORT_THREAD_LOCAL
//使此局部变量名看起来像成员变量，以便
//可以共享以下所有代码
  ThreadData* tls_ =
      static_cast<ThreadData*>(pthread_getspecific(Instance()->pthread_key_));
#endif

  if (UNLIKELY(tls_ == nullptr)) {
    auto* inst = Instance();
    tls_ = new ThreadData(inst);
    {
//在全局链中注册它，需要在线程退出之前完成
//处理程序注册
      MutexLock l(Mutex());
      inst->AddThreadData(tls_);
    }
//即使它不是操作系统Macosx，也需要为pthread键注册值，以便
//它的退出处理程序将被触发。
    if (pthread_setspecific(inst->pthread_key_, tls_) != 0) {
      {
        MutexLock l(Mutex());
        inst->RemoveThreadData(tls_);
      }
      delete tls_;
      abort();
    }
  }
  return tls_;
}

void* ThreadLocalPtr::StaticMeta::Get(uint32_t id) const {
  auto* tls = GetThreadLocal();
  if (UNLIKELY(id >= tls->entries.size())) {
    return nullptr;
  }
  return tls->entries[id].ptr.load(std::memory_order_acquire);
}

void ThreadLocalPtr::StaticMeta::Reset(uint32_t id, void* ptr) {
  auto* tls = GetThreadLocal();
  if (UNLIKELY(id >= tls->entries.size())) {
//需要互斥体来保护回收ID中的条目访问
    MutexLock l(Mutex());
    tls->entries.resize(id + 1);
  }
  tls->entries[id].ptr.store(ptr, std::memory_order_release);
}

void* ThreadLocalPtr::StaticMeta::Swap(uint32_t id, void* ptr) {
  auto* tls = GetThreadLocal();
  if (UNLIKELY(id >= tls->entries.size())) {
//需要互斥体来保护回收ID中的条目访问
    MutexLock l(Mutex());
    tls->entries.resize(id + 1);
  }
  return tls->entries[id].ptr.exchange(ptr, std::memory_order_acquire);
}

bool ThreadLocalPtr::StaticMeta::CompareAndSwap(uint32_t id, void* ptr,
    void*& expected) {
  auto* tls = GetThreadLocal();
  if (UNLIKELY(id >= tls->entries.size())) {
//需要互斥体来保护回收ID中的条目访问
    MutexLock l(Mutex());
    tls->entries.resize(id + 1);
  }
  return tls->entries[id].ptr.compare_exchange_strong(
      expected, ptr, std::memory_order_release, std::memory_order_relaxed);
}

void ThreadLocalPtr::StaticMeta::Scrape(uint32_t id, autovector<void*>* ptrs,
    void* const replacement) {
  MutexLock l(Mutex());
  for (ThreadData* t = head_.next; t != &head_; t = t->next) {
    if (id < t->entries.size()) {
      void* ptr =
          t->entries[id].ptr.exchange(replacement, std::memory_order_acquire);
      if (ptr != nullptr) {
        ptrs->push_back(ptr);
      }
    }
  }
}

void ThreadLocalPtr::StaticMeta::Fold(uint32_t id, FoldFunc func, void* res) {
  MutexLock l(Mutex());
  for (ThreadData* t = head_.next; t != &head_; t = t->next) {
    if (id < t->entries.size()) {
      void* ptr = t->entries[id].ptr.load();
      if (ptr != nullptr) {
        func(ptr, res);
      }
    }
  }
}

uint32_t ThreadLocalPtr::TEST_PeekId() {
  return Instance()->PeekId();
}

void ThreadLocalPtr::StaticMeta::SetHandler(uint32_t id, UnrefHandler handler) {
  MutexLock l(Mutex());
  handler_map_[id] = handler;
}

UnrefHandler ThreadLocalPtr::StaticMeta::GetHandler(uint32_t id) {
  Mutex()->AssertHeld();
  auto iter = handler_map_.find(id);
  if (iter == handler_map_.end()) {
    return nullptr;
  }
  return iter->second;
}

uint32_t ThreadLocalPtr::StaticMeta::GetId() {
  MutexLock l(Mutex());
  if (free_instance_ids_.empty()) {
    return next_instance_id_++;
  }

  uint32_t id = free_instance_ids_.back();
  free_instance_ids_.pop_back();
  return id;
}

uint32_t ThreadLocalPtr::StaticMeta::PeekId() const {
  MutexLock l(Mutex());
  if (!free_instance_ids_.empty()) {
    return free_instance_ids_.back();
  }
  return next_instance_id_;
}

void ThreadLocalPtr::StaticMeta::ReclaimId(uint32_t id) {
//未使用此ID，请检查所有线程本地数据并释放
//对应值
  MutexLock l(Mutex());
  auto unref = GetHandler(id);
  for (ThreadData* t = head_.next; t != &head_; t = t->next) {
    if (id < t->entries.size()) {
      void* ptr = t->entries[id].ptr.exchange(nullptr);
      if (ptr != nullptr && unref != nullptr) {
        unref(ptr);
      }
    }
  }
  handler_map_[id] = nullptr;
  free_instance_ids_.push_back(id);
}

ThreadLocalPtr::ThreadLocalPtr(UnrefHandler handler)
    : id_(Instance()->GetId()) {
  if (handler != nullptr) {
    Instance()->SetHandler(id_, handler);
  }
}

ThreadLocalPtr::~ThreadLocalPtr() {
  Instance()->ReclaimId(id_);
}

void* ThreadLocalPtr::Get() const {
  return Instance()->Get(id_);
}

void ThreadLocalPtr::Reset(void* ptr) {
  Instance()->Reset(id_, ptr);
}

void* ThreadLocalPtr::Swap(void* ptr) {
  return Instance()->Swap(id_, ptr);
}

bool ThreadLocalPtr::CompareAndSwap(void* ptr, void*& expected) {
  return Instance()->CompareAndSwap(id_, ptr, expected);
}

void ThreadLocalPtr::Scrape(autovector<void*>* ptrs, void* const replacement) {
  Instance()->Scrape(id_, ptrs, replacement);
}

void ThreadLocalPtr::Fold(FoldFunc func, void* res) {
  Instance()->Fold(id_, func, res);
}

}  //命名空间rocksdb
