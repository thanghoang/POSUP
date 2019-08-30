
/****************************************************************************************
#define __STRALIGN_H_ is here because 
sgx_urts.h -> Windows.h -> stralign.h which uses wcscpy,
a deprecated function from wchar.h
****************************************************************************************/
#define __STRALIGN_H_
#include <stdbool.h>
#define bool _Bool
#include "Enclave_t.h"
//#include "sgx_urts.h"
#include <vector>
#include "Enclave.h"
#include <cstring>

#include <intrin.h>

#include "iaesni.h"
#include <set>




/**
 * Function Name: RandBound
 *
 * Description:
 * Generate a random number in range [0...n]
 *
 * @param n: upper range
 *
 * @return	generated number
 */
TYPE_ID RandBound(TYPE_ID n)
{
	unsigned long int val;
	sgx_read_rand((unsigned char*)&val, sizeof(val));
	return (val %n);
}

int isEqual(unsigned char* arr1, unsigned char* arr2, int len)
{
	for (int i = 0; i < len; i++)
	{
		if (arr1[i] != arr2[i])
		{
			return 0;
		}
	}
	return 1;
}

char ocmp_eq(long long s1, long long s2)
{
	if (s1 == s2)
		return 1;
	return 0;
}

char ocmp_ne(long long s1, long long s2)
{
	char v = ocmp_eq(s1,s2);
	return abs(v-1);
}

char ocmp_g(long long s1, long long s2)
{
	if (s1 > s2)
		return 1;
	return 0;
}

char ocmp_ge(long long s1, long long s2)
{
	if (s1 >= s2)
		return 1;
	return 0;
}

void o_clearBlock(uint64_t is_valid_clear, BlockSGX *b)
{
	TYPE_ID tmp1 = b->getID();
	o_memcpy_8(is_valid_clear, &tmp1, &_ZERO, sizeof(_ZERO));
	b->setID(tmp1);

	o_memset_8(is_valid_clear, b->getDATA(), 0, b->getData_size());
	
	TYPE_ID tmp3 = b->getNextID();
	o_memcpy_8(is_valid_clear, &tmp3, &_ZERO, sizeof(_ZERO));
	b->setNextID(tmp3);

	TYPE_ID tmp4 = b->getNextPathID();
	o_memcpy_8(is_valid_clear, &tmp4, &_MINUS_ONE, sizeof(_MINUS_ONE));
	b->setNextPathID(tmp4);
}

void o_copyBlock(uint64_t is_valid_clear, BlockSGX *dest, BlockSGX* src)
{
	TYPE_ID src_ID, src_nextID, src_nextPathID;
	TYPE_ID dest_ID, dest_nextID, dest_nextPathID;

	src_ID = src->getID();
	src_nextID = src->getNextID();
	src_nextPathID = src->getNextPathID();

	dest_ID = dest->getID();
	dest_nextID = dest->getNextID();
	dest_nextPathID = dest->getNextPathID();

	o_memcpy_8(is_valid_clear, &dest_ID, &src_ID, sizeof(TYPE_ID));
	o_memcpy_8(is_valid_clear, &dest_nextID, &src_nextID, sizeof(TYPE_ID));
	o_memcpy_8(is_valid_clear, &dest_nextPathID, &src_nextPathID, sizeof(TYPE_ID));

	o_memcpy_8(is_valid_clear, dest->getDATA(), src->getDATA(), src->getData_size());

	dest->setID(dest_ID);
	dest->setNextID(dest_nextID);
	dest->setNextPathID(dest_nextPathID);
}

size_t o_memcpy_8(uint64_t is_valid_copy, void *dst, void *src, size_t size)
{
	if (is_valid_copy)
		memcpy(dst, src, size);
	return size;
}

size_t o_memcpy_byte(uint64_t is_valid_copy, void *dst, void *src, size_t size)
{
	if (is_valid_copy)
		memcpy(dst, src, 1);
	return size;
}

size_t o_memset_8(uint64_t is_valid_set, void *dst, char val, size_t size)
{
	if (is_valid_set)
		memset(dst, val, size);
	return size;
}

/**
 * Function Name: inc_dec_ctr
 *
 * Description:
 * Increase/decrease the crypto counter
 *
 * @param input: counter array
 * @param number: amount to be increased/decreased
 * @param isInc: is increase (true) or decrease (false)
 *
 * @return	NULL
 */
void inc_dec_ctr(unsigned char* input, unsigned long long number, bool isInc)
{
	//current implementation supports counter to be upto 64-bits
	
	//reverse ctr first
	unsigned char tmp[8];
	for (int i = 0; i < 8; i++)
	{
		tmp[i] = input[15 - i];
	}
	unsigned long long* ctr = (unsigned long long*)&tmp;

	if (isInc)
	{
		*ctr += number;
	}
	else
	{
		*ctr -= number;
	}
	//reverse it back
	for (int i = 0; i < 8; i++)
	{
		input[15 - i] = tmp[i];
	}
}

/**
 * Function Name: deserializeBlocks
 *
 * Description:
 * Convert the byte array data to block array
 *
 * @param input: byte array (input)
 * @param numBlock: number of blocks
 * @param data_size: size of the data component in a block
 * @param blocks: block structures to be converted
 * @param recurLevel: current recursion level
 * @param isStashBlock: is these blocks are in the stash (true) or otherwise (false)
 * @param blockIdx: index of the block in the path (only used for Path-ORAM eviction)
 * @param isReversed: is the path reversed (only used for Path-ORAM eviction)
 *
 * @return NULL
 */
void deserializeBlocks(unsigned char* input, int numBlock, int data_size, BlockSGX** blocks, int recurLevel, bool isStashBlock, int blockIdx, bool isReversed) //last two params are only for PathORAM eviction
{
	int block_size = sizeof(TYPE_ID) + data_size; 

	if (recurLevel == 0)
	{
		block_size += sizeof(TYPE_ID) + sizeof(TYPE_ID);
	}
	int numCipherBlock = ceil((double)(block_size) / ENCRYPT_BLOCK_SIZE);
	pos_tmp = 0;
	for (int i = 0; i < numBlock; i++)
	{
		blocks[i]->clear();
		if (isStashBlock)
		{
			sgx_aes_ctr_encrypt(master_key, &input[pos_tmp], block_size, ctr_data_stash, ENCRYPT_BLOCK_SIZE*BYTE_SIZE, blocks[i]->getPointer(recurLevel));
		}
		else
		{
			if (isReversed) 
			{
				blockLev = (blockIdx - i) / BUCKET_SIZE;
				blockOrder = blockIdx % BUCKET_SIZE;
				memcpy(ctr_tmp, &ctr_data_path[blockLev*ENCRYPT_BLOCK_SIZE], ENCRYPT_BLOCK_SIZE);
				inc_dec_ctr(ctr_tmp, numCipherBlock* blockOrder, true);
				sgx_aes_ctr_encrypt(master_key, &input[pos_tmp], block_size, ctr_tmp, ENCRYPT_BLOCK_SIZE*BYTE_SIZE, blocks[i]->getPointer(recurLevel));
			}
			else
			{
				blockLev = (blockIdx + i) / BUCKET_SIZE;
				sgx_aes_ctr_encrypt(master_key, &input[pos_tmp], block_size, &ctr_data_path[blockLev*ENCRYPT_BLOCK_SIZE], ENCRYPT_BLOCK_SIZE*BYTE_SIZE, blocks[i]->getPointer(recurLevel));
			}
		}
		pos_tmp += block_size;
		
	}

}

/**
 * Function Name: serializeBlocks
 *
 * Description:
 * Convert the block array data to byte array
 *
 * @param output: byte array (output)
 * @param numBlock: number of blocks
 * @param data_size: size of the data component in a block
 * @param blocks: block structures to be converted
 * @param recurLevel: current recursion level
 * @param isStashBlock: is these blocks are in the stash (true) or otherwise (false)
 * @param blockIdx: index of the block in the path (only used for Path-ORAM eviction)
 * @param isReversed: is the path reversed (only used for Path-ORAM eviction)
 *
 * @return NULL
 */
void serializeBlocks(unsigned char* output, int numBlock, int data_size, BlockSGX** blocks, int recurLevel, bool isStashBlock, int blockIdx, bool isReversed)
{
	int block_size = sizeof(TYPE_ID) + data_size; 
	if (recurLevel == 0)
		block_size += sizeof(TYPE_ID) + sizeof(TYPE_ID);
	pos_tmp = 0;
	int numCipherBlock = ceil((double)block_size / ENCRYPT_BLOCK_SIZE);
	for (int i = 0; i < numBlock; i++)
	{
		if (isStashBlock)
		{
			if ((blockIdx + i) % STASH_SIZE == 0)
			{
				memcpy(ctr_data_stash_tmp, global_counter, ENCRYPT_BLOCK_SIZE);
			}
			sgx_aes_ctr_encrypt(master_key, blocks[i]->getPointer(recurLevel), block_size, global_counter, ENCRYPT_BLOCK_SIZE*BYTE_SIZE, &output[pos_tmp]);
		}
		else
		{
			if (isReversed) 
			{
				blockLev = (blockIdx - i) / BUCKET_SIZE;
				blockOrder = (blockIdx -i) % BUCKET_SIZE; // slot 0 in each bucket
				memcpy(ctr_tmp, global_counter, ENCRYPT_BLOCK_SIZE);
				
				
				inc_dec_ctr(ctr_tmp, numCipherBlock * blockOrder, true);
				sgx_aes_ctr_encrypt(master_key, blocks[i]->getPointer(recurLevel), block_size, ctr_tmp, ENCRYPT_BLOCK_SIZE*BYTE_SIZE, &output[pos_tmp]);
				if (blockOrder == 0)
				{
					memcpy(&ctr_data_path_tmp[blockLev*ENCRYPT_BLOCK_SIZE], global_counter, ENCRYPT_BLOCK_SIZE);
					inc_dec_ctr(ctr_tmp, (BUCKET_SIZE - 1)*numCipherBlock, true);
					memcpy(global_counter, ctr_tmp, ENCRYPT_BLOCK_SIZE);
				}
			}
			else
			{
				blockLev = (blockIdx + i) / BUCKET_SIZE;
				if ((blockIdx + i) % BUCKET_SIZE == 0)
				{
					memcpy(&ctr_data_path_tmp[blockLev*ENCRYPT_BLOCK_SIZE], global_counter, ENCRYPT_BLOCK_SIZE);
				}
				sgx_aes_ctr_encrypt(master_key, blocks[i]->getPointer(recurLevel), block_size, global_counter, ENCRYPT_BLOCK_SIZE*BYTE_SIZE, &output[pos_tmp]);
			}
		}
		pos_tmp += block_size;
	}
}


