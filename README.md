# AdaptiveRadixSort
Adaptive radix-based sorting algorithm for 32-bit integers, achieving up to 100×+ speedups over libc qsort through data-aware, cache-efficient design.

 # Overview

Most standard sorting implementations (like qsort) rely on comparison-based algorithms with O(n log n) complexity and heavy branching.

This project takes a different approach.

It implements an adaptive, data-aware sorting pipeline that:

detects patterns in input data

selects the optimal sorting strategy

minimizes branching and memory overhead

The result is a system that operates close to hardware limits in many cases.

# Key Idea

The fastest sorting algorithm isn’t a single algorithm, it’s choosing the right one for the data.

This engine dynamically switches between:

Insertion Sort → for small inputs

Linear-time fixes → for nearly sorted arrays

Reverse pass → for reverse-sorted inputs

Early exit → for sorted / identical values

Radix Sort (2-pass, 16-bit buckets) → for general cases

# Performance

<img width="1579" height="509" alt="image" src="https://github.com/user-attachments/assets/a5a15d52-c0b7-40d7-a97f-a806a87a97ab" />

 
 
# Performance Visualization

<img width="640" height="480" alt="image" src="https://github.com/user-attachments/assets/91d31cdf-0894-4a13-944a-846765fcd821" />


# Benchmark Methodology

Same dataset used for both implementations

Same runtime environment

Measured using high-resolution timers (clock_gettime)

Compared against libc’s qsort

This ensures a fair, real-world comparison.

# Why It’s Faster
1. Avoids Comparisons

qsort → comparison-based (branch-heavy)

This → radix-based (branch-minimal)

2. Exploits Data Structure

Detects:

sorted arrays → O(n)

nearly sorted → O(n)

duplicates → near O(n)

3. Cache Efficiency

Sequential memory access

Minimal random jumps

Reduced cache misses

4. Branch Prediction Friendly

Fewer unpredictable branches

More linear execution

5. Memory Reuse

No repeated allocations

Preallocated workspace buffers

# Architecture

<img width="953" height="712" alt="image" src="https://github.com/user-attachments/assets/f0782929-2df2-43ca-9572-1a937062454d" />

# Build & Run
gcc -O3 -march=native -funroll-loops src/sort.c -o sort
./sort
