# SAV_535 — Computer Architecture Simulator

SAV_535 is a C++17 computer architecture simulator for a clean-sheet RISC-style instruction set architecture. It models assembly execution, register state, memory hierarchy timing, cache behavior, pipelining, stalls, squashes, and benchmark performance across four architectural modes.

The goal of this project was to make processor design tradeoffs visible through cycle-level simulation.

## Features

- Clean-sheet 32-bit RISC-style integer ISA
- 16-register file with `R0` hardwired to zero
- Assembler and parser for `.asm` programs
- Unified instruction/data memory model
- Direct-mapped L1 cache and optional L2 cache extension
- Five-stage pipeline simulation
- RAW hazard stalls and branch/jump squashes
- Four execution modes: baseline, cache only, pipeline only, cache + pipeline
- CLI simulator and optional Qt GUI
- Benchmark programs for exchange sort and matrix multiply
- Cycle-count, cache-hit, and cache-miss reporting

## Architecture Overview

The simulator models a simple integer processor with:

- 16 general-purpose registers: `R0`–`R15`
- `R0` as constant zero
- `R14` as stack pointer by convention
- `R15` as return address by convention
- Program Counter `PC`
- `STATUS` register with zero flag
- 32-bit fixed-width instructions
- Word-addressed unified memory
- Five-stage pipeline

Pipeline stages:

    IF → ID → EX → MEM → WB

The pipeline does not implement forwarding, so RAW dependencies are handled with stalls. Branches and jumps are resolved in the execute stage, and younger instructions are squashed when control flow changes.

## Instruction Set

Supported instruction categories:

- Arithmetic: `ADD`, `SUB`, `MUL`, `DIV`, `MOD`
- Logical: `AND`, `OR`, `XOR`, `NOT`
- Shifts: `SLL`, `SRL`, `SRA`
- Comparison: `SLT`
- Immediate arithmetic: `ADDI`
- Memory: `LW`, `SW`
- Branches: `BEQ`, `BNE`, `BLT`, `BGE`
- Control flow: `J`, `JAL`, `RET`
- System/basic: `NOP`, `HALT`

## Memory Hierarchy

The simulator uses unified instruction/data memory.

| Level | Configuration | Capacity | Latency |
|---|---|---:|---:|
| L1 Cache | 8 lines × 4 words | 32 words / 128 bytes | 1 cycle hit |
| L2 Cache | 32 lines × 4 words | 128 words / 512 bytes | 10 cycle hit |
| Main Memory | 32,768 words | 131,072 bytes | 50 cycles |

The L1 cache is unified, direct-mapped, write-through, and no-write-allocate.

## Execution Modes

| Mode | Cache | Pipeline | Purpose |
|---|---:|---:|---|
| 0 | Disabled | Disabled | Baseline sequential execution |
| 1 | Enabled | Disabled | Measures cache benefit alone |
| 2 | Disabled | Enabled | Measures pipeline benefit alone |
| 3 | Enabled | Enabled | Measures combined cache and pipeline behavior |

## Repository Structure

    SAV_535/
    ├── demo/
    │   ├── cache_stress.asm
    │   ├── comprehensive_demo.asm
    │   ├── exchange_sort_benchmark.asm
    │   ├── full_isa_demo.asm
    │   ├── matrix_multiply_benchmark.asm
    │   └── simple_demo.asm
    ├── src/
    │   ├── gui/
    │   ├── Assembler.cpp / Assembler.h
    │   ├── Cache.cpp / Cache.h
    │   ├── Instruction.cpp / Instruction.h
    │   ├── L2Cache.cpp / L2Cache.h
    │   ├── Memory.cpp / Memory.h
    │   ├── Parser.cpp / Parser.h
    │   ├── Simulator.cpp / Simulator.h
    │   ├── Types.h
    │   ├── UI.cpp / UI.h
    │   └── main.cpp
    ├── CMakeLists.txt
    └── README.md

## Build Instructions

### CLI Build

    mkdir build
    cd build
    cmake .. -G "MinGW Makefiles"
    cmake --build .

Run:

    ./sim_cli.exe

On Unix-like systems:

    ./sim_cli