/**
 * Function Name: getFullPathIdx
 *
 * Description:
 * Get the index of all nodes along the path
 *
 * @param fullPath: index array (output)
 * @param pathID: path ID
 * @param Height: height of the ORAM tree
 *
 * @return NULL
 */
int getFullPathIdx(TYPE_ID* fullPath, TYPE_ID pathID, int Height)
{
	TYPE_ID idx = pathID;
	if (Height == 0)
	{
		fullPath[0] = 0;
		return 0;
	}
	for (int i = Height; i >= 0; i--)
	{
		fullPath[i] = idx;
		idx = (idx - 1) >> 1;
	}
	return 0;
}


/**
 * Function Name: computeRecursiveBlockID
 *
 * Description:
 * Compute a list of block IDs in recursive structure, given the ID of the data block at recursion level 0
 *
 * @param BlockID: Block ID in level 0
 * @param output: block ID array
 * @param oram_info: ORAM info structure
 *
 * @return NULL
 */
int computeRecursiveBlockID(TYPE_ID BlockID, TYPE_ID* output, ORAM_INFO* oram_info)
{
	output[0] = BlockID - 1;
	for (int i = 1; i < _ORAM_HEIGHT; i++)
	{
		output[i] = output[i - 1] / COMPRESSION_RATIO;
	}
	for (int i = 0; i < _ORAM_HEIGHT; i++)
	{
		output[i] += 1;
	}
	return 0;
}


/**
 * Function Name: updateBlock
 *
 * Description:
 * update the Block data
 *
 * @param recurLevel: recursion level
 * @param oram_info: ORAM info structure
 *
 * @return NULL
 */
void updateBlock(int recurLevel, ORAM_INFO *oram_info)
{

	write_block[recurLevel]->clear();
	*write_block[recurLevel] = *read_block[recurLevel];
	
	
	// process position map
	if (recurLevel > 0)
	{
		int numBits = ceil(log2(oram_info[recurLevel - 1].NUM_BLOCKS)) + 1;
		int update_bit_pos = ((recursive_block_ids[recurLevel - 1] - 1) % COMPRESSION_RATIO)*numBits;
		// Update the block, in which new data will be the updated pos map of the retrieved blocks at level i-1
		for (int bit = 0; bit < numBits; bit++)
		{
			if (BIT_CHECK(&new_path_id[recurLevel - 1], bit))
			{
				BIT_SET(&write_block[recurLevel]->getDATA()[(bit + update_bit_pos) / BYTE_SIZE], (bit + update_bit_pos) % BYTE_SIZE);
			}
			else
			{
				BIT_CLEAR(&write_block[recurLevel]->getDATA()[(bit + update_bit_pos) / BYTE_SIZE], (bit + update_bit_pos) % BYTE_SIZE);
			}
		}
	}
	else
	{
		// In case search, read and store file IDs in accessed block
		if (currOp == OP_SEARCH)
		{
			if (oram_info[recurLevel].type == _TYPE_INDEX)
			{
				int numfileIDs = read_block[recurLevel]->getData_size() / sizeof(TYPE_ID);
				for (int i = 0; i < numfileIDs; i++)
				{
					TYPE_ID fileID;
					std::memcpy(&fileID, &read_block[recurLevel]->getDATA()[i * sizeof(TYPE_ID)], sizeof(TYPE_ID));
					if (lst_fid.find(-fileID) == lst_fid.end())
					{
						lst_fid.insert(fileID);
					}
				}

			}

			nextPathID = read_block[recurLevel]->getNextPathID();
			nextBlockID = read_block[recurLevel]->getNextID();

			newPathID = -1;
			char v = ocmp_ne(read_block[recurLevel]->getNextPathID(), -1);
			TYPE_ID tmp = RandBound(oram_info[recurLevel].NUM_LEAVES) + (oram_info[recurLevel].NUM_LEAVES - 1);
			o_memcpy_8(v, &newPathID, &tmp, sizeof(TYPE_ID));
			//if (read_block[recurLevel]->getNextPathID() != -1)
			//	newPathID = RandBound(oram_info[recurLevel].NUM_LEAVES) + (oram_info[recurLevel].NUM_LEAVES - 1);
			//else
			//	newPathID = -1;

			write_block[recurLevel]->setNextPathID(newPathID);
		}
		else // In case update, update fileID to the first empty position found in the first block
		{
			if (oram_info[recurLevel].type == _TYPE_INDEX)
			{
				int n = read_block[recurLevel]->getData_size() / sizeof(TYPE_ID);
				char v2 = 0;
				for (int i = n - 1; i >= 0; i--)
				{
					TYPE_ID fileID;
					std::memcpy(&fileID, &read_block[recurLevel]->getDATA()[i * sizeof(TYPE_ID)], sizeof(TYPE_ID));
					
					char v3 = ocmp_eq(fileID, 0);
					char v4 = ocmp_eq(v2, 0);
					o_memcpy_8(v3&v4 , &write_block[recurLevel]->getDATA()[i * sizeof(TYPE_ID)], &update_fileID, sizeof(TYPE_ID));
					v2 |= v3;

					//if (fileID == 0)
					//{
					//	memcpy(&write_block[recurLevel]->getDATA()[i * sizeof(TYPE_ID)], &update_fileID, sizeof(TYPE_ID));
					//	break;
					//}
				}
				TYPE_ID tmp1 = write_block[recurLevel]->getNextID();
				TYPE_ID tmp2 = write_block[recurLevel]->getNextPathID();

				o_memcpy_8(useEmptyBlock, &tmp1, &blockID_2, sizeof(TYPE_ID));
				o_memcpy_8(useEmptyBlock, &tmp2, &pathID_2, sizeof(TYPE_ID));
				write_block[recurLevel]->setNextID(tmp1);
				write_block[recurLevel]->setNextPathID(tmp2);
				TYPE_ID tmp3 = -1;
				TYPE_ID tmp4 = 0;
				o_memcpy_8(useEmptyBlock, &nextPathID, &tmp3, sizeof(TYPE_ID));
				o_memcpy_8(useEmptyBlock, &nextBlockID, &tmp4, sizeof(TYPE_ID));
				//if (useEmptyBlock) //need oblivious
				//{
				//	write_block[recurLevel]->setNextID(blockID_2);
				//	write_block[recurLevel]->setNextPathID(pathID_2);
				//	nextPathID = -1;
				//	nextBlockID = 0;
				//}
				//else
				//{
				//}
			}
		}
	}

}


/* Circuit-ORAM internal functions */
/**
 * Function Name: getDeepestLevel
 *
 * Description: get deepest intersection between two paths
 *
 *
 * @param PathID: first path (input)
 * @param blockPathID: second path (input)
 * @oram_info: oram info structure
 * @recurLev: recursion level
 * @PathID: path ID
 * @return deepest level
 */
int getDeepestLevel(TYPE_ID PathID, TYPE_ID blockPathID, ORAM_INFO* oram_info, int recurLevel)
{

	getFullPathIdx(full_path_idx, PathID, oram_info[recurLevel].HEIGHT);

	getFullPathIdx(full_path_idx_of_block, blockPathID, oram_info[recurLevel].HEIGHT);
	int ret = -1;
	for (int j = oram_info[recurLevel].HEIGHT; j >= 0; j--)
	{
		char v = ocmp_eq(full_path_idx_of_block[j], full_path_idx[j]);
		char v2 = ocmp_eq(ret, -1);
		v = v & v2;
		o_memcpy_8(v, &ret, &j, sizeof(int));
		//if (full_path_idx_of_block[j] == full_path_idx[j])
		//{
		//	return j;
		//}
	}
	//return -1;
	return ret;
}

/**
 * Function Name: getDeepestBucketIdx
 *
 * Description: get deepest bucket index in each level of the path
 *
 *
 * @param meta_path: meta data along the path (input)
 * @param evictPathID: path  ID (input)
 * @param output: deepest index at each level of the tree (output)
 * @oram_info: oram info structure
 * @recurLev: recursion level
 * @return NULL
 */
