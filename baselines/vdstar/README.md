# VD-STAR

This is the adapted **NoT v3** variant of VD-STAR, based on the authors' [v1.0.0 artifact](https://doi.org/10.5281/zenodo.15486055) and [upstream repository](https://github.com/alvinzhaowei/DynStrClu/tree/v1.0.0). The adapted source revision on the experiment instance is `20417884fd70a5db62c9e965da6218f467f0b83e`.

The source retains the native dynamic-maintenance mechanism. Adaptations address state consistency, object lifetime, query materialization, approximation initialization, and small-edge similarity maintenance. `Graph::materializeClustering(epsilon, mu)` returns similar-edge labels, core components, overlapping memberships, and vertex roles in memory. The three-argument `Graph` constructor accepts an explicit random seed. The reference approximation parameter is `rho = 0.02`; the source uses one-based vertex IDs.

The publication copy uses neutral names for the sampling counters. Those identifier changes preserve the computation. The CMake build files are provided separately; the bundled Boost tree is unnecessary for the included source.

## Build and validation

```sh
cmake -S baselines/vdstar -B baselines/vdstar/build -DCMAKE_BUILD_TYPE=Release
cmake --build baselines/vdstar/build --parallel
cd baselines/vdstar/build
ctest --output-on-failure
```

The native executable accepts the upstream binary graph and edge-update formats:

```sh
./baselines/vdstar/build/vdstar -graph GRAPH.bin -update UPDATES.txt -rho 0.02
```

For complete-state materialization, use the `Graph` API. The native executable retains the upstream query/timing interface; the complete-state evaluation uses `materializeClustering` with an external timer.

## Repair patches

The patch series in `patches/` records the eleven adaptations in this order:

1. `B-001-query-buffer-bounds-and-lifetime.patch`
2. `D-003-sorted-neighbor-delete-consistency.patch`
3. `B-002-large-vertex-nonlast-delete.patch`
4. `B-003-dtmanager-swap-delete-index-consistency.patch`
5. `D-001-core-threshold-degree-boundary.patch`
6. `D-002-complete-query-semantics.patch`
7. `D-005-affordability-reset-scale.patch`
8. `D-004-failure-probability-initialization.patch`
9. `B-005-owned-object-lifecycle.patch`
10. `C-001-explicit-approximation-seed.patch`
11. `D-006-small-edge-similarity-maintenance.patch`

To reconstruct the adapted algorithm source from an extracted upstream artifact, run:

```sh
python3 baselines/vdstar/replay_patches.py UPSTREAM_DIRECTORY NEW_OUTPUT_DIRECTORY
```

The helper copies only source and license files, normalizes the sampling-counter identifiers, and applies the patches in order. The focused tests in `tests/` check complete clustering, core thresholds, deletion consistency, affordability, probability initialization, and seeded replay.

## Attribution

VD-STAR is the work of Zhuowei Zhao, Junhao Gan, Boyu Ruan, Zhifeng Bao, Jianzhong Qi, and Sibo Wang, *Dynamic Structural Clustering Unleashed: Flexible Similarities, Versatile Updates and for All Parameters* (KDD 2025). The Zenodo artifact declares [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). The bundled Tessil robin-map headers retain their [MIT license](src/Tessil_robin_map/LICENSE). This publication identifies the above modifications and retains upstream copyright notices.
