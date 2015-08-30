#!/bin/bash
#set -x

INPUTFILE=/tmp/ramdisk/test
BASE=/home/sudarsun/Dropbox/nvml/src/examples/libpmemobj
APPBASE=$BASE/btree
APP=$APPBASE/btree
OPS=10000
CPULIST="1,2,3,4,5,6,7,8"
THREADCOUNT=4
RESULTS=$BASE/EAPRESULTS
resultout=FILELOGBENCH_ELEMENTSIZE_SENSITIVE.out

FlushDisk()
{
	sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
	sudo sh -c "sync"
	sudo sh -c "sync"
	sudo sh -c "echo 3 > /proc/sys/vm/drop_caches"
}

APPBASE=$BASE/tree_map
APP=$APPBASE/data_store_btree

PARAM=$1
#PARAM="8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32786 65572"
CPUS=4

mkdir $RESULTS

rm $RESULTS/$resultout

for ELEMENTSIZE  in `echo $PARAM`
do
sudo rm  $INPUTFILE
FlushDisk
cd $APPBASE
#~/devel/nvmalloc/scripts/likwid_instrcnt.sh "taskset --cpu-list 1,2,3,4 $APP -i -e $ELEMENTSIZE $THREADCOUNT $OPS $INPUTFILE" &>> $RESULTS/$resultout
#LD_PRELOAD=/usr/lib/librdpmc.so 
#/home/sudarsun/Dropbox/nvml/src/killer.sh &
time taskset --cpu-list 1,2,3,4 $APP $INPUTFILE
#gprof "$APP $INPUTFILE"
#gprof $APP
done


