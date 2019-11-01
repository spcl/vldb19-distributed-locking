# Humongous Lock

Implementation of concurrency control mechanisms on a large-scale compute infrastructure on several thousand processor cores.


## Dependencies

We provide two implementations of  concurrency control mechanisms.
General MPI implementation can be run on any commodity hardware, 
whereas Cray implementation requires Cray Aries network interconnects.

Both implementations require *Boost Serialization* and *Boost Program Options* library

### General MPI implementation 

It requires any MPI implementation such as MPICH or OpenMPI.


### Cray XC40/50 supercomputer implementation

It requires foMPI-NA [1], a scalable MPI RMA library that extends MPI’s
interface with notified accesses such as `MPI_Put_notify`. The project includes the version used in our paper.


## Building

The whole project can be compiled using a single Makefile which is located at  `HumongousLock/Makefile`.
The makefile can configured using Makefile.local, which should be created in the same folder as the main Makefile. 
Here we list arguments which are configurable with Makefile.local 


Possible content of `Makefile.local`:
```
BOOST_LIB=/path/to/boost/lib/
BOOST_INCLUDE=/path/to/boost/include/

USE_CRAY=1 # is used to enable compilation of Cray implementation
FOMPI_LOCATION=/path/to/fompi/ # path to compiled fompi-na. It must be specified for Cray implementation 

USE_LOGGING=1  # is used to enable more detailed latency measurements
DEBUG=1 # is used to enable compilation with debug flags
```

### Cray XC40/50 supercomputer implementation

The Cray implementation requires to be compiled with Cray compiler which can be usually loaded in Cray infrastructure with the following commands:

```bash
module load PrgEnv-cray
module switch PrgEnv-cray PrgEnv-gnu
```

To simplify the build on Daint supercomputer, we provide a bash script (`compile-daint.sh`) for compiling both fompi-na and our code on Daint system. 


## Usage

The project includes two binaries which will be at `HumongousLock/release/` after the compilation.


### hdb-lock

**hdb-lock** is an MPI program which has to be executed within MPI system.
In general MPI system the binary should be executed using *mpirun* command, and
on Cray supercomputers using *srun*. We provide scripts which simplifies 
deploying of hdb-lock. You can read more about them in section **How to run the software**.

The **hdb-lock** expects that each machine will have the same number of *transaction agents* and *lock agents*.
Each MPI process will be a *transaction agents* or *lock agents*. Therefore, 
each machine has the same number of MPI processes. 

MPI allows us to specify how many processes we need per each node. 
**hdb-lock** expects the user to specify only the number of transaction agents per node using `--processespernode` argument,
as the number of lock agents per node can be calculated from the number of processes per node which is provided by MPI. 
In our paper, the number of transaction agents and the number of lock agents is the same. 
During the execution, hdb-lock prints how many agents per node it has.


We provide scripts which simplify deploying of the **hdb-lock**.
Read **Traces** section to learn about the hdb-lock arguments related to TPCC workload.


To read more about the hdb-lock, use its help message:  
 
```
hdb-lock --help
```

### preprocess

**preprocess** binary has been implemented to speed-up trace parsing by serializing it to binary files.
Since execution on the supercomputer often induces some cost, we recommend to preprocess the traces to 
reduce the trace loading time during the execution of the main binary. 
You don't need to worry about **preprocess** if you use our scripts for deploying hdb-lock.
Its input arguments mimic the input arguments of hdb-lock. 

To read more about the preprocess, use its help message:  

```
preprocess --help
```

### TPCC Traces 

You can read about trace generation [here](workload-traces/README.md).
Our programs expect that each workload trace will be in a separate folder.
This folder needs to be accessible under the same path on all machines in the cluster,
so it is typically a network share.
Furthermore, the folder name should contain the isolation level of the trace,
i.e., one of the following: "read_com", "rep_read", "serializable".

The folder with a workload must have `max-locks-per-wh.meta` which contains the maximum number of locks among all warehouses.
We use that number as the number of locks per each warehouse. 

Each warehouse trace should be unpacked and have `*.csv` extension. 
During the execution, **hdb-lock** will create locks for all warehouses and will distribute them among all lock agents. 
Each transaction agent will open a single trace file and execute it. 
Therefore, **hdb-lock** will open warehouse traces as many as the total number of transaction agents. 
So some files will not be used.
If the number of transaction agents is bigger the number of warehouse traces, 
the transaction agents will start sharing warehouse traces. 

