
# README

## Overview

This project involves building a minimalistic **memory allocator** that can be used to manually manage virtual memory. 
It implements basic memory management functions like `malloc()`, `calloc()`, `realloc()`, and `free()`. The objective 
is to have a reliable library that handles explicit allocation, reallocation, and initialization of memory while 
minimizing bottlenecks.

## Project Structure

- **`src/`**: Contains the memory allocator.
- **`tests/`**: Includes test cases and a Python script (`run_tests.py`) to verify the implementation.
- **`utils/`**: Contains utility files like:
  - `osmem.h`: Describes the library interface.
  - `block_meta.h`: Manages metadata for memory blocks.
  - A custom `printf()` implementation that does **NOT** use the heap.

## API

The memory allocator exposes the following functions:

1. **`void *os_malloc(size_t size)`**
   - Allocates `size` bytes and returns a pointer to the allocated memory. 
   - Small memory chunks are allocated using `brk()`; larger chunks use `mmap()`.
   - Passing `0` as `size` will return `NULL`.

2. **`void *os_calloc(size_t nmemb, size_t size)`**
   - Allocates memory for an array of `nmemb` elements, each of `size` bytes.
   - The memory is initialized to zero.
   - Small memory chunks use `brk()`, while larger ones use `mmap()`.
   - Returns `NULL` if `nmemb` or `size` is `0`.

3. **`void *os_realloc(void *ptr, size_t size)`**
   - Changes the size of the memory block pointed to by `ptr` to `size` bytes.
   - If expanding, it tries to grow the block if there are adjacent free blocks.
   - Returns `NULL` if `ptr` is invalid or `size` is `0`.

4. **`void os_free(void *ptr)`**
   - Frees the memory block previously allocated by `os_malloc()`, `os_calloc()`, or `os_realloc()`.
   - Memory is reused but not returned to the OS, except for memory blocks allocated via `mmap()`.

## Building and Testing

1. Navigate to the `src/` directory to build the memory allocator:
   ```bash
   cd src/
   make
   ```

2. To run the provided test suite:
   ```bash
   cd ../tests/
   python3 run_tests.py
   ```

The `run_tests.py` script runs each test and compares the results with reference output.
