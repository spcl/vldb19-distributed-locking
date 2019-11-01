#!/bin/bash

usage () {
    echo "Script for running with mpi"
    echo "options:" 
    echo "        --dir=PATH (default=PWD)              #absolute path to the project"
    echo "        --workloadDir=PATH (required)         #absolute path to the workload"
    echo "        --out=FILENAME (default=out-PARAMS)   #output filename which will be create at --dir"
    echo "        --nodes=INT (default=1)               #the number of nodes"
    echo "        --sockets=INT (default=1)             #the number of sockets per node"
    echo "        --processes=INT (default=16)          #the number of transaction processes per node,"
    echo "                                              #it also specifies the number of lock processes per node"
    echo "        --workloadsize=INT (default=1000000)  #the number of transactions to be executed per client"
    echo "        --timelimit=INT (default=120)         #limit runtime in seconds"
    echo "        --nopreprocessing                     #disable preprocessing of the workload before execution"
    echo "        --comdelay=INT (default=0)            #add a delay in nanosec before each communication"
    echo ""
    echo ""
    echo " Transaction algorithm:"
    echo "        --simple2pl    (default)              #simple 2PL "
    echo "        --waitdie                             #wait-die strategy"
    echo "        --nowait                              #nowait lock strategy"
    echo "        --timestamp                           #timestamp lock strategy"
    echo ""
    echo ""
    echo " MPI additional arguments: "
    echo "        --mpiargs=STRING (default="")         #add additional MPI arguments. Can be used to pass --hostfile"
}


WORKDIR=${PWD}/
DATADIR=""
NODES=1
SOCKETS_PER_NODE=1
TRAN_PROCESSES_PER_NODE=16
WORKLOADSIZE=1000000
COMDELAY=0
PREPROCESSING=1
ALGORITHM="simple2pl"
TIMELIMIT=120
MPIARGS=""
FILENAME=""

for arg in "$@"
do
    case ${arg} in
    --help|-help|-h)
        usage
        exit 1
        ;;
    --dir=*)
        WORKDIR=`echo $arg | sed -e 's/--dir=//'`
        WORKDIR=`eval echo ${WORKDIR}`    # tilde and variable expansion
        ;;
    --workloadDir=*)
        DATADIR=`echo $arg | sed -e 's/--workloadDir=//'`
        DATADIR=`eval echo ${DATADIR}`    # tilde and variable expansion
        ;;
    --nodes=*)
        NODES=`echo $arg | sed -e 's/--nodes=//'`
        NODES=`eval echo ${NODES}`    # tilde and variable expansion
        ;;
    --sockets=*)
        SOCKETS_PER_NODE=`echo $arg | sed -e 's/--sockets=//'`
        SOCKETS_PER_NODE=`eval echo ${SOCKETS_PER_NODE}`    # tilde and variable expansion
        ;;
    --processes=*)
        TRAN_PROCESSES_PER_NODE=`echo $arg | sed -e 's/--processes=//'`
        TRAN_PROCESSES_PER_NODE=`eval echo ${TRAN_PROCESSES_PER_NODE}`    # tilde and variable expansion
        ;;
    --workloadsize=*)
        WORKLOADSIZE=`echo $arg | sed -e 's/--workloadsize=//'`
        WORKLOADSIZE=`eval echo ${WORKLOADSIZE}`    # tilde and variable expansion
        ;;
    --comdelay=*)
        COMDELAY=`echo $arg | sed -e 's/--comdelay=//'`
        COMDELAY=`eval echo ${COMDELAY}`    # tilde and variable expansion
        ;;
    --timelimit=*)
        TIMELIMIT=`echo $arg | sed -e 's/--timelimit=//'`
        TIMELIMIT=`eval echo ${TIMELIMIT}`    # tilde and variable expansion
        ;;
    --nopreprocessing)
        PREPROCESSING=0
        ;;
    --waitdie)
        ALGORITHM="waitdie"
        ;;
    --nowait)
        ALGORITHM="nowait"
        ;;
    --timestamp)
        ALGORITHM="timestamp"
        ;;
    --mpiargs=*)
        MPIARGS=`echo $arg | sed -e 's/--mpiargs=//'`
        MPIARGS=`eval echo ${MPIARGS}`    # tilde and variable expansion
        ;;
    --out=*)
        FILENAME=`echo $arg | sed -e 's/--out=//'`
        FILENAME=`eval echo ${FILENAME}`    # tilde and variable expansion
        ;;
    esac
done

if [ -z "$DATADIR" ]
then
      echo "[Error] --workloadDir is not specified"
      usage
      exit 1
fi

NUMWHS=$(ls ${DATADIR}/*.csv | sed 's!.*/!!' | grep -o -E '[0-9]+' | sort -nr | head -1 |  sed 's/^0*//')
LOCKSPERWH=$(cat ${DATADIR}/max-locks-per-wh.meta | grep -oP '^[^0-9]*\K[0-9]+')

FOLDERNAME=$(basename ${DATADIR})

if [ -z "${FILENAME}" ]; then
OUTFILENAME="out-${FOLDERNAME}-${NUMWHS}-${NODES}-${TRAN_PROCESSES_PER_NODE}-${TRAN_PROCESSES_PER_NODE}-${COMDELAY}-${ALGORITHM}"
else
OUTFILENAME=${FILENAME}
fi


if [ "$PREPROCESSING" -eq "1" ]; then
 echo "Start Preprocessing the workload"
  ${WORKDIR}/HumongousLock/release/preprocess --warehouses=${NUMWHS} --processespernode=${TRAN_PROCESSES_PER_NODE} --locksperwh=${LOCKSPERWH} --localworkloadsize=${WORKLOADSIZE} --workloadlocation=${DATADIR} --numberofnodes=${NODES}
  echo "The workload has been preprocessed"
fi


echo "Start mpi job"

MPIPROC_PER_NODE=$((2*${TRAN_PROCESSES_PER_NODE}))
TOTAL=$((${NODES}*${MPIPROC_PER_NODE}))
MPIPROC_PER_SOCKET=$((${MPIPROC_PER_NODE}/${SOCKETS_PER_NODE}))

mpirun -np ${TOTAL} -npersocket ${MPIPROC_PER_SOCKET} ${MPIARGS} ${WORKDIR}/HumongousLock/release/hdb-lock \
								--timelimit=${TIMELIMIT} \
								--warehouses=${NUMWHS} \
								--processespernode=${TRAN_PROCESSES_PER_NODE} \
								--locksperwh=${LOCKSPERWH} --socketspernode=${SOCKETS_PER_NODE} \
								--localworkloadsize=${WORKLOADSIZE} \
								--workloadlocation=${DATADIR} \
                                --comdelay=${COMDELAY} \
								--logname=${WORKDIR}/${OUTFILENAME} --${ALGORITHM}


echo "mpi job has been finished"

