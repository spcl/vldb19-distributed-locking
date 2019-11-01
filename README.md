# Humongous Lock

Implementation of concurrency control mechanisms on a large-scale compute infrastructure on several thousand processor cores.

This repository hosts the code used for the following paper:
Claude Barthels, Ingo Müller, Konstantin Taranov, Torsten Hoefler, Gustavo Alonso. "Strong consistency is not hard to get: Two-Phase Locking and Two-Phase Commit on Thousands of Cores." In: PVLDB, 2020. [[DOI](https://doi.org/10.14778/3358701.3358702)]

It consists of three parts:
1. Instrumentialized MySQL, TPC-C driver to produce workload traces: [`workload-traces/`](workload-traces/README.md)
1. MPI-based prototype implementations of distributed concurrency control mechanisms: [INSTALL.md](INSTALL.md)
1. Analysis and plotting scripts for the experiment results: [`data/`](data/README.md)

## Git submodules

Clone this repository using `git clone --recursive <URL>` or run the following commands after cloning:

```bash
git submodule init
git submodule update
```
