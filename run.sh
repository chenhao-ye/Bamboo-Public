#!/bin/bash
LOADS=("TPCC" "YCSB")
ALGS=("BAMBOO" "SILO")

for a in "${ALGS[@]}"
do
	
	for l in "${LOADS[@]}"
	do
		#echo "workload:$l"
		for i in {1..20}
		do
			#echo "thread cnt:$i"
			echo "python test.py experiments/default.json WORKLOAD=$l THREAD_CNT=$i CC_ALG=$a"
			python test.py experiments/default.json WORKLOAD=$l THREAD_CNT=$i CC_ALG=$a | tee tmp.txt
			res=$(tail -n6 tmp.txt | head -n1 | tr "=, " "\n" | sed -n '1p;0~2p' | sed -n 'p;n' | tail -n +2 | tr "\n" " ")
			echo "$a $l $i $res" >> ~/perfs.txt
		done

	done
done
