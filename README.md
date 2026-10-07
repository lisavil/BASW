# BASW

Exact structural clustering over sliding-window temporal graph streams.

This repository provides three algorithm implementations and a minimal runnable example:

- **BASW** (`basw_batch`): batch-exact maintenance on the final graph of each window slide.
- **STATIC** (`basw_static`): full recomputation after each window slide.
- **SEQ** (`basw_seq`): exact maintenance after each effective edge deletion or insertion.

All three use closed-neighborhood Jaccard similarity with exact rational threshold comparison. They maintain core components, non-core cluster memberships, and core/border/hub/outlier roles. Repeated temporal interactions contribute to edge multiplicity; an undirected simple edge is active while its multiplicity is positive. SEQ and BASW consume the same coalesced effective topology changes.

## Build

Requirements: CMake 3.20 or later and a C++20 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

On Windows with Visual Studio, an explicit generator can be used:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

## Input

The input is a whitespace-separated event file with exactly three fields per data line:

```text
timestamp source destination
```

Timestamps are signed 64-bit integers in nondecreasing order. Vertex identifiers are strings and are remapped to dense IDs. Blank lines and lines beginning with `#` are ignored. Self-loops are skipped, and interactions are interpreted as undirected edges.

The active window at time `t` is `(t - W, t]`. The width `W` must be positive, and the slide interval must satisfy `0 < DELTA <= W`. All time arguments use the same unit as the input timestamps.

## Run

All three programs accept the same arguments:

```text
PROGRAM INPUT OUTPUT W DELTA INITIAL_TIME EPS_NUM EPS_DEN MU MAX_SLIDES [METRICS_CSV]
```

- `OUTPUT`: canonical clustering snapshots for the initial window and subsequent slides.
- `EPS_NUM / EPS_DEN`: the exact similarity threshold, with `0 <= EPS_NUM <= EPS_DEN` and `EPS_DEN > 0`.
- `MU`: the minimum number of similar adjacent vertices for a core vertex; at least one.
- `MAX_SLIDES`: the maximum number of window slides to process.
- `METRICS_CSV`: optional diagnostic metrics. In BASW, requesting metrics also runs a SEQ reference outside the BASW timer and checks final snapshots.

Example on Linux or another single-configuration build:

```sh
./build/basw_static examples/tiny_events.txt output/static.txt 4 2 3 1 2 2 10
./build/basw_seq examples/tiny_events.txt output/seq.txt 4 2 3 1 2 2 10
./build/basw_batch examples/tiny_events.txt output/basw.txt 4 2 3 1 2 2 10
```

On Windows with Visual Studio:

```powershell
.\build\Release\basw_static.exe examples\tiny_events.txt output\static.txt 4 2 3 1 2 2 10
.\build\Release\basw_seq.exe examples\tiny_events.txt output\seq.txt 4 2 3 1 2 2 10
.\build\Release\basw_batch.exe examples\tiny_events.txt output\basw.txt 4 2 3 1 2 2 10
```

The snapshot sections, beginning at `=== snapshot 0 ===`, should be identical across all three methods. Metadata headers differ because they include method-specific fields. Programs create output directories as needed and print a run summary.

## Source layout

- `include/basw/`: graph, window, clustering-state, and runner interfaces.
- `src/`: shared implementation and the three clustering methods.
- `apps/`: the three command-line entry points.
- `examples/tiny_events.txt`: a small temporal stream with repeated interactions.
- `SOURCE_SHA256`: checksums of the algorithm sources and example as published.

The algorithm sources are preserved unchanged from the implementation. The CMake configuration builds the three programs and their required shared modules.
