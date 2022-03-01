#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include "global.h"
#include "helper.h"
#include "stats.h"
#include "mem_alloc.h"

#define BILLION 1000000000UL

void Stats_thd::init(uint64_t thd_id) {
  //latency_record_short = (uint64_t *)
  //    _mm_malloc(sizeof(uint64_t) * MAX_TXN_PER_PART, 64);
  latency_record_long = (uint64_t *)
      _mm_malloc(sizeof(uint64_t) * MAX_TXN_PER_PART * 0.1, 64);

  latency_record = (uint64_t *)
      _mm_malloc(sizeof(uint64_t) * MAX_TXN_PER_PART, 64);
  all_debug1 = (uint64_t *)
      _mm_malloc(sizeof(uint64_t) * MAX_TXN_PER_PART, 64);
  all_debug2 = (uint64_t *)
      _mm_malloc(sizeof(uint64_t) * MAX_TXN_PER_PART, 64);
  clear();
}

void Stats_thd::clear() {
  ALL_METRICS(INIT_VAR, INIT_VAR, INIT_VAR)
#if CC_ALG == SILO_PRIO
  INIT_CNT(uint64_t, prio_txn_cnt, SILO_PRIO_NUM_PRIO_LEVEL);
#if SPLIT_ABORT_COUNT_PRIO
  INIT_CNT(uint64_t, high_prio_abort_txn_cnt, STAT_MAX_NUM_ABORT + 1);
#endif
#endif
  INIT_CNT(uint64_t, abort_txn_cnt, STAT_MAX_NUM_ABORT + 1);
  memset(latency_record_long, 0, sizeof(uint64_t) * MAX_TXN_PER_PART * 0.1);
  latency_record_long_len = 0;
  //memset(latency_record_short, 0, sizeof(uint64_t) * MAX_TXN_PER_PART);
  //latency_record_short_len = 0;
  
  memset(latency_record, 0, sizeof(uint64_t) * MAX_TXN_PER_PART);
  latency_record_len = 0;
}

void Stats_tmp::init() {
  clear();
}

void Stats_tmp::clear() {
  TMP_METRICS(INIT_VAR, INIT_VAR)
}

void Stats::init() {
  if (!STATS_ENABLE)
    return;
  _stats = (Stats_thd**)
      _mm_malloc(sizeof(Stats_thd*) * g_thread_cnt, 64);
  tmp_stats = (Stats_tmp**)
      _mm_malloc(sizeof(Stats_tmp*) * g_thread_cnt, 64);
  dl_detect_time = 0;
  dl_wait_time = 0;
  deadlock = 0;
  cycle_detect = 0;
}

void Stats::init(uint64_t thread_id) {
  if (!STATS_ENABLE)
    return;
  _stats[thread_id] = (Stats_thd *)
      _mm_malloc(sizeof(Stats_thd), 64);
  tmp_stats[thread_id] = (Stats_tmp *)
      _mm_malloc(sizeof(Stats_tmp), 64);

  _stats[thread_id]->init(thread_id);
  tmp_stats[thread_id]->init();
}

void Stats::clear(uint64_t tid) {
  if (STATS_ENABLE) {
    _stats[tid]->clear();
    tmp_stats[tid]->clear();
    dl_detect_time = 0;
    dl_wait_time = 0;
    cycle_detect = 0;
    deadlock = 0;
  }
}

void Stats::add_debug(uint64_t thd_id, uint64_t value, uint32_t select) {
  if (g_prt_lat_distr && warmup_finish) {
    uint64_t tnum = _stats[thd_id]->txn_cnt;
    if (select == 1)
      _stats[thd_id]->all_debug1[tnum] = value;
    else if (select == 2)
      _stats[thd_id]->all_debug2[tnum] = value;
  }
}

void Stats::commit(uint64_t thd_id) {
  if (STATS_ENABLE) {
    _stats[thd_id]->time_man += tmp_stats[thd_id]->time_man;
    _stats[thd_id]->time_index += tmp_stats[thd_id]->time_index;
    _stats[thd_id]->time_wait += tmp_stats[thd_id]->time_wait;
    tmp_stats[thd_id]->init();
  }
}

void Stats::abort(uint64_t thd_id) {
  if (STATS_ENABLE)
    tmp_stats[thd_id]->init();
}

