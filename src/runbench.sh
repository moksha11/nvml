#!/bin/bash
#set -x

INPUTFILE=/tmp/ramdisk/test
LIBBASE=/home/sudarsun/devel/Docs/nvml/src
BASE=$LIBBASE/examples/libpmemobj
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



RUNEXPERIMENT() {
	#PARAM="8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32786 65572"
	CPUS=4

	mkdir $RESULTS

	rm $RESULTS/$resultout

	for ELEMENTSIZE  in `echo $PARAM`
	do
		sudo rm  $INPUTFILE
		FlushDisk
		cd $APPBASE
		#/home/sudarsun/Dropbox/nvml/src/killer.sh &
		$APP_PREFIX "taskset --cpu-list 4,5,6,7 $APP $INPUTFILE"
		#taskset --cpu-list 4,5,6,7 $APP $INPUTFILE
		#gprof "$APP $INPUTFILE"
		#gprof $APP
	done
}

ACIRD_THRESHOLDS(){
#sed -i '/#define EAP_BUDGET_THRESHOLD/c\#define EAP_BUDGET_THRESHOLD 100' $LIBBASE/libpmemobj/tx.c 
./install.sh &>del.txt
}

ACIRD_THRESHOLDS

echo "**********BTREE**************"
echo " "
echo " "
echo " "
APPBASE=$BASE/tree_map
APP=$APPBASE/data_store_btree
PARAM=$1
RUNEXPERIMENT
exit

echo "**********HASHSET**************"
echo " "
echo " "
echo " "
APPBASE=$BASE/hashset
APP=$APPBASE/hashset_tx
PARAM=$1
#RUNEXPERIMENT
#exit

echo "**********BINARY TREE**************"
echo " "
echo " "
echo " "
APPBASE=$BASE/btree_eap
APP=$APPBASE/btree
PARAM=$1
RUNEXPERIMENT
exit

echo "**********SNAPPY*************"
echo " "
echo " "
echo " "
APPBASE=$BASE/snappy
APP=$APPBASE/run_snappy.sh
cd $APPBASE
$APP 0
