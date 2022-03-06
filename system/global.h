#pragma once 

#include "stdint.h"
#include <unistd.h>
#include <cstddef>
#include <cstdlib>
#define NDEBUG
#include <cassert>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <typeinfo>
#include <list>
#include <mm_malloc.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <time.h> 
#include <sys/time.h>
#include <math.h>

#if LATCH == LH_MCSLOCK
#include "mcs_spinlock.h"
#endif
#include "pthread.h"
#include "config.h"
#include "stats.h"
#include "dl_detect.h"
#ifndef NOGRAPHITE
#include "carbon_user.h"
#endif
#include "helper.h"

#if ENABLE_NANOLOG
#include "NanoLogCpp17.h"
#define SET_LOG_DEBUG() NanoLog::setLogLevel(NanoLog::LogLevels::DEBUG)
#define SET_LOG_NOTICE() NanoLog::setLogLevel(NanoLog::LogLevels::NOTICE)
#define SET_LOG_WARNING() NanoLog::setLogLevel(NanoLog::LogLevels::WARNING)
#define SET_LOG_ERROR() NanoLog::setLogLevel(NanoLog::LogLevels::ERROR)
#define LOG_DEBUG(msg, ...) NANO_LOG(DEBUG, msg, ##__VA_ARGS__)
#define LOG_NOTICE(msg, ...) NANO_LOG(NOTICE, msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) NANO_LOG(WARNING, msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) NANO_LOG(ERROR, msg, ##__VA_ARGS__)
#else // ENABLE_NANOLOG
#define SET_LOG_DEBUG() (void(0))
#define SET_LOG_NOTICE() (void(0))
#define SET_LOG_WARNING() (void(0))
#define SET_LOG_ERROR() (void(0))
#define LOG_DEBUG(msg, ...) (void(msg))
#define LOG_NOTICE(msg, ...) (void(msg))
#define LOG_WARNING(msg, ...) (void(msg))
#define LOG_ERROR(msg, ...) (void(msg))
#endif // ENABLE_NANOLOG

/**
 * Example usage of NanoLog:
 * 	LOG_DEBUG("DBx%d: %s", 1000, "An Evaluation of Concurrency Control")
 * This line will only be logged if ENABLE_NANOLOG is set.
 */

using namespace std;

class mem_alloc;
class Stats;
class DL_detect;
class Manager;
class Query_queue;
class Plock;
class OptCC;
class VLLMan;

typedef uint32_t UInt32;
typedef int32_t SInt32;
typedef uint64_t UInt64;
typedef int64_t SInt64;

typedef uint64_t ts_t; // time stamp type

/******************************************/
// Global Data Structure 
/******************************************/
extern mem_alloc mem_allocator;
extern Stats stats;
extern DL_detect dl_detector;
extern Manager * glob_manager;
extern Query_queue * query_queue;
extern Plock part_lock_man;
extern OptCC occ_man;
#if CC_ALG == VLL
extern VLLMan vll_man;
#endif

extern bool volatile warmup_finish;
extern bool volatile enable_thread_mem_pool;
extern pthread_barrier_t warmup_bar;
#ifndef NOGRAPHITE
extern carbon_barrier_t enable_barrier;
#endif

/******************************************/
// Global Parameter
/******************************************/
extern bool g_part_alloc;
extern bool g_mem_pad;
extern bool g_prt_lat_distr;
extern UInt32 g_part_cnt;
extern UInt32 g_virtual_part_cnt;
extern UInt32 g_thread_cnt;
extern ts_t g_abort_penalty; 
extern bool g_central_man;
extern UInt32 g_ts_alloc;
extern bool g_key_order;
extern bool g_no_dl;
extern ts_t g_timeout;
extern ts_t g_dl_loop_detect;
extern bool g_ts_batch_alloc;
extern UInt32 g_ts_batch_num;

extern map<string, string> g_params;

// YCSB
extern UInt32 g_cc_alg;
extern ts_t g_query_intvl;
extern UInt32 g_part_per_txn;
extern double g_perc_multi_part;
extern double g_read_perc;
extern double g_write_perc;
extern double g_zipf_theta;
extern UInt64 g_synth_table_size;
extern UInt32 g_req_per_query;
extern UInt32 g_field_per_tuple;
extern UInt32 g_init_parallelism;
extern double g_last_retire;
extern double g_specified_ratio;
extern double g_flip_ratio;
extern double g_long_txn_ratio;
extern double g_long_txn_read_ratio;

// TPCC
extern UInt32 g_num_wh;
extern double g_perc_payment;
extern double g_perc_delivery;
extern double g_perc_orderstatus;
extern double g_perc_stocklevel;
extern double g_perc_neworder;
extern bool g_wh_update;
extern char * output_file;
extern UInt32 g_max_items;
extern UInt32 g_cust_per_dist;

enum RC { RCOK, Commit, Abort, WAIT, ERROR, FINISH};

/* Thread */
typedef uint64_t txnid_t;

/* Txn */
typedef uint64_t txn_t;

/* Table and Row */
typedef uint64_t rid_t; // row id
typedef uint64_t pgid_t; // page id



/* INDEX */
enum latch_t {LATCH_EX, LATCH_SH, LATCH_NONE};
// accessing type determines the latch type on nodes
enum idx_acc_t {INDEX_INSERT, INDEX_READ, INDEX_NONE};
typedef uint64_t idx_key_t; // key id for index
typedef uint64_t (*func_ptr)(idx_key_t);	// part_id func_ptr(index_key);

/* general concurrency control */
enum access_t {RD, WR, XP, SCAN, CM};
/* LOCK */
enum lock_t {LOCK_EX, LOCK_SH, LOCK_NONE };
enum loc_t {RETIRED, OWNERS, WAITERS, LOC_NONE};
enum lock_status {LOCK_DROPPED, LOCK_WAITER, LOCK_OWNER, LOCK_RETIRED};
/* TIMESTAMP */
enum TsType {R_REQ, W_REQ, P_REQ, XP_REQ};
/* TXN STATUS */
// XXX(zhihan): bamboo requires the enumeration order to be unchanged
enum status_t: unsigned int {RUNNING, ABORTED, COMMITED, HOLDING}; 

/* COMMUTATIVE OPERATIONS */
enum com_t {COM_INC, COM_DEC, COM_NONE};


#define MSG(str, args...) { \
	printf("[%s : %d] " str, __FILE__, __LINE__, args); } \
//	printf(args); }

// principal index structure. The workload may decide to use a different 
// index structure for specific purposes. (e.g. non-primary key access should use hash)
#if (INDEX_STRUCT == IDX_BTREE)
#define INDEX		index_btree
#else  // IDX_HASH
#define INDEX		IndexHash
#endif

/************************************************/
// constants
/************************************************/
#ifndef UINT64_MAX
#define UINT64_MAX 		18446744073709551615UL
#endif // UINT64_MAX
