# Project 4 - Concurrency Control

[![Travis](https://img.shields.io/badge/language-C++-green.svg)]()

>   Date: 2022-06-03

[Project Link](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project4)

**Note**: The project repo is held private according to the course requirements. If you really want the code, please contact me via my email.

----

## Specification

[BusTub](https://github.com/cmu-db/bustub) DBMS is used as the skeleton code. In this project, I implement a <u>lock manager</u> and use it to support concurrent query execution. The lock manager is responsible for keeping track of the tuple-level locks issued to transactions and supporting shared & exclusive locks granted and released based on isolation levels. We use **pessimistic concurrency control** (use read/write locks) in this project.

The project is comprised of the following three tasks:

+   Task 1 - Lock Manager
+   Task 2 - Deadlock Prevention
+   Task 3 - Concurrent Query Execution

The implementation is thread-safe.

## Task 1 - Lock Manager

The DBMS will use a lock manager (LM) to control when transactions are allowed to access data items. The LM is an internal data structure about the locks currently held by active transactions. Transactions issues lock requests to the LM before they are allowed to access a data item. The LM will either grant the lock to the calling transaction, block that transaction, or abort it.

In the implementation, there will be a global LM for the entire system. The `TableHeap` (in memory representation of a table on disk) and `Executor` (query executor) classes will use the LM to acquire locks on tuple records (by record id `RID`) whenever a transaction wants to access/modify a tuple.

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

We assume that logically there is no locking/unlocking request when a transaction is in its `Committed` stage. The logic is implemented on the application level. If our client is client enough (i.e. the client will not do any read/write when he/she decides commit the transaction).

The workflow is as following:

1.   If the current transaction is in the `TransactionState::ABORTED` state, do nothing and return false
2.   If 2PL is used and the current transaction is in the  `TransactionState::SHRINKING` state, **abort** and transaction and throw and exception
3.   If the current transaction tries locking locking an already locked `RID`. Do nothing and return true
4.   Otherwise acquire the request queue on this `RID`
5.   If at the moment there is no request queue for the current `RID`, which means that the current request is the first one to acquire a lock on it, then we create and initialize a new request queue for this `RID` and add it to the lock table
6.   If there is a queue, then we take different locking policies depending on different isolation levels
     1.   If the isolation level is `IsolationLevel::READ_UNCOMMITTED`, which is the lowest isolation level, concurrent transaction is permitted to read uncommitted writes by other transactions.

**LockExclusive**

Similar to `LockShared`

**LockUpgrade**







**Unlock**

`Unlock` releases the lock held by the transaction. The workflow is as following:

1.   Check the state of the transaction, if it is in `TransactionState::ABORTED` state, do nothing and return false
2.   Check whether the current transaction holds a lock, if it does not, do nothing and return false
3.   Check the isolation level for the current transaction
     1.   If it is in `IsolationLevel::READ_UNCOMMITTED`
     2.   If it is in `IsolationLevel::READ_COMMITTED`
     3.   If it is in `IsolationLevel::REPEATABLE_READ`, set the transaction state to `TransactionState::SHRINKING`, iterate through the request queue for the specified `RID` to find the request corresponding to the current transaction. Release that lock/request and notify all other waiting threads. Return true





## Task 2 - Deadlock Prevention

The lock manager uses the ***WOUND-WAIT*** algorithm for deciding which transactions to abort.





Release lock is implemented in `TransactionManager::Abort()`, when it is called, according to 2PL, all the locks in one transaction is released first, then notify_all is called to wake up all the threads waiting for locks. So locking requests can be processed.









## Task 3 - Concurrent Query Execution

During concurrent query execution, executors are required to lock/unlock tuples appropriately to achieve different isolation levels.

We add support for concurrent query execution in the following executors:

+   `src/execution/seq_scan_executor.cpp`
+   `src/execution/insert_executor.cpp`
+   `src/execution/update_executor.cpp`
+   `src/execution/delete_executor.cpp`









## Testing

**Waning! Waning! Warning!**

**`cmake -DCMAKE_BUILD_TYPE=Debug ..` enables <u>AddressSanitizer</u> which may produces false positives for overflow on STL containers. Set the environment variable `ASAN_OPTIONS=detect_container_overflow=0` help resolve the issue!**

**Run the following command from the CLI if you encounter this issue:** 

```bash
$ export ASAN_OPTIONS=detect_odr_violation=0
```

[GTest](https://github.com/google/googletest) is used to test each individual components of the assignment. The test files for this assignment locate in:

+   `Executor`: `test/execution/executor_test.cpp`

Compile and run each test individually by:

```bash
$ mkdir build
$ cd build
$ make -j executor_test
$ ./test/executor_test
```

Or use `make check-tests` to run all of the test cases. Disable tests in GTest by adding `DISABLE_` prefix to the test name.

## Memory Safety

Use [Valgrind](https://valgrind.org/) on a Linux machine to check memory leaks:

```bash
$ make -j executor_test
$ valgrind --trace-children=yes \
   --leak-check=full \
   --track-origins=yes \
   --soname-synonyms=somalloc=*jemalloc* \
   --error-exitcode=1 \
   --suppressions=../build_support/valgrind.supp \
   ./test/executor_test
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



## Known Issues

Not found yet

## Credits

Zecheng Qian (qian0102@umn.edu)
