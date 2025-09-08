#include "../../include/iterator/iterator.h"
#include <algorithm>
#include <tuple>
#include <vector>

namespace tiny_lsm {
// *************************** HeapIterator ***************************
HeapIterator::HeapIterator(std::vector<SearchItem> item_vec,
                           uint64_t max_tranc_id)
    : max_tranc_id_(max_tranc_id) {
  // TODO: Lab2.2 实现 HeapIterator 构造函数
  std::ranges::for_each(item_vec,
                        [&](const SearchItem& item) { items.emplace(item); });

  while (!top_value_legal() && is_valid()) {
    update_current();
    items.pop();
  }

  update_current();
}

auto HeapIterator::operator->() const -> HeapIterator::pointer {
  // TODO: Lab2.2 实现 -> 重载
  return current.get();
}

HeapIterator::value_type HeapIterator::operator*() const {
  // TODO: Lab2.2 实现 * 重载
  return *current;
}

BaseIterator& HeapIterator::operator++() {
  // TODO: Lab2.2 实现 ++ i 重载

  while (!top_value_legal() && is_valid()) {
    update_current();
    items.pop();
  }

  update_current();

  if (is_end()) {
    return *this;
  }

  items.pop();

  return *this;
}

bool HeapIterator::operator==(const BaseIterator& other) const {
  // TODO: Lab2.2 实现 == 重载
  if (this->get_type() != other.get_type()) {
    return false;
  }

  auto& other_ = dynamic_cast<const HeapIterator&>(other);

  if (this->is_end() != other_.is_end()) {
    return false;
  }
  if (this->is_end()) {
    return this->current == other_.current;
  }

  return this->items.top() == other_.items.top();
}

bool HeapIterator::operator!=(const BaseIterator& other) const {
  // TODO: Lab2.2 实现 != 重载
  return !(*this == other);
}

bool HeapIterator::top_value_legal() const {
  // TODO: Lab2.2 判断顶部元素是否合法
  // ? 被删除的值是不合法
  // ? 不允许访问的事务创建或更改的键值对不合法(暂时忽略)
  if (items.empty()) {
    return false;
  }
  if ((current != nullptr && items.top().key_ == current->first) ||
      items.top().value_ == "") {
    return false;
  }
  return true;
}

void HeapIterator::skip_by_tranc_id() {
  // TODO: Lab2.2 后续的Lab实现, 只是作为标记提醒
}

bool HeapIterator::is_end() const {
  return items.empty();
}
bool HeapIterator::is_valid() const {
  return !items.empty();
}

void HeapIterator::update_current() const {
  // current 缓存了当前键值对的值, 你实现 -> 重载时可能需要
  // TODO: Lab2.2 更新当前缓存值
  if (is_end()) {
    this->current = nullptr;
    return;
  }
  this->current =
      std::make_shared<value_type>(items.top().key_, items.top().value_);
}

IteratorType HeapIterator::get_type() const {
  return IteratorType::HeapIterator;
}

uint64_t HeapIterator::get_tranc_id() const {
  return max_tranc_id_;
}
}  // namespace tiny_lsm