Each transaction agent will issue as many transactions as specified with `--localworkloadsize`, unless 
the limit on runtime is exceeded (`--timelimit`). 
If a trace contains less transactions than is required to execute, the transaction agent will re-execute transactions from the beginning until it reaches the limit.
When a trace file is shared between multiple agents, they will read transactions from the trace in round-robin manner. 

At the loading stage, a transaction agent will first try to find a parsed binary file with the trace.
If it does not exist, it will parse the trace and always create one. 
In the subsequent executions, the transaction agent will work with the binary file directly. 
So if you forget to use `preprocess` before an experiment, hdb-lock will preprocess the traces instead.


## How to test the software

hdb-lock can be tested using synthetic workload which can be enabled with --synthetic flag. 
We provide two scripts for testing the code for two configurations: 
*test-general-mpi.sh* and *test-cray-mpi.sh*. 

After compilation you can test the software on your local machine by running:

```bash
# it internally uses mpirun to start the job
 ./test_general_mpi.sh --mpiargs="-H localhost:4"
```

The arguments of scripts mostly mimic the arguments of hdb-lock.
To read more about the tests, use their help messages:
 
```
./test-general-mpi.sh --help
./test-cray-mpi.sh --help
```


## How to run the software

hdb-lock can execute traces generated by our tracing tool. 
We provide two helper scripts for running the code for two configurations: 
*run-general-mpi.sh* and *run-cray-mpi.sh*.

The arguments of scripts mostly mimic the arguments of hdb-lock.
However, they will help with MPI parameters and 
with retrieving the number of warehouses and locks per warehouse from traces.
In addition, the scripts by default will run preprocess to parse the traces and generate bin files. 
You can disable preprocessing using `--nopreprocessing` flag. 

To read more about the scripts, use their help messages:
 
```
./run-general-mpi.sh --help
./run-cray-mpi.sh --help
```

## Output filename format

All the scripts outputs files in the format below. 
All our scripts for plotting data also expect that format.

```
out-{foldername}-{numwhs}-{nodenum}-{lockagents}-{transactionagents}-{comdelay}-{transactionAlg}.exp
```
where:

- foldername - the name of the folder with a workload traces. The test scripts set it to "synthetic".
  - To use our scripts for generating plots, we expect that the folder name includes one of the following: "read_com", "rep_read", "serializable" 
- numwhs - the total number of warehouses
- nodenum - the number of nodes/machines
- lockagents - the number of lock agents per node
- transactionagents - the number of transaction agents per node
- comdelay - communication delay in nanoseconds before each communication
- transactionAlg - the transaction algorithm: "simple2pl", "waitdie",  "nowait", "timestamp" 




## Passing additional options to mpirun
When general MPI implementation is used, there can be a necessity to use 
additional mpirun options such as *--hostfile*. 
hostfile option is used to list hosts on which to launch MPI processes. 
You can read more about it [here](https://www.open-mpi.org/faq/?category=running#mpirun-hostfile). 
Our scripts provide *--mpiargs=* argument to pass such options to mpirun.


## How to reproduce the experiments from the paper

All the experiments have been evaluated on Cray XC40/50 supercomputer using fompi-NA library.
We provide a script for compiling the code on daint supercomputer *compile-daint.sh*,
and also the script (*reproducability_daint.sh*) which generates all the job files used in the plots.
The generated job scripts should be submitted to the job queue of your supercomputer using:

```
sbatch ${JOBNAME}
```

Please, modify manually `reproducability_daint.sh` to add paths to the compiled executables and the workloads.
`reproducability_daint.sh` uses another script (`generate_batch_job.sh`), which helps to batch multiple experiment in one job script. 

To see all parameters of `generate_batch_job.sh`, please, use its help message: 

```
./generate_batch_job.sh --help
```


## References
[1] R. Belli and T. Hoefler. Notified Access: Extending Remote
Memory Access Programming Models for Producer-Consumer Synchronization. In IPDPS, pages 871–881, 2015. D O I :
10.1109/IPDPS.2015.30. 

