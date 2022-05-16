# Project 2 - Extendible Hash Index

[![Travis](https://img.shields.io/badge/language-C++-green.svg)]()

>   Date: 2022-05-16

[Project Link](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project2)

**Note**: The project repo is held private according to the course requirements. If you really want the code, please contact me via my email.

----

## Specification

[BusTub](https://github.com/cmu-db/bustub) DBMS is used as the skeleton code. In this project, a disk-backed hash table is implemented based on [extendible hashing scheme](https://en.wikipedia.org/wiki/Extendible_hashing) which is a form of dynamic hashing schemes. The hash table index is responsible for fast data retrieval without having to do full scan of a table.

The index comprises a directory page that contains pointers (page Ids) to bucket pages. The table will access pages through the buffer pool that is implemeted in [Project #1](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project1). The hash table supports bucket splitting/merging for full/empty buckets, and directory expansion/contraction for when the global depth must change.

The implementation is **thread-safe**. Multiple threads can access the internal hash table data structures simultaneously and concurrency control is properly handled.

The following components in the storage manager are implemented in this project:

+   Page Layouts
+   Extendible Hashing Implementation
+   Concurrency Control

## Task 1 - Page Layouts







## Task 2 - Extendible Hashing Implementation







## Task 3 - Concurrency Control











## Testing

[GTest](https://github.com/google/googletest) is used to test each individual components of the assignment. The test files for this assignment locate in:

+   `LRUReplacer`: `test/container/hash_table_page_test.cpp`
+   `BufferPoolManagerInstance`: `test/container/hash_table_test.cpp`

Compile and run each test individually by:

```bash
# take LRUReplacer test for example
$ mkdir build
$ cd build
$ make -j hash_table_test
$ ./test/hash_table_test
```

Or use `make check-tests` to run all of the test cases. Disable tests in GTest by adding `DISABLE_` prefix to the test name.

## Formatting

The code is compliant with [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

To lint, run the following commands:

```bash
$ make format
$ make check-lint
$ make check-clang-tidy
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

## Hints

To speed up the build/test process, pass the `-j` flag to enable multi-threading compiling:

```bash
$ make -j 4
```

## Gradescope







## Known Issues

Not found yet

## Credits

Zecheng Qian (qian0102@umn.edu)
