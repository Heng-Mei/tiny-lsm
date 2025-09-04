#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include "../iterator/iterator.h"
#include "../skiplist/skiplist.h"

namespace tiny_lsm {

class BlockCache;
class SST;
class SSTBuilder;
class TranContext;

class MemTable {
  friend class TranContext;
  friend class HeapIterator;

 private:
  void put_(const std::string& key,
            const std::string& value,
            uint64_t tranc_id);

  auto get_(const std::string& key, uint64_t tranc_id) -> SkipListIterator;

  auto cur_get_(const std::string& key, uint64_t tranc_id) -> SkipListIterator;

  auto frozen_get_(const std::string& key,
                   uint64_t tranc_id) -> SkipListIterator;

  void remove_(const std::string& key, uint64_t tranc_id);
  void frozen_cur_table_();  // _ 表示不需要锁的版本

 public:
  MemTable();
  ~MemTable();

  void put(const std::string& key, const std::string& value, uint64_t tranc_id);
  void put_batch(const std::vector<std::pair<std::string, std::string>>& kvs,
                 uint64_t tranc_id);

  auto get(const std::string& key, uint64_t tranc_id) -> SkipListIterator;
  auto get_batch(const std::vector<std::string>& keys, uint64_t tranc_id)
      -> std::vector<
          std::pair<std::string,
                    std::optional<std::pair<std::string, uint64_t>>>>;
  void remove(const std::string& key, uint64_t tranc_id);
  void remove_batch(const std::vector<std::string>& keys, uint64_t tranc_id);

  void clear();
  auto flush_last(SSTBuilder& builder,
                  std::string& sst_path,
                  size_t sst_id,
                  std::shared_ptr<BlockCache> block_cache)
      -> std::shared_ptr<SST>;
      
  void frozen_cur_table();
  size_t get_cur_size();
  size_t get_frozen_size();
  size_t get_total_size();
  auto begin(uint64_t tranc_id) -> HeapIterator;
  auto iters_preffix(const std::string& preffix,
                     uint64_t tranc_id) -> HeapIterator;

  auto iters_monotony_predicate(
      uint64_t tranc_id,
      std::function<int(const std::string&)> predicate)
      -> std::optional<std::pair<HeapIterator, HeapIterator>>;

  auto end() -> HeapIterator;

 private:
  std::shared_ptr<SkipList> current_table;
  std::list<std::shared_ptr<SkipList>> frozen_tables;
  size_t frozen_bytes;
  std::shared_mutex frozen_mtx;  // 冻结表的锁
  std::shared_mutex cur_mtx;     // 活跃表的锁
};
}  // namespace tiny_lsm