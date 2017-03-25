#!/bin/bash

PWD=`pwd`

numArms=5
horizon=10
port=5001
hostname="localhost"
banditFile="$PWD/data/instance-01.txt"
randomSeed=0

SERVERDIR=./server
CLIENTDIR=./client

OUTPUTFILE=$PWD/serverlog.txt

pushd $SERVERDIR
cmd="./startserver.sh $numArms $horizon $port $banditFile $randomSeed $OUTPUTFILE &"
#echo $cmd
$cmd 
popd

sleep 1

pushd $CLIENTDIR
cmd="./startclient.sh $numArms $horizon $hostname $port $randomSeed &"
#echo $cmd
$cmd > /dev/null 
popd

sleep 1

cat $OUTPUTFILE | grep "Regret ="
