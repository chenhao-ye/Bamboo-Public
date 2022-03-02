cd ..

for i in 0 1 2
do
for zipf in 0.9 0.99 1.2
do
for thd in 1 2 4 8 16 32 64
do
for alg in SILO_PRIO # SILO
do
	python test.py experiments/binary_prio.json ZIPF_THETA=${zipf} THREAD_CNT=${thd} CC_ALG=${alg} > outputs/${i}_${alg}_YCSB_${zipf}_${thd}_output.txt
done
done
done
done

for i in 0 1 2
do
for thd in 1 2 4 8 16 32 64
do
for alg in SILO_PRIO #SILO
do
	python test.py experiments/binary_prio.json THREAD_CNT=${thd} CC_ALG=${alg} WORKLOAD=TPCC > outputs/${i}_${alg}_TPCC_${thd}_output.txt
done
done
done