#!/bin/bash

set -e
set -o pipefail

SCRIPTDIR="$(dirname "$(readlink -f "$0")")"

(
    mkdir "$SCRIPTDIR/tpcc-mysql"
    cd "$SCRIPTDIR/tpcc-mysql"
    wget https://github.com/Percona-Lab/tpcc-mysql/archive/1ec1c5.tar.gz -O - | \
        tar -xz --strip-components=1
    patch -p1 < ../tpcc-mysql.patch
)