### Optional Qt GUI Build

    mkdir build_gui
    cd build_gui
    cmake .. -G "MinGW Makefiles" -DBUILD_GUI=ON -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\mingw_64"
    cmake --build .

Run:

    ./sim_gui.exe

If CMake was previously configured with a different generator, delete the build folder or remove `CMakeCache.txt` before reconfiguring.

## Assembly Syntax

Programs are written as `.asm` files.

Example:

    ADDI R1, R0, 5
    ADDI R2, R0, 10
    ADD R3, R1, R2
    HALT

Labels and comments are supported:

    loop:
        ADDI R1, R1, 1
        BNE R1, R2, loop
        HALT

    # This is a comment

## CLI Usage

Example benchmark run:

    LOADASM ../demo/exchange_sort_benchmark.asm
    MODE 0
    RUN 2000000
    STATE

Run the same benchmark in all four modes:

    MODE 0    # No cache / no pipeline
    MODE 1    # Cache only
    MODE 2    # Pipeline only
    MODE 3    # Cache + pipeline

Useful inspection commands:

    REGS
    PIPE
    CACHE1
    CACHE2
    MEM 200 64
    STATE

## Benchmarks

### Exchange Sort

Sorts a 40-element integer array.

    Input: 40, 39, ..., 1
    Array addresses: 200–239
    Expected output: 1, 2, ..., 40
    Checksum: 820
    Checksum address: 250

The 40-word array exceeds the 32-word L1 cache, so this benchmark stresses L1 capacity and repeated memory access.

### Matrix Multiply

Computes `C = A × B` for 7×7 integer matrices.

    Matrix A addresses: 300–348
    Matrix B addresses: 400–448
    Matrix C addresses: 500–548
    Checksum: 8575
    Checksum address: 700

The 147-word working set exceeds the 128-word L2 cache, so this benchmark stresses the full memory hierarchy.

## Results

### Cycle Count Summary

| Program | No Cache / No Pipe | Cache Only | Pipe Only | Cache + Pipe |
|---|---:|---:|---:|---:|
| `simple_demo.asm` | 1,554 | 662 | 1,483 | 594 |
| `cache_stress.asm` | 676 | 268 | 662 | 255 |
| `comprehensive_demo.asm` | 3,954 | 1,517 | 3,843 | 1,252 |
| `full_isa_demo.asm` | 1,552 | 660 | 1,538 | 600 |
| `exchange_sort_benchmark.asm` | 574,993 | 145,375 | 562,608 | 131,165 |
| `matrix_multiply_benchmark.asm` | 399,973 | 83,676 | 384,472 | 71,527 |

### Required Benchmark Results

| Benchmark | Baseline | Cache Only | Pipeline Only | Cache + Pipeline | Best Speedup |
|---|---:|---:|---:|---:|---:|
| Exchange Sort | 574,993 | 145,375 | 562,608 | 131,165 | 4.4× |
| Matrix Multiply | 399,973 | 83,676 | 384,472 | 71,527 | 5.6× |

## Key Takeaways

- Cache was the dominant performance improvement for both benchmarks.
- Pipeline-only mode improved performance only slightly because the design has no forwarding or branch prediction.
- Cache + pipeline was the fastest mode for both required benchmarks.
- Exchange sort stressed L1 cache capacity with a 40-word working set.
- Matrix multiply stressed the full memory hierarchy with a 147-word working set.
- The simulator makes DRAM latency, cache hits/misses, RAW stalls, and branch squashes visible through cycle counts.

## Tech Stack

- C++17
- CMake
- Qt for optional GUI
- MinGW / Windows build support
- Assembly-style benchmark programs

## Future Improvements

- Add forwarding to reduce RAW stalls
- Add branch prediction to reduce squash overhead
- Add configurable cache size and associativity
- Add separate instruction and data caches
- Add automated regression tests
- Export benchmark results to CSV
- Add GUI screenshots and demo GIFs

## Status

Completed computer architecture simulator project demonstrating ISA design, C++ systems programming, assembly parsing, memory hierarchy simulation, pipelining, cache behavior, CLI/GUI tooling, and benchmark-based performance analysis.
