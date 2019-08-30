#include "Enclave_u.h"
#include <errno.h>

typedef struct ms_ecall_prepareMemory_t {
	ORAM_INFO* ms_oram_info;
} ms_ecall_prepareMemory_t;

typedef struct ms_ecall_readMeta_t {
	unsigned char* ms_meta_bucket;
	unsigned char* ms_meta_stash;
	ORAM_INFO* ms_oram_info;
	int ms_recurLev;
} ms_ecall_readMeta_t;

typedef struct ms_ecall_getStash_meta_t {
	unsigned char* ms_meta_stash;
	ORAM_INFO* ms_oram_info;
} ms_ecall_getStash_meta_t;

typedef struct ms_ecall_getPath_meta_t {
	unsigned char* ms_meta_path;
	ORAM_INFO* ms_oram_info;
	int ms_recurLev;
} ms_ecall_getPath_meta_t;

typedef struct ms_ecall_getPathID_t {
	int* ms_output;
} ms_ecall_getPathID_t;

typedef struct ms_ecall_prepareAccess_t {
	ORAM_INFO* ms_oram_info;
	int ms_access_structure;
} ms_ecall_prepareAccess_t;

typedef struct ms_ecall_readPathData_PORAM_t {
	unsigned char* ms_blocks_in_bucket;
	int ms_startBlockIdx;
	unsigned char* ms_blocks_in_stash;
	int ms_startStashIdx;
	unsigned int ms_data_size;
	ORAM_INFO* ms_oram_info;
	int ms_recurLevel;
} ms_ecall_readPathData_PORAM_t;

typedef struct ms_ecall_writePathData_PORAM_t {
	unsigned char* ms_blocks_in_bucket;
	int ms_startBlocktIdx;
	unsigned char* ms_blocks_in_stash;
	int ms_startStashIdx;
	unsigned int ms_data_size;
	ORAM_INFO* ms_oram_info;
	int ms_recurLevel;
	TYPE_ID ms_PathID;
} ms_ecall_writePathData_PORAM_t;

typedef struct ms_ecall_readPathData_CORAM_t {
	unsigned char* ms_blocks_in_bucket;
	int ms_startBlocktIdx;
	ORAM_INFO* ms_oram_info;
	int ms_recurLevel;
} ms_ecall_readPathData_CORAM_t;

typedef struct ms_ecall_readStashData_CORAM_t {
	unsigned char* ms_blocks_in_stash;
	int ms_startStashIdx;
	ORAM_INFO* ms_oram_info;
	int ms_recurLevel;
} ms_ecall_readStashData_CORAM_t;

typedef struct ms_ecall_prepareEviction_CORAM_t {
	unsigned char* ms_meta_bucket;
	unsigned char* ms_meta_stash;
	TYPE_ID ms_evictPath;
	ORAM_INFO* ms_oram_info;
	int ms_recurLevel;
} ms_ecall_prepareEviction_CORAM_t;

typedef struct ms_ecall_evictCORAM_t {
	unsigned char* ms_blocks;
	int ms_startBlocktIdx;
	int ms_pathLev;
	ORAM_INFO* ms_oram_info;
	int ms_recurLevel;
} ms_ecall_evictCORAM_t;

typedef struct ms_ecall_getNextPathID_t {
	ORAM_INFO* ms_oram_info;
	int ms_recurLevel;
	TYPE_ID* ms_nextPathID;
} ms_ecall_getNextPathID_t;

typedef struct ms_ecall_setSearchKeyword_t {
	unsigned char* ms_kw;
	int ms_len;
	int* ms_kwmap_size;
	unsigned char* ms_ctr_kwmap;
} ms_ecall_setSearchKeyword_t;

typedef struct ms_ecall_setUpdateKeyword_t {
	unsigned char* ms_kw;
	int ms_kw_len;
	TYPE_ID ms_fileID;
	int* ms_kwmap_size;
	unsigned char* ms_ctr_kwmap;
} ms_ecall_setUpdateKeyword_t;

