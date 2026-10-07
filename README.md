# BASW

Exact structural clustering over sliding-window temporal graph streams.

This repository provides three algorithm implementations and a minimal runnable example:

- **BASW** (`basw`): exact maintenance after each window slide.
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
./build/basw examples/tiny_events.txt output/basw.txt 4 2 3 1 2 2 10
```

On Windows with Visual Studio:

```powershell
.\build\Release\basw_static.exe examples\tiny_events.txt output\static.txt 4 2 3 1 2 2 10
.\build\Release\basw_seq.exe examples\tiny_events.txt output\seq.txt 4 2 3 1 2 2 10
.\build\Release\basw.exe examples\tiny_events.txt output\basw.txt 4 2 3 1 2 2 10
```

The snapshot sections, beginning at `=== snapshot 0 ===`, should be identical across all three methods. Metadata headers differ because they include method-specific fields. Programs create output directories as needed and print a run summary.

## Source layout

- `include/basw/`: graph, window, clustering-state, and runner interfaces.
- `src/`: shared implementation and the three clustering methods.
- `apps/`: the three command-line entry points.
- `examples/tiny_events.txt`: a small temporal stream with repeated interactions.

The CMake configuration builds the three programs and their required shared modules.

## External baselines

The [baselines directory](https://github.com/lisavil/BASW/tree/main/baselines) provides the adapted VD-STAR and BOTBIN source.

## Real-world datasets

These are the ten real datasets described in Section 6.1 and Table 2 of the paper. The links point to the original data releases; Table 2 reports statistics after preprocessing.

| Dataset in the paper | Official source | Download |
| --- | --- | --- |
| Bitcoin-OTC | [SNAP](https://snap.stanford.edu/data/soc-sign-bitcoin-otc.html) | [soc-sign-bitcoinotc.csv.gz](https://snap.stanford.edu/data/soc-sign-bitcoinotc.csv.gz) |
| CollegeMsg | [SNAP](https://snap.stanford.edu/data/CollegeMsg.html) | [CollegeMsg.txt.gz](https://snap.stanford.edu/data/CollegeMsg.txt.gz) |
| Email-Eu-core | [SNAP temporal network](https://snap.stanford.edu/data/email-Eu-core-temporal.html) | [email-Eu-core-temporal.txt.gz](https://snap.stanford.edu/data/email-Eu-core-temporal.txt.gz) |
| sx-MathOverflow | [SNAP](https://snap.stanford.edu/data/sx-mathoverflow.html) | [sx-mathoverflow.txt.gz](https://snap.stanford.edu/data/sx-mathoverflow.txt.gz) |
| sx-AskUbuntu | [SNAP](https://snap.stanford.edu/data/sx-askubuntu.html) | [sx-askubuntu.txt.gz](https://snap.stanford.edu/data/sx-askubuntu.txt.gz) |
| sx-SuperUser | [SNAP](https://snap.stanford.edu/data/sx-superuser.html) | [sx-superuser.txt.gz](https://snap.stanford.edu/data/sx-superuser.txt.gz) |
| Wiki-Talk | [SNAP temporal network](https://snap.stanford.edu/data/wiki-talk-temporal.html) | [wiki-talk-temporal.txt.gz](https://snap.stanford.edu/data/wiki-talk-temporal.txt.gz) |
| sx-StackOverflow | [SNAP](https://snap.stanford.edu/data/sx-stackoverflow.html) | [sx-stackoverflow.txt.gz](https://snap.stanford.edu/data/sx-stackoverflow.txt.gz) |
| Ethereum NFT | [Live Graph Lab](https://livegraphlab.github.io/) | [meta-data.csv.zip](https://zenodo.org/records/8267012/files/meta-data.csv.zip?download=1) ([Google Drive mirror](https://drive.google.com/file/d/1lyCcfGbmU0eW7aHijKSMvmVqBVmwTsmV/view)) |
| OpenSea NFT | [Sliti et al.](https://github.com/slitiWassim/NFT-Suspicious-Activity) | [NFTs_Dataset.zip](https://drive.upm.es/s/sLgeSrNxMEzXaEB/download) ([download page](https://drive.upm.es/s/sLgeSrNxMEzXaEB?openfile=true)) |

Data selection and conversion:

- **SNAP:** use the full temporal Email-Eu-core network, the complete interaction union for each `sx-*` network, and the temporal Wiki-Talk edit stream. Bitcoin-OTC rows are `source,target,rating,timestamp`; discard the rating and represent decimal-second timestamps exactly as integer microseconds. The other SNAP files use `source target timestamp` with timestamps in seconds.
- **Ethereum NFT:** extract `logs-erc721-merge-with-value.csv` from `meta-data.csv.zip`. Use the ERC-721 transfer fields `timestamp`, `from`, and `to`; lowercase addresses and remove transfers involving the zero address and self-loops.
- **OpenSea NFT:** extract `NFTs_Dataset/opensea_nft_transactions.parquet`. Use `closing_date`, `seller_num`, and `buyer_num`, retaining transactions from all ten chains.

Convert the selected records to the `timestamp source destination` input format with integer timestamps, sort by timestamp, remove self-loops, and relabel vertices. Use seconds for the NFT datasets and match all time arguments to the selected timestamp unit. Interpret each interaction as an undirected edge and retain repeated interactions, including those with equal timestamps. Raw data counts can therefore differ from the processed counts in Table 2.
