#!/bin/sh

numArms=$1
horizon=$2
hostname=$3
port=$4
randomSeed=$5

#echo "Inside Client"

cmd="./bandit-agent --numArms $numArms --randomSeed $randomSeed --horizon $horizon --hostname $hostname --port $port"
#echo $cmd
$cmd