void getDeepestBucketIdx(TYPE_ID* meta_path, TYPE_ID evictPathID, int* output, ORAM_INFO* oram_info, int recurLevel)
{
	for (int i = 0; i < oram_info[recurLevel].HEIGHT + 2; i++)
		output[i] = -1;
	int deepest = -1;
	for (int i = 0; i < STASH_SIZE; i++)
	{
		char v = ocmp_ne(meta_stash[i], -1);
		int k = getDeepestLevel(evictPathID, meta_stash[i], oram_info, recurLevel);
		char v2 = ocmp_ge(k, deepest);
		char v3 = v & v2;
		o_memcpy_8(v3, &deepest, &k, sizeof(int));
		o_memcpy_8(v3, &output[0], &i, sizeof(int));
		//if (meta_stash[i] != -1) //NEED OBLIVIOUS
		//{
		//	int k = getDeepestLevel(evictPathID, meta_stash[i], oram_info, recurLevel);
		//	if (k >= deepest) 
		//	{
		//		deepest = k;
		//		output[0] = i;
		//	}
		//}
	}
	for (int i = 0; i < oram_info[recurLevel].HEIGHT + 1; i++)
	{
		deepest = getDeepestLevel(evictPathID, meta_path[i*BUCKET_SIZE], oram_info, recurLevel);
		
		int tmp = 0;
		char v = ocmp_eq(deepest, -1);
		o_memcpy_8(v, &output[i+1], &tmp, sizeof(int));

		//if (deepest != -1)	
		//	output[i + 1] = 0;
		for (int j = 1; j < BUCKET_SIZE; j++)
		{
			int k = getDeepestLevel(evictPathID, meta_path[i*BUCKET_SIZE + j], oram_info, recurLevel);
			
			char v = ocmp_g(k, deepest);
			o_memcpy_8(v, &deepest, &k, sizeof(int));
			o_memcpy_8(v, &output[i+1], &j, sizeof(int));
			//if (k > deepest)
			//{
			//	deepest = k;
			//	output[i + 1] = j;
			//}
		}
	}
}

/**
 * Function Name: PrepareDeepest
 *
 * Description: prepare the deepest array as described in C-ORAM paper
 *
 *
 * @param meta_path: meta data along the path (input)
 * @param evictPathID: path  ID (input)
 * @param deepest: deepest index at each level of the tree (output)
 * @oram_info: oram info structure
 * @recurLev: recursion level
 * @return NULL
 */
int PrepareDeepest(TYPE_ID* meta_path, TYPE_ID PathID, int* deepest, ORAM_INFO* oram_info, int recurLevel)
{
	int goal = -2;
	int src = -2;
	getDeepestBucketIdx(meta_path, PathID, deeperBlockIdx, oram_info, recurLevel);

	for (int i = 0; i < oram_info[recurLevel].HEIGHT + 2; i++)
	{
		deepest[i] = -2;
	}

	//if the stash is not empty
	char v = ocmp_ne(deeperBlockIdx[0], -1);
	int tmp = -1;
	o_memcpy_8(v, &src, &tmp, sizeof(int));
	int tmp2 = getDeepestLevel(PathID, meta_stash[deeperBlockIdx[0]], oram_info, recurLevel);
	o_memcpy_8(v, &goal, &tmp2, sizeof(int));
	//if (deeperBlockIdx[0] != -1)
	//{
	//	src = -1;
	//	goal = getDeepestLevel(PathID, meta_stash[deeperBlockIdx[0]], oram_info, recurLevel);
	//}

	for (int i = 0; i < oram_info[recurLevel].HEIGHT + 1; i++)
	{
		char v = ocmp_ge(goal, i);
		o_memcpy_8(v, &deepest[i], &src, sizeof(int));
		//if (goal >= i)
		//{
		//	deepest[i] = src;
		//}

		int l = -2;

		v = ocmp_ne(deeperBlockIdx[i + 1], -1);
		int tmp = getDeepestLevel(PathID, meta_path[i*BUCKET_SIZE + deeperBlockIdx[i + 1]], oram_info, recurLevel);
		o_memcpy_8(v, &l, &tmp, sizeof(int));
		//if (deeperBlockIdx[i + 1] != -1)
		//	l = getDeepestLevel(PathID, meta_path[i*BUCKET_SIZE + deeperBlockIdx[i + 1]], oram_info, recurLevel);

		v = ocmp_g(l, goal);
		o_memcpy_8(v, &goal, &l, sizeof(int));
		o_memcpy_8(v, &src, &i, sizeof(int));
		//if (l > goal)
		//{
		//	goal = l;
		//	src = i;
		//}
	}

	for (int i = oram_info[recurLevel].HEIGHT + 1; i >= 0; i--)
	{
		deepest[i + 1] = deepest[i];

		v = ocmp_ne(deepest[i + 1], -2);
		tmp = deepest[i + 1] + 1;
		o_memcpy_8(v, &deepest[i + 1], &tmp, sizeof(int));
		//if (deepest[i + 1] != -2)
		//	deepest[i + 1] += 1;
	}
	deepest[0] = -2;
	return 0;
}

/**
 * Function Name: getEmptySlot
 *
 * Description: find an empty slot in a specified level
 *
 *
 * @param meta_path: meta data along the path (input)
 * @param level: path level (input)
 * @return an empty slot or -1 if all full
 */
int getEmptySlot(TYPE_ID* meta_path, int level)
{
	int res = -1;
	for (int i = level * BUCKET_SIZE; i < (level + 1)*BUCKET_SIZE; i++)
	{
		char v = ocmp_eq(meta_path[i], -1);
		int tmp = i % BUCKET_SIZE;
		o_memcpy_8(v, &res, &tmp, sizeof(int));
		//if (meta_path[i] == -1) 
		//	res = i % BUCKET_SIZE;
	}
	return res;
}

/**
 * Function Name: prepareTarget
 *
 * Description: prepare the target array as described in C-ORAM paper
 *
 *
 * @param meta_path: meta data along the path (input)
 * @param evictPathID: path  ID (input)
 * @param deepest: deepest index at each level of the tree (input)
 * @param target: target index at each level of the tree (output)
 * @oram_info: oram info structure
 * @recurLev: recursion level
 * @return NULL
 */
int prepareTarget(TYPE_ID* meta_path, TYPE_ID pathID, int *deepest, int* target, ORAM_INFO* oram_info, int recurLevel)
{
	int dest = -2;
	int src = -2;
	for (int i = 0; i < oram_info[recurLevel].HEIGHT + 2; i++)
	{
		target[i] = -2;
	}
	for (int i = oram_info[recurLevel].HEIGHT + 1; i > 0; i--)
	{
		char v = ocmp_eq(i, src);
		int tmp = -2;
		o_memcpy_8(v, &target[i], &dest, sizeof(int));
		o_memcpy_8(v, &dest, &tmp, sizeof(int));
		o_memcpy_8(v, &src, &tmp, sizeof(int));
		//if (i == src)
		//{
		//	target[i] = dest;
		//	dest = -2;
		//	src = -2;
		//}

		v = ocmp_ge(i, 0);
		char v2 = ocmp_eq(dest, -2);
		tmp = getEmptySlot(meta_path, i - 1);
		char v3 = ocmp_ne(tmp, -1);
		v2 &= v3;
		char v4 = ocmp_ne(target[i], -2);
		v2 |= v4;
		char v5 = ocmp_ne(deepest[i], -2);
		v2 &= v5;
		v2 &= v;
		o_memcpy_8(v2, &src, &deepest[i], sizeof(int));
		o_memcpy_8(v2, &dest, &i, sizeof(int));
		//if (i >= 0)
		//{
		//	if (((dest == -2 && getEmptySlot(meta_path, i - 1) != -1) || target[i] != -2) && deepest[i] != -2)
		//	{
		//		src = deepest[i];
		//		dest = i;
		//	}
		//}
	}
	
	//Stash case:
	char v = ocmp_eq(src, 0);
	o_memcpy_8(v, &target[0], &dest, sizeof(int));
	//if (src == 0)
	//{
	//	target[0] = dest;
	//}
	
	return 0;
}




/* Misc functions */
/**
 * Function Name: ecall_prepareMemory
 *
 * Description:
 * Allocate suffice memory to be used inside SGX for each access operation
 *
 * @param oram_info: ORAM info structure
 *
 * @return NULL
 */
