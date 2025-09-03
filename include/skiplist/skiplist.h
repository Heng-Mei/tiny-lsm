#pragma once
#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "../iterator/iterator.h"

namespace tiny_lsm {

// ************************ SkipListNode ************************
struct SkipListNode {
  std::string key_;    // 节点存储的键
  std::string value_;  // 节点存储的值
  uint64_t tranc_id_;  // 事务 id
  std::vector<std::shared_ptr<SkipListNode>>
      forward_;  // 指向不同层级的下一个节点的指针数组
  std::vector<std::weak_ptr<SkipListNode>>
      backward_;  // 指向不同层级的下一个节点的指针数组
  SkipListNode(const std::string& k,
               const std::string& v,
               int level,
               uint64_t tranc_id)
      : key_(k),
        value_(v),
        forward_(level, nullptr),
        backward_(level, std::weak_ptr<SkipListNode>()),
        tranc_id_(tranc_id) {}

  bool operator==(const SkipListNode& other) const {
    return key_ == other.key_ && value_ == other.value_ &&
           tranc_id_ == other.tranc_id_;
  }

  bool operator!=(const SkipListNode& other) const { return !(*this == other); }

  bool operator<(const SkipListNode& other) const {
    if (key_ == other.key_) {
      // key 相等时，trans_id 更大的优先级更高
      return tranc_id_ > other.tranc_id_;
    }
    return key_ < other.key_;
  }
  bool operator>(const SkipListNode& other) const {
    if (key_ == other.key_) {
      // key 相等时，trans_id 更大的优先级更高
      return tranc_id_ < other.tranc_id_;
    }
    return key_ > other.key_;
  }

  void set_backward(int level, std::shared_ptr<SkipListNode> node) {
    backward_[level] = std::weak_ptr<SkipListNode>(node);
  }

  size_t get_size() { return 0; };
};

// ************************ SkipListIterator ************************

class SkipListIterator : public BaseIterator {
 public:
  // ! deprecated: 构造函数，接收锁
  // SkipListIterator(std::shared_ptr<SkipListNode> node, std::shared_mutex
  // &mutex)
  //     : current(node),
  //       lock(std::make_shared<std::shared_lock<std::shared_mutex>>(mutex)) {}

  // 构造函数
  SkipListIterator(std::shared_ptr<SkipListNode> node) : current(node) {}

  // 空迭代器构造函数
  SkipListIterator() : current(nullptr), lock(nullptr) {}

  virtual BaseIterator& operator++() override;
  virtual bool operator==(const BaseIterator& other) const override;
  virtual bool operator!=(const BaseIterator& other) const override;
  virtual value_type operator*() const override;
  virtual IteratorType get_type() const override;
  virtual bool is_end() const override;
  virtual bool is_valid() const override;
  std::string get_key() const;
  std::string get_value() const;
  uint64_t get_tranc_id() const override;

 private:
  std::shared_ptr<SkipListNode> current;
  std::shared_ptr<std::shared_lock<std::shared_mutex>>
      lock;  // 持有读锁, 整个迭代器有效期间都持有读锁
};

// ************************ SkipList ************************

class SkipList {
 private:
  std::shared_ptr<SkipListNode>
      head;               // 跳表的头节点，不存储实际数据，用于遍历跳表
  int max_level;          // 跳表的最大层级数，限制跳表的高度
  int current_level;      // 跳表当前的实际层级数，动态变化
  size_t size_bytes = 0;  // 跳表当前占用的内存大小（字节数），用于跟踪内存使用
  // std::shared_mutex rw_mutex; // ! 目前看起来这个锁是冗余的, 在上层控制即可,
  // 后续考虑是否需要细粒度的锁

  std::uniform_int_distribution<> dis_01;
  std::uniform_int_distribution<> dis_level;
  std::mt19937 gen;

 private:
  int random_level();  // 生成新节点的随机层级数

 public:
  SkipList(int max_lvl = 16);  // 构造函数，初始化跳表

  // 析构函数需要确保没有其他线程访问
  ~SkipList() {
    // std::unique_lock<std::shared_mutex> lock(rw_mutex);
    // ... 清理资源
  }

