# External baselines

This directory contains the VD-STAR and BOTBIN source versions exported from the experiment instance, with independent Linux builds.

- [VD-STAR](vdstar/README.md): adapted NoT v3 source, the eleven repair patches, and focused validation programs.
- [BOTBIN](botbin/README.md): native sketch and bucket-index maintenance with a temporal manifest runner.

Build both on Linux with a C++17 compiler and CMake 3.16 or later:

```sh
cmake -S baselines -B baselines/build -DCMAKE_BUILD_TYPE=Release
cmake --build baselines/build --parallel
cd baselines/build
ctest --output-on-failure
```

Executables are `baselines/build/vdstar/vdstar`, `baselines/build/botbin/botbin`, and `baselines/build/botbin/botbin_temporal`. The BASW programs retain their separate build in the repository root. See each baseline's README for its input and output contract and upstream attribution.