void ecall_prepareMemory(ORAM_INFO *oram_info)
{
	_ORAM_HEIGHT = oram_info[0].N_LEVELS + 1;

	//those 6 variables do not need to be inside sgx. Will try to put it outside of enclave later
	ctr_meta_path = (unsigned char*)malloc(ENCRYPT_BLOCK_SIZE*(oram_info[0].HEIGHT + 1));
	ctr_data_path = (unsigned char*)malloc(ENCRYPT_BLOCK_SIZE*(oram_info[0].HEIGHT + 1));

	ctr_meta_stash = (unsigned char*)malloc(ENCRYPT_BLOCK_SIZE);
	ctr_data_stash = (unsigned char*)malloc(ENCRYPT_BLOCK_SIZE);

	ctr_data_path_tmp = (unsigned char*)malloc(ENCRYPT_BLOCK_SIZE*(oram_info[0].HEIGHT + 1));
	ctr_data_stash_tmp = (unsigned char*)malloc(ENCRYPT_BLOCK_SIZE);


	block_pt = (unsigned char*)malloc(oram_info[0].BLOCK_SIZE);	// ensure the block size of recursive ORAM at level >0 is always smaller than the one  at level 0 (it should be)
	meta_path_pt = (unsigned char*) malloc(BUCKET_SIZE*sizeof(TYPE_ID));
	meta_stash_pt = (unsigned char*) malloc(STASH_SIZE *sizeof(TYPE_ID));


	meta_path = (TYPE_ID*)malloc(sizeof(TYPE_ID)*(BUCKET_SIZE*(oram_info[0].HEIGHT + 1)));
	meta_stash = (TYPE_ID*)malloc(sizeof(TYPE_ID)*STASH_SIZE);


	full_path_idx_of_block = (TYPE_ID*)malloc(sizeof(TYPE_ID)*(oram_info[0].HEIGHT + 1));
	full_path_idx = (TYPE_ID*)malloc(sizeof(TYPE_ID)*(oram_info[0].HEIGHT + 1));


	recursive_block_ids = (TYPE_ID*)malloc(_ORAM_HEIGHT * sizeof(TYPE_ID));
	new_path_id = (TYPE_ID*)malloc(_ORAM_HEIGHT * sizeof(TYPE_ID));


	read_block = new BlockSGX*[_ORAM_HEIGHT];
	write_block = new BlockSGX*[_ORAM_HEIGHT];


	blocks_in_bucket = new BlockSGX**[_ORAM_HEIGHT];
	blocks_in_stash = new BlockSGX**[_ORAM_HEIGHT];

	for (int i = 0; i < _ORAM_HEIGHT; i++)
	{
		int addSize = 0;
		if (i > 0)
		{
			addSize = sizeof(TYPE_ID) + sizeof(TYPE_ID); // next ID & next Path
		}

		read_block[i] = (BlockSGX*)malloc(oram_info[i].BLOCK_SIZE + sizeof(int) + addSize);
		write_block[i] = (BlockSGX*)malloc(oram_info[i].BLOCK_SIZE + sizeof(int) + addSize);
		
		read_block[i]->setData_size(oram_info[i].DATA_SIZE);
		write_block[i]->setData_size(oram_info[i].DATA_SIZE);

		blocks_in_bucket[i] = new BlockSGX*[NUMBLOCK_PATH_LOAD];
		blocks_in_stash[i] = new BlockSGX*[NUMBLOCK_STASH_LOAD];
		for (int ii = 0; ii < NUMBLOCK_PATH_LOAD; ii++)
		{
			blocks_in_bucket[i][ii] = (BlockSGX*)malloc(oram_info[i].BLOCK_SIZE + sizeof(int) + addSize);
			blocks_in_bucket[i][ii]->setData_size(oram_info[i].DATA_SIZE);
		}
		for (int ii = 0; ii < NUMBLOCK_STASH_LOAD; ii++)
		{
			blocks_in_stash[i][ii] = (BlockSGX*)malloc(oram_info[i].BLOCK_SIZE + sizeof(int) + addSize);
			blocks_in_stash[i][ii]->setData_size(oram_info[i].DATA_SIZE);
		}
	}

	/* for Circuit ORAM*/
	deepest = (int*)malloc(sizeof(int)*(oram_info[0].HEIGHT + 2));
	target = (int*)malloc(sizeof(int)*(oram_info[0].HEIGHT + 2));
	deepestIdx = (int*)malloc(sizeof(int)*(oram_info[0].HEIGHT + 2));
	deeperBlockIdx = (int*)malloc(sizeof(int)*(oram_info[0].HEIGHT + 2));
	/*******************/
}



/* General tree ORAM functions */

/**
 * Function Name: ecall_readMeta
 *
 * Description:
 * Load meta data from outside and decrypt
 * @param serializedMeta_path: byte array of meta data along the path
 * @param serializedMeta_stash: byte array of meta data inthe stash
 * @param oram_info: oram info structure
 * @recurLev: recursion level
 *
 * @return NULL
 */
void ecall_readMeta(unsigned char* serializedMeta_path, unsigned char* serializedMeta_stash, ORAM_INFO* oram_info, int recurLev)
{
	//Path meta
	int pos = 0;
	for (int i = 0; i < (oram_info[recurLev].HEIGHT + 1); i++)
	{
		memcpy(&ctr_meta_path[i*ENCRYPT_BLOCK_SIZE], &serializedMeta_path[pos], ENCRYPT_BLOCK_SIZE);
		pos += ENCRYPT_BLOCK_SIZE;
		memcpy(&ctr_data_path[i*ENCRYPT_BLOCK_SIZE], &serializedMeta_path[pos], ENCRYPT_BLOCK_SIZE);
		pos += ENCRYPT_BLOCK_SIZE;

		//decrypt the data
		sgx_aes_ctr_encrypt(master_key, &serializedMeta_path[pos], sizeof(TYPE_ID)*BUCKET_SIZE, &ctr_meta_path[i*ENCRYPT_BLOCK_SIZE], ENCRYPT_BLOCK_SIZE*BYTE_SIZE, (unsigned char*)&meta_path[i*BUCKET_SIZE]);

		pos += sizeof(TYPE_ID)*BUCKET_SIZE;
	}

	// Stash meta
	pos = 0;
	memcpy(ctr_meta_stash, &serializedMeta_stash[pos], ENCRYPT_BLOCK_SIZE);
	pos += ENCRYPT_BLOCK_SIZE;
	memcpy(ctr_data_stash, &serializedMeta_stash[pos], ENCRYPT_BLOCK_SIZE);
	pos += ENCRYPT_BLOCK_SIZE;

	//decrypt the meta
	sgx_aes_ctr_encrypt(master_key, &serializedMeta_stash[pos], sizeof(TYPE_ID)*STASH_SIZE, ctr_meta_stash, ENCRYPT_BLOCK_SIZE*BYTE_SIZE, (unsigned char*)meta_stash);
}

/**
 * Function Name: ecall_getStash_meta
 *
 * Description:
 * Re-encrypt stash meta to be stored outside
 * @param serializedMeta_stash: byte array of meta data for the stash (output)
 * @oram_info: oram info structure
 *
 * @return NULL
 */
void ecall_getStash_meta(unsigned char* serializedMeta_stash, ORAM_INFO* oram_info)
{

	int pos = 0;
	memcpy(&serializedMeta_stash[pos], ctr_meta_stash, ENCRYPT_BLOCK_SIZE);

	pos += ENCRYPT_BLOCK_SIZE;
	memcpy(&serializedMeta_stash[pos], ctr_data_stash_tmp, ENCRYPT_BLOCK_SIZE);

	sgx_aes_ctr_encrypt(master_key, (unsigned char*)meta_stash, sizeof(TYPE_ID)*STASH_SIZE, ctr_meta_stash, ENCRYPT_BLOCK_SIZE*BYTE_SIZE, &serializedMeta_stash[ENCRYPT_BLOCK_SIZE * 2]);

}

/**
 * Function Name: ecall_getPath_meta
 *
 * Description:
 * Re-encrypt meta along the path to be stored outside
 * @param serializedMeta_path: byte array of meta data in the path (output)
 * @oram_info: oram info structure
 * @recurLev: recursion level
 *
 * @return NULL
 */
void ecall_getPath_meta(unsigned char* serializedMeta_path, ORAM_INFO* oram_info, int recurLev)
{
	int pos = 0;
	for (int i = 0; i < (oram_info[recurLev].HEIGHT + 1); i++)
	{
		memcpy(&serializedMeta_path[pos], &ctr_meta_path[i*ENCRYPT_BLOCK_SIZE], ENCRYPT_BLOCK_SIZE);
		pos += ENCRYPT_BLOCK_SIZE;
		memcpy(&serializedMeta_path[pos], &ctr_data_path_tmp[i*ENCRYPT_BLOCK_SIZE], ENCRYPT_BLOCK_SIZE);

		pos += ENCRYPT_BLOCK_SIZE;
		sgx_aes_ctr_encrypt(master_key, (unsigned char*)&meta_path[i*BUCKET_SIZE], sizeof(TYPE_ID)*BUCKET_SIZE, &ctr_meta_path[i*ENCRYPT_BLOCK_SIZE], ENCRYPT_BLOCK_SIZE*BYTE_SIZE, &serializedMeta_path[pos]);

		pos += (sizeof(TYPE_ID)*BUCKET_SIZE);

	}

}

void ecall_getPathID(int* output)
{
	*output = pathID;
}

/**
 * Function Name: ecall_prepareAccess
 *
 * Description:
 * Prepare some data prior to perfoming oblivious search/update
 * @param oram_info: ORAM info structure
 * @param blockID: ID of the block to be accessed
 * @param newPathID: a new path of the block ID
 *
 * @return NULL
 */
void ecall_prepareAccess(ORAM_INFO* oram_info, int access_structure)
{
	if (access_structure == RECURSIVE_ACCESS)
	{
		//recursive access follows ods access
		blockID = *iter_lst_fid;
		computeRecursiveBlockID(blockID, recursive_block_ids, oram_info);
		if (iter_lst_fid != lst_fid.end()) 
			iter_lst_fid++;
		for (int i = 0; i < _ORAM_HEIGHT; i++)
		{
			new_path_id[i] = RandBound(oram_info[i].NUM_LEAVES) + (oram_info[i].NUM_LEAVES - 1);
			if (i == _ORAM_HEIGHT - 1)
				new_path_id[i] = 1;
		}

	}
	else //ACCESS_TYPE == ODS_ACCESS
	{
		if (oram_info[0].type == _TYPE_INDEX) //if this is the ODS access on index
		{
			new_path_id[0] = randPath;
		}
		else //if the current access is ODS on file, and the previous access is recursive  on file pos map
		{
			prevGen_newPathID = new_path_id[0];
		}
	}
	recursive_block_ids[0] = blockID;  

	for (int i = 0; i < _ORAM_HEIGHT; i++)
	{
		read_block[i]->clear();
		write_block[i]->clear();

		for (int ii = 0; ii < NUMBLOCK_PATH_LOAD; ii++)
		{
			blocks_in_bucket[i][ii]->clear();
		}
		for (int ii = 0; ii < NUMBLOCK_STASH_LOAD; ii++)
		{
			blocks_in_stash[i][ii]->clear();
		}
	}
	for (int i = 0; i < STASH_SIZE; i++)
	{
		meta_stash[i] = -1;
	}
	for (int i = 0; i < BUCKET_SIZE*(oram_info[0].HEIGHT + 1); i++)
	{
		meta_path[i] = -1;
	}

	if (oram_info[0].type == _TYPE_INDEX)
		newPathID_of_file.clear();

}


