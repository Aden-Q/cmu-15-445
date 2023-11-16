# Project 0 - C++ Primer

[![Travis](https://img.shields.io/badge/language-C++-green.svg)]()

>   Date: 2022-05-12

[Project Link](https://github.com/Aden-Q/CMU-15-445/tree/main/projects/project0)

**Note**: The project repo is held private according to the course requirements. If you really want the code, please contact me via my email.

----

## Specification

I implement three classes: `Matrix`, `RowMatrix`, and `RowMatrixOperations`. Those are two-dimensional matrices that support <u>addition</u>, <u>matrix multiplication</u>, and a simplified <u>General Matrix Multiply (GEMM)</u> operations.

## Testing

[GTest](https://github.com/google/googletest) is used to test each individual components of the assignment. The test file for this assignment locates in:

+   `Starter`: `test/primer/starter_test.cpp`

Compile and run each test individually by:

```bash
$ mkdir build
$ cd build
$ make starter_test
$ ./test/starter_test
```

Or `make check-tests` to run all of the test cases.

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

## Implementations

Because the implementation for this project is too naive, I chose not to write anything but post the code on my GitHub.

## Known Issues

Not found yet
