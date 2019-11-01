#!/bin/bash


echo "Script for generating scripts for reproducability experiments"

PROJECTDIR=""
WORKLOADSDIR=""
DELAYWORKLOADDIR=""

if [ -z "${PROJECTDIR}" ]
then
	echo "The PROJECTDIR must be specified. For example: ${HOME}/HumongousLock/"
	exit 1
fi


if [ -z "${WORKLOADSDIR}" ]
then
	echo "The WORKLOADSDIR must be specified. For example: ${SCRATCH}/tpcc/"
	exit 1
fi


if [ -z "${DELAYWORKLOADDIR}" ]
then
	echo "The DELAYWORKLOADDIR must be specified for delay experiment. For example: ${SCRATCH}/tpcc/serializable_1024/"
	exit 1
fi


for i in {0..7}; 
do 

NODES=$((2**i)); 
./generate_batch_job.sh --dir=${PROJECTDIR} --workloadsDir=${WORKLOADSDIR} --nodes=${NODES} --simple2pl --waitdie --nowait --timestamp --jobname=job${NODES}.sh  

done


 
./generate_batch_job.sh --dir=${PROJECTDIR} --workloadDir=${DELAYWORKLOADDIR} --nodes=8 --delays=0,250,1000,4000,16000,64000,256000,1024000 --simple2pl --waitdie --nowait --timestamp --jobname=delay_job8.sh  
