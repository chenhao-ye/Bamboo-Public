#include <iostream>
#include <fstream>
#include "global.h"
#include "helper.h"
#include "stats.h"
#include "mem_alloc.h"
#include <algorithm>
#include <queue>
#include <functional>
#include <vector>

#define BILLION 1000000000UL

void Stats_thd::init(uint64_t thd_id) {
  clear();
  all_debug1 = (uint64_t *)
      _mm_malloc(sizeof(uint64_t) * MAX_TXN_PER_PART, 64);
  all_debug2 = (uint64_t *)
      _mm_malloc(sizeof(uint64_t) * MAX_TXN_PER_PART, 64);
  memset(&all_latency, 0, sizeof(uint64_t) * MAX_TXN_PER_PART);
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
  uint64_t global_prios[16]={0};
  uint64_t total = 0;

  ALL_METRICS(INIT_TOTAL_VAR, INIT_TOTAL_VAR, INIT_TOTAL_VAR)
  for (uint64_t tid = 0; tid < g_thread_cnt; tid ++) {
    ALL_METRICS(SUM_UP_STATS, SUM_UP_STATS, MAX_STATS)
    printf("[tid=%lu] txn_cnt=%lu,abort_cnt=%lu, user_abort_cnt=%lu\n",
        tid, _stats[tid]->txn_cnt, _stats[tid]->abort_cnt,
        _stats[tid]->user_abort_cnt);

     #if CC_ALG == SILO_PRIO || CC_ALG == SILO
     for(int i=0; i<16;i++) {
           std::cout << "priority" << i << "= " << _stats[tid]->prios[i] << ", ";
           global_prios[i]+=_stats[tid]->prios[i];
           total+=_stats[tid]->prios[i];
     }
     std::cout << "\n";
     #endif

     // sort latency
     std::sort(_stats[tid]->all_latency, _stats[tid]->all_latency + _stats[tid]->txn_cnt, std::greater<uint64_t>());
  }
  total_latency = total_latency / total_txn_cnt;
  total_commit_latency = total_commit_latency / total_txn_cnt;
  total_time_man = total_time_man - total_time_wait;
  
  // find p99 95 90 50  latency
  uint64_t tail_idx[4] = {total_txn_cnt * 1, total_txn_cnt * 5, total_txn_cnt * 10, total_txn_cnt * 50};
  for (int i = 0; i < 4; ++i) tail_idx[i] /= 100;
  uint64_t tail_latency[4];

  struct lat_thd
  {
    uint64_t latency, thread;
  };
  auto cmp = [](lat_thd l, lat_thd r) { return l.latency < r.latency; };
  priority_queue<lat_thd, std::vector<lat_thd>, decltype(cmp)> pq(cmp);
  for (uint64_t tid = 0; tid < g_thread_cnt; ++tid) pq.push({_stats[tid]->all_latency[0], tid});
  std::vector<uint64_t> _latency_idx(g_thread_cnt, 1);
  
  uint64_t idx = 0, ii = 0;
  while (ii < 4) {
    if (pq.empty()) {
      std::cout << "Error finding tail latency! \n";
      break;
    }
    lat_thd l = pq.top();
    pq.pop();
    if (idx == tail_idx[ii]) {
      tail_latency[ii] = l.latency;
      ++ii;
    }
    if (_latency_idx[l.thread] < _stats[l.thread]->txn_cnt) {
      pq.push({_stats[l.thread]->all_latency[_latency_idx[l.thread]], l.thread});
      ++_latency_idx[l.thread];
    }
    ++idx;
  }

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
       for(int i=0; i<16 ;i++) outf << "priority " << i << "= " << (double)global_prios[i]/total << ", ";
       outf << "\n";
#endif
      outf << "[tail_latency] p99= " << tail_latency[0] << ", p95= " << tail_latency[1] << \
              ", p90= " << tail_latency[2] << ", p50= " << tail_latency[3] << "\n";
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

  #if CC_ALG == SILO_PRIO || CC_ALG == SILO
  for(int i=0; i<16 ;i++) std::cout << "priority perc" << i << "= " << (double)global_prios[i]/total << ", ";
  std::cout << "\n";
  for(int i=0; i<16 ;i++) std::cout << "priority" << i << "= " << global_prios[i] << ", ";
  std::cout << "\n";
  #endif

  std::cout << "[tail_latency] p99= " << tail_latency[0] << ", p95= " << tail_latency[1] << \
              ", p90= " << tail_latency[2] << ", p50= " << tail_latency[3] << "\n";

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