/* Path-ORAM functions*/

/**
 * Function Name: ecall_readPathData_PORAM
 *
 * Description:
 * Oblivious retrieval data along the path (in chunks) following P-ORAM retrieval blueprint
 *
 *
 * @param serializedBlocks_in_bucket: byte array of block data in the path (input)
 * @param start_block_idx: START index of the block along the path
 * @param serializedBlocks_in_stash: byte array of block data in the stash (input)
 * @param start_stash_idx: START index of the block in the stash.
 * @param data_size: size of the data payload in the block
 * @oram_info: oram info structure
 * @recurLev: recursion level
 * @PathID: path ID
 * @return NULL
 */
void ecall_readPathData_PORAM(unsigned char* serializedBlocks_in_bucket, int start_block_idx, unsigned char* serializedBlocks_in_stash, int start_stash_idx, unsigned int data_size, ORAM_INFO* oram_info, int recurLev)
{

	//parse into SGX objects
	if (start_stash_idx == 0)
	{
		deserializeBlocks(serializedBlocks_in_bucket, NUMBLOCK_PATH_LOAD, data_size, blocks_in_bucket[recurLev], recurLev, false, start_block_idx, false);
	}
	deserializeBlocks(serializedBlocks_in_stash, NUMBLOCK_STASH_LOAD, data_size, blocks_in_stash[recurLev], recurLev, true, start_stash_idx, false);
	//dec Bucket and stash first

	//add to stash
	for (int j = 0; j < NUMBLOCK_PATH_LOAD; j++)
	{
		char t = ocmp_ne(blocks_in_bucket[recurLev][j]->getID(), 0);
		//if (blocks_in_bucket[recurLev][j]->getID() != 0)
		//{
			char v = ocmp_eq(blocks_in_bucket[recurLev][j]->getID(), recursive_block_ids[recurLev]);
			o_clearBlock(v&t, read_block[recurLev]);
			o_copyBlock(v&t, read_block[recurLev], blocks_in_bucket[recurLev][j]);
			//if (blocks_in_bucket[recurLev][j]->getID() == recursive_block_ids[recurLev])
			//{
				//read_block[recurLev]->clear();
				//*read_block[recurLev] = *blocks_in_bucket[recurLev][j];
				
				////call this function to update the block
				if (recurLev > 0)
				{
					o_memcpy_8(v&t, &meta_path[j + start_block_idx], &new_path_id[recurLev],sizeof(TYPE_ID));
					//meta_path[j + start_block_idx] = new_path_id[recurLev];
				}
				else
				{
					o_memcpy_8(v&t, &meta_path[j + start_block_idx], &prevGen_newPathID, sizeof(TYPE_ID));
					//meta_path[j + start_block_idx] = prevGen_newPathID;
				}
				if (v == 1 &&  t ==1)  // This update should be oblivious (i.e., we should perform update after scanning both path & stash). However, for the code readability, we will keep it *insecure* like this
				{
					updateBlock(recurLev, oram_info);
				}
					
				o_copyBlock(v&t, blocks_in_bucket[recurLev][j], write_block[recurLev]);
				o_clearBlock(v&t, write_block[recurLev]);
				//*blocks_in_bucket[recurLev][j] = *write_block[recurLev];
				//write_block[recurLev]->clear();
			//}
			char u = 0;
			for (int i = 0; i < NUMBLOCK_STASH_LOAD; i++)
			{
				char v2 = ocmp_eq(blocks_in_stash[recurLev][i]->getID(),0);
				char v3 = ocmp_eq(u, 0);
				o_copyBlock(v2&v3, blocks_in_stash[recurLev][i], blocks_in_bucket[recurLev][j]);
				o_memcpy_8(v2&v3, &meta_stash[i + start_stash_idx], &meta_path[j + start_block_idx], sizeof(TYPE_ID));
				o_clearBlock(v2&v3 , blocks_in_bucket[recurLev][j]);
				u |= v2;
				//if (blocks_in_stash[recurLev][i]->getID() == 0) //!? change to be oblivious comparison later
				//{
				//	*blocks_in_stash[recurLev][i] = *blocks_in_bucket[recurLev][j];
				//	meta_stash[i + start_stash_idx] = meta_path[j + start_block_idx];
				//	meta_path[j + start_block_idx] = -1;
				//	blocks_in_bucket[recurLev][j]->clear();
				//	break;
				//}

				char v4 = ocmp_eq(blocks_in_stash[recurLev][i]->getID(), recursive_block_ids[recurLev]);
				v4 &= ocmp_eq(read_block[recurLev]->getID(), 0);
				o_clearBlock(v4&t, read_block[recurLev]);
				o_copyBlock(v4&t, read_block[recurLev], blocks_in_stash[recurLev][i]);
				//if (blocks_in_stash[recurLev][i]->getID() == recursive_block_ids[recurLev] && read_block[recurLev]->getID() == 0) //if the desired block is in the stash instead of in bucket //change ot be oblivious later
				//{
				//	read_block[recurLev]->clear();
				//	*read_block[recurLev] = *blocks_in_stash[recurLev][i];
				//	//call this function to update the block
				
				if(v4&& t)	// This update should be oblivious (i.e., we should perform update after scanning both path & stash). However, for the code readability, we will keep it *insecure* like this
					updateBlock(recurLev, oram_info);

				o_copyBlock(v4&t, blocks_in_stash[recurLev][i], write_block[recurLev]);

				//	*blocks_in_stash[recurLev][i] = *write_block[recurLev];
					if (recurLev > 0)
					{
						o_memcpy_8(v4&t, &meta_stash[i + start_stash_idx], &new_path_id[recurLev], sizeof(TYPE_ID));
					//	meta_stash[i + start_stash_idx] = new_path_id[recurLev];
					}
					else
					{
						o_memcpy_8(v4&t, &meta_stash[i + start_stash_idx], &prevGen_newPathID, sizeof(TYPE_ID));
						//meta_stash[i + start_stash_idx] = prevGen_newPathID;
					}

					o_clearBlock(v4&t, read_block[recurLev]);
				//	write_block[recurLev]->clear();
				//}
			//}
		}
	}


	//serialize the stash
	serializeBlocks(serializedBlocks_in_stash, NUMBLOCK_STASH_LOAD, data_size, blocks_in_stash[recurLev], recurLev, true, start_stash_idx, false);

	// at the end of each whole stash scan operation
	if (start_stash_idx + NUMBLOCK_STASH_LOAD == STASH_SIZE)
	{
		memcpy(ctr_data_stash, ctr_data_stash_tmp, ENCRYPT_BLOCK_SIZE);
	}
	//at the end of read operation (required by ORAM+ODS)

	if (((start_block_idx + NUMBLOCK_PATH_LOAD) == BUCKET_SIZE * (oram_info[recurLev].HEIGHT + 1)) && \
		(start_stash_idx + NUMBLOCK_STASH_LOAD) == STASH_SIZE && \
		recurLev == 0)
	{
		prevGen_newPathID = newPathID;
		recursive_block_ids[0] = nextBlockID; //check this
	}

}

/**
 * Function Name: ecall_writePathData_PORAM
 *
 * Description:
 * Evict data along the path (in chunks) following P-ORAM eviction blueprint
 *
 * IMPORTANT NOTE: serializedBlocks_in_bucket is in reversed order. Therefore, start_block_idx should be in reversed order.
 *
 * @param serializedBlocks_in_bucket: byte array of block data in the path (output)
 * @param start_block_idx: START index of the block along the path
 * @param serializedBlocks_in_stash: byte array of block data in the stash (output)
 * @param start_stash_idx: START index of the block in the stash.
 * @param data_size: size of the data payload in the block
 * @oram_info: oram info structure
 * @recurLev: recursion level
 * @PathID: path ID
 * @return NULL
 */
