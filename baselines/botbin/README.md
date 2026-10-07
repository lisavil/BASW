# BOTBIN

This source is based on the authors' [BOTBIN repository](https://github.com/zzzzzfy/BOTBIN), *Effective Indexing for Dynamic Structural Graph Clustering*. It preserves the bottom-k sketches, bucket-index maintenance, and native core-query procedure. The upstream acknowledgment of GS-index is retained in the source.

The exported version includes Linux compilation fixes, bounds and iterator checks, initialized query pointers, stream-based file output, and public update/query wrappers from the experiment instance. The command-line compilation fix in `patches/cli-compilation.patch` is also applied. Dataset placeholders have been made relative paths for portability. Source files are in `src/`, including the original command-line program and `temporal_runner.cpp`.

## Build

```sh
cmake -S baselines/botbin -B baselines/botbin/build -DCMAKE_BUILD_TYPE=Release
cmake --build baselines/botbin/build --parallel
```

The native `botbin` program uses the dataset configuration in `src/Config.h`. For temporal updates, supply a tab-separated manifest with zero-based vertex IDs:

```text
meta	vertex_count	3
0	0	INITIAL	0	1
0	0	INITIAL	1	2
0	0	WINDOW	-1	-1
1	1	DELETE	0	1
1	1	INSERT	0	2
1	1	WINDOW	-1	-1
```

```sh
./baselines/botbin/build/botbin_temporal \
  --manifest UPDATES.tsv --run-dir output/botbin \
  --rho 0.10 --delta 100 --epsilon 0.5 --mu 5
```

The manifest must declare the vertex count and use effective simple-edge changes; deletions are applied before insertions in each window. The runner builds the initial index and queries after each listed window. `--max-windows N` optionally limits the number of windows.

## Output contract

This instance's temporal runner calls the native core-component query and writes per-window result files and `timing.csv`. Its `query_ns` includes native result-file output. Initialization uses the source's native random-device seeding. This exported runner provides native BOTBIN execution; it does not implement the paper's complete common-state in-memory output adapter. Complete-state quality and end-to-end comparisons require that adapter and its corresponding evaluation harness separately.

## Attribution

The original source and its authors retain their ownership and notices. The upstream repository does not declare a repository-wide license in the inspected release; this directory does not assign a new license to the third-party code. Consult the upstream release for its terms.
