#include "global.h"
#include "txn.h"
#include "row.h"
#include "row_silo_prio.h"
#include "mem_alloc.h"
#include "NanoLogCpp17.h"

#if CC_ALG == SILO_PRIO

void 
Row_silo_prio::init(row_t * row) 
{
	_row = row;
	_tid_word_prio.raw_bits = 0;
}

RC
Row_silo_prio::access(txn_man * txn, TsType type, row_t * local_row) {
	TID_prio_t v, v2;
	const uint32_t prio = txn->prio;
	bool is_owner;
retry:
	v = _tid_word_prio;
	if (v.is_locked()) {
#if DEBUG_SVEN || DEBUG_TMP
		// if (!(++retry_count % 100)) {
		// 	NANO_LOG(LogLevels::NOTICE, "[thd-%lu txn-%lu] row %p TID is locked. retry count-%5u.", txn->get_thd_id(), txn->get_txn_id(), this->_row, retry_count);
		// 	NANO_LOG(LogLevels::NOTICE, "TID: latch-%1u prio_ver-%1u, prio-%2u, ref_cnt-%1u, data_ver-%u\n", v.is_locked(), v.get_prio_ver(), v.get_prio(), v.get_ref_cnt(), v.get_data_ver());
		// }
#endif
		PAUSE
		goto retry;
	}
#if DEBUG_SVEN || DEBUG_TMP
		// retry_count = 0;
#endif
	// for a write, abort if the current priority is higher
	if (prio < v.get_prio()) {
		if (type != R_REQ) return Abort;
		COMPILER_BARRIER
		// reread to ensure we read a consistent copy
		if (v != _tid_word_prio) goto retry;
	}
	v2 = v;
	is_owner = v2.acquire_prio(prio);
	local_row->copy(_row);
	if (!__sync_bool_compare_and_swap(&_tid_word_prio.raw_bits, v.raw_bits,
		v2.raw_bits))
		goto retry;
	txn->last_is_owner = is_owner;
	txn->last_data_ver = v2.get_data_ver();
	if (is_owner) txn->last_prio_ver = v2.get_prio_ver();
	return RCOK;
}

void Row_silo_prio::write(row_t * data) {
	_row->copy(data);
}

#endif
