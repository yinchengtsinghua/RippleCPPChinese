
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
#include "memtable/inlineskiplist.h"
#include "db/memtable.h"
#include "rocksdb/memtablerep.h"
#include "util/arena.h"

namespace rocksdb {
namespace {
class SkipListRep : public MemTableRep {
  InlineSkipList<const MemTableRep::KeyComparator&> skip_list_;
  const MemTableRep::KeyComparator& cmp_;
  const SliceTransform* transform_;
  const size_t lookahead_;

  friend class LookaheadIterator;
public:
 explicit SkipListRep(const MemTableRep::KeyComparator& compare,
                      Allocator* allocator, const SliceTransform* transform,
                      const size_t lookahead)
     : MemTableRep(allocator),
       skip_list_(compare, allocator),
       cmp_(compare),
       transform_(transform),
       lookahead_(lookahead) {}

 virtual KeyHandle Allocate(const size_t len, char** buf) override {
   *buf = skip_list_.AllocateKey(len);
   return static_cast<KeyHandle>(*buf);
  }

//在列表中插入键。
//要求：列表中当前没有与键比较的内容。
  virtual void Insert(KeyHandle handle) override {
    skip_list_.Insert(static_cast<char*>(handle));
  }

  virtual void InsertWithHint(KeyHandle handle, void** hint) override {
    skip_list_.InsertWithHint(static_cast<char*>(handle), hint);
  }

  virtual void InsertConcurrently(KeyHandle handle) override {
    skip_list_.InsertConcurrently(static_cast<char*>(handle));
  }

//返回true iff列表中比较等于key的条目。
  virtual bool Contains(const char* key) const override {
    return skip_list_.Contains(key);
  }

  virtual size_t ApproximateMemoryUsage() override {
//所有内存都是通过分配器分配的；这里没有要报告的内容
    return 0;
  }

  virtual void Get(const LookupKey& k, void* callback_args,
                   bool (*callback_func)(void* arg,
                                         const char* entry)) override {
    SkipListRep::Iterator iter(&skip_list_);
    Slice dummy_slice;
    for (iter.Seek(dummy_slice, k.memtable_key().data());
         iter.Valid() && callback_func(callback_args, iter.key());
         iter.Next()) {
    }
  }

  uint64_t ApproximateNumEntries(const Slice& start_ikey,
                                 const Slice& end_ikey) override {
    std::string tmp;
    uint64_t start_count =
        skip_list_.EstimateCount(EncodeKey(&tmp, start_ikey));
    uint64_t end_count = skip_list_.EstimateCount(EncodeKey(&tmp, end_ikey));
    return (end_count >= start_count) ? (end_count - start_count) : 0;
  }

  virtual ~SkipListRep() override { }

//跳过列表内容的迭代
  class Iterator : public MemTableRep::Iterator {
    InlineSkipList<const MemTableRep::KeyComparator&>::Iterator iter_;

   public:
//在指定列表上初始化迭代器。
//返回的迭代器无效。
    explicit Iterator(
        const InlineSkipList<const MemTableRep::KeyComparator&>* list)
        : iter_(list) {}

    virtual ~Iterator() override { }

//如果迭代器位于有效节点，则返回true。
    virtual bool Valid() const override {
      return iter_.Valid();
    }

//返回当前位置的键。
//要求：有效（）
    virtual const char* key() const override {
      return iter_.key();
    }

//前进到下一个位置。
//要求：有效（）
    virtual void Next() override {
      iter_.Next();
    }

//前进到上一个位置。
//要求：有效（）
    virtual void Prev() override {
      iter_.Prev();
    }

//使用大于等于target的键前进到第一个条目
    virtual void Seek(const Slice& user_key, const char* memtable_key)
        override {
      if (memtable_key != nullptr) {
        iter_.Seek(memtable_key);
      } else {
        iter_.Seek(EncodeKey(&tmp_, user_key));
      }
    }

//使用键<=target返回到最后一个条目
    virtual void SeekForPrev(const Slice& user_key,
                             const char* memtable_key) override {
      if (memtable_key != nullptr) {
        iter_.SeekForPrev(memtable_key);
      } else {
        iter_.SeekForPrev(EncodeKey(&tmp_, user_key));
      }
    }

//在列表中的第一个条目处定位。
//迭代器的最终状态为valid（）iff list不为空。
    virtual void SeekToFirst() override {
      iter_.SeekToFirst();
    }

//在列表中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff list不为空。
    virtual void SeekToLast() override {
      iter_.SeekToLast();
    }
   protected:
std::string tmp_;       //用于传递到encodekey
  };

//迭代器遍历跳过列表的内容，该列表还跟踪
//以前访问过的节点。在seek（）中，它检查后面的几个节点
//首先，只有当
//找不到目标密钥。
  class LookaheadIterator : public MemTableRep::Iterator {
   public:
    explicit LookaheadIterator(const SkipListRep& rep) :
        rep_(rep), iter_(&rep_.skip_list_), prev_(iter_) {}