void ecall_writePathData_PORAM(unsigned char* serializedBlocks_in_bucket, int start_block_idx, unsigned char* serializedBlocks_in_stash, int start_stash_idx, unsigned int data_size, ORAM_INFO* oram_info, int recurLev, TYPE_ID PathID)
{
	//parse into SGX objects
	if (start_stash_idx == 0)
	{
		for (int i = 0; i < NUMBLOCK_PATH_LOAD; i++)
		{
			blocks_in_bucket[recurLev][i]->clear();
		}
	}
	
	deserializeBlocks(serializedBlocks_in_stash, NUMBLOCK_STASH_LOAD, data_size, blocks_in_stash[recurLev], recurLev, true, start_stash_idx,false);


	getFullPathIdx(full_path_idx, PathID, oram_info[recurLev].HEIGHT);

	for (int i = 0; i < NUMBLOCK_PATH_LOAD; i++)
	{
		int pathLevel = (start_block_idx - i) / BUCKET_SIZE; // - because of reversed order of startBlockIdx
		char u = 0;
		for (int s = 0; s < NUMBLOCK_STASH_LOAD; s++)
		{
			char v = ocmp_ne(meta_stash[s + start_stash_idx], -1);
			getFullPathIdx(full_path_idx_of_block, meta_stash[s + start_stash_idx], oram_info[recurLev].HEIGHT);
			char v2 = ocmp_eq(full_path_idx_of_block[pathLevel], full_path_idx[pathLevel]);
			char v3 = v & v2;
			char v4 = ocmp_eq(u, 0);
			o_copyBlock(v3&v4, blocks_in_bucket[recurLev][i], blocks_in_stash[recurLev][s]);
			o_memcpy_8(v3&v4, &meta_path[start_block_idx - i], &meta_stash[s + start_stash_idx],sizeof(TYPE_ID));
			o_clearBlock(v3&v4, blocks_in_stash[recurLev][s]);
			o_memcpy_8(v3&v4, &meta_stash[s + start_stash_idx], &_MINUS_ONE, sizeof(TYPE_ID));
			u |= v3;
			//if (meta_stash[s + start_stash_idx] != -1)
			//{
			//	getFullPathIdx(full_path_idx_of_block, meta_stash[s + start_stash_idx], oram_info[recurLev].HEIGHT);
			//	if (full_path_idx_of_block[pathLevel] == full_path_idx[pathLevel])
			//	{
			//		*blocks_in_bucket[recurLev][i] = *blocks_in_stash[recurLev][s];
			//		meta_path[start_block_idx - i] = meta_stash[s + start_stash_idx]; // - because of reversed order of startBlockIdx
			//		blocks_in_stash[recurLev][s]->clear();
			//		meta_stash[s + start_stash_idx] = -1;
			//		break;
			//	}
			//}
		}
	}
	if (start_stash_idx + NUMBLOCK_STASH_LOAD == STASH_SIZE)
	{
		serializeBlocks(serializedBlocks_in_bucket, NUMBLOCK_PATH_LOAD, data_size, blocks_in_bucket[recurLev], recurLev, false, start_block_idx, true);
	}
	serializeBlocks(serializedBlocks_in_stash, NUMBLOCK_STASH_LOAD, data_size, blocks_in_stash[recurLev], recurLev, true, start_stash_idx, false);
	
	// at the end of each whole stash scan operation
	if (start_stash_idx + NUMBLOCK_STASH_LOAD == STASH_SIZE)
	{
		memcpy(ctr_data_stash, ctr_data_stash_tmp, ENCRYPT_BLOCK_SIZE);
	}
}






/* Circuit-ORAM functions */


 /**
  * Function Name: ecall_readPathData_CORAM
  *
  * Description:
  * Oblivious retrieval data along the path (in chunks) following C-ORAM retrieval blueprint
  *
  *
  * @param serializedBlocks_in_bucket: byte array of block data in the path (input)
  * @param start_block_idx: START index of the block along the path
  * @oram_info: oram info structure
  * @recurLev: recursion level
  * @return NULL
  */
void ecall_readPathData_CORAM(unsigned char* serializedBlocks_in_bucket, int start_block_idx,   ORAM_INFO* oram_info, int recurLevel)
{

	//parse into SGX objects
	deserializeBlocks(serializedBlocks_in_bucket, NUMBLOCK_PATH_LOAD, oram_info[recurLevel].DATA_SIZE, blocks_in_bucket[recurLevel], recurLevel, false, start_block_idx,false);

	for (int j = 0; j < NUMBLOCK_PATH_LOAD; j++)
	{
		char v = ocmp_eq(blocks_in_bucket[recurLevel][j]->getID(), recursive_block_ids[recurLevel]);
		v &= ocmp_ne(meta_path[j + start_block_idx],-1);
		o_clearBlock(v, read_block[recurLevel]);
		o_copyBlock(v, read_block[recurLevel], blocks_in_bucket[recurLevel][j]);
		o_memcpy_8(v, &meta_path[j + start_block_idx], &_MINUS_ONE, sizeof(TYPE_ID));

		//if (blocks_in_bucket[recurLevel][j]->getID() == recursive_block_ids[recurLevel] && meta_path[j+ start_block_idx] != -1)
		//{
		//	read_block[recurLevel]->clear();
		//	*read_block[recurLevel] = *blocks_in_bucket[recurLevel][j];
		//	meta_path[j + start_block_idx] = -1;
		
		if(v) // This update should be oblivious (i.e., we should perform update after scanning both path & stash). However, for the code readability, we will keep it *insecure* like this
			updateBlock(recurLevel, oram_info);
		//}
	}

}

/**
 * Function Name: readStashData_CORAM_SGX
 *
 * Description:
 * Oblivious retrieval data from the stash (in chunks) following C-ORAM retrieval blueprint
 *
 *
 * @param serializedBlocks_in_stash: byte array of block data in the Stash (input)
 * @param start_stash_idx: START index of the block from the Stash (input)
 * @oram_info: oram info structure
 * @recurLev: recursion level
 * @return NULL
 */
void ecall_readStashData_CORAM(unsigned char* serializedBlocks_in_stash, int start_stash_idx, ORAM_INFO* oram_info, int recurLev)
{

	//parse into SGX objects
	deserializeBlocks(serializedBlocks_in_stash, NUMBLOCK_STASH_LOAD, oram_info[recurLev].DATA_SIZE, blocks_in_stash[recurLev], recurLev,  true, start_stash_idx,false);
	for (int i = 0; i < NUMBLOCK_STASH_LOAD; i++)
	{
		char v = ocmp_eq(blocks_in_stash[recurLev][i]->getID(), 0 );
		v &= ocmp_ne(write_block[recurLev]->getID(), 0);
		o_copyBlock(v, blocks_in_stash[recurLev][i], write_block[recurLev]);

		//if (blocks_in_stash[recurLev][i]->getID() == 0 && write_block[recurLev]->getID() != 0) 
		//{
			//*blocks_in_stash[recurLev][i] = *write_block[recurLev];
			if (recurLev > 0)
			{
				o_memcpy_8(v, &meta_stash[i + start_stash_idx], &new_path_id[recurLev],sizeof(TYPE_ID));
				//meta_stash[i + start_stash_idx] = new_path_id[recurLev];
			}
			else
			{
				o_memcpy_8(v, &meta_stash[i + start_stash_idx], &prevGen_newPathID, sizeof(TYPE_ID));
				//meta_stash[i + start_stash_idx] = prevGen_newPathID;
			}

			o_clearBlock(v, write_block[recurLev]);
			//write_block[recurLev]->clear();
		//}

		char v2 = abs(v - 1);
		v2 &= ocmp_eq(blocks_in_stash[recurLev][i]->getID(), recursive_block_ids[recurLev]);
		v2 &= ocmp_eq(read_block[recurLev]->getID(), 0);
		o_clearBlock(v2, read_block[recurLev]);
		o_copyBlock(v2, read_block[recurLev], blocks_in_stash[recurLev][i]);

		//else if (blocks_in_stash[recurLev][i]->getID() == recursive_block_ids[recurLev] && read_block[recurLev]->getID() == 0) //!?
		//{
		//	read_block[recurLev]->clear();
		//	*read_block[recurLev] = *blocks_in_stash[recurLev][i];
			
			if(v2)  // This update should be oblivious (i.e., we should perform update after scanning both path & stash). However, for the code readability, we will keep it *insecure* like this
				updateBlock(recurLev, oram_info);
			
			o_copyBlock(v2, blocks_in_stash[recurLev][i], write_block[recurLev]);
			//*blocks_in_stash[recurLev][i] = *write_block[recurLev];

			if (recurLev > 0)
			{
				o_memcpy_8(v2, &meta_stash[i + start_stash_idx], &new_path_id[recurLev], sizeof(TYPE_ID));
				//meta_stash[i + start_stash_idx] = new_path_id[recurLev];
			}
			else
			{
				o_memcpy_8(v2, &meta_stash[i + start_stash_idx], &prevGen_newPathID, sizeof(TYPE_ID));
				//meta_stash[i + start_stash_idx] = prevGen_newPathID;
			}

			o_clearBlock(v2, write_block[recurLev]);
			//write_block[recurLev]->clear();
		//}
	}
	

	//serialize the stash & blocks in bucket
	
	serializeBlocks(serializedBlocks_in_stash, NUMBLOCK_STASH_LOAD, oram_info[recurLev].DATA_SIZE, blocks_in_stash[recurLev], recurLev, true, start_stash_idx, false);

	//at the end of read operation (required by ODS)
	if ((start_stash_idx + NUMBLOCK_STASH_LOAD) == STASH_SIZE && recurLev == 0)
	{
		prevGen_newPathID = newPathID;
		recursive_block_ids[0] = nextBlockID;
	}
}

/**
 * Function Name: ecall_prepareEviction_CORAM
 *
 * Description:
 * Prepare some data to before performing eviction by C-ORAM blueprint. This function is called before evictCORAM_SGX
 *
 *
 * @param meta_bucket: meta data along the path
 * @param meta_stash: meta data of the Stash
 * @param evictPath: eviction path ID
 * @oram_info: oram info structure
 * @recurLev: recursion level
 * @return NULL
 */
