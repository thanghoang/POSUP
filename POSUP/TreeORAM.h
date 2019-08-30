#pragma once
#include "config.h"
#include "Bucket.h"
#include "Stash.h"
class TreeORAM
{
	

public:

	TreeORAM();
	~TreeORAM();

	int getFullPathIdx(TYPE_ID* fullPath, TYPE_ID pathID, int Height);
	int build(vector<TYPE_ID> &blockIDs, unsigned char* masterKey, unsigned long long &counter, ORAM_INFO* oram_info);


	int enc_decBucket(BUCKET &bucket, int recurLev, ORAM_INFO* oram_info);
	int enc_decStash(STASH &S, int recurLev, ORAM_INFO* oram_info);

	int readBucket_from_file(BUCKET &bucket, TYPE_ID bucketID, TYPE_ID data_size, int recurLev, ORAM_INFO* oram_info);
	int writeBucket_to_file(BUCKET &bucket, TYPE_ID bucketID, TYPE_ID data_size, int recurLev, ORAM_INFO* oram_info);

	int readStash_from_file(STASH &S, TYPE_ID data_size, int recurLevel, ORAM_INFO* oram_info);
	int writeStash_to_file(STASH S, TYPE_ID data_size, int recurLevel, ORAM_INFO* oram_info);


	int readBucket_from_file(unsigned char* metaBucket, unsigned char* blocksBucket, TYPE_ID bucketID, TYPE_ID data_size, int recurLevel, ORAM_INFO* oram_info);
	int writeBucket_to_file(unsigned char* metaBucket, unsigned char* blocksBucket, TYPE_ID bucketID, TYPE_ID data_size, int recurLevel, ORAM_INFO* oram_info);

	int readStash_from_file(unsigned char* metaStash, unsigned char* blocksStash, TYPE_ID data_size, int recurLevel, ORAM_INFO* oram_info);
	int writeStash_to_file(unsigned char* metaStash, unsigned char* blocksStash, TYPE_ID data_size, int recurLevel, ORAM_INFO* oram_info);


	int getFullEvictPathIdx(TYPE_ID *fullPathIdx, string str_evict, int Height);
	string getEvictString(TYPE_ID n_evict, int Height);

	int writeCurrentState_to_file(ORAM_INFO* oram_info);
	int readCurrentState_from_file(ORAM_INFO* oram_info);

	void reserveORAM_disk(ORAM_INFO* oram_info);
	int buildRecursiveORAM(TYPE_POS_MAP* pos_map, int pos_map_size, ORAM_INFO* oram_info);


	
};

