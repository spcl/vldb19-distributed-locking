## Installation

The traces used for the plots in the paper are available for download [here](https://doi.org/10.3929/ethz-b-000374738).
This guide explains how to reproduce them.

This guide assumes that `MAINREPO` is set to the path of this repository.

### Instrumentalized MySQL

Checkout repository:

```bash
git clone git@gitlab.inf.ethz.ch:muellein/mysql-server-tracing.git $MAINREPO/workload-traces/mysql/3rdparty/mysql-server --branch locktrace
# or
$MAINREPO/workload-traces/mysql/3rdparty/download-mysql-server.sh
cd $MAINREPO/workload-traces/mysql/3rdparty/mysql-server
MYSQL_REPO=$PWD
```

Build and install.
If the build fails with a recent compiler, try an older one;
the MySQL version we use is from 2015 and does not compile with some newer compilers
(GCC 6, 7, and 8 work).
Make sure that any user has read acces to `MYSQL_PREFIX`
(for that reason, avoid setting it to a folder under your home folder).

```bash
sudo apt install bison cmake
MYSQL_PREFIX=/mnt/local/muellein/mysql/
mkdir -p $MYSQL_PREFIX $MYSQL_REPO/build
cd $MYSQL_REPO/build
rm -rf *
cmake .. -DCMAKE_INSTALL_PREFIX=$MYSQL_PREFIX/usr/local/mysql -DTMPDIR=$MYSQL_PREFIX/tmp/ -DSYSCONFDIR=$MYSQL_PREFIX/usr/local/mysql
make
make install
```

Copy the config file from this repository.
Modify `innodb_buffer_pool_size` to a value of main memory that you can dedicate to MySQL.
The current value is `200GB`, which may be too much for your machine.

```bash
cp $MAINREPO/workload-traces/mysql/my.cnf $MYSQL_PREFIX/usr/local/mysql/
```

Set up MySQL server:

```bash
sudo groupadd mysql
sudo useradd -g mysql mysql
sudo mkdir $MYSQL_PREFIX/tmp
sudo chown -R mysql:mysql $MYSQL_PREFIX/tmp
cd $MYSQL_PREFIX/usr/local/mysql/
sudo scripts/mysql_install_db --defaults-file=$MYSQL_PREFIX/usr/local/mysql/my.cnf --user=mysql
export PATH=$MYSQL_PREFIX/usr/local/mysql/bin:$PATH
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MYSQL_PREFIX/usr/local/mysql/lib
```

Create a folder for the trace files and export `TRACEDIR` with its path. Start MySQL server:

```bash
# pick one
sudo -u mysql $MYSQL_PREFIX/usr/local/mysql/bin/mysqld --defaults-file=$MYSQL_PREFIX/usr/local/mysql/my.cnf --transaction-isolation READ-COMMITTED 2> $TRACEDIR/server.trc &
sudo -u mysql $MYSQL_PREFIX/usr/local/mysql/bin/mysqld --defaults-file=$MYSQL_PREFIX/usr/local/mysql/my.cnf --transaction-isolation REPEATABLE-READ 2> $TRACEDIR/server.trc &
sudo -u mysql $MYSQL_PREFIX/usr/local/mysql/bin/mysqld --defaults-file=$MYSQL_PREFIX/usr/local/mysql/my.cnf --transaction-isolation SERIALIZABLE 2> $TRACEDIR/server.trc &
```

### Benchmark drivers

#### TPC-C

Checkout repository:

```bash
git clone git@gitlab.inf.ethz.ch:muellein/tpcc-mysql.git $MAINREPO/workload-traces/mysql/3rdparty/tpcc-mysql
# or
$MAINREPO/workload-traces/mysql/3rdparty/download-tpcc-mysql.sh
```

Build and install (this assumes that `PATH` has been modified as described above):

```bash
cd $MAINREPO/workload-traces/mysql/3rdparty/tpcc-mysql
export TPCC_REPO=$PWD
cd $TPCC_REPO/src
make
```

Load data:

```bash
mysql --defaults-file=$MYSQL_PREFIX/usr/local/mysql/my.cnf -u root -e "CREATE DATABASE tpcc;"
mysql --defaults-file=$MYSQL_PREFIX/usr/local/mysql/my.cnf -u root tpcc < $TPCC_REPO/create_table.sql
mysql --defaults-file=$MYSQL_PREFIX/usr/local/mysql/my.cnf -u root tpcc < $TPCC_REPO/add_fkey_idx.sql
$TPCC_REPO/tpcc_load -h 127.0.0.1 -d tpcc -u root -w 2
```

**Restart MySQL** with a fresh trace file, then run the benchmark:

```bash
$TPCC_REPO/tpcc_start -h 127.0.0.1 -d tpcc -u root -w 2 -c 1 -r 10 -l$((1*60)) -i 10 > $TRACEDIR/client.trc
sudo -u mysql killall mysqld
```

##### Configurations used in the paper

Generate data sets for `NUMWH` warehouses using this configuration of `tpcc_load`:
(the paper shows numbers for `NUMWH`=64, 1024, 2048).

```bash
$TPCC_REPO/tpcc_load  -h 127.0.0.1 -d tpcc$NUMWH -u root -w $NUMWH
```

Run benchmark using this configuration of `tpcc_start`
for all data sets and all isolation modes mentioned above
(nine configurations in total, each running for 24 hours):

```bash
$TPCC_REPO/tpcc_start -h 127.0.0.1 -d tpcc$NUMWH -u root -w $NUMWH -c 1 -r 10 -l$((24*60*60)) -i 10
```

#### Common

Backup generated data (to reuse it in another experiment):

```bash
sudo -u mysql killall mysqld
cd $MYSQL_PREFIX/usr/local/mysql
sudo find . -regextype egrep -iregex "./(data/(ibdata.*|.*.ibd|.*.frm|ib_logfile.*)|my.cnf)" \
    | xargs sudo tar -cpSf tpcc-1024wh.tar
```

Restore backup:

```bash
sudo -u mysql killall mysqld
cd $MYSQL_PREFIX/usr/local/mysql
sudo rm -rf $MYSQL_PREFIX/usr/local/mysql/data
sudo scripts/mysql_install_db --user=mysql
sudo tar -xpf tpcc-1024wh.tar
```

### Parse results

Install prerequisites:

```bash
sudo apt install \
    cmake \
    jq \
    libboost-program-options-dev \
    pixz \
    xz-utils
```

Build parsing utility:

```bash
cd $MAINREPO/workload-traces/mysql/tpcc/make_trace/build
cmake ../src
make
```

#### TPC-C

Produce final trace file (you may need to adapt the flags in `SORTFLAGS_BIG` and `SORTFLAGS_SMALL` to fit the main memory of your machine):

```bash
cd $TRACEDIR
make -f $MAINREPO/workload-traces/mysql/tpcc/Makefile-parse.mk
```

The result files are the `wh*.csv.xz` files with the locks traced for each warehouse as well as `max-locks-per-wh.meta` with the maximum number of locks assigned to any warehouse (which defines the size of a lock range of lock table servers).
