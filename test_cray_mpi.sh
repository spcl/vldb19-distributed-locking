#!/bin/bash

usage () {
    echo "Script for running with mpi"
    echo "options:" 
    echo "        --dir=PATH (default=PWD)              #absolute path to the project"
    echo "        --out=FILENAME (default=out-PARAMS)   #output filename which will be create at --dir"
    echo "        --nodes=INT (default=1)               #the number of nodes"
    echo "        --sockets=INT (default=2)             #the number of sockets per node"
    echo "        --processes=INT (default=16)          #the number of transaction processes per node,"
    echo "                                              #it also specifies the number of lock processes per node"
    echo "        --locksperwh=INT (default=1024)       #the number of locks per warehouse"
    echo "        --numwhs=INT (default=1024)           #the number of warehouses"
    echo "        --remotelockprob=0-100 (default=0)    #the probability of remote lock"
    echo "        --workloadsize=INT (default=1000000)  #the number of transactions to be executed per client"
    echo "        --timelimit=INT (default=120)         #limit runtime in seconds"
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
    echo " Job additional arguments: "
    echo "        --jobname=STRING (default=job.sh)            #the name if output script"
    echo "        --jobtime=HOURS:MIN:SEC (default=00:18:00)   #add time limit of a job script"
}


WORKDIR=${PWD}/
NODES=1
SOCKETS_PER_NODE=2
TRAN_PROCESSES_PER_NODE=16
WORKLOADSIZE=1000000
COMDELAY=0
ALGORITHM="simple2pl"
TIMELIMIT=120
LOCKSPERWH="1024"
NUMWHS="1024"
REMOTEPROB=0
JOBTIME="00:18:00"
JOBNAME="job.sh"
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
    --losksperwh=*)
        LOCKSPERWH=`echo $arg | sed -e 's/--losksperwh=//'`
        LOCKSPERWH=`eval echo ${LOCKSPERWH}`    # tilde and variable expansion
        ;;
    --numwhs=*)
        NUMWHS=`echo $arg | sed -e 's/--numwhs=//'`
        NUMWHS=`eval echo ${NUMWHS}`    # tilde and variable expansion
        ;;
    --remotelockprob=*)
        REMOTEPROB=`echo $arg | sed -e 's/--remotelockprob=//'`
        REMOTEPROB=`eval echo ${REMOTEPROB}`    # tilde and variable expansion
        ;;
    --jobtime=*)
        JOBTIME=`echo $arg | sed -e 's/--jobtime=//'`
        JOBTIME=`eval echo ${JOBTIME}`    # tilde and variable expansion
        ;;
    --jobname=*)
        JOBNAME=`echo $arg | sed -e 's/--jobname=//'`
        JOBNAME=`eval echo ${JOBNAME}`    # tilde and variable expansion
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
    --out=*)
        FILENAME=`echo $arg | sed -e 's/--out=//'`
        FILENAME=`eval echo ${FILENAME}`    # tilde and variable expansion
        ;;
    esac
done

echo "Start Generating the job script"

if [ -z "${FILENAME}" ]; then
OUTFILENAME="out-synthetic-${NUMWHS}-${NODES}-${TRAN_PROCESSES_PER_NODE}-${TRAN_PROCESSES_PER_NODE}-${COMDELAY}-${ALGORITHM}"
else
OUTFILENAME=${FILENAME}
fi

echo "#!/bin/bash -l" > ${JOBNAME}
echo "#SBATCH --job-name=${OUTFILENAME}"  >> ${JOBNAME}
echo "#SBATCH --time=${JOBTIME}"  >> ${JOBNAME}
echo "#SBATCH --nodes=${NODES}"  >> ${JOBNAME}
echo "#SBATCH --ntasks-per-node=$((${TRAN_PROCESSES_PER_NODE}*2))"  >> ${JOBNAME}
echo "#SBATCH --ntasks-per-core=1"  >> ${JOBNAME}
echo "#SBATCH --partition=normal"  >> ${JOBNAME}
echo "#SBATCH --constraint=mc"  >> ${JOBNAME}
echo "#SBATCH --hint=nomultithread"  >> ${JOBNAME}
echo "" >> ${JOBNAME}

echo "srun ${WORKDIR}/HumongousLock/release/hdb-lock --synthetic --remotelockprob=${REMOTEPROB} --timelimit=${TIMELIMIT} --warehouses=${NUMWHS} --processespernode=${TRAN_PROCESSES_PER_NODE} --locksperwh=${LOCKSPERWH} --socketspernode=${SOCKETS_PER_NODE} --localworkloadsize=${WORKLOADSIZE} --comdelay=${COMDELAY} --logname=${WORKDIR}/${OUTFILENAME} --${ALGORITHM}" >> ${JOBNAME}

echo "The job script has been created"

echo "Now, you can submit the job using:"
echo "sbatch ${JOBNAME}"


