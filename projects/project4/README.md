# Project 4 - Concurrency Control

[![Travis](https://img.shields.io/badge/language-C++-green.svg)]()

>   Date: 2022-06-03

[Project Link](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project4)

**Note**: The project repo is held private according to the course requirements. If you really want the code, please contact me via my email.

----

## Specification

[BusTub](https://github.com/cmu-db/bustub) DBMS is used as the skeleton code. In this project, I implement a <u>lock manager</u> and use it to support concurrent query execution. The lock manager is responsible for keeping track of the tuple-level locks issued to transactions and supporting shared & exclusive locks granted and released based on isolation levels. We use a two-phase locking (2PL) based **pessimistic concurrency control** (use read/write locks) in this project.

The project is comprised of the following three tasks:

+   Task 1 - Lock Manager
+   Task 2 - Deadlock Prevention
+   Task 3 - Concurrent Query Execution

The implementation is thread-safe.

## Task 1 - Lock Manager && Task 2 - Deadlock Prevention

The DBMS will use a lock manager (LM) to control when transactions are allowed to access data items. The LM is an internal data structure about the locks currently held by active transactions. Transactions issues lock requests to the LM before they are allowed to access a data item. The LM will either grant the lock to the calling transaction, block that transaction, or abort it.

In the implementation, there will be a global LM for the entire system. The `TableHeap` (in memory representation of a table on disk) and `Executor` (query executor) classes will use the LM to acquire locks on tuple records (by record id `RID`) whenever a transaction wants to access/modify a tuple.

The lock manager uses the ***WOUND-WAIT*** algorithm for deciding which transactions to abort.

I implement a tuple-level LM that supports the three common isolation levels:

+   `READ_UNCOMMITED`
+   `READ_COMMITTED`
+   `REPEATABLE_READ`

The LM grants or releases locks according to a transaction's isolation level. Failed lock operation leads to an ABORTED transaction state and throw an exception. The transaction manager would further catch this exception and rollback write operations executed by the transaction.

I implement the`LockManager` class which locates in the following files:

+   `concurrency/lock_manager.cpp`
+   `concurrency/lock_manager.h`

The following functions are implemented:

+   `LockShared(Transaction, RID)`: Transaction **txn** tries to take a shared lock on record id **rid**. This function is blocked on waiting and returns true when granted. Return false if transaction is rolled back (aborts).
+   `LockExclusive(Transaction, RID)`: Transaction **txn** tries to take an exclusive lock on record id **rid**. This should be blocked on waiting and returns true when granted. Return false if transaction is rolled back (aborts).
+   `LockUpgrade(Transaction, RID)`: Transaction **txn** tries to upgrade a shared to exclusive lock on record id **rid**. This should be blocked on waiting and returns true when granted. Return false if transaction is rolled back (aborts). This function also aborts the tranasction and returns false if another transaction is already waiting to upgrade their lock.
+   `Unlock(Transaction, RID)`: Unlock the record identified by the given record id that is held by the transaction.

There are four transaction states: `GROWING`, `SHRINKING`, `COMMITTED`, `ABORTED`, among which the `SHRINKING` state is only used in 2PL.

**LockShared**

We assume that logically there is no locking/unlocking request when a transaction is in its `Committed` stage. The logic is implemented on the application level. If our client is smart enough (i.e. the client will not do any read/write when he/she decides commit the transaction).

The workflow is as following:

1.   If the current transaction is in the `TransactionState::ABORTED` state, do nothing and return false
2.   If in `REPEATABLE_READ` or `READ_UNCOMMITTED` isolation levels, and the current transaction is in the  `TransactionState::SHRINKING` state, **abort** the transaction and rollback
3.   If the current transaction has a pending request for `RID` in its `LockRequest`. Do nothing and return true (we do not allow duplicate requests in the same locking mode)
4.   If the current isolation level is `READ_UNCOMMITTED`, **abort** the transaction and rollback. We should not grant a read lock for `READ_UNCOMITTED` because we are using 2PL. If a read lock is granted, then uncommitted writes cannot be read before releasing the locks (right before the commit stage)
5.   Otherwise acquire the request queue on this `RID`
6.   Insert the lock request into the request queue, in `LockMode::SHARED` mode
7.   Populating the current transactions's shared lock set

8.   Iterate through the request queue, if a younger `LockMode::EXCLUSIVE` request is found in the request queue before the current one, abort it and rollback the corresponding transaction
9.   If any transaction is aborted during the previous stage, call `notify_all` to wake up all waiting threads
10.   Check whether the current locking request can be granted in a while loop (busy waiting is undesirable, we can use a while-wait loop instead)
11.   If the locking condition is met, grant the lock by adding it to the current transaction's shared lock set, return true

**LockExclusive**

Similar to `LockShared`

**LockUpgrade**

Similar to the previous two, there can be different implementations, depending on how you define `fairness`

**Unlock**

`Unlock` releases the lock held by some transaction. The workflow is as following:

1.   Check the state of the transaction, if it is in `TransactionState::GROWING` state, and the current isolation  is either `REPEATABLE_READ` or `READ_COMMITTED`, change the transaction state to `TransactionState::SHRINKING`. The isolation level `READ_UNCOMMITTED` does not have a shrinking stage in order for all the uncommitted writes to be read by other transactions
2.   Otherwise acquire the request queue on the specified `RID`
3.   Find all the requests in the request queue with a transaction id equal to the specified transaction id
4.   Release the lock for that transaction by removing the request from the request queue and erase the transaction's shared/exclusive lock set
5.   Explicitly wake up all the other waiting threads on the current request queue

## Task 3 - Concurrent Query Execution

During concurrent query execution, executors are required to lock/unlock tuples appropriately to achieve different isolation levels.

We add support for concurrent query execution in the following executors:

+   `src/execution/seq_scan_executor.cpp`
+   `src/execution/insert_executor.cpp`
+   `src/execution/update_executor.cpp`
+   `src/execution/delete_executor.cpp`

**SeqScanExecutor**

+   For any tuple to be read from a table, acquire a read lock (we do not need to worry about isolation levels when acquiring locks if it has been properly handled in the lock manager)
+   When finish reading, whether to release the lock depends on different isolation levels:
    +   If in `TransactionState::REPEATABLE_READ`, do not release the read lock, otherwise you cannot perform any read/write unlesse you know that you will perform a read/write in the future and acquire a write lock beforehead. However, you cannot predict what a transaction will do in the future!
    +   If in `TransactionState::READ_COMITTED`, release the lock and do not enter the `SHRINKING` stage cuz we want to let other transactions acquire write locks on it and read what other transactions has committed. If we do not release, then it prevents from unrepeatable read, which is not necessary in this isolation level. If we release the read lock and let the current transaction enter the `SHRINKING` stage, then the current transaction cannot perform any read on it in the future, this is stupid because you don't know whether the current transaction will perform a read in the future or not...
    +   If in `TransactionState::READ_UNCOMITTED`, as we illustrated above, we do not have any read lock on any record in this isolation level, the underlying data structure is only protected by OS latches (so we can ignore race condition and concurrency issues on the system level). This is the lowest isolation level in which many unpredictable things may happen. Cuz we don't have read locks on it, we do not perform unlock when finish reading

**InsertExecutor**

+   Before insertion, the record is not present in the table, so we do nothing before insertion actually happen on the table/index level
+   After inserting a record into the table, acquire a write lock
+   Append the change into the current transaction table write set and index write set, which are used for rollbacks
+   When finish writing, whether to release the lock depends on different isolation levels:
    +   If in `TransactionState::REPEATABLE_READ`, do not release the write lock, for the same reason as before
    +   If in `TransactionState::READ_COMITTED`, do not release the write lock because we don't write **dirty** **write** to happen (writes in the current transaction is overwritten by other transactions, which is not allowed in all isolation levels)
    +   If in `TransactionState::READ_UNCOMITTED`, do not release the write lock because we don't write **dirty** **write** to happen

**DeleteExecutor**

Similar to `InsertExecutor`

**UpdateExecutor**

Similar to `InsertExecutor`

**Note**: It is worth noting that the phantom phenomenon can possibly happen in all isolation levels because we didn't implement the gap-lock.

## Testing

**Waning! Waning! Warning!**

**`cmake -DCMAKE_BUILD_TYPE=Debug ..` enables <u>AddressSanitizer</u> which may produces false positives for overflow on STL containers. Set the environment variable `ASAN_OPTIONS=detect_container_overflow=0` help resolve the issue!**

**Run the following command from the CLI if you encounter this issue:** 

```bash
$ export ASAN_OPTIONS=detect_odr_violation=0
```

[GTest](https://github.com/google/googletest) is used to test each individual components of the assignment. The test files for this assignment locate in:

+   `LockManager`: ``test/concurrency/lock_manager_test.cpp``
+   `Transaction`: `test/concurrency/transaction_test.cpp`

Compile and run each test individually by:

```bash
$ mkdir build
$ cd build
$ make -j lock_manager_test
$ ./test/lock_manager_test
```

Or use `make check-tests` to run all of the test cases. Disable tests in GTest by adding `DISABLE_` prefix to the test name.

## Memory Safety

Use [Valgrind](https://valgrind.org/) on a Linux machine to check memory leaks:

```bash
$ make -j lock_manager_test
$ valgrind --trace-children=yes \
   --leak-check=full \
   --track-origins=yes \
   --soname-synonyms=somalloc=*jemalloc* \
   --error-exitcode=1 \
   --suppressions=../build_support/valgrind.supp \
   ./test/lock_manager_test
```

## Formatting

The code is compliant with [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

To lint, run the following commands:

```bash
$ make -j format
$ make -j check-lint
$ make -j check-clang-tidy
```

## Debugging

Instead of using `printf`, logging the output using `LOG_*` macros:

```c++
LOG_INFO("# Pages: %d", num_pages);
LOG_DEBUG("Fetching page %d", page_id);
```

To enable logging, do the following configuration:

```bash
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=DEBUG ..  # configure in DeBug mode
$ make
```

Add `#include "common/logger.h"` to any file in which you want to make use of the logging infrastructure.

## Gradescope

Make a submission:

```bash
$ zip project4-submission.zip \
    src/include/buffer/lru_replacer.h \
    src/buffer/lru_replacer.cpp \
    src/include/buffer/buffer_pool_manager_instance.h \
    src/buffer/buffer_pool_manager_instance.cpp \
    src/include/buffer/parallel_buffer_pool_manager.h \
    src/buffer/parallel_buffer_pool_manager.cpp \
    src/include/storage/page/hash_table_directory_page.h \
    src/storage/page/hash_table_directory_page.cpp \
    src/include/storage/page/hash_table_bucket_page.h \
    src/storage/page/hash_table_bucket_page.cpp \
    src/include/container/hash/extendible_hash_table.h \
    src/container/hash/extendible_hash_table.cpp \
    src/include/execution/execution_engine.h \
    src/include/execution/executors/seq_scan_executor.h \
    src/include/execution/executors/insert_executor.h \
    src/include/execution/executors/update_executor.h \
    src/include/execution/executors/delete_executor.h \
    src/include/execution/executors/nested_loop_join_executor.h \
    src/include/execution/executors/hash_join_executor.h \
    src/include/execution/executors/aggregation_executor.h \
    src/include/execution/executors/limit_executor.h \
    src/include/execution/executors/distinct_executor.h \
    src/execution/seq_scan_executor.cpp \
    src/execution/insert_executor.cpp \
    src/execution/update_executor.cpp \
    src/execution/delete_executor.cpp \
    src/execution/nested_loop_join_executor.cpp \
    src/execution/hash_join_executor.cpp \
    src/execution/aggregation_executor.cpp \
    src/execution/limit_executor.cpp \
    src/execution/distinct_executor.cpp \
    src/include/concurrency/lock_manager.h \
    src/concurrency/lock_manager.cpp \
    src/include/concurrency/transaction.h \
    src/include/concurrency/transaction_manager.h \
    src/concurrency/transaction_manager.cpp \
		src/include/storage/page/tmp_tuple_page.h
```

![](https://raw.githubusercontent.com/Aden-Q/blogImages/main/img/202206052159464.jpg)

## Known Issues

Not found yet
