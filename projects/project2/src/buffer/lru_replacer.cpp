//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer.cpp
//
// Identification: src/buffer/lru_replacer.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_replacer.h"

namespace bustub {

LRUReplacer::LRUReplacer(size_t num_pages) { num_pages_ = num_pages; }

LRUReplacer::~LRUReplacer() = default;

bool LRUReplacer::Victim(frame_id_t *frame_id) {
  // If the LRU buffer is empty, do nothing
  latch_.lock();
  if (replacer_.empty()) {
    latch_.unlock();
    return false;
  }
  *frame_id = replacer_.back();
  replacer_.pop_back();
  lru_map_.erase(*frame_id);
  latch_.unlock();
  return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
  latch_.lock();
  if (lru_map_.count(frame_id) != 0) {
    replacer_.erase(lru_map_[frame_id]);
    lru_map_.erase(frame_id);
  }
  latch_.unlock();
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
  latch_.lock();
  if (this->Size() == num_pages_) {
    frame_id_t tmp;
    Victim(&tmp);
  }
  if (lru_map_.count(frame_id) != 0) {
    latch_.unlock();
    return;
  }
  replacer_.push_front(frame_id);
  lru_map_[frame_id] = replacer_.begin();
  latch_.unlock();
}

size_t LRUReplacer::Size() { return replacer_.size(); }

}  // namespace bustub
