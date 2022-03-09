#!/bin/bash

perf_file="/users/kChen/perfs-mar4.txt"
prio_file="/users/kChen/prios-mar4.txt"
abort_file="/users/kChen/aborts-mar4.txt"

log_files="/users/kChen/log-mar4/"

#init output files
echo "log throughput avg_latency tail_latencies" > $perf_file
echo "log priority_counts" > $prio_file
echo "log aborts_count" > $abort_file

for log in "$log_files"/*
do
	echo $log
	haslong='True'
	IFS=$'\n' res=($(python3 Bamboo-Public/parse_res.py $log $haslong))
	echo ${res[@]}

	perfs=${res[0]}
	#if SILO_PRIO, gather priorities
	if [[ $log == *"PRIO"* ]]; then
		prios=${res[1]}
		aborts=${res[2]}
		lats=${res[3]}
		echo "$log,$perfs,$lats" >> $perf_file
		echo "$log,$prios" >> $prio_file
		echo "$log,$aborts" >> $abort_file
	else
                aborts=${res[1]}
                lats=${res[2]}
                echo "$log,$perfs,$lats" >> $perf_file
                #echo "$log $prios" >> $prio_file
                echo "$log,$aborts" >> $abort_file
	fi
done