typedef struct ms_ecall_scanKWMap_t {
	unsigned char* ms_input;
	int ms_len;
} ms_ecall_scanKWMap_t;

typedef struct ms_ecall_getNumFileIDs_t {
	int* ms_numFile;
} ms_ecall_getNumFileIDs_t;

typedef struct ms_ecall_getFileIDs_t {
	unsigned char* ms_fid_arr;
} ms_ecall_getFileIDs_t;

typedef struct ms_ecall_setFileIDs_t {
	unsigned char* ms_fid_arr;
	int ms_num_fileIDs;
} ms_ecall_setFileIDs_t;

typedef struct ms_ecall_scanEmptyBlock_t {
	unsigned char* ms_empty_block_arr;
	long int ms_empty_block_arr_len;
} ms_ecall_scanEmptyBlock_t;

typedef struct ms_sgx_oc_cpuidex_t {
	int* ms_cpuinfo;
	int ms_leaf;
	int ms_subleaf;
} ms_sgx_oc_cpuidex_t;

typedef struct ms_sgx_thread_wait_untrusted_event_ocall_t {
	int ms_retval;
	const void* ms_self;
} ms_sgx_thread_wait_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_set_untrusted_event_ocall_t {
	int ms_retval;
	const void* ms_waiter;
} ms_sgx_thread_set_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_setwait_untrusted_events_ocall_t {
	int ms_retval;
	const void* ms_waiter;
	const void* ms_self;
} ms_sgx_thread_setwait_untrusted_events_ocall_t;

typedef struct ms_sgx_thread_set_multiple_untrusted_events_ocall_t {
	int ms_retval;
	const void** ms_waiters;
	size_t ms_total;
} ms_sgx_thread_set_multiple_untrusted_events_ocall_t;