  // 插入或更新键值对
  // 这里不对 tranc_id 进行检查，由上层保证 tranc_id 的合法性
  void put(const std::string& key, const std::string& value, uint64_t tranc_id);

  // 查找键对应的值
  // 事务 id 为0 表示没有开启事务
  // 否则只能查找事务 id 小于等于 tranc_id 的值
  // 返回值: 如果找到，返回 value 和 tranc_id，否则返回空
  SkipListIterator get(const std::string& key, uint64_t tranc_id);

  // !!! 这里的 remove 是跳表本身真实的 remove,  lsm 应该使用 put 空值表示删除
  void remove(const std::string& key);  // 删除键值对

  // 将跳表数据刷出，返回有序键值对列表
  // value 为 真实 value 和 tranc_id 的二元组
  std::vector<std::tuple<std::string, std::string, uint64_t>> flush();

  size_t get_size();

  void clear();  // 清空跳表，释放内存

  SkipListIterator begin();
  SkipListIterator begin_preffix(const std::string& preffix);

  SkipListIterator end();
  SkipListIterator end_preffix(const std::string& preffix);

  // ? 这里单调谓词的含义是, 整个数据库只会有一段连续区间满足此谓词
  // ? 例如之前特化的前缀查询，以及后续可能的范围查询，都可以转化为谓词查询
  // ? 返回第一个满足谓词的位置和最后一个满足谓词的迭代器
  // ? 如果不存在, 范围nullptr
  // ? 谓词作用于key, 且保证满足谓词的结果只在一段连续的区间内,
  // 例如前缀匹配的谓词 ? predicate返回值: ?   0: 满足谓词 ?   >0: 不满足谓词,
  // 需要向右移动 ?   <0: 不满足谓词, 需要向左移动 ! Skiplist
  // 中的谓词查询不会进行事务id的判断, 需要上层自己进行判断
  template <typename F>
    requires std::is_invocable_r_v<int, F, const std::string&>
  auto iters_monotony_predicate(F&& predicate)
      -> std::optional<std::pair<SkipListIterator, SkipListIterator>> {
    auto start = lower_bound(std::string{},
                             [&](const std::string& key, const std::string&) {
                               return predicate(key) < 0;
                             });

    if (!start) {
      return std::nullopt;
    }

    auto end = upper_bound(std::string{},
                           [&](const std::string&, const std::string& key) {
                             return predicate(key) <= 0;
                           });

    return std::make_optional(
        std::make_pair(*start, end ? *end : SkipListIterator{nullptr}));
  }

  template <typename Value,
            std::predicate<const std::string&, const Value&> Compare>
  auto lower_bound(const Value& value, Compare comp)
      -> std::optional<SkipListIterator> {
    auto ptr = head;
    for (int i = max_level - 1; i >= 0; i--) {
      while (ptr->forward_[i] != nullptr &&
             comp(ptr->forward_[i]->value_, value)) {
        ptr = ptr->forward_[i];
      }
    }
    if (ptr->forward_[0] == nullptr) {
      return std::nullopt;
    }
    return std::make_optional(SkipListIterator{ptr->forward_[0]});
  }

  template <typename Value>
  auto lower_bound(const Value& value) -> std::optional<SkipListIterator> {
    return lower_bound(value, std::less<std::string>{});
  }

  template <typename Value,
            std::predicate<const Value&, const std::string&> Compare>
  auto upper_bound(const Value& value, Compare comp)
      -> std::optional<SkipListIterator> {
    auto ptr = head;
    for (int i = max_level - 1; i >= 0; i--) {
      while (ptr->forward_[i] != nullptr &&
             !comp(value, ptr->forward_[i]->value_)) {
        ptr = ptr->forward_[i];
      }
    }
    if (ptr->forward_[0] == nullptr) {
      return std::nullopt;
    }
    return std::make_optional(SkipListIterator{ptr->forward_[0]});
  }

  template <typename Value>
  auto upper_bound(const Value& value) -> std::optional<SkipListIterator> {
    return upper_bound(value, std::less<std::string>{});
  }

  void print_skiplist();
};
}  // namespace tiny_lsm