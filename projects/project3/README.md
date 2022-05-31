# Project 3 - Query Execution

[![Travis](https://img.shields.io/badge/language-C++-green.svg)]()

>   Date: 2022-05-25

[Project Link](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project3)

**Note**: The project repo is held private according to the course requirements. If you really want the code, please contact me via my email.

----

## Specification

[BusTub](https://github.com/cmu-db/bustub) DBMS is used as the skeleton code. In this project, I add support for query execution to the database system. I create executors that are able to take query plans and execute them. The following operations are supported:

+   **Access Methods**: Sequential Scan
+   **Modifications**: Insert, Update, Delete
+   **Miscellaneous**: Nested Loop Join, Hash Join, Aggregation, Limit, Distinct

We use the *iterator* query processing model (the Volcano model). In this model, every query plan executor implements a `Next` function. When the DBMS invokes an executor's `Next` function, it returns either a single tuple or an indicator that there are no more tuples available. In the implementation, each executor implements a loop that continues calling `Next` on its children to retrieve tuples and process them one-by-one. In BusTub's implementation, the `Next` function returns a record identifier `RID` along with a tuple. The correctness of the project depends on our previous implementations of [buffer pool manager](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project1) and [extendible hash table](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project2).

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

Plan nodes are the individual elements that compose a query plan. Each plan node defines information specific to the operator that it represents. Each executor is responsible for processing a single plan node type. The plan nodes are defined in the following header files:

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

BusTub maintains an internal catalog to keep track of meta-data about the database. In this project, some executors will interact with the system catalog to query information regarding tables, indexes, and the schemas. The catalog implementation is in `src/include/catalog.h`.

**Sequential Scan**

The `SeqScanExecutor` iterates over a table and return its tuples, one-at-a-time (per the Volcano model's requirement). A sequential scan operation is specified by a `SeqScanPlanNode`. The plan node specifies the table (identified by `table_oid_t` which is defined in the system catalog file) over which the scan is performed, along with an optional predicate (e.g. it can be any filter predicate in the where clause such as `a > 100`).

Each time a single record is scanned, along with an evaluation of an optimal predicate.

**Insert**

The `InsertExecutor` inserts tuples into a table and update indexes. The system supports two types of insertions.

+   The first type of insertion is embedded inside the plan node, referred as raw insert.
+   The second type of insertion takes values to be inserted from a child executor. For example, we can have an `InsertPlanNode` with a `SeqScanPlanNode` as its child to implement an `INSERT INTO ... SELECT ...`

We assume that the `InsertExecutor` is always at the root of the query plan and should not modify its result set, it only modifies the table and indexes in place.

Multiple rows may be inserted sequentially for a single execution plan. We perform the insertion first, followed by index update.

**Update**

The `UpdateExecutor` modified existing tuples in a specified table and update the indexes. It always has a child executor that tells the tuple to be updated (identified by `RID`). In our implementation, `UpdatePlanNode` will have a (at most one child) `SeqScanPlanNode` as its child.

The `GenerateUpdateTuple` method defined in `UpdateExecutor` class helps construct an updated tuple based on the provided update attributes.

The workflow for the `UpdateExecutor` is as following:

1.   Initialize the `child_executor_`
2.   Run the `child_executor_`, gather the results
3.   Generate updated tuples based on the `update_attrs` given in an `UpdatePlanNode`
4.   Update the records by invoking `UpdateTuple` method for a specified `TableHeap`
5.   Update indexes: there is no direct way to update an index in place, so delete it first, followed by another insertion

**Delete**

The `DeleteExecutor` deletes tuples from tables and removes their entries from all table indexes. Tuples to be deleted are pulled from a child executor. In our implementation, it is a `SeqScanExecutor` instance.

We assume that the `DeleteExecutor` is always the root of the query plan.

**Note:** As we learn from a database course, in most modern DBMSs, a deletion of records from a table is implemented in a lazy approach, which means that the disk space is not freed immediately. Instead, the records are marked as deleted and the space is freed afterwards.

The workflow for the `DeleteExecutor` is as following:

1.   Initialize the `child_executor_`
2.   Run the `child_executor_`, gather the results
3.   Delete records from the table, identified by `RID`, by invoking the `MarkDelete` method of a `TableHeap` object
4.   Delete corresponding entries from the indexes by invoking the `DeleteEntry` method of an `Index` object

**Nested Loop Join**

The `NestedLoopJoinExecutor` implements a basic nested loop join that combines that tuples from its two child executors together.

The algorithm checks for each tuple in the outer table, if there is match in the inner table, then it emits an output tuple if the join predicate (an `ON` clause) is satisfied. The tuples are joined if `predicate(tuple) = true` or `predicate = nullptr`.

The workflow for the `NestedLoopJoinExecutor` is as following:

1.   Initialize the `left_executor_`
2.   Initialize the `right_executor_`
3.   Run `left_executor_->Next()`, for each tuple fetched, iterate through all the tuples produced by the `right_executor_`

4.   For each pair of tuples, evaluate the `Predicate` of the query plan. A predicate of a join query can be understood as a `ON` clause, such as:

     ```sql
     select *
     from table1 inner join table2
     on table1.colA = table2.colA;
     ```

5.   If the predicate evaluates to be true, then compute the output tuple, this can be done by iterate through all the columns of the query plan output schema, for each column, evaluate the expression to produce a value for a single column. Finally all column values are concatenated together to form a single tuple.
6.   For each call `Next`, increment a `Iterator` object to fetch the next tuple, until the end.

**Note:** We don't calculate all the join results before emitting. We try to find a tuple satisfying the join predicate in the `Next` function call instead, in order to be pipelined.

**Hash Join**

The `HashJoinExecutor` implements a hash join operation that combines the tuples from two child executors using a hash table.

In our implementation, we make the simplifying assumption that the hash table fits entirely in memory, which means that we don't need to worry about spilling temporary partitions of the build-side table to disk.

The `HashJoinPlanNode` uses the `LeftJoinKeyExpression()` and `RightJoinKeyExpression()` accessors to get the join keys in the hash table. The left join key and right join key are both of type `ColumnValueExpression`.

Remembering that the build phase of a hash join is a pipeline breaker, meaning that the whole hash table has to be built before `Next` the probe phase and `Next` can be called. So the build phase for the hash table has to be performed in `HashJoinExecutor::Init()`. The probe phase, however, can be pipeline for parallel execution.

There are two phases for the basic hash join algorithm:

+   Phase #1: Build
    +   Scan the outer relation and populate a hash table using the hash function on the join attributes.
    +   Each hash table entry is a pair of `<Key, Value>`, for which the `Key` is hashed value of the attributes to be joined on, and the `Value` may vary per implementation. In my implementation, it is a collection of full tuples. We may also store a tuple identifier (`RID` for example) in order to reduce the space overhead. However, there will be an extra search time overhead to fetch a tuple in the outer table for each comparison.
+   Phase #2: Probe
    +   Scan the inner relation and use the same hash function on each tuple to jump to a location in the hash table (which is previously built), do a brute force search to find a match.

Optimization approach: We may use a **Bloom Filter** to pre-filter out some keys before actually doing probing.

**Note**: Currently in my implementation, a hash join can only be performed on a single join attribute. **It is not my fault. That is due to the signature of the `HashJoinPlanNode` constructor. The constructor only accepts two `AbstractExpression` instead of vectors of `AbstractExpression`. **

**Aggregation**

The `AggregationExecutor` implements `COUNT`, `SUM`, `MIN`, and `MAX` aggregations. It also supports `GROUP By` and `HAVING` clauses. The executor supports group by on multiple columns and at most one `HAVING` clause, as well as multiple aggregations in a single query. The aggregation is performed over the results produced by a child executor.

An example of an aggregation query:

```sql
SELECT count(col_a), col_b, sum(col_c) FROM test_1 Group By col_b HAVING count(col_a) > 100;
```

We use a hash table to implement aggregations.

In our implementation, we make the simplifying assumption that the aggregating hash table fits entirely in memory, which means that we don't need to worry about implementing the two-stage (Partition, Rehash) strategy, and may assume that all of the aggregation results can reside in a in-memory hash table.

In the context of a query plan, aggregations are pipeline breakers. Because not until the aggregation is run for all tuples in the table it can be emitted to other plan nodes. This means that the Build phase of the aggragation must be performed in `AggregationExecutor::Init()`.

The workflow for the `AggregationExecutor` is as following:

1.   Initialize the `child_executor_`
2.   Run the `child_executor_`, gather the results

3.   For each tuple produced by the `child_executor->Next()`, compute a running aggregation, depending on the aggregation type, performing group by if necessary
4.   Query a result in the `Next` method, by checking whether the current tuple pointed by the `SimpleAggregationHashTable::Iterator` satisfies the `HAVING` predicate. If it is, then reconstruct an output tuple and emit the result. If not, check the next tuple until reaching the end

**Limit**

The `LimitExecutor` constrains the number of output tuples produced by its child executor. If the number of tuples produced by its child executor is less than the limit specified in the `LimitPlanNode`, this executor has no effect and yield all of the tuples that it receives.

**Distinct**

The `DistinctExecutor` eliminates duplicates tuples received from its child executor.

We use a hash table to implement distinct operators.

In our implementation, we make the simplifying assumption that the hash table fits entirely in memory, which means that we don't need to worry about implementing the two-stage (Partition, Rehash) strategy and moving blocks forward and backward between memory and disk.

We should notice that the hash distinct operation can be pipelined. The only requirement for the operation is not to produce a previously produced tuple from the child executor.

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
$ zip project3-submission.zip \
src/include/buffer/lru_replacer.h \
src/buffer/lru_replacer.cpp \
src/include/buffer/buffer_pool_manager_instance.h \
src/buffer/buffer_pool_manager_instance.cpp \
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
src/include/storage/page/tmp_tuple_page.h
```

![12361654030329_.pic](/Users/qianzecheng/Library/Containers/com.tencent.xinWeChat/Data/Library/Application Support/com.tencent.xinWeChat/2.0b4.0.9/4db9bdab375d6565216e197f61225841/Message/MessageTemp/9e20f478899dc29eb19741386f9343c8/Image/12361654030329_.pic.jpg)

## Known Issues

Not found yet

## Credits

Zecheng Qian (qian0102@umn.edu)
