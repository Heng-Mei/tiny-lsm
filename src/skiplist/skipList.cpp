#include "../../include/skiplist/skiplist.h"
#include <spdlog/spdlog.h>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace tiny_lsm {

// ************************ SkipListIterator ************************
BaseIterator& SkipListIterator::operator++() {
  // TODO: Lab1.2 任务：实现SkipListIterator的++操作符
  if (current != nullptr) {
    current = current->forward_.front();
  }

  return *this;
}

bool SkipListIterator::operator==(const BaseIterator& other) const {
  // TODO: Lab1.2 任务：实现SkipListIterator的==操作符
  if (get_type() != other.get_type()) {
    return false;
  }

  return current == dynamic_cast<decltype(*this)&>(other).current;
}

bool SkipListIterator::operator!=(const BaseIterator& other) const {
  // TODO: Lab1.2 任务：实现SkipListIterator的!=操作符
  return !(*this == other);
}

SkipListIterator::value_type SkipListIterator::operator*() const {
  // TODO: Lab1.2 任务：实现SkipListIterator的*操作符
  if (current == nullptr) {
    throw std::runtime_error("SkipListIterator is nullptr");
  }

  return {current->key_, current->value_};
}

IteratorType SkipListIterator::get_type() const {
  // TODO: Lab1.2 任务：实现SkipListIterator的get_type
  // ? 主要是为了熟悉基类的定义和继承关系
  return IteratorType::SkipListIterator;
}

bool SkipListIterator::is_valid() const {
  return current && !current->key_.empty();
}
bool SkipListIterator::is_end() const {
  return current == nullptr;
}

std::string SkipListIterator::get_key() const {
  return current->key_;
}
std::string SkipListIterator::get_value() const {
  return current->value_;
}
uint64_t SkipListIterator::get_tranc_id() const {
  return current->tranc_id_;
}

// ************************ SkipList ************************
// 构造函数
SkipList::SkipList(int max_lvl) : max_level(max_lvl), current_level(1) {
  head = std::make_shared<SkipListNode>("", "", max_level, 0);
  dis_01 = std::uniform_int_distribution<>(0, 1);
  dis_level = std::uniform_int_distribution<>(0, (1 << max_lvl) - 1);
  gen = std::mt19937(std::random_device()());
}

int SkipList::random_level() {
  // ? 通过"抛硬币"的方式随机生成层数：
  // ? - 每次有50%的概率增加一层
  // ? - 确保层数分布为：第1层100%，第2层50%，第3层25%，以此类推
  // ? - 层数范围限制在[1, max_level]之间，避免浪费内存
  // TODO: Lab1.1 任务：插入时随机为这一次操作确定其最高连接的链表层数
  return std::min(
      max_level,
      max_level - std::bit_width(static_cast<size_t>(dis_level(gen))) + 1);
}

// 插入或更新键值对
void SkipList::put(const std::string& key,
                   const std::string& value,
                   uint64_t tranc_id) {
  spdlog::trace("SkipList--put({}, {}, {})", key, value, tranc_id);

  // TODO: Lab1.1  任务：实现插入或更新键值对
  // ? Hint: 你需要保证不同`Level`的步长从底层到高层逐渐增加
  // ? 你可能需要使用到`random_level`函数以确定层数, 其注释中为你提供一种思路
  // ? tranc_id 为事务id, 现在你不需要关注它, 直接将其传递到
  // SkipListNode的构造函数中即可

  auto level = random_level();
  auto new_node_ptr =
      std::make_shared<SkipListNode>(key, value, level, tranc_id);

  auto ptr = head;
  for (int i = max_level - 1; i >= 0; i--) {
    while (ptr->forward_[i] != nullptr && ptr->forward_[i]->key_ < key) {
      ptr = ptr->forward_[i];
    }
    if (ptr->forward_[i] != nullptr && ptr->forward_[i]->key_ == key) {
      ptr->forward_[i]->value_ = value;
      return;
    }
    if (i >= level) {
      continue;
    }
    new_node_ptr->set_backward(i, ptr);
    new_node_ptr->forward_[i] = ptr->forward_[i];
  }

  for (size_t i = 0; i < level; i++) {
    auto prev = new_node_ptr->backward_[i].lock();
    assert(prev != nullptr &&
           "SkipList invariant broken: backward_ is nullptr");
    prev->forward_[i] = new_node_ptr;
    if (new_node_ptr->forward_[i] != nullptr) {
      new_node_ptr->forward_[i]->set_backward(i, new_node_ptr);
    }
  }

  size_bytes += strlen(key.c_str()) * sizeof(char) +
                strlen(value.c_str()) * sizeof(char) + sizeof(tranc_id);
}

