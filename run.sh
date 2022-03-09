#!/bin/bash
LOAD="YCSB"
ALGS=("SILO_PRIO" "SILO" "NO_WAIT" "WOUND_WAIT" "WAIT_DIE")
LONG_RATIO=(0.2)
LONG_READ_RATIO=(1 0.5)
THREAD_CNT=(1 2 4 8 16 32 64)
zipf_thetea=(0.99)
PRIO_BIT_COUNT=4

for a in "${ALGS[@]}"
do
	for r in "${LONG_RATIO[@]}"
	do
		for rr in "${LONG_READ_RATIO[@]}"
		do
			for t in "${THREAD_CNT[@]}"
			do
				for zipf in "${zipf_thetea[@]}"
				do
					python test.py experiments/long_txn.json PRIO_BIT_COUNT=4 CC_ALG=$a LONG_TXN_RATIO=$r LONG_READ_RATIO=$rr THREAD_CNT=$t ZIPF_THETA=$zipf | tee ~/$a-$r-$rr-$t-$zipf.log
					thrupt=$(grep "summary" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
					shrt_50=$(grep "short p50=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
					shrt_90=$(grep "short p90=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
					shrt_99=$(grep "short p99=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
					shrt_999=$(grep "short p999=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
					ln_50=$(grep "long p50=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
					ln_90=$(grep "long p90=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
					ln_99=$(grep "long p99=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
					echo "YCSB $a $r $rr $t $zipf $thrupt $shrt_50 $shrt_90 $shrt_99 $shrt_999 $ln_50 $ln_90 $ln_99" >> ~/perfs.txt
					#exit

				done
			done
		done
	done
done
exit
LOAD="TPCC"
echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~TPCC~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
for t in "${THREAD_CNT[@]}"
do
	exit
	for a in "${ALGS[@]}"
		do
		python test.py experiments/default.json WORKLOAD=$LOAD THREAD_CNT=$t | tee ~/tpcc-$t.log
		thrupt=$(grep "summary" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
		shrt_50=$(grep "short p50=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
        	shrt_90=$(grep "short p90=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
	        shrt_99=$(grep "short p99=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
        	shrt_999=$(grep "short p999=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
 	        ln_50=$(grep "long p50=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
        	ln_90=$(grep "long p90=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
	        ln_99=$(grep "long p99=" ~/$a-$r-$rr-$t-$zipf.log | awk '{print $3}')
       	 	echo "TPCC $a 0 0 $t 0 $thrupt $shrt_50 $shrt_90 $shrt_99 $shrt_999 $ln_50 $ln_90 $ln_99" >> ~/perfs.txt
done

