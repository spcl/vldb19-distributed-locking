#!/bin/bash

usage () {
    echo "Script for generating batch job scripts"
    echo "options:" 
    echo "        --dir=PATH (default=PWD)              #absolute path to the project"
    echo "        --workloadsDir=PATH (required)        #absolute path to the folder with workloads"
    echo "                                              #the script will iterate through folders to find workloads"
    echo "        --workloadDir=PATH (required)         #absolute path to the folder with the workload you want to execute"
    echo "                                              #workloadsDir or workloadDir must be specified"
    echo "        --nodes=INT (default=1)               #the number of nodes"
    echo "        --comdelays=INT[,INT] (default=0)     #add a delay in nanosec before each communication"
    echo "                                              #specify delays separated with comma"
    echo ""
    echo " Transaction algorithms:"
    echo " Use the flags you want to use to generate scripts for the following algorithms "
    echo "        --simple2pl                           #simple 2PL "
    echo "        --waitdie                             #wait-die strategy"
    echo "        --nowait                              #nowait lock strategy"
    echo "        --timestamp                           #timestamp lock strategy"
    echo ""
    echo " Job additional arguments: "
    echo "        --jobname=STRING (default=job.sh)            #the name if output script"
    echo "        --jobtime=INT  (default=5)                    #expected time of 1 job in minutes"
}


WORKDIR=${PWD}/
DATADIR=""
THEWORKLOADDIR=""
NODES="1"
COMDELAYS="0"
ALGORITHMS=()
JOBTIME="5"
JOBNAME="job.sh"


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
    --workloadsDir=*)
        DATADIR=`echo $arg | sed -e 's/--workloadsDir=//'`
        DATADIR=`eval echo ${DATADIR}`    # tilde and variable expansion
        ;;
    --workloadDir=*)
        THEWORKLOADDIR=`echo $arg | sed -e 's/--workloadDir=//'`
        THEWORKLOADDIR=`eval echo ${THEWORKLOADDIR}`    # tilde and variable expansion
        ;;
    --nodes=*)
        NODES=`echo $arg | sed -e 's/--nodes=//'`
        NODES=`eval echo ${NODES}`    # tilde and variable expansion
        ;;
    --comdelays=*)
        COMDELAYS=`echo $arg | sed -e 's/--comdelay=//'`
        COMDELAYS=`eval echo ${COMDELAYS}`    # tilde and variable expansion
        ;;
    --jobtime=*)
        JOBTIME=`echo $arg | sed -e 's/--jobtime=//'`
        JOBTIME=`eval echo ${JOBTIME}`    # tilde and variable expansion
        ;;
    --jobname=*)
        JOBNAME=`echo $arg | sed -e 's/--jobname=//'`
        JOBNAME=`eval echo ${JOBNAME}`    # tilde and variable expansion
        ;;
    --simple2pl)
        ALGORITHMS+=("simple2pl")
        ;;
    --waitdie)
        ALGORITHMS+=("waitdie")
        ;;
    --nowait)
        ALGORITHMS+=("nowait")
        ;;
    --timestamp)
        ALGORITHMS+=("timestamp")
        ;;
    esac
done


NUMJOBS=1

convertmins() {
 ((h=${1}/60))
 ((m=${1}%60))
 printf "%02d:%02d:00\n" $h $m
}

if [ -z "$DATADIR" ] &&  [ -z "$THEWORKLOADDIR" ] 
then
      echo "[Error] --workloadsDir or --workloadDir must be specified"
      usage
      exit 1
fi

counter=0
ALGORITHMSSTR=""
for var in "${ALGORITHMS[@]}"
do  
    counter=$(($counter+1))
    if [ -z "${ALGORITHMSSTR}" ]
    then
        ALGORITHMSSTR="\"${var}\""
    else
        ALGORITHMSSTR="${ALGORITHMSSTR} \"${var}\""
    fi
done