// 查找键值对
SkipListIterator SkipList::get(const std::string& key, uint64_t tranc_id) {
  // spdlog::trace("SkipList--get({}) called", key);
  // ? 你可以参照上面的注释完成日志输出以便于调试
  // ? 日志为输出到你执行二进制所在目录下的log文件夹

  // TODO: Lab1.1 任务：实现查找键值对,
  // TODO: 并且你后续需要额外实现SkipListIterator中的TODO部分(Lab1.2)

  auto ptr = head;
  for (int i = max_level - 1; i >= 0; i--) {
    while (ptr->forward_[i] != nullptr && ptr->forward_[i]->key_ < key) {
      ptr = ptr->forward_[i];
    }
    if (ptr->forward_[i] != nullptr && ptr->forward_[i]->key_ == key) {
      return SkipListIterator{ptr->forward_[i]};
    }
  }
  return SkipListIterator{};
}

// 删除键值对
// ! 这里的 remove 是跳表本身真实的 remove,  lsm 应该使用 put 空值表示删除,
// ! 这里只是为了实现完整的 SkipList 不会真正被上层调用
void SkipList::remove(const std::string& key) {
  // TODO: Lab1.1 任务：实现删除键值对
  auto ptr = head;
  for (int i = max_level - 1; i >= 0; i--) {
    while (ptr->forward_[i] != nullptr && ptr->forward_[i]->key_ < key) {
      ptr = ptr->forward_[i];
    }
    if (ptr->forward_[i] != nullptr && ptr->forward_[i]->key_ == key) {
      ptr = ptr->forward_[i];
      break;
    }
  }
  if (ptr->key_ != key) {
    return;
  }
  for (size_t i = 0; i < ptr->forward_.size(); i++) {
    auto prev = ptr->backward_[i].lock();
    assert(prev != nullptr &&
           "SkipList invariant broken: backward_ is nullptr");
    prev->forward_[i] = ptr->forward_[i];
    if (ptr->forward_[i] != nullptr) {
      ptr->forward_[i]->set_backward(i, prev);
    }
  }

  size_bytes -= strlen(ptr->key_.c_str()) * sizeof(char) +
                strlen(ptr->value_.c_str()) * sizeof(char) +
                sizeof(ptr->tranc_id_);
}

// 刷盘时可以直接遍历最底层链表
std::vector<std::tuple<std::string, std::string, uint64_t>> SkipList::flush() {
  // std::shared_lock<std::shared_mutex> slock(rw_mutex);
  spdlog::debug("SkipList--flush(): Starting to flush skiplist data");

  std::vector<std::tuple<std::string, std::string, uint64_t>> data;
  auto node = head->forward_[0];
  while (node) {
    data.emplace_back(node->key_, node->value_, node->tranc_id_);
    node = node->forward_[0];
  }

  spdlog::debug("SkipList--flush(): Flushed {} entries", data.size());

  return data;
}

size_t SkipList::get_size() {
  // std::shared_lock<std::shared_mutex> slock(rw_mutex);
  return size_bytes;
}

// 清空跳表，释放内存
void SkipList::clear() {
  // std::unique_lock<std::shared_mutex> lock(rw_mutex);
  head = std::make_shared<SkipListNode>("", "", max_level, 0);
  size_bytes = 0;
}

SkipListIterator SkipList::begin() {
  // return SkipListIterator(head->forward[0], rw_mutex);
  return SkipListIterator(head->forward_[0]);
}

SkipListIterator SkipList::end() {
  return SkipListIterator();  // 使用空构造函数
}

// 找到前缀的起始位置
// 返回第一个前缀匹配或者大于前缀的迭代器
SkipListIterator SkipList::begin_prefix(const std::string& prefix) {
  // TODO: Lab1.3 任务：实现前缀查询的起始位置
  auto it_opt = lower_bound(prefix);
  return it_opt ? *it_opt : SkipListIterator{nullptr};
}

// 找到前缀的终结位置
SkipListIterator SkipList::end_prefix(const std::string& prefix) {
  // TODO: Lab1.3 任务：实现前缀查询的终结位置
  std::string next_prefix = prefix;
  if (!next_prefix.empty()) {
    next_prefix.back()++;
  } else {
    next_prefix = "\xff";
  }

  auto it_opt = lower_bound(next_prefix);
  return it_opt ? *it_opt : SkipListIterator{nullptr};
}

// ? 打印跳表, 你可以在出错时调用此函数进行调试
void SkipList::print_skiplist() {
  for (int level = 0; level < current_level; level++) {
    std::cout << "Level " << level << ": ";
    auto current = head->forward_[level];
    while (current) {
      std::cout << current->key_;
      current = current->forward_[level];
      if (current) {
        std::cout << " -> ";
      }
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}
}  // namespace tiny_lsm