static sgx_status_t SGX_CDECL Enclave_sgx_oc_cpuidex(void* pms)
{
	ms_sgx_oc_cpuidex_t* ms = SGX_CAST(ms_sgx_oc_cpuidex_t*, pms);
	sgx_oc_cpuidex(ms->ms_cpuinfo, ms->ms_leaf, ms->ms_subleaf);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_wait_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_wait_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_wait_untrusted_event_ocall(ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_set_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_set_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_untrusted_event_ocall(ms->ms_waiter);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_setwait_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_setwait_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_setwait_untrusted_events_ocall(ms->ms_waiter, ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_set_multiple_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_multiple_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_multiple_untrusted_events_ocall(ms->ms_waiters, ms->ms_total);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * func_addr[5];
} ocall_table_Enclave = {
	5,
	{
		(void*)(uintptr_t)Enclave_sgx_oc_cpuidex,
		(void*)(uintptr_t)Enclave_sgx_thread_wait_untrusted_event_ocall,
		(void*)(uintptr_t)Enclave_sgx_thread_set_untrusted_event_ocall,
		(void*)(uintptr_t)Enclave_sgx_thread_setwait_untrusted_events_ocall,
		(void*)(uintptr_t)Enclave_sgx_thread_set_multiple_untrusted_events_ocall,
	}
};

sgx_status_t ecall_prepareMemory(sgx_enclave_id_t eid, ORAM_INFO* oram_info)
{
	sgx_status_t status;
	ms_ecall_prepareMemory_t ms;
	ms.ms_oram_info = oram_info;
	status = sgx_ecall(eid, 0, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_readMeta(sgx_enclave_id_t eid, unsigned char* meta_bucket, unsigned char* meta_stash, ORAM_INFO* oram_info, int recurLev)
{
	sgx_status_t status;
	ms_ecall_readMeta_t ms;
	ms.ms_meta_bucket = meta_bucket;
	ms.ms_meta_stash = meta_stash;
	ms.ms_oram_info = oram_info;
	ms.ms_recurLev = recurLev;
	status = sgx_ecall(eid, 1, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_getStash_meta(sgx_enclave_id_t eid, unsigned char* meta_stash, ORAM_INFO* oram_info)
{
	sgx_status_t status;
	ms_ecall_getStash_meta_t ms;
	ms.ms_meta_stash = meta_stash;
	ms.ms_oram_info = oram_info;
	status = sgx_ecall(eid, 2, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_getPath_meta(sgx_enclave_id_t eid, unsigned char* meta_path, ORAM_INFO* oram_info, int recurLev)
{
	sgx_status_t status;
	ms_ecall_getPath_meta_t ms;
	ms.ms_meta_path = meta_path;
	ms.ms_oram_info = oram_info;
	ms.ms_recurLev = recurLev;
	status = sgx_ecall(eid, 3, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_getPathID(sgx_enclave_id_t eid, int* output)
{
	sgx_status_t status;
	ms_ecall_getPathID_t ms;
	ms.ms_output = output;
	status = sgx_ecall(eid, 4, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_prepareAccess(sgx_enclave_id_t eid, ORAM_INFO* oram_info, int access_structure)
{
	sgx_status_t status;
	ms_ecall_prepareAccess_t ms;
	ms.ms_oram_info = oram_info;
	ms.ms_access_structure = access_structure;
	status = sgx_ecall(eid, 5, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_readPathData_PORAM(sgx_enclave_id_t eid, unsigned char* blocks_in_bucket, int startBlockIdx, unsigned char* blocks_in_stash, int startStashIdx, unsigned int data_size, ORAM_INFO* oram_info, int recurLevel)
{
	sgx_status_t status;
	ms_ecall_readPathData_PORAM_t ms;
	ms.ms_blocks_in_bucket = blocks_in_bucket;
	ms.ms_startBlockIdx = startBlockIdx;
	ms.ms_blocks_in_stash = blocks_in_stash;
	ms.ms_startStashIdx = startStashIdx;
	ms.ms_data_size = data_size;
	ms.ms_oram_info = oram_info;
	ms.ms_recurLevel = recurLevel;
	status = sgx_ecall(eid, 6, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_writePathData_PORAM(sgx_enclave_id_t eid, unsigned char* blocks_in_bucket, int startBlocktIdx, unsigned char* blocks_in_stash, int startStashIdx, unsigned int data_size, ORAM_INFO* oram_info, int recurLevel, TYPE_ID PathID)
{
	sgx_status_t status;
	ms_ecall_writePathData_PORAM_t ms;
	ms.ms_blocks_in_bucket = blocks_in_bucket;
	ms.ms_startBlocktIdx = startBlocktIdx;
	ms.ms_blocks_in_stash = blocks_in_stash;
	ms.ms_startStashIdx = startStashIdx;
	ms.ms_data_size = data_size;
	ms.ms_oram_info = oram_info;
	ms.ms_recurLevel = recurLevel;
	ms.ms_PathID = PathID;
	status = sgx_ecall(eid, 7, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_readPathData_CORAM(sgx_enclave_id_t eid, unsigned char* blocks_in_bucket, int startBlocktIdx, ORAM_INFO* oram_info, int recurLevel)
{
	sgx_status_t status;
	ms_ecall_readPathData_CORAM_t ms;
	ms.ms_blocks_in_bucket = blocks_in_bucket;
	ms.ms_startBlocktIdx = startBlocktIdx;
	ms.ms_oram_info = oram_info;
	ms.ms_recurLevel = recurLevel;
	status = sgx_ecall(eid, 8, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_readStashData_CORAM(sgx_enclave_id_t eid, unsigned char* blocks_in_stash, int startStashIdx, ORAM_INFO* oram_info, int recurLevel)
{
	sgx_status_t status;
	ms_ecall_readStashData_CORAM_t ms;
	ms.ms_blocks_in_stash = blocks_in_stash;
	ms.ms_startStashIdx = startStashIdx;
	ms.ms_oram_info = oram_info;
	ms.ms_recurLevel = recurLevel;
	status = sgx_ecall(eid, 9, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_prepareEviction_CORAM(sgx_enclave_id_t eid, unsigned char* meta_bucket, unsigned char* meta_stash, TYPE_ID evictPath, ORAM_INFO* oram_info, int recurLevel)
{
	sgx_status_t status;
	ms_ecall_prepareEviction_CORAM_t ms;
	ms.ms_meta_bucket = meta_bucket;
	ms.ms_meta_stash = meta_stash;
	ms.ms_evictPath = evictPath;
	ms.ms_oram_info = oram_info;
	ms.ms_recurLevel = recurLevel;
	status = sgx_ecall(eid, 10, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_evictCORAM(sgx_enclave_id_t eid, unsigned char* blocks, int startBlocktIdx, int pathLev, ORAM_INFO* oram_info, int recurLevel)
{
	sgx_status_t status;
	ms_ecall_evictCORAM_t ms;
	ms.ms_blocks = blocks;
	ms.ms_startBlocktIdx = startBlocktIdx;
	ms.ms_pathLev = pathLev;
	ms.ms_oram_info = oram_info;
	ms.ms_recurLevel = recurLevel;
	status = sgx_ecall(eid, 11, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_getNextPathID(sgx_enclave_id_t eid, ORAM_INFO* oram_info, int recurLevel, TYPE_ID* nextPathID)
{
	sgx_status_t status;
	ms_ecall_getNextPathID_t ms;
	ms.ms_oram_info = oram_info;
	ms.ms_recurLevel = recurLevel;
	ms.ms_nextPathID = nextPathID;
	status = sgx_ecall(eid, 12, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_setSearchKeyword(sgx_enclave_id_t eid, unsigned char* kw, int len, int* kwmap_size, unsigned char* ctr_kwmap)
{
	sgx_status_t status;
	ms_ecall_setSearchKeyword_t ms;
	ms.ms_kw = kw;
	ms.ms_len = len;
	ms.ms_kwmap_size = kwmap_size;
	ms.ms_ctr_kwmap = ctr_kwmap;
	status = sgx_ecall(eid, 13, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_setUpdateKeyword(sgx_enclave_id_t eid, unsigned char* kw, int kw_len, TYPE_ID fileID, int* kwmap_size, unsigned char* ctr_kwmap)
{
	sgx_status_t status;
	ms_ecall_setUpdateKeyword_t ms;
	ms.ms_kw = kw;
	ms.ms_kw_len = kw_len;
	ms.ms_fileID = fileID;
	ms.ms_kwmap_size = kwmap_size;
	ms.ms_ctr_kwmap = ctr_kwmap;
	status = sgx_ecall(eid, 14, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_scanKWMap(sgx_enclave_id_t eid, unsigned char* input, int len)
{
	sgx_status_t status;
	ms_ecall_scanKWMap_t ms;
	ms.ms_input = input;
	ms.ms_len = len;
	status = sgx_ecall(eid, 15, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_getNumFileIDs(sgx_enclave_id_t eid, int* numFile)
{
	sgx_status_t status;
	ms_ecall_getNumFileIDs_t ms;
	ms.ms_numFile = numFile;
	status = sgx_ecall(eid, 16, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_getFileIDs(sgx_enclave_id_t eid, unsigned char* fid_arr)
{
	sgx_status_t status;
	ms_ecall_getFileIDs_t ms;
	ms.ms_fid_arr = fid_arr;
	status = sgx_ecall(eid, 17, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_setFileIDs(sgx_enclave_id_t eid, unsigned char* fid_arr, int num_fileIDs)
{
	sgx_status_t status;
	ms_ecall_setFileIDs_t ms;
	ms.ms_fid_arr = fid_arr;
	ms.ms_num_fileIDs = num_fileIDs;
	status = sgx_ecall(eid, 18, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_scanEmptyBlock(sgx_enclave_id_t eid, unsigned char* empty_block_arr, long int empty_block_arr_len)
{
	sgx_status_t status;
	ms_ecall_scanEmptyBlock_t ms;
	ms.ms_empty_block_arr = empty_block_arr;
	ms.ms_empty_block_arr_len = empty_block_arr_len;
	status = sgx_ecall(eid, 19, &ocall_table_Enclave, &ms);
	return status;
}

