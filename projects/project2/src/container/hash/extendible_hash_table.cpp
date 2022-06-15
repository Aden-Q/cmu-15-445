//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_hash_table.cpp
//
// Identification: src/container/hash/extendible_hash_table.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/exception.h"
#include "common/logger.h"
#include "common/rid.h"
#include "container/hash/extendible_hash_table.h"

namespace bustub {

template <typename KeyType, typename ValueType, typename KeyComparator>
HASH_TABLE_TYPE::ExtendibleHashTable(const std::string &name, BufferPoolManager *buffer_pool_manager,
                                     const KeyComparator &comparator, HashFunction<KeyType> hash_fn)
    : name_(name), buffer_pool_manager_(buffer_pool_manager), comparator_(comparator), hash_fn_(std::move(hash_fn)) {
  // implement me!
  // Allocate a directory page in the buffer pool
  table_latch_.WLock();
  HashTableDirectoryPage *dir_page = nullptr;
  HASH_TABLE_BUCKET_TYPE *bucket_page = nullptr;
  page_id_t bucket_page_id;
  Page *page = buffer_pool_manager_->NewPage(&directory_page_id_);
  // check whether the page is successfully allocated on disk
  if (page == nullptr) {
    table_latch_.WUnlock();
    return;
  }
  dir_page = reinterpret_cast<HashTableDirectoryPage *>(page->GetData());
  assert(dir_page != nullptr);
  // Initialize the directory page
  dir_page->SetPageId(directory_page_id_);
  // Create a bucket page and let the first slot of directory point to it
  page = nullptr;
  page = buffer_pool_manager_->NewPage(&bucket_page_id);
  // check whether the page is successfully allocated on disk
  if (page == nullptr) {
    table_latch_.WUnlock();
    return;
  }
  bucket_page = reinterpret_cast<HASH_TABLE_BUCKET_TYPE *>(page->GetData());
  assert(bucket_page != nullptr);
  // Set bucket page id in the directory page
  dir_page->SetBucketPageId(static_cast<uint32_t>(0), bucket_page_id);
  dir_page->SetLocalDepth(0, 0);
  // The two new pages can be unpinned
  assert(buffer_pool_manager_->UnpinPage(bucket_page_id, false, nullptr));
  assert(buffer_pool_manager->UnpinPage(directory_page_id_, true, nullptr));
  table_latch_.WUnlock();
}

/*****************************************************************************
 * HELPERS
 *****************************************************************************/
/**
 * Hash - simple helper to downcast MurmurHash's 64-bit hash to 32-bit
 * for extendible hashing.
 *
 * @param key the key to hash
 * @return the downcasted 32-bit hash
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
uint32_t HASH_TABLE_TYPE::Hash(KeyType key) {
  return static_cast<uint32_t>(hash_fn_.GetHash(key));
}

template <typename KeyType, typename ValueType, typename KeyComparator>
uint32_t HASH_TABLE_TYPE::KeyToDirectoryIndex(KeyType key, HashTableDirectoryPage *dir_page) {
  uint32_t res = Hash(key) & dir_page->GetGlobalDepthMask();
  return res;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
page_id_t HASH_TABLE_TYPE::KeyToPageId(KeyType key, HashTableDirectoryPage *dir_page) {
  page_id_t res = dir_page->GetBucketPageId(KeyToDirectoryIndex(key, dir_page));
  return res;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
HashTableDirectoryPage *HASH_TABLE_TYPE::FetchDirectoryPage() {
  HashTableDirectoryPage *dir_page = nullptr;
  // Make a call to the buffer manager to fetch the page into
  // the buffer pool
  Page *page = buffer_pool_manager_->FetchPage(directory_page_id_);
  assert(page != nullptr);
  dir_page = reinterpret_cast<HashTableDirectoryPage *>(page->GetData());
  return dir_page;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
HASH_TABLE_BUCKET_TYPE *HASH_TABLE_TYPE::FetchBucketPage(page_id_t bucket_page_id) {
  HASH_TABLE_BUCKET_TYPE *bucket_page = nullptr;
  Page *page = buffer_pool_manager_->FetchPage(bucket_page_id);
  assert(page != nullptr);
  bucket_page = reinterpret_cast<HASH_TABLE_BUCKET_TYPE *>(page->GetData());
  return bucket_page;
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::GetValue(Transaction *transaction, const KeyType &key, std::vector<ValueType> *result) {
  table_latch_.RLock();
  bool res = false;
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  HASH_TABLE_BUCKET_TYPE *bucket_page = nullptr;
  page_id_t bucket_page_id = 0;
  // Check whether the directory page has bee fetched from buffer pool successfully or not
  assert(dir_page != nullptr);
  // Fetch the bucket page
  bucket_page_id = KeyToPageId(key, dir_page);
  bucket_page = FetchBucketPage(bucket_page_id);
  Page *page = reinterpret_cast<Page *>(bucket_page);
  assert(page != nullptr);
  page->RLatch();
  // Get values
  res = bucket_page->GetValue(key, comparator_, result);
  // Unpin the directory page and the bucket page
  assert(buffer_pool_manager_->UnpinPage(dir_page->GetPageId(), false, nullptr));
  assert(buffer_pool_manager_->UnpinPage(bucket_page_id, false, nullptr));
  page->RUnlatch();
  table_latch_.RUnlock();
  return res;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::Insert(Transaction *transaction, const KeyType &key, const ValueType &value) {
  table_latch_.RLock();
  bool res = false;
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  HASH_TABLE_BUCKET_TYPE *bucket_page = nullptr;
  page_id_t bucket_page_id = 0;
  // If fail to fetch the directory page, then insertion fails
  assert(dir_page != nullptr);
  // Fetch the bucket page for insertion
  bucket_page_id = KeyToPageId(key, dir_page);
  bucket_page = FetchBucketPage(bucket_page_id);
  Page *page = reinterpret_cast<Page *>(bucket_page);
  // If fail to fetch the bucket page, then insertion fails
  assert(page != nullptr);
  page->WLatch();
  // Insert the KV pair into the bucket
  // First check whether the bucket is full
  if (!bucket_page->IsFull()) {
    // If is not full, insert into the current bucket
    // The insertion will fail if there is a duplicate KV pair
    res = bucket_page->Insert(key, value, comparator_);
    assert(buffer_pool_manager_->UnpinPage(dir_page->GetPageId(), false, nullptr));
    // After insertion, the bucket page is updated, so it is marked as a dirty page
    assert(buffer_pool_manager_->UnpinPage(bucket_page_id, true, nullptr));
    page->WUnlatch();
    table_latch_.RUnlock();
  } else {
    // In the case of a bucket page is full,
    // perform a split insertion recursively
    // Unpin the pages after the split insertion
    // because we hope that the directory page and the bucket page
    // are still in the buffer pool, to avoid additional page
    // movements between memory and disk
    // We can unpin the old bucket page because it may not be
    // the target page for SplitInsert, it makes no sense
    // to always stitch it into the buffer pool
    assert(buffer_pool_manager_->UnpinPage(bucket_page_id, false, nullptr));
    assert(buffer_pool_manager_->UnpinPage(dir_page->GetPageId(), false, nullptr));
    page->WUnlatch();
    table_latch_.RUnlock();
    res = SplitInsert(transaction, key, value);
    // Important: release the write latch before SplitInsert
    // There might be recursive call such as Insert->SplitInsert->Insert->SplitInsert
    // which may require bucket write latch again. So always release release the latch
    // before the next function call
  }
  return res;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::SplitInsert(Transaction *transaction, const KeyType &key, const ValueType &value) {
  table_latch_.WLock();
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  HASH_TABLE_BUCKET_TYPE *bucket_page = nullptr;
  HASH_TABLE_BUCKET_TYPE *split_bucket_page = nullptr;
  Page *page = nullptr;
  uint32_t bucket_idx = 0;
  page_id_t bucket_page_id = 0;
  uint32_t split_bucket_idx = 0;
  page_id_t split_bucket_page_id = 0;
  assert(dir_page != nullptr);
  bucket_idx = KeyToDirectoryIndex(key, dir_page);
  bucket_page_id = KeyToPageId(key, dir_page);
  bucket_page = FetchBucketPage(bucket_page_id);
  Page *buck_page = reinterpret_cast<Page *>(bucket_page);
  assert(buck_page != nullptr);
  if (!bucket_page->IsFull()) {
    // If the bucket page becomes not full, apply normal insertion
    // this situation may be due to some intermediate deletions before
    // acquiring the write latch
    bool res = bucket_page->Insert(key, value, comparator_);
    assert(buffer_pool_manager_->UnpinPage(dir_page->GetPageId(), false, nullptr));
    assert(buffer_pool_manager_->UnpinPage(bucket_page_id, true, nullptr));
    table_latch_.WUnlock();
    return res;
  }
  // Split the current bucket
  if (dir_page->GetLocalDepth(bucket_idx) < dir_page->GetGlobalDepth()) {
    // In the first case, the local depth of the bucket is strictly less than the global depth,
    // we do not increase the global depth in the first round, but do the following instead:
    // 1. Increment the local depth of the old bucket page
    dir_page->IncrLocalDepth(bucket_idx);
  } else {
    // In this case, we need to double the directory size
    // we do the following
    // 1. Increment the global depth of the hash table
    // 2. Increment the local depth of the old bucket page
    // Remember to increment the local depth first,
    // because the call to IncGlobalDepth will copy the content
    if (dir_page->Size() > (DIRECTORY_ARRAY_SIZE >> 1)) {
      // Insertion fails in this case
      assert(buffer_pool_manager_->UnpinPage(dir_page->GetPageId(), false, nullptr));
      assert(buffer_pool_manager_->UnpinPage(bucket_page_id, false, nullptr));
      table_latch_.WUnlock();
      return false;
    }
    dir_page->IncrGlobalDepth();
    dir_page->IncrLocalDepth(bucket_idx);
  }
  // We do the following next
  // 1. Allocate a new page in the buffer pool and cast it into a bucket page
  // 2. Update the hash table directory
  // 3. Redistribute the KV pairs
  split_bucket_idx = dir_page->GetSplitImageIndex(bucket_idx);
  page = buffer_pool_manager_->NewPage(&split_bucket_page_id);
  assert(page != nullptr);
  split_bucket_page = reinterpret_cast<HASH_TABLE_BUCKET_TYPE *>(page->GetData());
  assert(split_bucket_page != nullptr);
  // Initialize the split image
  dir_page->SetBucketPageId(split_bucket_idx, split_bucket_page_id);
  dir_page->SetLocalDepth(split_bucket_idx, dir_page->GetLocalDepth(bucket_idx));
  // Update the hash table directory
  // we only need to move half of the entries pointing to
  // the old bucket page so that they point to the split image
  assert(dir_page->GetLocalDepthMask(split_bucket_idx) == dir_page->GetLocalDepthMask(bucket_idx));
  uint32_t curr_idx = split_bucket_idx & dir_page->GetLocalDepthMask(split_bucket_idx);
  uint32_t step = dir_page->GetLocalHighBit(split_bucket_idx) << 1;
  for (; curr_idx < dir_page->Size(); curr_idx += step) {
    dir_page->SetBucketPageId(curr_idx, split_bucket_page_id);
    dir_page->SetLocalDepth(curr_idx, dir_page->GetLocalDepth(split_bucket_idx));
  }
  curr_idx = bucket_idx & dir_page->GetLocalDepthMask(bucket_idx);
  for (; curr_idx < dir_page->Size(); curr_idx += step) {
    dir_page->SetLocalDepth(curr_idx, dir_page->GetLocalDepth(bucket_idx));
  }
  assert(dir_page->GetLocalDepthMask(split_bucket_idx) == dir_page->GetLocalDepthMask(bucket_idx));
  // Finally we redistribute the KV pairs that are previously
  // in the old bucket page.
  // We implement it by iterate through all records in the old bucket page
  // The old bucket is supposed to be full and each entry should be readable
  assert(bucket_page->IsFull());
  assert(split_bucket_page->IsEmpty());
  KeyType temp_key;
  ValueType temp_value;
  for (size_t arr_idx = 0; arr_idx < BUCKET_ARRAY_SIZE; arr_idx++) {
    if (bucket_page->IsReadable(arr_idx)) {
      temp_key = bucket_page->KeyAt(arr_idx);
      temp_value = bucket_page->ValueAt(arr_idx);
      if (KeyToPageId(temp_key, dir_page) != bucket_page_id) {
        // Move the KV pair to the split bucket page
        // and delete it from the old page (place a tombstone at the corresponding position)
        split_bucket_page->Insert(temp_key, temp_value, comparator_);
        bucket_page->RemoveAt(arr_idx);
      }
    }
  }
  // In either case, the hash table page, the bucket page, and
  // the split bucket page all become dirty pages
  // Unpin after insertion
  // Unpin the directory page is necessary because we need to set the dirty flag
  assert(buffer_pool_manager_->UnpinPage(dir_page->GetPageId(), true, nullptr));
  assert(buffer_pool_manager_->UnpinPage(bucket_page_id, true, nullptr));
  assert(buffer_pool_manager_->UnpinPage(split_bucket_page_id, true, nullptr));
  // Recursively call Insert after split,
  // the result is false if insertion fails
  // (either hash table error or buffer pool error)
  // the result is true if insertion succeeds
  table_latch_.WUnlock();
  return Insert(transaction, key, value);
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_TYPE::Remove(Transaction *transaction, const KeyType &key, const ValueType &value) {
  table_latch_.RLock();
  bool res = false;
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  HASH_TABLE_BUCKET_TYPE *bucket_page = nullptr;
  page_id_t bucket_page_id = 0;
  // If fail to fetch the directory page, then insertion fails
  assert(dir_page != nullptr);
  // Fetch the bucket page for insertion
  bucket_page_id = KeyToPageId(key, dir_page);
  bucket_page = FetchBucketPage(bucket_page_id);
  Page *page = reinterpret_cast<Page *>(bucket_page);
  assert(page != nullptr);
  page->WLatch();
  // Delete the KV pair from the hash table
  // Deletion can either succeed or fail
  res = bucket_page->Remove(key, value, comparator_);
  assert(buffer_pool_manager_->UnpinPage(bucket_page_id, true, nullptr));
  assert(buffer_pool_manager_->UnpinPage(dir_page->GetPageId(), false, nullptr));
  page->WUnlatch();
  table_latch_.RUnlock();
  Merge(transaction, key, value);
  return res;
}

/*****************************************************************************
 * MERGE
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_TYPE::Merge(Transaction *transaction, const KeyType &key, const ValueType &value) {
  table_latch_.WLock();
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  HASH_TABLE_BUCKET_TYPE *bucket_page = nullptr;
  uint32_t bucket_idx = 0;
  page_id_t bucket_page_id = 0;
  uint32_t split_bucket_idx = 0;
  page_id_t split_bucket_page_id = 0;
  assert(dir_page != nullptr);
  bucket_idx = KeyToDirectoryIndex(key, dir_page);
  bucket_page_id = KeyToPageId(key, dir_page);
  bucket_page = FetchBucketPage(bucket_page_id);
  if (dir_page->GetLocalDepth(bucket_idx) == 0 || !bucket_page->IsEmpty()) {
    // If the local depth is 0, do not merge
    assert(buffer_pool_manager_->UnpinPage(bucket_page_id, false, nullptr));
    assert(buffer_pool_manager_->UnpinPage(dir_page->GetPageId(), false, nullptr));
    table_latch_.WUnlock();
    return;
  }
  split_bucket_idx = dir_page->GetSplitImageIndex(bucket_idx);
  split_bucket_page_id = dir_page->GetBucketPageId(split_bucket_idx);
  assert(split_bucket_page_id != bucket_page_id);
  if (dir_page->GetLocalDepth(bucket_idx) != dir_page->GetLocalDepth(split_bucket_idx)) {
    // If local depths are not equal, do not merge
    assert(buffer_pool_manager_->UnpinPage(bucket_page_id, false, nullptr));
    assert(buffer_pool_manager_->UnpinPage(dir_page->GetPageId(), false, nullptr));
    table_latch_.WUnlock();
    return;
  }
  // Check whether the local depths are equal and greater than 0
  uint8_t local_depth = dir_page->GetLocalDepth(bucket_idx);
  uint8_t split_local_depth = dir_page->GetLocalDepth(split_bucket_idx);
  assert(local_depth != 0 && local_depth == split_local_depth);
  // Merge the target page and the split image
  // Unpin the target page and delete it
  assert(buffer_pool_manager_->UnpinPage(bucket_page_id, false, nullptr));
  assert(buffer_pool_manager_->DeletePage(bucket_page_id, nullptr));
  // Update the hash directory page
  assert(dir_page->GetLocalDepthMask(bucket_idx) == dir_page->GetLocalDepthMask(split_bucket_idx));
  uint32_t curr_idx = bucket_idx & dir_page->GetLocalDepthMask(bucket_idx);
  uint32_t step = dir_page->GetLocalHighBit(bucket_idx) << 1;
  for (; curr_idx < dir_page->Size(); curr_idx += step) {
    dir_page->SetBucketPageId(curr_idx, split_bucket_page_id);
    dir_page->DecrLocalDepth(curr_idx);
  }
  curr_idx = split_bucket_idx & dir_page->GetLocalDepthMask(split_bucket_idx);
  for (; curr_idx < dir_page->Size(); curr_idx += step) {
    dir_page->DecrLocalDepth(curr_idx);
  }
  assert(dir_page->GetLocalDepthMask(bucket_idx) == dir_page->GetLocalDepthMask(split_bucket_idx));
  // If we can shrink the directory
  // shrink its size by a factor of 2
  while (dir_page->CanShrink()) {
    dir_page->DecrGlobalDepth();
  }
  // Unpin the directory page
  assert(buffer_pool_manager_->UnpinPage(dir_page->GetPageId(), true, nullptr));
  table_latch_.WUnlock();
}

/*****************************************************************************
 * GETGLOBALDEPTH - DO NOT TOUCH
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
uint32_t HASH_TABLE_TYPE::GetGlobalDepth() {
  table_latch_.RLock();
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  uint32_t global_depth = dir_page->GetGlobalDepth();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, false, nullptr));
  table_latch_.RUnlock();
  return global_depth;
}

/*****************************************************************************
 * VERIFY INTEGRITY - DO NOT TOUCH
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_TYPE::VerifyIntegrity() {
  table_latch_.RLock();
  HashTableDirectoryPage *dir_page = FetchDirectoryPage();
  dir_page->VerifyIntegrity();
  assert(buffer_pool_manager_->UnpinPage(directory_page_id_, false, nullptr));
  table_latch_.RUnlock();
}

/*****************************************************************************
 * TEMPLATE DEFINITIONS - DO NOT TOUCH
 *****************************************************************************/
template class ExtendibleHashTable<int, int, IntComparator>;

template class ExtendibleHashTable<GenericKey<4>, RID, GenericComparator<4>>;
template class ExtendibleHashTable<GenericKey<8>, RID, GenericComparator<8>>;
template class ExtendibleHashTable<GenericKey<16>, RID, GenericComparator<16>>;
template class ExtendibleHashTable<GenericKey<32>, RID, GenericComparator<32>>;
template class ExtendibleHashTable<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
