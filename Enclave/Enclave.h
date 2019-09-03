#pragma once
#include "BlockSGX.h"
#include "conf.h"
#include <cstring>


#include "iaesni.h"
#include <set>

/* ORAM variables */
TYPE_ID* new_path_id;
BlockSGX** read_block;
BlockSGX** write_block;

TYPE_ID* meta_path;
TYPE_ID* meta_stash;

TYPE_ID* full_path_idx_of_block;
TYPE_ID* full_path_idx;

BlockSGX*** blocks_in_bucket;
BlockSGX*** blocks_in_stash;

TYPE_ID* recursive_block_ids;

unsigned char* ctr_meta_path; 
unsigned char* ctr_data_path;

unsigned char* ctr_data_path_tmp;

unsigned char* ctr_meta_stash;
unsigned char* ctr_data_stash;
unsigned char* ctr_data_stash_tmp;



unsigned char* block_pt;
unsigned char* meta_path_pt; 
unsigned char* meta_stash_pt; 


int pos, pos_tmp, blockLev, blockOrder; // for decryption purpose
unsigned char ctr_tmp[ENCRYPT_BLOCK_SIZE]; // for decryption purpose


sgx_cmac_128bit_tag_t tag;
unsigned char ctr_decrypt[16];
unsigned char ctr_reencrypt[16];
unsigned char buffer[32];


/* ODS variables */
TYPE_ID newPathID;
TYPE_ID nextBlockID;
TYPE_ID prevGen_newPathID;
TYPE_ID nextPathID;


int _ORAM_HEIGHT;



/* Circuit ORAM variable*/
int* deepest;
int* target;
int* deepestIdx;
int* deeperBlockIdx;
TYPE_ID hold_pathID;
TYPE_ID to_write_pathID;
int dest = -2;





/*
* Some global variables
*/

std::vector<TYPE_ID> newPathID_of_file; //for searchable index

std::set<TYPE_ID> lst_fid;
std::set<TYPE_ID>::iterator iter_lst_fid;

long long numFileIDs;
int currOp;

TYPE_ID blockID;
TYPE_ID pathID;


/* Update variables */
char useEmptyBlock;

TYPE_ID update_fileID;
TYPE_ID blockID_2; //use for empty block during update
TYPE_ID pathID_2; //use for empty block during update
TYPE_ID randPath; //to generate new random path for search/update kw to be stored in kwmap



TYPE_ID _ZERO = 0;
TYPE_ID _MINUS_ONE = -1;
int _MINUS_TWO = -2;

/******************************************************Functions*****************************************/

/******************INTERNAL FUNCTIONS*********************/

void inc_dec_ctr(unsigned char* input, unsigned long long number, bool isInc);
TYPE_ID RandBound(TYPE_ID n);

int isEqual(unsigned char* arr1, unsigned char* arr2, int len);

//char ocmp_eq(long long s1, long long s2);
//char ocmp_ne(long long s1, long long s2);

//char ocmp_g(long long s1, long long s2);
//char ocmp_ge(long long s1, long long s2);

void o_clearBlock(uint64_t is_valid_clear, BlockSGX *b);
void o_copyBlock(uint64_t is_valid_clear, BlockSGX *dest, BlockSGX* src);


//size_t o_memcpy(uint64_t is_valid_copy, void *dst, void *src, size_t size);
//size_t o_memset(uint64_t is_valid_set, void *dst, char val, size_t size);

void deserializeBlocks(unsigned char* input, int numBlock, int data_size, BlockSGX** blocks, int recurLevel, bool isStashBlock, int startIdx, bool isReversed);
void serializeBlocks(unsigned char* output, int numBlock, int data_size, BlockSGX** blocks, int recurLevel, bool isStashBlock, int blockIdx, bool isReversed);

int getFullPathIdx(TYPE_ID* fullPath, TYPE_ID pathID, int Height);
int computeRecursiveBlockID(TYPE_ID BlockID, TYPE_ID* output, ORAM_INFO* oram_info);

void updateBlock(int recurLevel, ORAM_INFO *oram_info);





/* Circuit-ORAM functions */
int getDeepestLevel(TYPE_ID PathID, TYPE_ID blockPathID,  ORAM_INFO* oram_info, int recurLevel);
void getDeepestBucketIdx(TYPE_ID* meta_path, TYPE_ID evictPathID, int* output, ORAM_INFO* oram_info, int recurLevel);
int PrepareDeepest(TYPE_ID* meta_path, TYPE_ID PathID, int* deepest, ORAM_INFO* oram_info, int recurLevel);
int getEmptySlot(TYPE_ID* meta_path, int level);
int prepareTarget(TYPE_ID* meta_path, TYPE_ID pathID, int *deepest, int* target,  ORAM_INFO* oram_info, int recurLevel);





/******************EXTERNAL (ECALL) FUNCTIONS*********************/


/* Misc functions */
void ecall_prepareMemory(ORAM_INFO* oram_info);


/* General tree ORAM functions */
void ecall_readMeta(unsigned char* serializedMeta_path, unsigned char* serializedMeta_stash, ORAM_INFO* oram_info, int recurLev);
void ecall_getStash_meta(unsigned char* serializedMeta_stash, ORAM_INFO* oram_info);
void ecall_getPath_meta(unsigned char* serializedMeta_path, ORAM_INFO* oram_info, int recurLev);
void ecall_getPathID(TYPE_ID* output);
void ecall_prepareAccess(ORAM_INFO* oram_info, int access_structure);


/* Path-ORAM functions */
void ecall_readPathData_PORAM(unsigned char* serializedBlocks_in_bucket, int startBlocktIdx, unsigned char* serializedBlocks_in_stash, int startStashIdx, unsigned int data_size, ORAM_INFO* oram_info, int recurLevel);
void ecall_writePathData_PORAM(unsigned char* serializedBlocks_in_bucket, int start_blockt_idx, unsigned char* serializedBlocks_in_stash, int startStashIdx, unsigned int data_size, ORAM_INFO* oram_info, int recurLevel, TYPE_ID PathID);


/* CIRCUIT-ORAM functions */
void ecall_readPathData_CORAM(unsigned char* serializedBlocks_in_bucket, int startBlocktIdx, ORAM_INFO* oram_info, int recurLevel);
void ecall_readStashData_CORAM(unsigned char* serializedBlocks_in_stash, int startStashIdx, ORAM_INFO* oram_info, int recurLevel);
void ecall_prepareEviction_CORAM(unsigned char* meta_bucket, unsigned char* meta_stash, TYPE_ID evictPath, ORAM_INFO* oram_info, int recurLevel);
void ecall_evictCORAM(unsigned char* blocks, int startBlocktIdx, int pathLev, ORAM_INFO* oram_info, int recurLevel);


/* ODS functions */
void ecall_getNextPathID(ORAM_INFO* oram_info, int recurLevel, TYPE_ID* nextPathID);


/* Search & update functions */
void ecall_setSearchKeyword(unsigned char* kw, int len, int* kwmap_size, unsigned char* kwmap_ctr);
void ecall_setUpdateKeyword(unsigned char* kw, int kw_len, TYPE_ID fileID, int* kwmap_size, unsigned char* ctr_kwmap);

void ecall_scanKWMap(unsigned char* input, int len);

void ecall_getNumFileIDs(unsigned long* numFile);
void ecall_getFileIDs(unsigned char* fid_arr);
void ecall_setFileIDs(unsigned char* fid_arr, int num_fileIDs);

void ecall_scanEmptyBlock(unsigned char* empty_block_arr, long empty_block_arr_len);