    virtual ~LookaheadIterator() override {}

    virtual bool Valid() const override {
      return iter_.Valid();
    }

    virtual const char *key() const override {
      assert(Valid());
      return iter_.key();
    }

    virtual void Next() override {
      assert(Valid());

      bool advance_prev = true;
      if (prev_.Valid()) {
        auto k1 = rep_.UserKey(prev_.key());
        auto k2 = rep_.UserKey(iter_.key());

        if (k1.compare(k2) == 0) {
//相同的用户密钥，不要移动上一个
          advance_prev = false;
        } else if (rep_.transform_) {
//只有当prev与iter前缀相同时才进行prev_u
          auto t1 = rep_.transform_->Transform(k1);
          auto t2 = rep_.transform_->Transform(k2);
          advance_prev = t1.compare(t2) == 0;
        }
      }

      if (advance_prev) {
        prev_ = iter_;
      }
      iter_.Next();
    }

    virtual void Prev() override {
      assert(Valid());
      iter_.Prev();
      prev_ = iter_;
    }

    virtual void Seek(const Slice& internal_key, const char *memtable_key)
        override {
      const char *encoded_key =
        (memtable_key != nullptr) ?
            memtable_key : EncodeKey(&tmp_, internal_key);

      if (prev_.Valid() && rep_.cmp_(encoded_key, prev_.key()) >= 0) {
//prev.key（）小于或等于我们的目标键；请快速
//从上一步开始的线性搜索（最多向前看一步）
        iter_ = prev_;

        size_t cur = 0;
        while (cur++ <= rep_.lookahead_ && iter_.Valid()) {
          if (rep_.cmp_(encoded_key, iter_.key()) <= 0) {
            return;
          }
          Next();
        }
      }

      iter_.Seek(encoded_key);
      prev_ = iter_;
    }

    virtual void SeekForPrev(const Slice& internal_key,
                             const char* memtable_key) override {
      const char* encoded_key = (memtable_key != nullptr)
                                    ? memtable_key
                                    : EncodeKey(&tmp_, internal_key);
      iter_.SeekForPrev(encoded_key);
      prev_ = iter_;
    }

    virtual void SeekToFirst() override {
      iter_.SeekToFirst();
      prev_ = iter_;
    }

    virtual void SeekToLast() override {
      iter_.SeekToLast();
      prev_ = iter_;
    }

   protected:
std::string tmp_;       //用于传递到encodekey

   private:
    const SkipListRep& rep_;
    InlineSkipList<const MemTableRep::KeyComparator&>::Iterator iter_;
    InlineSkipList<const MemTableRep::KeyComparator&>::Iterator prev_;
  };

  virtual MemTableRep::Iterator* GetIterator(Arena* arena = nullptr) override {
    if (lookahead_ > 0) {
      void *mem =
        arena ? arena->AllocateAligned(sizeof(SkipListRep::LookaheadIterator))
              : operator new(sizeof(SkipListRep::LookaheadIterator));
      return new (mem) SkipListRep::LookaheadIterator(*this);
    } else {
      void *mem =
        arena ? arena->AllocateAligned(sizeof(SkipListRep::Iterator))
              : operator new(sizeof(SkipListRep::Iterator));
      return new (mem) SkipListRep::Iterator(&skip_list_);
    }
  }
};
}

MemTableRep* SkipListFactory::CreateMemTableRep(
    const MemTableRep::KeyComparator& compare, Allocator* allocator,
    const SliceTransform* transform, Logger* logger) {
  return new SkipListRep(compare, allocator, transform, lookahead_);
}

} //命名空间rocksdb