NUMJOBS=$((${NUMJOBS}*${counter}))

if [ -z "${ALGORITHMSSTR}" ]
then
      echo "[Error] algorithms are not specified"
      usage
      exit 1
fi

counter=0
COMDELAYSSTR=""
for var in $(echo $COMDELAYS | sed "s/,/ /g")
do
    counter=$(($counter+1))
    if [ -z "${COMDELAYSSTR}" ]
    then
        COMDELAYSSTR="\"${var}\""
    else
        COMDELAYSSTR="${COMDELAYSSTR} \"${var}\""
    fi
done
NUMJOBS=$((${NUMJOBS}*${counter}))

if [ -z "${COMDELAYSSTR}" ]
then
     COMDELAYSSTR=\"0\"
fi

counter=0
WORKLOADSSTR=""

if [ ! -z "$DATADIR" ] 
then
	for entry in $(ls -d ${DATADIR}/*/  )
	do
		if [ -f "$entry/max-locks-per-wh.meta" ]; then
		    counter=$(($counter+1))
		    if [ -z "${WORKLOADSSTR}" ]
		    then
			WORKLOADSSTR="\"$(basename $entry)\""
		    else
			WORKLOADSSTR="${WORKLOADSSTR} \"$(basename $entry)\""
		    fi
		fi
	done
else
	if [ -f "${THEWORKLOADDIR}/max-locks-per-wh.meta" ]; then
		counter=$(($counter+1))
		if [ -z "${WORKLOADSSTR}" ]
		then
			WORKLOADSSTR="\"$(basename ${THEWORKLOADDIR})\""
		else
			WORKLOADSSTR="${WORKLOADSSTR} \"$(basename ${THEWORKLOADDIR})\""
		fi
	fi

fi

NUMJOBS=$((${NUMJOBS}*${counter}))


if [ -z "${WORKLOADSSTR}" ]
then
      echo "[Error] no workload found in ${DATADIR} or ${THEWORKLOADDIR}"
      usage
      exit 1
fi

TOTALTIME=$(convertmins $(($JOBTIME*$NUMJOBS)))
EXECUTABLE="${WORKDIR}/HumongousLock/release/hdb-lock"

if [ ! -f "$EXECUTABLE" ]; then
      echo "[Error] executable is not found at ${EXECUTABLE}"
      usage
      exit 1
fi

cp job.template ${JOBNAME}
sed -i "s/\$JOBNAME/${JOBNAME}/g" ${JOBNAME}
sed -i "s/\$NODES/${NODES}/g" ${JOBNAME}
sed -i "s|\$EXECUTABLE|${EXECUTABLE}|" ${JOBNAME}
sed -i "s|\$TOTALTIME|${TOTALTIME}|" ${JOBNAME}
sed -i "s|\$ALGORITHMS|${ALGORITHMSSTR}|" ${JOBNAME}
sed -i "s|\$DELAYS|${COMDELAYSSTR}|" ${JOBNAME}
sed -i "s|\$WORKLOADS|${WORKLOADSSTR}|" ${JOBNAME}
sed -i "s|\$DATADIR|${DATADIR}|" ${JOBNAME}


echo "Script ${JOBNAME} has been created"

echo "We recommend to run preprocess_${JOBNAME} before submitting ${JOBNAME}"

EXECUTABLE="${WORKDIR}/HumongousLock/release/preprocess"
cp preprocess.template preprocess_${JOBNAME}
chmod +x preprocess_${JOBNAME}
sed -i "s/\$NODES/${NODES}/g" preprocess_${JOBNAME}
sed -i "s|\$EXECUTABLE|${EXECUTABLE}|" preprocess_${JOBNAME}
sed -i "s|\$WORKLOADS|${WORKLOADSSTR}|" preprocess_${JOBNAME}
sed -i "s|\$DATADIR|${DATADIR}|" preprocess_${JOBNAME}