void Stats::print() {
#if CC_ALG == SILO_PRIO
  printf("use_fixed_prio: %s\n", SILO_PRIO_FIXED_PRIO ? "true" : "false");
  printf("inc_prio_after_num_abort: %d\n", SILO_PRIO_INC_PRIO_AFTER_NUM_ABORT);
#endif
  ALL_METRICS(INIT_TOTAL_VAR, INIT_TOTAL_VAR, INIT_TOTAL_VAR)
#if CC_ALG == SILO_PRIO
  INIT_TOTAL_CNT(uint64_t, prio_txn_cnt, SILO_PRIO_NUM_PRIO_LEVEL)
#if SPLIT_ABORT_COUNT_PRIO
  INIT_TOTAL_CNT(uint64_t, high_prio_abort_txn_cnt, STAT_MAX_NUM_ABORT + 1);
#endif
#endif
  INIT_TOTAL_CNT(uint64_t, abort_txn_cnt, STAT_MAX_NUM_ABORT + 1)
  for (uint64_t tid = 0; tid < g_thread_cnt; tid ++) {
    ALL_METRICS(SUM_UP_STATS, SUM_UP_STATS, MAX_STATS)
    printf("[tid=%lu] txn_cnt=%lu,abort_cnt=%lu, user_abort_cnt=%lu\n",
        tid, _stats[tid]->txn_cnt, _stats[tid]->abort_cnt,
        _stats[tid]->user_abort_cnt);
#if CC_ALG == SILO_PRIO
    SUM_UP_CNT(prio_txn_cnt, SILO_PRIO_NUM_PRIO_LEVEL)
    printf("\tprio_txn_cnt = [\n");
    for (int i = 0; i < SILO_PRIO_NUM_PRIO_LEVEL; ++i) \
      if (_stats[tid]->prio_txn_cnt[i])
        printf("\t\t%d: %lu,\n", i, _stats[tid]->prio_txn_cnt[i]);
    printf("\t]\n");
#if SPLIT_ABORT_COUNT_PRIO
    SUM_UP_CNT(high_prio_abort_txn_cnt, SILO_PRIO_NUM_PRIO_LEVEL)
    printf("\thigh_prio_txn_cnt = [\n");
    for (int i = 0; i < STAT_MAX_NUM_ABORT + 1; ++i) \
      if (_stats[tid]->high_prio_abort_txn_cnt[i])
        printf("\t\t%d: %lu,\n", i, _stats[tid]->high_prio_abort_txn_cnt[i]);
    printf("\t]\n");
#endif
#endif
    SUM_UP_CNT(abort_txn_cnt, STAT_MAX_NUM_ABORT + 1)
    printf("\tabort_txn_cnt = [\n");
    for (int i = 0; i < STAT_MAX_NUM_ABORT + 1; ++i) \
      if (_stats[tid]->abort_txn_cnt[i])
        printf("\t\t%d: %lu,\n", i, _stats[tid]->abort_txn_cnt[i]);
    printf("\t]\n");
  }
  total_latency = total_latency / total_txn_cnt;
  total_commit_latency = total_commit_latency / total_txn_cnt;
  total_time_man = total_time_man - total_time_wait;
  if (output_file != NULL) {
    ofstream outf(output_file);
    if (outf.is_open()) {
      outf << "[summary] throughput= " << total_txn_cnt / total_run_time *
      BILLION * THREAD_CNT << " , ";
      ALL_METRICS(WRITE_STAT_X, WRITE_STAT_Y, WRITE_STAT_Y)
      outf << "deadlock_cnt=" << deadlock << ", ";
      outf << "cycle_detect=" << cycle_detect << ", ";
      outf << "dl_detect_time=" << dl_detect_time / BILLION << ", ";
      outf << "dl_wait_time=" << dl_wait_time / BILLION << "\n";
      outf.close();
#if CC_ALG == SILO_PRIO
      PRINT_TOTAL_CNT(outf, prio_txn_cnt, SILO_PRIO_NUM_PRIO_LEVEL)
#if SPLIT_ABORT_COUNT_PRIO
      PRINT_TOTAL_CNT(outf, high_prio_abort_txn_cnt, STAT_MAX_NUM_ABORT + 1)
#endif
#endif
      PRINT_TOTAL_CNT(outf, abort_txn_cnt, STAT_MAX_NUM_ABORT + 1)
    }
  }
  std::cout << "[summary] throughput= " << total_txn_cnt / total_run_time *
      BILLION * THREAD_CNT << " , ";
  ALL_METRICS(PRINT_STAT_X, PRINT_STAT_Y, PRINT_STAT_Y)
  std::cout << "deadlock_cnt=" << deadlock << ", ";
  std::cout << "cycle_detect=" << cycle_detect << ", ";
  std::cout << "dl_detect_time=" << dl_detect_time / BILLION << ", ";
  std::cout << "dl_wait_time=" << dl_wait_time / BILLION << "\n";
#if CC_ALG == SILO_PRIO
  PRINT_TOTAL_CNT(std::cout, prio_txn_cnt, SILO_PRIO_NUM_PRIO_LEVEL)
#if SPLIT_ABORT_COUNT_PRIO
  PRINT_TOTAL_CNT(std::cout, high_prio_abort_txn_cnt, STAT_MAX_NUM_ABORT + 1)
#endif
#endif
  PRINT_TOTAL_CNT(std::cout, abort_txn_cnt, STAT_MAX_NUM_ABORT + 1)

  std::vector<uint64_t> total_latency_record, total_latency_record_long, total_latency_record_short;
  total_latency_record.reserve(MAX_TXN_PER_PART * g_thread_cnt);
  
  for (uint32_t i = 0; i < g_thread_cnt; ++i)
    for (uint64_t j = 0; j < _stats[i]->latency_record_len; ++j)
      total_latency_record.emplace_back(_stats[i]->latency_record[j]);
  std::sort(total_latency_record.begin(), total_latency_record.end());

  // if smaller than 1000, we will have problems for p999
  assert (total_latency_record.size() > 1000);
 std::cout << "all lats count= " << total_latency_record.size() << "\n";
  std::cout << "all p50= " << total_latency_record[total_latency_record.size() * 50 / 100] / 1000.0 << " us\n";
  std::cout << "all p90= " << total_latency_record[total_latency_record.size() * 90 / 100] / 1000.0 << " us\n";
  std::cout << "all p99= " << total_latency_record[total_latency_record.size() * 99 / 100] / 1000.0 << " us\n";
  std::cout << "all p999= " << total_latency_record[total_latency_record.size() * 999 / 1000] / 1000.0 << " us\n";

  if (total_latency_record.size() > 10000)
    std::cout << "all p9999=" << total_latency_record[total_latency_record.size() * 9999 / 10000] / 1000.0 << "us\n";

  ///////////////short
  /*total_latency_record_short.reserve(MAX_TXN_PER_PART * g_thread_cnt);

  for (uint32_t i = 0; i < g_thread_cnt; ++i)
    for (uint64_t j = 0; j < _stats[i]->latency_record_short_len; ++j)
      total_latency_record_short.emplace_back(_stats[i]->latency_record_short[j]);
  std::sort(total_latency_record_short.begin(), total_latency_record_short.end());

  // if smaller than 1000, we will have problems for p999
  assert (total_latency_record_short.size() > 1000);
std::cout << "short txn lats count= "<<  total_latency_record_short.size() << "\n";
  std::cout << "short p50= " << total_latency_record_short[total_latency_record_short.size() * 50 / 100] / 1000.0 << " us\n";
  std::cout << "short p90= " << total_latency_record_short[total_latency_record_short.size() * 90 / 100] / 1000.0 << " us\n";
  std::cout << "short p99= " << total_latency_record_short[total_latency_record_short.size() * 99 / 100] / 1000.0 << " us\n";
  std::cout << "short p999= " << total_latency_record_short[total_latency_record_short.size() * 999 / 1000] / 1000.0 << " us\n";

  if (total_latency_record_short.size() > 10000)
    std::cout << "short p9999=" << total_latency_record_short[total_latency_record_short.size() * 9999 / 10000] / 1000.0 << "us\n";
 */
  //////////////long
  //total_latency_record_long.reserve(MAX_TXN_PER_PART * LONG_TXN_RATIO * 2 * g_thread_cnt);

  total_latency_record_long.reserve(MAX_TXN_PER_PART * g_thread_cnt * 0.1);
  for (uint32_t i = 0; i < g_thread_cnt; ++i)
    for (uint64_t j = 0; j < _stats[i]->latency_record_long_len; ++j)
      total_latency_record_long.emplace_back(_stats[i]->latency_record_long[j]);
  std::sort(total_latency_record_long.begin(), total_latency_record_long.end());

  // if smaller than 100, we will have problems for p99
  assert (total_latency_record_long.size() > 100);
std::cout << "long txn lats count= "<<  total_latency_record_long.size() <<"\n";
  std::cout << "long p50= " << total_latency_record_long[total_latency_record_long.size() * 50 / 100] / 1000.0 << " us\n";
  std::cout << "long p90= " << total_latency_record_long[total_latency_record_long.size() * 90 / 100] / 1000.0 << " us\n";
  std::cout << "long p99= " << total_latency_record_long[total_latency_record_long.size() * 99 / 100] / 1000.0 << " us\n";
  //std::cout << "long p999= " << total_latency_record[total_latency_record.size() * 999 / 1000] / 1000.0 << " us\n";

  if (total_latency_record.size() > 1000)
    std::cout << "long p999=" << total_latency_record_long[total_latency_record_long.size() * 9999 / 10000] / 1000.0 << "us\n";

  // dump the latency distribution in case we want to have a plot
  if (DUMP_LATENCY) {
    std::ofstream of(DUMP_LATENCY_FILENAME);
    for (uint64_t lat: total_latency_record) of << lat << '\n';
  }

  // it doesn't make sense to have a zero-latency txn
  assert(total_latency_record[0] > 0);
  assert(total_latency_record.size() == total_txn_cnt);

  if (g_prt_lat_distr)
    print_lat_distr();
}

void Stats::print_lat_distr() {
  FILE * outf;
  if (output_file != NULL) {
    outf = fopen(output_file, "a");
    for (UInt32 tid = 0; tid < g_thread_cnt; tid ++) {
      fprintf(outf, "[all_debug1 thd=%d] ", tid);
      for (uint32_t tnum = 0; tnum < _stats[tid]->txn_cnt; tnum ++)
        fprintf(outf, "%ld,", _stats[tid]->all_debug1[tnum]);
      fprintf(outf, "\n[all_debug2 thd=%d] ", tid);
      for (uint32_t tnum = 0; tnum < _stats[tid]->txn_cnt; tnum ++)
        fprintf(outf, "%ld,", _stats[tid]->all_debug2[tnum]);
      fprintf(outf, "\n");
    }
    fclose(outf);
  }
}
