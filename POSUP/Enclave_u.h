#ifndef ENCLAVE_U_H__
#define ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */

#include "conf.h"

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SGX_OC_CPUIDEX_DEFINED__
#define SGX_OC_CPUIDEX_DEFINED__
void SGX_UBRIDGE(SGX_CDECL, sgx_oc_cpuidex, (int cpuinfo[4], int leaf, int subleaf));
#endif
#ifndef SGX_THREAD_WAIT_UNTRUSTED_EVENT_OCALL_DEFINED__
#define SGX_THREAD_WAIT_UNTRUSTED_EVENT_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_wait_untrusted_event_ocall, (const void* self));
#endif
#ifndef SGX_THREAD_SET_UNTRUSTED_EVENT_OCALL_DEFINED__
#define SGX_THREAD_SET_UNTRUSTED_EVENT_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_untrusted_event_ocall, (const void* waiter));
#endif
#ifndef SGX_THREAD_SETWAIT_UNTRUSTED_EVENTS_OCALL_DEFINED__
#define SGX_THREAD_SETWAIT_UNTRUSTED_EVENTS_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_setwait_untrusted_events_ocall, (const void* waiter, const void* self));
#endif
#ifndef SGX_THREAD_SET_MULTIPLE_UNTRUSTED_EVENTS_OCALL_DEFINED__
#define SGX_THREAD_SET_MULTIPLE_UNTRUSTED_EVENTS_OCALL_DEFINED__
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_multiple_untrusted_events_ocall, (const void** waiters, size_t total));
#endif

sgx_status_t ecall_prepareMemory(sgx_enclave_id_t eid, ORAM_INFO* oram_info);
sgx_status_t ecall_readMeta(sgx_enclave_id_t eid, unsigned char* meta_bucket, unsigned char* meta_stash, ORAM_INFO* oram_info, int recurLev);
sgx_status_t ecall_getStash_meta(sgx_enclave_id_t eid, unsigned char* meta_stash, ORAM_INFO* oram_info);
sgx_status_t ecall_getPath_meta(sgx_enclave_id_t eid, unsigned char* meta_path, ORAM_INFO* oram_info, int recurLev);
sgx_status_t ecall_getPathID(sgx_enclave_id_t eid, int* output);
sgx_status_t ecall_prepareAccess(sgx_enclave_id_t eid, ORAM_INFO* oram_info, int access_structure);
sgx_status_t ecall_readPathData_PORAM(sgx_enclave_id_t eid, unsigned char* blocks_in_bucket, int startBlockIdx, unsigned char* blocks_in_stash, int startStashIdx, unsigned int data_size, ORAM_INFO* oram_info, int recurLevel);
sgx_status_t ecall_writePathData_PORAM(sgx_enclave_id_t eid, unsigned char* blocks_in_bucket, int startBlocktIdx, unsigned char* blocks_in_stash, int startStashIdx, unsigned int data_size, ORAM_INFO* oram_info, int recurLevel, TYPE_ID PathID);
sgx_status_t ecall_readPathData_CORAM(sgx_enclave_id_t eid, unsigned char* blocks_in_bucket, int startBlocktIdx, ORAM_INFO* oram_info, int recurLevel);
sgx_status_t ecall_readStashData_CORAM(sgx_enclave_id_t eid, unsigned char* blocks_in_stash, int startStashIdx, ORAM_INFO* oram_info, int recurLevel);
sgx_status_t ecall_prepareEviction_CORAM(sgx_enclave_id_t eid, unsigned char* meta_bucket, unsigned char* meta_stash, TYPE_ID evictPath, ORAM_INFO* oram_info, int recurLevel);
sgx_status_t ecall_evictCORAM(sgx_enclave_id_t eid, unsigned char* blocks, int startBlocktIdx, int pathLev, ORAM_INFO* oram_info, int recurLevel);
sgx_status_t ecall_getNextPathID(sgx_enclave_id_t eid, ORAM_INFO* oram_info, int recurLevel, TYPE_ID* nextPathID);
sgx_status_t ecall_setSearchKeyword(sgx_enclave_id_t eid, unsigned char* kw, int len, int* kwmap_size, unsigned char* ctr_kwmap);
sgx_status_t ecall_setUpdateKeyword(sgx_enclave_id_t eid, unsigned char* kw, int kw_len, TYPE_ID fileID, int* kwmap_size, unsigned char* ctr_kwmap);
sgx_status_t ecall_scanKWMap(sgx_enclave_id_t eid, unsigned char* input, int len);
sgx_status_t ecall_getNumFileIDs(sgx_enclave_id_t eid, int* numFile);
sgx_status_t ecall_getFileIDs(sgx_enclave_id_t eid, unsigned char* fid_arr);
sgx_status_t ecall_setFileIDs(sgx_enclave_id_t eid, unsigned char* fid_arr, int num_fileIDs);
sgx_status_t ecall_scanEmptyBlock(sgx_enclave_id_t eid, unsigned char* empty_block_arr, long int empty_block_arr_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
