#!/bin/bash

set -e
set -o pipefail

SCRIPTDIR="$(dirname "$(readlink -f "$0")")"

(
    mkdir "$SCRIPTDIR/mysql-server"
    cd "$SCRIPTDIR/mysql-server"
    wget https://github.com/mysql/mysql-server/archive/mysql-5.6.27.tar.gz -O - | \
        tar -xz --strip-components=1
    patch -p1 < ../mysql-server.patch
)
