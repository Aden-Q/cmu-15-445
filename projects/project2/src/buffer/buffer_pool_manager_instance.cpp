//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager_instance.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager_instance.h"

#include "common/macros.h"

namespace bustub {

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, DiskManager *disk_manager,
                                                     LogManager *log_manager)
    : BufferPoolManagerInstance(pool_size, 1, 0, disk_manager, log_manager) {}

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, uint32_t num_instances, uint32_t instance_index,
                                                     DiskManager *disk_manager, LogManager *log_manager)
    : pool_size_(pool_size),
      num_instances_(num_instances),
      instance_index_(instance_index),
      next_page_id_(instance_index),
      disk_manager_(disk_manager),
      log_manager_(log_manager) {
  BUSTUB_ASSERT(num_instances > 0, "If BPI is not part of a pool, then the pool size should just be 1");
  BUSTUB_ASSERT(
      instance_index < num_instances,
      "BPI index cannot be greater than the number of BPIs in the pool. In non-parallel case, index should just be 1.");
  // We allocate a consecutive memory space for the buffer pool.
  pages_ = new Page[pool_size_];
  replacer_ = new LRUReplacer(pool_size);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManagerInstance::~BufferPoolManagerInstance() {
  delete[] pages_;
  delete replacer_;
}

bool BufferPoolManagerInstance::FlushPgImp(page_id_t page_id) {
  // Make sure you call DiskManager::WritePage!
  std::lock_guard<std::mutex> lck(latch_);
  if (page_id == INVALID_PAGE_ID || page_table_.count(page_id) == 0) {
    return false;
  }
  disk_manager_->WritePage(page_id, pages_[page_table_[page_id]].GetData());
  return true;
}

void BufferPoolManagerInstance::FlushAllPgsImp() {
  // You can do it!
  std::lock_guard<std::mutex> lck(latch_);
  int32_t tmp = 0;
  for (tmp = 0; tmp < static_cast<int32_t>(pool_size_); tmp++) {
    disk_manager_->WritePage(pages_[tmp].GetPageId(), pages_[tmp].GetData());
  }
}

Page *BufferPoolManagerInstance::NewPgImp(page_id_t *page_id) {
  // 0.   Make sure you call AllocatePage!
  // 1.   If all the pages in the buffer pool are pinned, return nullptr.
  // 2.   Pick a victim page P from either the free list or the replacer. Always pick from the free list first.
  // 3.   Update P's metadata, zero out memory and add P to the page table.
  // 4.   Set the page ID output parameter. Return a pointer to P.
  // Check whether all the pages in the buffer pool are pinned
  std::lock_guard<std::mutex> lck(latch_);
  int32_t tmp = 0;
  for (tmp = 0; tmp < static_cast<int32_t>(pool_size_); tmp++) {
    if (pages_[tmp].GetPinCount() == 0) {
      break;
    }
  }
  if (tmp >= static_cast<int32_t>(pool_size_)) {
    // In the case that all the pages in the buffer pool are pinned
    return nullptr;
  }
  // In the case that both the replacer and the free list are unavailable
  if (replacer_->Size() == 0 && free_list_.empty()) {
    return nullptr;
  }
  frame_id_t frame_id = -1;
  *page_id = AllocatePage();
  // Pick from free list if nonempty
  if (!free_list_.empty()) {
    frame_id = free_list_.back();
    free_list_.pop_back();
  } else {
    replacer_->Victim(&frame_id);
    // Check whether this page is dirty
    if (pages_[frame_id].IsDirty()) {
      disk_manager_->WritePage(pages_[frame_id].GetPageId(), pages_[frame_id].GetData());
    }
    // Delete the old page from the page table
    page_table_.erase(pages_[frame_id].GetPageId());
  }
  // Update page P's metadata
  // zero out data held within the page
  pages_[frame_id].ResetMemory();
  pages_[frame_id].is_dirty_ = false;
  pages_[frame_id].page_id_ = *page_id;
  pages_[frame_id].pin_count_ = 1;
  replacer_->Pin(frame_id);
  // Add the new entry into the page table
  page_table_[*page_id] = frame_id;
  // Flush to disk
  disk_manager_->WritePage(*page_id, pages_[frame_id].GetData());
  return pages_ + frame_id;
}

Page *BufferPoolManagerInstance::FetchPgImp(page_id_t page_id) {
  // 1.     Search the page table for the requested page (P).
  // 1.1    If P exists, pin it and return it immediately.
  // 1.2    If P does not exist, find a replacement page (R) from either the free list or the replacer.
  //        Note that pages are always found from the free list first.
  // 2.     If R is dirty, write it back to the disk.
  // 3.     Delete R from the page table and insert P.
  // 4.     Update P's metadata, read in the page content from disk, and then return a pointer to P.
  std::lock_guard<std::mutex> lck(latch_);
  frame_id_t frame_id = -1;
  // Search the page table
  if (page_table_.count(page_id) != 0) {
    // P exists, pin it and return the page
    frame_id = page_table_[page_id];
    pages_[frame_id].pin_count_++;
    replacer_->Pin(frame_id);
    return pages_ + frame_id;
  }
  // P does not exists, need page replacement
  // Check whether the free list is empty and all pages are pinned,
  // in which case return nullptr
  if (replacer_->Size() == 0 && free_list_.empty()) {
    return nullptr;
  }
  // First find from the free list
  if (!free_list_.empty()) {
    // The free list is available
    frame_id = free_list_.back();
    free_list_.pop_back();
  } else {
    // The free list is unavailable, search from the LRU replacer
    // edge case has been checked before so we can assumed that
    // the replacer can always find a page
    replacer_->Victim(&frame_id);
    // Check whether this page is dirty
    if (pages_[frame_id].IsDirty()) {
      disk_manager_->WritePage(pages_[frame_id].GetPageId(), pages_[frame_id].GetData());
    }
    // Delete the old page from the page table
    page_table_.erase(pages_[frame_id].GetPageId());
  }
  // Insert the new page
  page_table_[page_id] = frame_id;
  disk_manager_->ReadPage(page_id, pages_[frame_id].GetData());
  pages_[frame_id].is_dirty_ = false;
  pages_[frame_id].page_id_ = page_id;
  pages_[frame_id].pin_count_ = 1;
  replacer_->Pin(frame_id);
  return pages_ + frame_id;
}

bool BufferPoolManagerInstance::DeletePgImp(page_id_t page_id) {
  // 0.   Make sure you call DeallocatePage!
  // 1.   Search the page table for the requested page (P).
  // 1.   If P does not exist, return true.
  // 2.   If P exists, but has a non-zero pin-count, return false. Someone is using the page.
  // 3.   Otherwise, P can be deleted. Remove P from the page table, reset its metadata and return it to the free list.
  // Search the page table for the requested page
  std::lock_guard<std::mutex> lck(latch_);
  if (page_table_.count(page_id) == 0) {
    // In the case that P does not exist
    // P is not in the buffer pool
    return true;
  }
  // In the case that P exists in the buffer pool
  Page &my_page = pages_[page_table_[page_id]];
  if (my_page.GetPinCount() > 0) {
    // Someone is using the page, cannot delete it
    return false;
  }
  // Delete page P
  if (my_page.IsDirty()) {
    disk_manager_->WritePage(my_page.GetPageId(), my_page.GetData());
  }
  my_page.is_dirty_ = false;
  my_page.page_id_ = INVALID_PAGE_ID;
  my_page.pin_count_ = 0;
  my_page.ResetMemory();
  replacer_->Pin(page_table_[page_id]);
  free_list_.push_back(page_table_[page_id]);
  page_table_.erase(page_id);
  DeallocatePage(page_id);
  return true;
}

bool BufferPoolManagerInstance::UnpinPgImp(page_id_t page_id, bool is_dirty) {
  std::lock_guard<std::mutex> lck(latch_);
  // First check whether the parameter is valid, i.e. whether
  // the page resides in the buffer pool
  if (page_id == INVALID_PAGE_ID || page_table_.count(page_id) == 0) {
    return false;
  }
  Page &my_page = pages_[page_table_[page_id]];
  // Change the dirty flag
  // if it is dirty previsouly, then retain the status
  // otherwise change the flag accordingly
  if (is_dirty) {
    my_page.is_dirty_ = is_dirty;
  }
  // Check the pin_count of the page in the buffer pool
  if (my_page.GetPinCount() == 0) {
    return false;
  }
  my_page.pin_count_--;
  if (my_page.GetPinCount() == 0) {
    replacer_->Unpin(page_table_[page_id]);
  }
  return true;
}

page_id_t BufferPoolManagerInstance::AllocatePage() {
  const page_id_t next_page_id = next_page_id_;
  next_page_id_ += num_instances_;
  ValidatePageId(next_page_id);
  return next_page_id;
}

void BufferPoolManagerInstance::ValidatePageId(const page_id_t page_id) const {
  assert(page_id % num_instances_ == instance_index_);  // allocated pages mod back to this BPI
}

}  // namespace bustub