void ecall_prepareEviction_CORAM(unsigned char* meta_bucket, unsigned char* meta_stash, TYPE_ID evictPath, ORAM_INFO* oram_info, int recurLevel)
{
	ecall_readMeta(meta_bucket, meta_stash, oram_info, recurLevel);
	PrepareDeepest(meta_path, evictPath, deepest, oram_info, recurLevel);
	prepareTarget(meta_path, evictPath, deepest, target, oram_info, recurLevel);
	getDeepestBucketIdx(meta_path, evictPath, deepestIdx, oram_info, recurLevel);
	dest = -2;
	read_block[recurLevel]->clear();
	write_block[recurLevel]->clear();
	hold_pathID = -1;
	to_write_pathID = -1;
}

/**
 * Function Name: ecall_evictCORAM
 *
 * Description:
 * Perform eviction following C-ORAM blue-print
 *
 *
 * @param blocks: blocks along the path (including the stash as level 0)
 * @param start_block_idx: starting index of the block to be processed (due to chunk-by-chunk)
 * @oram_info: oram info structure
 * @recurLev: recursion level
 * @return NULL
 */
void ecall_evictCORAM(unsigned char* blocks, int start_block_idx, int pathLev, ORAM_INFO* oram_info, int recurLevel)
{
	if (pathLev == 0) //stash
	{
		deserializeBlocks(blocks, NUMBLOCK_STASH_LOAD, oram_info[recurLevel].DATA_SIZE, blocks_in_stash[recurLevel], recurLevel, true, start_block_idx, false);
		for (int i = 0; i < NUMBLOCK_STASH_LOAD; i++)
		{
			char v = ocmp_eq(i + start_block_idx, deepestIdx[0]);
			v &= ocmp_ne(target[0], -2);
			o_copyBlock(v, read_block[recurLevel], blocks_in_stash[recurLevel][i]);
			o_memcpy_8(v, &hold_pathID, &meta_stash[deepestIdx[0]], sizeof(TYPE_ID));
			o_clearBlock(v, blocks_in_stash[recurLevel][i]);
			o_memcpy_8(v, &meta_stash[deepestIdx[0]], &_MINUS_ONE, sizeof(TYPE_ID));
			o_memcpy_8(v, &dest, &target[0], sizeof(int));
			//if (i + start_block_idx == deepestIdx[0] && target[0] != -2) // !? 
			//{
			//	*read_block[recurLevel] = *blocks_in_stash[recurLevel][i];
			//	hold_pathID = meta_stash[deepestIdx[0]];
			//	blocks_in_stash[recurLevel][i]->clear();
			//	meta_stash[deepestIdx[0]] = -1;
			//	dest = target[0];
			//}
		}
		serializeBlocks(blocks, NUMBLOCK_STASH_LOAD, oram_info[recurLevel].DATA_SIZE, blocks_in_stash[recurLevel], recurLevel, true, start_block_idx, false);
	}
	else
	{
		deserializeBlocks(blocks, NUMBLOCK_PATH_LOAD, oram_info[recurLevel].DATA_SIZE, blocks_in_bucket[recurLevel], recurLevel, false, start_block_idx, false);
		
		char v = ocmp_ne(read_block[recurLevel]->getID(), 0);
		v &= ocmp_eq(pathLev, dest);
		o_copyBlock(v, write_block[recurLevel], read_block[recurLevel]);
		o_memcpy_8(v, &to_write_pathID, &hold_pathID, sizeof(TYPE_ID));
		o_clearBlock(v, read_block[recurLevel]);
		o_memcpy_8(v, &hold_pathID, &_MINUS_ONE, sizeof(TYPE_ID));
		o_memcpy_8(v, &dest, &_MINUS_TWO, sizeof(int));

		//if (read_block[recurLevel]->getID() != 0 && pathLev == dest)
		//{
		//	*write_block[recurLevel] = *read_block[recurLevel];
		//	to_write_pathID = hold_pathID;
		//	read_block[recurLevel]->clear();
		//	hold_pathID = -1;
		//	dest = -2;
		//}
		for (int i = 0; i < NUMBLOCK_PATH_LOAD; i++)
		{
			char v2 = ocmp_ne(target[pathLev], -2);
			v2 &= ocmp_eq(i + start_block_idx, (deepestIdx[pathLev] + ((pathLev - 1)*BUCKET_SIZE)));
			o_copyBlock(v2, read_block[recurLevel], blocks_in_bucket[recurLevel][i]);
			TYPE_ID tmp = meta_path[deepestIdx[pathLev] + ((pathLev - 1)*BUCKET_SIZE)];
			o_memcpy_8(v2,&hold_pathID, &tmp, sizeof(TYPE_ID));
			o_clearBlock(v2, blocks_in_bucket[recurLevel][i]);
			o_memcpy_8(v2, &meta_path[deepestIdx[pathLev] + ((pathLev - 1)*BUCKET_SIZE)], &_MINUS_ONE, sizeof(TYPE_ID));
			o_memcpy_8(v2, &dest, &target[pathLev], sizeof(int));

			//if ((target[pathLev] != -2) && ((i + start_block_idx) == (deepestIdx[pathLev] + ((pathLev - 1)*BUCKET_SIZE)))) //!?
			//{
			//	*read_block[recurLevel] = *blocks_in_bucket[recurLevel][i];
			//	hold_pathID = meta_path[deepestIdx[pathLev] + ((pathLev - 1)*BUCKET_SIZE)];

			//	blocks_in_bucket[recurLevel][i]->clear();
			//	meta_path[deepestIdx[pathLev] + ((pathLev - 1)*BUCKET_SIZE)] = -1;
			//	dest = target[pathLev];
			//}

			char v3 = ocmp_ne(write_block[recurLevel]->getID(), 0);
			v3 &= ocmp_eq(meta_path[i + start_block_idx], -1);
			o_copyBlock(v3, blocks_in_bucket[recurLevel][i], write_block[recurLevel]);
			o_clearBlock(v3, write_block[recurLevel]);
			o_memcpy_8(v3, &meta_path[i + start_block_idx], &to_write_pathID, sizeof(TYPE_ID));
			o_memcpy_8(v3, &to_write_pathID, &_MINUS_ONE, sizeof(TYPE_ID));

			//if (write_block[recurLevel]->getID() != 0 && meta_path[i + start_block_idx] == -1) 
			//{
			//	// place towrite into bucket path[i];
			//	*blocks_in_bucket[recurLevel][i] = *write_block[recurLevel];
			//	write_block[recurLevel]->clear();
			//	meta_path[i + start_block_idx] = to_write_pathID;
			//	to_write_pathID = -1;
			//}
		}
		serializeBlocks(blocks, NUMBLOCK_PATH_LOAD, oram_info[recurLevel].DATA_SIZE, blocks_in_bucket[recurLevel], recurLevel, false, start_block_idx, false);
	}
}


/* ODS functions */
/**
 * Function Name: ecall_getNextPathID
 *
 * Description:
 * retrieve the path ID of the next block to be accessed
 * @param oram_info: ORAM info structure
 * @param recurLevel: recursion level
 * @param nextPathID: next path ID to be accessed (output)
 *
 * @return NULL
 */
void ecall_getNextPathID(ORAM_INFO* oram_info, int recurLevel, TYPE_ID* pid)
{
	*pid = 0;
	if (recurLevel > 0)
	{

		int numBits = ceil(log2(oram_info[recurLevel - 1].NUM_BLOCKS)) + 1;
		int update_bit_pos = ((recursive_block_ids[recurLevel - 1] - 1) % COMPRESSION_RATIO)*numBits;
		for (int bit = 0; bit < numBits; bit++)
		{
			if (BIT_CHECK(&read_block[recurLevel]->getDATA()[(bit + update_bit_pos) / BYTE_SIZE], (bit + update_bit_pos) % BYTE_SIZE))
			{
				BIT_SET(pid, bit);
			}
		}
	}
	else
	{
		*pid = nextPathID;
	}
}


/* Search & update functions*/
void ecall_setSearchKeyword(unsigned char* kw, int kw_len, int* kwmap_size, unsigned char* ctr_kwmap)
{
	sgx_rijndael128_cmac_msg(master_key, kw, kw_len, &tag);
	memcpy(ctr_decrypt, ctr_kwmap, 16);
	memcpy(ctr_reencrypt, ctr_kwmap, 16);
	
	inc_dec_ctr(ctr_reencrypt, ceil((double)*kwmap_size*KWMAP_VALUE_SIZE/ENCRYPT_BLOCK_SIZE), true);

	randPath = RandBound(IDX_NUM_BLOCKS) + (IDX_NUM_BLOCKS - 1);
	currOp = OP_SEARCH;
	lst_fid.clear();
}

void ecall_setUpdateKeyword(unsigned char* kw, int kw_len, TYPE_ID fileID, int* kwmap_size, unsigned char* ctr_kwmap)
{
	sgx_rijndael128_cmac_msg(master_key, kw, kw_len, &tag);
	memcpy(ctr_decrypt, ctr_kwmap, 16);
	memcpy(ctr_reencrypt, ctr_kwmap, 16);

	inc_dec_ctr(ctr_reencrypt, ceil((double)*kwmap_size*KWMAP_VALUE_SIZE / ENCRYPT_BLOCK_SIZE), true);

	randPath = RandBound(IDX_NUM_BLOCKS) + (IDX_NUM_BLOCKS - 1);


	update_fileID = fileID;
	currOp = OP_UPDATE;
	blockID_2 = 0;
	pathID_2 = 0;
	blockID = 0;
	pathID = 0;
}

