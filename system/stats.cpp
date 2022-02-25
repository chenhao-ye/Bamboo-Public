#include <iostream>
#include <fstream>
#include <algorithm>
#include "global.h"
#include "helper.h"
#include "stats.h"
#include "mem_alloc.h"

#define BILLION 1000000000UL

void Stats_thd::init(uint64_t thd_id) {
  clear();
  all_debug1 = (uint64_t *)
      _mm_malloc(sizeof(uint64_t) * MAX_TXN_PER_PART, 64);
  all_debug2 = (uint64_t *)
      _mm_malloc(sizeof(uint64_t) * MAX_TXN_PER_PART, 64);
  memset(prios, 0, 16 * sizeof(uint64_t));
}

void Stats_thd::clear() {
  ALL_METRICS(INIT_VAR, INIT_VAR, INIT_VAR)
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

#if CC_ALG == SILO_PRIO
void Stats::add_latency(uint64_t thd_id, uint64_t latency, uint32_t prio) {
  if (warmup_finish) {
    uint64_t tnum = _stats[thd_id]->prios[prio];
    if (prio == 0)
      _stats[thd_id]->all_debug1[tnum] = latency;
    else if (prio == 1)
      _stats[thd_id]->all_debug2[tnum] = latency;
  }
}
#elif CC_ALG == SILO
void Stats::add_latency(uint64_t thd_id, uint64_t latency, uint32_t prio) {
  if (warmup_finish) {
    uint64_t tnum = _stats[thd_id]->prios[prio];
    if (prio == 0)
      _stats[thd_id]->all_debug1[tnum] = latency;
    else if (prio == 1)
      _stats[thd_id]->all_debug2[tnum] = latency;
  }
}
#endif

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
  uint64_t txn_cnt_of_prio[16]={0};

  ALL_METRICS(INIT_TOTAL_VAR, INIT_TOTAL_VAR, INIT_TOTAL_VAR)
  for (uint64_t tid = 0; tid < g_thread_cnt; tid ++) {
    ALL_METRICS(SUM_UP_STATS, SUM_UP_STATS, MAX_STATS)
    printf("[tid=%lu] txn_cnt=%lu,abort_cnt=%lu, user_abort_cnt=%lu\n",
        tid, _stats[tid]->txn_cnt, _stats[tid]->abort_cnt,
        _stats[tid]->user_abort_cnt);

#if CC_ALG == SILO_PRIO || CC_ALG == SILO
  for(int i = 0; i < 2; i++) {
        std::cout << "priority" << i << "= " << _stats[tid]->prios[i] << ", ";
        txn_cnt_of_prio[i]+=_stats[tid]->prios[i];
  }
  std::cout << "\n";
#endif
  }
  total_latency = total_latency / total_txn_cnt;
  total_commit_latency = total_commit_latency / total_txn_cnt;
  total_time_man = total_time_man - total_time_wait;
  if (output_file != NULL) {
    ofstream outf(output_file);
    if (outf.is_open()) {
      outf << "[summary] throughput= " << total_txn_cnt / total_run_time *
      BILLION * THREAD_CNT << ", ";
      ALL_METRICS(WRITE_STAT_X, WRITE_STAT_Y, WRITE_STAT_Y)
      outf << "deadlock_cnt= " << deadlock << ", ";
      outf << "cycle_detect= " << cycle_detect << ", ";
      outf << "dl_detect_time= " << dl_detect_time / BILLION << ", ";
      outf << "dl_wait_time= " << dl_wait_time / BILLION << "\n";
#if CC_ALG == SILO_PRIO || CC_ALG == SILO
       for(int i=0; i<16 ;i++) outf << "priority " << i << "= " << (double)txn_cnt_of_prio[i] / total_txn_cnt << ", ";
       outf << "\n";
#endif
      outf.close();
    }
  }
  std::cout << "[summary] throughput= " << total_txn_cnt / total_run_time *
      BILLION * THREAD_CNT << ", ";
  ALL_METRICS(PRINT_STAT_X, PRINT_STAT_Y, PRINT_STAT_Y)
  std::cout << "deadlock_cnt= " << deadlock << ", ";
  std::cout << "cycle_detect= " << cycle_detect << ", ";
  std::cout << "dl_detect_time= " << dl_detect_time / BILLION << ", ";
  std::cout << "dl_wait_time= " << dl_wait_time / BILLION << "\n";

  if (g_prt_lat_distr) {
#if CC_ALG == SILO_PRIO || CC_ALG == SILO 
    uint64_t p99, p999;
    for (uint32_t tid = 0; tid < g_thread_cnt; ++tid) {
      p99  = _stats[tid]->prios[0] *  99 /  100 - 1;
      p999 = _stats[tid]->prios[0] * 999 / 1000 - 1;
      std::sort(_stats[tid]->all_debug1, _stats[tid]->all_debug1 + _stats[tid]->prios[0]);
      std::cout << "tid=" << tid << ", prio=0, p99=" << _stats[tid]->all_debug1[p99]
                << ", p999=" << _stats[tid]->all_debug1[p999] << std::endl;
#if CC_ALG == SILO_PRIO
      p99  = _stats[tid]->prios[1] *  99 /  100 - 1;
      p999 = _stats[tid]->prios[1] * 999 / 1000 - 1;
      std::sort(_stats[tid]->all_debug2, _stats[tid]->all_debug2 + _stats[tid]->prios[1]);
      std::cout << "tid=" << tid << ", prio=1, p99=" << _stats[tid]->all_debug2[p99]
                << ", p999=" << _stats[tid]->all_debug2[p999] << std::endl;
#endif
    }
    // total latency
    uint64_t *all_latency = (uint64_t *)malloc(sizeof(uint64_t) * std::max(txn_cnt_of_prio[0], txn_cnt_of_prio[1]));
    if (all_latency) {
      memset(all_latency, 0, sizeof(uint64_t) * std::max(txn_cnt_of_prio[0], txn_cnt_of_prio[1]));
      uint64_t pos = 0;
      for (uint32_t tid = 0; tid < g_thread_cnt; ++tid) {
        memcpy(all_latency + pos, _stats[tid]->all_debug1, sizeof(uint64_t) * _stats[tid]->prios[0]);
        pos += _stats[tid]->prios[0];
      }
      std::sort(all_latency, all_latency + pos);
      p99  = pos *  99 /  100 - 1;
      p999 = pos * 999 / 1000 - 1;
      std::cout << "total" << ", prio=0, p99=" << all_latency[p99]
                << ", p999=" << all_latency[p999] << std::endl;

#if CC_ALG == SILO_PRIO
      memset(all_latency, 0, sizeof(uint64_t) * std::max(txn_cnt_of_prio[0], txn_cnt_of_prio[1]));
      pos = 0;
      for (uint32_t tid = 0; tid < g_thread_cnt; ++tid) {
        memcpy(all_latency + pos, _stats[tid]->all_debug2, sizeof(uint64_t) * _stats[tid]->prios[1]);
        pos += _stats[tid]->prios[1];
      }
      std::sort(all_latency, all_latency + pos);
      p99  = pos *  99 /  100 - 1;
      p999 = pos * 999 / 1000 - 1;
      std::cout << "total" << ", prio=1, p99=" << all_latency[p99]
                << ", p999=" << all_latency[p999] << std::endl;
#endif

      free(all_latency);
    }
    
    // for(int i=0; i<16 ;i++) std::cout << "priority perc" << i << "= " << (double)global_prios[i]/total << ", ";
    // std::cout << "\n";
    // for(int i=0; i<16 ;i++) std::cout << "priority" << i << "= " << global_prios[i] << ", ";
    // std::cout << "\n";
#else
    print_lat_distr();
#endif
  }
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
