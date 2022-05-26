# Project 3 - Query Execution

[![Travis](https://img.shields.io/badge/language-C++-green.svg)]()

>   Date: 2022-05-25

[Project Link](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project3)

**Note**: The project repo is held private according to the course requirements. If you really want the code, please contact me via my email.

----

## Specification

[BusTub](https://github.com/cmu-db/bustub) DBMS is used as the skeleton code. In this project, I add support for query execution to the database system. I create executors that are able to take query plans and execute them. The following operations are supported:

+   **Access Methods**: Sequential Scan
+   **Modications**: Insert, Update, Delete
+   **Miscellaneous**: Nested Loop Join, Hash Join, Aggregation, Limit, Distinct

We use the *iterator* query processing model (the Volcano model). In this model, every query plan executor implements a `Next` function. When the DBMS invokes an executor's `Next` function, it returns either a single tuple or an indicator that there are no more tuples available. In the implemention, each executor implements a loop that continues calling `Next` on its children to retrieve tuples and process them one-by-one. In BusTub's implementation, the `Next` function returns a record identifier `RID` along with a tuple. The correctness of the project depends on our previous implementations of [buffer pool manager](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project1) and [extendible hash table](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project2).

This project assumes a single-threaded context.

## Task 1 - Executors

We implement nine executors. For each query plan operator type, there is a corresponding executor object that implements the `Init` and `Next` methods. The `Init` method initializes the internal stage of the operator. The `Next` method provides the iterator interface that returns a tuple and corresponding RID on each invocation.

The interfaces of executors are defined in the following header files:

+   `src/include/execution/executors/seq_scan_executor.h`
+   `src/include/execution/executors/insert_executor.h`
+   `src/include/execution/executors/update_executor.h`
+   `src/include/execution/executors/delete_executor.h`
+   `src/include/execution/executors/nested_loop_join_executor.h`
+   `src/include/execution/executors/hash_join_executor.h`
+   `src/include/execution/executors/aggregation_executor_executor.h`
+   `src/include/execution/executors/limit_executor.h`
+   `src/include/execution/executors/distinct_executor.h`

Several other executors such as `index_scan_executor` and `nested_index_join_executor` already exist in the repository.

Plan nodes are the individual elements that compose a query plan. Each plan node defines information specific to the operator that it represents. Each executor is reponsible for processing a single plan node type. The plan nodes are defined in the following header files:

+   `src/include/execution/plans/seq_scan_plan.h`

+   `src/include/execution/plans/insert_plan.h`
+   `src/include/execution/plans/update_plan.h`
+   `src/include/execution/plans/delete_plan.h`

+   `src/include/execution/plans/nested_loop_join_plan.h`
+   `src/include/execution/plans/hash_join_plan.h`

+   `src/include/execution/plans/aggregation_plan.h`
+   `src/include/execution/plans/limit_plan.h`
+   `src/include/execution/plans/distinct_plan.h`

The `ExecutionEngine` helper class converts the input query plan to a query executor, and executes it until all results have been produced. This helper class may also catch any potential exceptions during the execution of a query plan.

Some executors may modify the table as well as the index. We use the extendible hash table implementation from Project 2 as the underlying data structure for all indexes in this project.

BusTub maintains an internal catalog to keep track of meta-data about the database. In this project, some executors will interact with the system catalog to query information regarding tables, indexes, and their schemas. The catalog implementation is in `src/include/catalog.h`.

**Sequential Scan**









**Insert**









**Update**









**Delete**









**Nested Loop Join**









**Hash Join**









**Aggregation**









**Limit**









**Distinct**











## Testing

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
$ make executor_test
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
$ cmake -DCMAKE_BUILD_TYPE=DEBUG ..
$ make
```

Add `#include "common/logger.h"` to any file in which you want to make use of the logging infrastructure.

## Gradescope



## Useful Links



## Known Issues

Not found yet

## Credits

Zecheng Qian (qian0102@umn.edu)
