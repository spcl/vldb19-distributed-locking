#!/bin/bash

usage () {
    echo "Script for compiling the project on daint cluster"
    echo "options:" 
    echo "        --dir=PATH (default=PWD) #absolute path to the project"
}


WORKDIR=${PWD}

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
    esac
done

echo "Start Compiling fompi-NA"

cd ${WORKDIR}
tar -xf foMPI-NA-0.2.4.tar.gz
cd fompi-na/
make libfompi.a
rm -f fompi_fortran.o module_fompi.o FOMPI.mod
module load daint-mc
make libfompi.a

echo "fompi-NA has been compiled"

echo "Start Compiling HumongousLock"

module load Boost
cd ${WORKDIR}
cd HumongousLock

echo "USE_CRAY=1" > Makefile.local
echo "FOMPI_LOCATION=${WORKDIR}/fompi-na/" >> Makefile.local

make
cd ${WORKDIR}

echo "HumongousLock has been compiled"


