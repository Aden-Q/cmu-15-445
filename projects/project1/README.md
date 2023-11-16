# Project 1 - Buffer Pool

[![Travis](https://img.shields.io/badge/language-C++-green.svg)]()

>   Date: 2022-05-11

[Project Link](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project1)

**Note**: The project repo is held private according to the course requirements. If you really want the code, please contact me via my email.

----

## Specification

[BusTub](https://github.com/cmu-db/bustub) DBMS is used as the skeleton code. In this project, a **buffer pool** is implemented in the storage manager. The buffer pool is responsible for moving physical pages back and forth from main memory to disk. It uses the idea of virtual memory / page mapping that allows a DBMS to support databases that are larger than the amount of memory that is available to the system. The buffer pool provides an abstraction layer to the system so that users/programmers do not need to interact with physical storage directly.

The implementation is **thread-safe**. Multiple threads can access the internal data structures simultaneously and concurrency control is properly handled.

The following components in the storage manager are implemented in this project:

+   LRU Replacement Policy
+   Buffer Pool Manager Instance
+   Parallel Buffer Pool Manager

## Task 1 - LRU Replacement Policy

The `LRUReplacer` component is responsible for tracking page usage in the buffer pool. The maximum number of pages for `LRUReplacer` is the same as the size of the buffer pool since it contains placeholders for all of the frams in the `BufferPoolManager`.

The `list` container in C++ is used to implement this component. APIs of `LRUReplacer` are listed below:

+   `Victim(frame_id_t*)`: Remove the object that was accessed least recently
+   `Pin(frame_id_t)`: Called after a frame is pinned (marked as in use) to a frame in the buffer pool
+   `Unpin(frame_id_t)`: Called when the `pin_count` of a page in the buffer pool becomes 0
+   `Size()`: Return the number of frames that are currently in the `LRUReplacer`

## Task 2 - Buffer Pool Manager Instance

The `BufferPoolManagerInstance` is responsible for fetching database pages from the `DiskManager` and storing them in memory. The `BufferPoolManagerInstance` should also flush dirty pages out to disk when it is explicitly instructed to do so or when it needs to evict a page to make space for a new page.

All in-memory pages in the system are represented by `Page` objects. Each `Page` object acts as a container that the `DiskManager` will use as a location to copy the contents of a physical page that it reads from disk. `Page` can be reused as it moves back and forth to disk, which means that the same `Page` object may contain a different physical page throughout the life of the system. The `page_id` is an identifier for each page that keeps track of the location of the physical page. The `page_id` field must be set to `INVALID_PAGE_ID` if it does not contain a physical page.

Each `Page` object also maintains a counter for the number of threads that have 'pinned' that page. A pinned page is not allowed to be freed. Each page object also keeps track of whether it is dirty or not. A dirty page must be written to disk before it can be unpinned.

The `BufferPoolManagerInstance` object uses `LRUReplacer` to keep track of when `Page` objects are accessed so that it can decide which one to evict when necessary. It also uses a free list to keep track of the current available slots in the buffer pool.

The following functions are implemented:

+   `FetchPgImp(page_id)`: Fetch the requested page from the buffer pool, identified by `page_id`. The target page must exist on disk
    +   Search the page table, if exists, pin it and return it
    +   Otherwise if the page does not exist, find a replacement page from either the free list or the replacer
    +   If both the free list and the replacer are not available, return nullptr, indicating that this page cannot be swapped into the memory
    +   If a replaced page is found via the replacer, flush it onto disk if it is dirty
    +   Delete the replaced page from the page table and add a new entry for the new page
    +   Update the metadata for the new page and read the contents from disk
    +   Return the results
+   `UnpinPgImp(page_id, is_dirty)`: Unpin the target page from the buffer pool
    +   Check whether the page exists in the buffer pool, if not, do nothing
    +   Modify the dirty flag of the unpinned page
    +   Check the `pin_count` of the target page to be unpinned, if already 0, which means that this page has been unpinned previously and no actions are needed, return
    +   Otherwise decrement the `pin_count`, if it decreases to 0, call replacer's `Unpin` function to notify it that this page can be replaced
    +   Return the results
+   `FlushPgImp(page_id)`: Flushes the target page to disk regardless of its pin status
    +   Check whether the page exits in the buffer pool, if not, do nothing
    +   Otherwise call `disk_manager_->WritePage` to flush the page onto disk
+   `NewPgImg(page_id)`: Creates a new page in the buffer pool. The target page does not exist on disk
    +   Check whether all the pages in the buffer pool are pinned, if they are, do nothing and return nullptr
    +   Otherwise pick a victim page from either the freelist or the replacer
    +   Flush the old page onto disk if necessary, then update the page's metadata, return the results
+   `DeletePgImg(page_id)`: Delete a page from the buffer page
    +   Search the page table, if cannot find the target page, do nothing and return true
    +   If find the target page in the page table, which means that the target page is in the buffer pool, check the `pin_count` for the page
    +   If the `pin_count` is non-zero, which means that someone is using the page, do nothing and return false
    +   Otherwise flush the page onto disk if dirty, and excute subroutines to reset and deallocate the page
+   `FlushAllPagesImpl()`: Flush all the pages in the buffer pool onto disk
    +   Only need a `for` loop to flush every page onto disk

**Pay attention to the implementation of `NewPgImp` and `FetchPgImp`, especially some boundary checking. Other functions are trivial.**

## Task 3 - Parallel Buffer Pool Manager

To support parallelism, we can use multiple buffer pools, each with its own latch.

In this task, we implement a `ParallelBufferPoolManager` class that holds multiple `BufferPoolManagerInstance`s. We use `page_id mod num_instances` to map a request to some page to some instance of `BufferPoolManagerInstance`.

The following functions are implemented:

+   `ParallelBufferPoolManager(num_instances, pool_size, disk_manager, log_manager)`: Constructor, allocate and create individual BufferPoolManagerInstances
+   `~ParallelBufferPoolManager()`: Destructor, destruct all BufferPoolManagerInstances and deallocate any associated memory
+   `GetPoolSize()`: Get size of the buffer pool (sum of the buffer pool size for each `BufferPoolManagerInstance`)
    +   As simple as `num_instances_ * pool_size_`
+   `GetBufferPoolManager(page_id)`: Retrieve a pointer to the BufferPoolManager responsible for handling given page id, determine by `page_id mod num_instances`
+   `FetchPgImp(page_id)`: Fetch the requested page from the buffer pool
+   `UnpinPgImg(page_id, is_dirty)`: Unpin the target page from the buffer pool
+   `FlushPgImg(page_id)`: Flushes the target page to disk
+   `NewPgImg(page_id)`: Creates a new page in the buffer pool
+   `DeletePgImp(page_id)`: Deletes a page from the buffer pool
+   `FlushAllPagesImpl()`: Flushes all the pages in the buffer pool to disk

Task 3 is just a just trivial wrap up of task 2, don't need to do any concurrency control as long as you have done it in part 2.

## Testing

[GTest](https://github.com/google/googletest) is used to test each individual components of the assignment. The test file for this assignment locates in:

+   `LRUReplacer`: `test/buffer/lru_replacer_test.cpp`
+   `BufferPoolManagerInstance`: `test/buffer/buffer_pool_manager_instance_test.cpp`
+   `ParallelBufferPoolManager`: `test/buffer/parallel_buffer_pool_manager_test.cpp`

Compile and run each test individually by:

```bash
# take LRUReplacer test for example
$ mkdir build
$ cd build
$ make lru_replacer_test
$ ./test/lru_replacer_test
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

To speed up the build process, pass the `-j` flag to enable multi-threading compiling:

```bash
$ make -j 4
```

## Gradescope

![](https://raw.githubusercontent.com/Aden-Q/blogImages/main/img/202205120402984.jpg)

## Known Issues

Not found yet
