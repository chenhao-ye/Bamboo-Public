#include "global.h"
#include "txn.h"
#include "row.h"
#include "row_silo_prio.h"
#include "mem_alloc.h"

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
	//uint32_t prio;
	bool is_owner;
retry:
	//re-obtain priority in case of change
	prio = txn->prio;

	v = _tid_word_prio;
	if (v.is_locked()) {
		PAUSE
		goto retry;
	}
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
	//if row metadata did not change since the start of this txn, this can commit
	//otherwise, increment txn priority and retry
	if (!__sync_bool_compare_and_swap(&_tid_word_prio.raw_bits, v.raw_bits,
		v2.raw_bits)) {
		//if (txn->prio<16) ++txn->prio;
		goto retry;
	}
	txn->last_is_owner = is_owner;
	txn->last_data_ver = v2.get_data_ver();
	if (is_owner) txn->last_prio_ver = v2.get_prio_ver();
	return RCOK;
}

void Row_silo_prio::write(row_t * data) {
	_row->copy(data);
}

#endif