void ecall_scanKWMap( unsigned char* input, int len)
{
	int matched = 0;
	for (int i = 0; i < len; i+= (KWMAP_VALUE_SIZE+16))
	{
		// first 16 bytes are the cmac of keyword
		char v = 1;
		for (int j = 0; j < 16; j+=sizeof(long long))
		{
			v &= ocmp_eq(*((long long*)&tag[j]),*((long long*) &input[i+j]));
		}
		//matched = isEqual(tag, &input[i], 16);

		// latter 32 bytes are the encrypted blockID, pathID, and number of fileIDs in the first block of keyword
		sgx_aes_ctr_encrypt(master_key, &input[i+16], 32, ctr_decrypt, 128, buffer);

		o_memcpy_8(v, &blockID, buffer, 8);
		o_memcpy_8(v, &pathID, &buffer[8], 8);
		o_memcpy_8(v, &buffer[8], &randPath, sizeof(randPath));
		o_memcpy_8(v, &numFileIDs, &buffer[16], 8);
		
		//if (matched == 1)
		//{
			//memcpy(&blockID, buffer, 8);
			//memcpy(&pathID, &buffer[8], 8);
			//memcpy(&buffer[8], &randPath, sizeof(randPath));
			//memcpy(&numFileIDs, &buffer[16], 8);
			if (currOp == OP_UPDATE) 
			{
				char v2 = ocmp_eq(numFileIDs, INDEX_DATA_SIZE / sizeof(TYPE_ID));
				v2 &= v;
				useEmptyBlock = 0;
				o_memcpy_8(v2, &buffer[0], &blockID_2, sizeof(TYPE_ID));
				long long tmp = 0;
				o_memcpy_8(v2, &numFileIDs, &tmp, sizeof(tmp));
				TYPE_ID tmp2;
				o_memcpy_8(v2, &tmp2, &blockID, sizeof(blockID));
				o_memcpy_8(v2, &blockID, &blockID_2, sizeof(blockID));
				o_memcpy_8(v2, &blockID_2, &tmp2, sizeof(tmp2));

				o_memcpy_8(v2, &tmp2, &pathID, sizeof(pathID));
				o_memcpy_8(v2, &pathID, &pathID_2, sizeof(pathID));
				o_memcpy_8(v2, &pathID_2, &tmp2, sizeof(tmp2));

				char tmp3 = 1;
				o_memcpy_byte(v2, &useEmptyBlock, &tmp3, sizeof(char));

				//if (numFileIDs == INDEX_DATA_SIZE / sizeof(TYPE_ID)) // curBlock is full
				//{
					
					//memcpy(&buffer[0], &blockID_2, sizeof(TYPE_ID));
					//numFileIDs = 0;
					//swap 
					//TYPE_ID tmp;
					//tmp = blockID;
					//blockID = blockID_2;
					//blockID_2 = tmp;

					//tmp = pathID;
					//pathID = pathID_2;
					//pathID_2 = tmp;

					//useEmptyBlock = 1;
				//}
				//else
				//{
				//	useEmptyBlock = 0;
					//no swap
				//}
				long long tmp4 = numFileIDs++;
				o_memcpy_8(v2, &numFileIDs, &tmp4, sizeof(long long));
				o_memcpy_8(v2, &buffer[16], &numFileIDs, 8);

				//numFileIDs++;
				//memcpy(&buffer[16], &numFileIDs, 8);
			}
		//}
		sgx_aes_ctr_encrypt(master_key, buffer, 32, ctr_reencrypt, 128, &input[i + 16]);

		
	}
}

void ecall_getNumFileIDs(int* numFile)
{
	int tmp = 0;
	for (iter_lst_fid = lst_fid.begin(); iter_lst_fid != lst_fid.end(); iter_lst_fid++)
	{
		char v = ocmp_g(*iter_lst_fid, 0);
		int tmp2 = tmp + 1;
		o_memcpy_8(v, &tmp, &tmp2, sizeof(int));

		//if (*iter_lst_fid > 0)
		//	tmp++;
	}
	*numFile = tmp;
}

void ecall_getFileIDs(unsigned char* fid_arr) // put encryption prior to copy
{
	int i = 0;
	for (iter_lst_fid = lst_fid.begin(); iter_lst_fid != lst_fid.end(); iter_lst_fid++)
	{
		char v = ocmp_g(*iter_lst_fid, 0);
		
		int tmp2 = i + 1;
		TYPE_ID tmp3 = *iter_lst_fid;
		o_memcpy_8(v, &fid_arr[i * sizeof(TYPE_ID)], &tmp3, sizeof(TYPE_ID));
		o_memcpy_8(v, &i, &tmp2, sizeof(int));

		//if (*iter_lst_fid > 0)
		//{
		//	memcpy(&fid_arr[i * sizeof(TYPE_ID)], &(*iter_lst_fid), sizeof(TYPE_ID));
		//	i++;
		//}
			
	}
}

void ecall_setFileIDs(unsigned char* fid_arr,int num_fileIDs) // put decryption prior to copy
{
	lst_fid.clear();
	for (int i = 0; i < num_fileIDs; i++)
	{
		TYPE_ID tmp;
		memcpy(&tmp, &fid_arr[i * sizeof(TYPE_ID)], sizeof(TYPE_ID));
		lst_fid.insert(abs(tmp));
	}
	iter_lst_fid = lst_fid.begin();
}

void ecall_scanEmptyBlock(unsigned char* empty_block_arr, long empty_block_arr_len)
{
	TYPE_ID tmp1, tmp2;
	TYPE_ID tmp = 0;
	for (TYPE_ID i = 0; i < empty_block_arr_len; i+=sizeof(TYPE_ID)*2)
	{
		memcpy(&tmp1, &empty_block_arr[i], sizeof(TYPE_ID));
		memcpy(&tmp2, &empty_block_arr[i+sizeof(TYPE_ID)], sizeof(TYPE_ID));

		char v = ocmp_ne(tmp1, 0);
		v &= ocmp_eq(blockID_2, 0);
		v &= ocmp_eq(pathID_2, 0);
		o_memcpy_8(v, &blockID_2, &tmp1, sizeof(TYPE_ID));
		o_memcpy_8(v, &pathID_2, &tmp2, sizeof(TYPE_ID));
		o_memcpy_8(v, &empty_block_arr[i], &tmp, sizeof(TYPE_ID));
		o_memcpy_8(v, &empty_block_arr[i + sizeof(TYPE_ID)], &tmp, sizeof(TYPE_ID));
		//if (tmp1 != 0 && blockID_2 == 0 && pathID_2 == 0)
		//{
		//	blockID_2 = tmp1;
		//	pathID_2 = tmp2;
		//	
		//	memset(&empty_block_arr[i], 0, sizeof(TYPE_ID));
		//	memset(&empty_block_arr[i + sizeof(TYPE_ID)], 0, sizeof(TYPE_ID));
		//}
	}
}

















/*
constexpr auto _MEMCPY_BLOCK_SIZE = 8;

size_t o_memcpy_8(uint64_t is_valid_copy, void *dst, void *src, size_t size) {

	size_t n_blocks = size / _MEMCPY_BLOCK_SIZE;
	size_t large_size = n_blocks * _MEMCPY_BLOCK_SIZE;
	size_t small_size = size - large_size;

	uint64_t i_dst = (uint64_t)dst;
	uint64_t i_src = (uint64_t)src;

	uint64_t *p_dst = (uint64_t*)dst;
	uint64_t *p_src = (uint64_t*)src;

	size_t copied = large_size;

	__asm volatile("mov %0, %%rbx\n\t" : : "r"(is_valid_copy) : "rbx");

	for (uint64_t i = 0; i < n_blocks; ++i)
	{
		__asm
		{
			movq(%[dst]), %%r14;
			movq(%[src]), %%r15;
			test %%rbx, %%rbx;
			cmovnz %%r15, %%r14;
			cmovnz %%r15, %%r14;
			movq %%r14, (%[dst]);

		}
	}
			: :[dst] "r"  (p_dst + i),
			[src] "r"  (p_src + i)
			: "rbx", "r13", "r14", "r15");
	}

	if (small_size == 0) return copied;

	return copied + o_memcpy_byte(is_valid_copy,
		(void*)(i_dst + copied),
		(void*)(i_src + copied),
		small_size);
}


size_t o_memcpy_byte(uint64_t is_valid_copy, void *dst, void *src, size_t size) {
	//assert(1!=1);
	//while(1);
	size_t n_blocks = size;
	uint8_t p_dst = (uint8_t)dst;
	uint8_t p_src = (uint8_t)src;
	__asm volatile(
		"mov %0, %%rbx\n\t"
		"xor %%rcx, %%rcx\n\t"
		:: "r"(is_valid_copy) : "rbx", "rcx");
	for (uint64_t i = 0; i < n_blocks; ++i) {
		__asm volatile(
			"movb (%[dst]), %%r14b\n\t"
			"movb (%[src]), %%r15b\n\t"
			"test %%rbx, %%rbx\n\t"
			"cmovnz %%r15w, %%r14w\n\t"
			"movb %%r14b, (%[dst])\n\t"
			::[dst] "r"  (p_dst + i),
			[src] "r"  (p_src + i)
			: "rbx", "rcx", "r13", "r14", "r15");
	}
	return size;
}
*/

