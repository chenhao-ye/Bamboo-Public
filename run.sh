#!/bin/bash
LOADS=("YCSB")
ALGS=("SILO" "SILO_PRIO" "WAIT_DIE" "WOUND_WAIT")
T_CNT=(1 2 4 8 16 32 64)
# LOADS=("TPCC")
# ALGS=("SILO")
# T_CNT=(8)
mkdir exp_result
for a in "${ALGS[@]}"
do
	for l in "${LOADS[@]}"
	do
		#echo "workload:$l"
		for i in "${T_CNT[@]}"
		do
			#echo "thread cnt:$i"
			#echo "python test.py experiments/default.json WORKLOAD=$l THREAD_CNT=$i CC_ALG=$a"
			#python test.py experiments/default.json WORKLOAD=$l THREAD_CNT=$i CC_ALG=$a | tee tmp.txt
			sed -i "8s/.*/#define THREAD_CNT $i/" ./config.h
			sed -i "20s/.*/#define WORKLOAD $l/" ./config.h
			sed -i "43s/.*/#define CC_ALG $a/" ./config.h
			echo "WORKLOAD=$l THREAD_CNT=$i CC_ALG=$a"
			make clean
			make -j
			./rundb | tee tmp.txt
			
			# #res=$(tail -n1 tmp.txt | tr "=, " "\n" | sed -n '1p;0~2p' | sed -n 'p;n' | tail -n +2 | tr "\n" " ")
			# res=$(tail -n2 tmp.txt | head -n1 | tr "=, " "\n" | tail -n +3 | awk 'NR == 1 || NR % 4 == 1' | tr "\n" " ")
			# prios=$(tail -n1 tmp.txt | tr "=, " "\n" | tail -n +4 | awk 'NR == 1 || NR % 5 == 1' | tr "\n" " ")
			# #res=$(tail -n6 tmp.txt | head -n1 | tr "=, " "\n" | awk "NR % 4 == 3"  | tr "\n" " ")
			# #res=$(tail -n6 tmp.txt | head -n1 | tr "=, " "\n" | sed -n '1p;0~2p' | sed -n 'p;n' | tail -n +2 | tr "\n" " ")
			# echo "$a-compensate $l $i $res" >> ~/perfs.txt
			# echo "$a-compensate $l $i $prios" >> ~/prios.txt

			mv tmp.txt exp_result/$a-$l-$i.log
		done
	done
done
