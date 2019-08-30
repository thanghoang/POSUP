#pragma once

#include "config.h"
#include "Block.h"
#include "Stash.h"
#include "Bucket.h"


#include "Enclave_u.h"
#include "sgx_urts.h"

class POSUP
{
private:

	unsigned char* masterKey;
	unsigned long long counter;
	TYPE_POS_MAP* pos_map_index;
	TYPE_POS_MAP* pos_map_file;

	sgx_enclave_id_t eid_index;
	sgx_enclave_id_t eid_file;


	/* For empty blocks used during Update */
	int numEmptyBlocks;
	long long empty_block_arr_len;
	unsigned char* empty_block_arr;

#if defined(K_TOP_CACHING)
	Bucket** BUCKETS;
#endif
#if defined(STASH_CACHING)
	STASH* S;
#endif
	STASH* file_pos_map_S;
	Bucket** file_pos_map_bucket;
public:

	POSUP();
	~POSUP();

	static ORAM_INFO* recursion_info_index;
	static ORAM_INFO* recursion_info_file;

/* Initializations */
	int init();
	int initEnclave();
	int buildRecursiveORAMInfo();

/* Setup functions */
	int buildIndexORAM();
	int buildFilesORAM();

/* Save & Load POSUP States */
	int loadState();
	int saveState();


/* Circuit-ORAM controllers */
	int retrieve_CORAM(TYPE_ID pathID, int recurLevel, ORAM_INFO* oram_info);
	int evict_CORAM(TYPE_ID evictPath, int recurLevel, ORAM_INFO* oram_info);

	int ODS_CORAM(TYPE_ID pathID, ORAM_INFO* oram_info);
	int recursive_CORAM(TYPE_ID &pid_output, ORAM_INFO* oram_info);


/* Path-ORAM controllers */
	int retrieve_PORAM(TYPE_ID &pathID, int recurLevel, ORAM_INFO* oram_info);
	int evict_PORAM(TYPE_ID pathID, int recurLevel, ORAM_INFO* oram_info);

	int recursive_PORAM(TYPE_ID &pid_output, ORAM_INFO* oram_info);
	int ODS_PORAM(TYPE_ID pathID, ORAM_INFO* oram_info);


/* POSUP main functions */
	int search(string keyword, int &num);
	int update(string keyword, TYPE_ID fileID);



/* I/O and Miscellanies */
#if defined(K_TOP_CACHING) 	// K_TOP_CACHING functions
	int loadCache_from_disk(ORAM_INFO* oram_info);
	int saveCache_to_disk(ORAM_INFO* oram_info);

	int loadBucket_from_cache(unsigned char* serializedMetaBucket, int* pos_meta, unsigned char* serializedBlockBucket, int* pos, ORAM_INFO* oram_info, TYPE_ID* fullPathIdx, int PathLevel);
	int saveBucket_to_cache(unsigned char* serializedMetaBucket, int* pos_meta, unsigned char* serializedBlockBucket, int* pos, ORAM_INFO* oram_info,  TYPE_ID* fullPathIdx, int PathLevel);
#endif

#if defined(STASH_CACHING)
	int loadStash_from_cache(unsigned char* serializedMetaStash, unsigned char* serializedBlockStash, ORAM_INFO* oram_info);
	int saveStash_to_cache(unsigned char* serializedMetaStash, unsigned char* serializedBlockStash, ORAM_INFO* oram_info);
#endif

	int loadPosMapStash_from_cache(unsigned char* serializedMetaStash, unsigned char* serializedBlockStash, ORAM_INFO* oram_info,int recurLev);
	int savePosMapStash_to_cache(unsigned char* serializedMetaStash, unsigned char* serializedBlockStash, ORAM_INFO* oram_info, int recurLev);
	int loadPosMapBucket_from_cache(unsigned char* serializedMetaBucket, unsigned char* serializedBlockBucket,  ORAM_INFO* oram_info, TYPE_ID* fullPathIdx, int recurLev);
	int savePosMapBucket_to_cache(unsigned char* serializedMetaBucket,  unsigned char* serializedBlockBucket,  ORAM_INFO* oram_info, TYPE_ID* fullPathIdx, int recurLev);
	int loadPosMapCache_from_disk(ORAM_INFO* oram_info);
	int savePosMapCache_to_disk(ORAM_INFO* oram_info);
	



};

