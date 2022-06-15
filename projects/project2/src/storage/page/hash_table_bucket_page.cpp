//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_table_bucket_page.cpp
//
// Identification: src/storage/page/hash_table_bucket_page.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/hash_table_bucket_page.h"
#include "common/logger.h"
#include "common/util/hash_util.h"
#include "storage/index/generic_key.h"
#include "storage/index/hash_comparator.h"
#include "storage/table/tmp_tuple.h"

namespace bustub {

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_BUCKET_TYPE::GetValue(KeyType key, KeyComparator cmp, std::vector<ValueType> *result) {
  bool res = false;
  // Iterate through the region for KV pairs, check equality for the key
  for (size_t bucket_idx = 0; bucket_idx < BUCKET_ARRAY_SIZE; bucket_idx++) {
    if (IsReadable(bucket_idx) && cmp(key, KeyAt(bucket_idx)) == 0) {
      result->push_back(ValueAt(bucket_idx));
      res = true;
    } else if (!IsOccupied(bucket_idx)) {
      break;
    }
  }
  return res;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_BUCKET_TYPE::Insert(KeyType key, ValueType value, KeyComparator cmp) {
  // If the bucket is full, insertion fails
  if (IsFull()) {
    return false;
  }
  size_t insert_idx = BUCKET_ARRAY_SIZE;
  // Check duplicate, insertion in one pass
  for (size_t bucket_idx = 0; bucket_idx < BUCKET_ARRAY_SIZE; bucket_idx++) {
    if (IsReadable(bucket_idx)) {
      if (cmp(key, KeyAt(bucket_idx)) == 0 && value == ValueAt(bucket_idx)) {
        return false;
      }
    } else if (insert_idx == BUCKET_ARRAY_SIZE) {
      // Update the index for an available slot (in the case that it is not readable)
      // no matter whether it is occupied or not
      // only update once because we only need to find the first available slot for insertion
      // do not break immediately because duplicate keys may appear after the slot
      // This bug takes a loooooong time to find and fix it!
      insert_idx = bucket_idx;
    }
    if (!IsOccupied(bucket_idx)) {
      break;
    }
  }
  if (insert_idx == BUCKET_ARRAY_SIZE) {
    // If no slot is found, meaning that all slots are readable
    // we can insert a new KV pari in this case
    return false;
  }
  array_[insert_idx] = std::make_pair(key, value);
  SetReadable(insert_idx);
  SetOccupied(insert_idx);
  return true;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_BUCKET_TYPE::Remove(KeyType key, ValueType value, KeyComparator cmp) {
  if (IsEmpty()) {
    return false;
  }
  for (size_t bucket_idx = 0; bucket_idx < BUCKET_ARRAY_SIZE; bucket_idx++) {
    if (!IsOccupied(bucket_idx)) {
      break;
    }
    if (!IsReadable(bucket_idx)) {
      continue;
    }
    if (cmp(key, KeyAt(bucket_idx)) == 0 && value == ValueAt(bucket_idx)) {
      // The entries to be deleted is found
      // Reset the readable_ bitmap
      RemoveAt(bucket_idx);
      return true;
    }
  }
  return false;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
KeyType HASH_TABLE_BUCKET_TYPE::KeyAt(uint32_t bucket_idx) const {
  if (IsReadable(bucket_idx)) {
    return array_[bucket_idx].first;
  }
  return {};
}

template <typename KeyType, typename ValueType, typename KeyComparator>
ValueType HASH_TABLE_BUCKET_TYPE::ValueAt(uint32_t bucket_idx) const {
  if (IsReadable(bucket_idx)) {
    return array_[bucket_idx].second;
  }
  return {};
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_BUCKET_TYPE::RemoveAt(uint32_t bucket_idx) {
  if (!IsReadable(bucket_idx)) {
    // If non-readable, then do not delete
    return;
  }
  // Reset the readable_ bitmap
  uint32_t arr_idx = static_cast<uint32_t>(bucket_idx / 8);
  readable_[arr_idx] ^= (static_cast<char>(1) << (bucket_idx - 8 * arr_idx));
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_BUCKET_TYPE::IsOccupied(uint32_t bucket_idx) const {
  if (bucket_idx >= static_cast<uint32_t>(BUCKET_ARRAY_SIZE)) {
    // illegal bucket_idx
    return false;
  }
  uint32_t arr_idx = static_cast<uint32_t>(bucket_idx / 8);
  bucket_idx -= 8 * arr_idx;
  return (occupied_[arr_idx] & (static_cast<char>(1) << bucket_idx)) != 0;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_BUCKET_TYPE::SetOccupied(uint32_t bucket_idx) {
  if (bucket_idx >= static_cast<uint32_t>(BUCKET_ARRAY_SIZE)) {
    // illegal bucket_idx
    return;
  }
  uint32_t arr_idx = static_cast<uint32_t>(bucket_idx / 8);
  bucket_idx -= 8 * arr_idx;
  occupied_[arr_idx] |= (static_cast<char>(1) << bucket_idx);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_BUCKET_TYPE::IsReadable(uint32_t bucket_idx) const {
  if (bucket_idx >= static_cast<uint32_t>(BUCKET_ARRAY_SIZE)) {
    // illegal bucket_idx
    return true;
  }
  uint32_t arr_idx = static_cast<uint32_t>(bucket_idx / 8);
  bucket_idx -= 8 * arr_idx;
  return (readable_[arr_idx] & (static_cast<char>(1) << bucket_idx)) != 0;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_BUCKET_TYPE::SetReadable(uint32_t bucket_idx) {
  if (bucket_idx >= static_cast<uint32_t>(BUCKET_ARRAY_SIZE)) {
    // illegal bucket_idx
    return;
  }
  uint32_t arr_idx = static_cast<uint32_t>(bucket_idx / 8);
  bucket_idx -= 8 * arr_idx;
  readable_[arr_idx] |= (static_cast<char>(1) << bucket_idx);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_BUCKET_TYPE::IsFull() {
  for (size_t bucket_idx = 0; bucket_idx < BUCKET_ARRAY_SIZE; bucket_idx++) {
    if (!IsReadable(bucket_idx)) {
      return false;
    }
  }
  return true;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
uint32_t HASH_TABLE_BUCKET_TYPE::NumReadable() {
  uint32_t count = 0;
  for (size_t bucket_idx = 0; bucket_idx < BUCKET_ARRAY_SIZE; bucket_idx++) {
    if (!IsOccupied(bucket_idx)) {
      break;
    }
    if (IsReadable(bucket_idx)) {
      count += 1;
    }
  }
  return count;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
bool HASH_TABLE_BUCKET_TYPE::IsEmpty() {
  for (size_t bucket_idx = 0; bucket_idx < BUCKET_ARRAY_SIZE; bucket_idx++) {
    if (!IsOccupied(bucket_idx)) {
      break;
    }
    if (IsReadable(bucket_idx)) {
      return false;
    }
  }
  return true;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void HASH_TABLE_BUCKET_TYPE::PrintBucket() {
  uint32_t size = 0;
  uint32_t taken = 0;
  uint32_t free = 0;
  for (size_t bucket_idx = 0; bucket_idx < BUCKET_ARRAY_SIZE; bucket_idx++) {
    if (!IsOccupied(bucket_idx)) {
      break;
    }

    size++;

    if (IsReadable(bucket_idx)) {
      taken++;
    } else {
      free++;
    }
  }

  LOG_INFO("Bucket Capacity: %lu, Size: %u, Taken: %u, Free: %u", BUCKET_ARRAY_SIZE, size, taken, free);
}

// DO NOT REMOVE ANYTHING BELOW THIS LINE
template class HashTableBucketPage<int, int, IntComparator>;

template class HashTableBucketPage<GenericKey<4>, RID, GenericComparator<4>>;
template class HashTableBucketPage<GenericKey<8>, RID, GenericComparator<8>>;
template class HashTableBucketPage<GenericKey<16>, RID, GenericComparator<16>>;
template class HashTableBucketPage<GenericKey<32>, RID, GenericComparator<32>>;
template class HashTableBucketPage<GenericKey<64>, RID, GenericComparator<64>>;

// template class HashTableBucketPage<hash_t, TmpTuple, HashComparator>;

}  // namespace bustub
