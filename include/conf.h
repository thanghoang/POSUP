#pragma once
#include "string.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"
#include "sgx_eid.h"
#include "sgx_tcrypto.h"
#include "sgx_trts.h"
#include "sgx_tae_service.h"
typedef int TYPE_ID;
typedef struct type_pos_map
{
	TYPE_ID pathID;
}TYPE_POS_MAP;




//=== PARAMETERS ============================================================
//#define PATH_ORAM

#define CIRCUIT_ORAM

#if defined CIRCUIT_ORAM
	#define DETERMINISTIC_EVICTION
#endif


// this cache is for big ORAM
#define STASH_CACHING
#define K_TOP_CACHING

//this cache is for file pos map
#define POSITION_MAP_CACHING

#if defined(K_TOP_CACHING)
	const int CachedLevel[2] = { 3,4 }; //2 means ORAM index and ORAM files 
#endif




#define STASH_SIZE 40


#define INDEX_DATA_SIZE  1012 //64 //byte
#define FILE_DATA_SIZE 3060 //512 //byte

#define IDX_NUM_BLOCKS 512 //16777216//2048//16777216
#define FILE_NUM_BLOCKS 32 //8388608//128//8388608

#define NUMBLOCK_PATH_LOAD BUCKET_SIZE
#define NUMBLOCK_STASH_LOAD 40

#define NUM_EMPTY_BLOCK_LOAD 20
#define NUM_KWMAP_ENTRIES_LOAD 20


const sgx_aes_ctr_128bit_key_t master_key[16] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };



/***** DO NOT CHANGE ANYTHING BELOW!!!!! *****/

#if defined(PATH_ORAM)
	#define BUCKET_SIZE 4 
#elif defined (CIRCUIT_ORAM)
	#if defined(DETERMINISTIC_EVICTION)
		#define BUCKET_SIZE 2 
	#else
		#define BUCKET_SIZE 3
	#endif
#endif
		
		
#define KWMAP_VALUE_SIZE 32

#define ENCRYPT_BLOCK_SIZE 16	//in bytes

#if !defined(DETERMINISTIC_EVICTION)
	#define RANDOM_EVICTION
#endif


#define BUCKET_META_SIZE (ENCRYPT_BLOCK_SIZE*2+(BUCKET_SIZE*sizeof(TYPE_ID))) // + ENCRYPT_BLOCK_SIZE)
#define STASH_META_SIZE (STASH_SIZE*sizeof(TYPE_ID)+(2*ENCRYPT_BLOCK_SIZE))

#define COMPRESSION_RATIO 2
#define BYTE_SIZE 8
typedef unsigned char TYPE_DATA;

typedef struct oram_info
{
	TYPE_ID NUM_NODES;
	TYPE_ID NUM_BLOCKS;
	TYPE_ID DATA_SIZE;
	TYPE_ID BLOCK_SIZE;
	TYPE_ID HEIGHT;
	TYPE_ID NUM_LEAVES;
	int N_LEVELS;
	char* ORAMPath;
	char* BlockPath;
	int type;
	int evict_order; //in case of using deterministic eviction
	sgx_enclave_id_t eid;
}ORAM_INFO;

//MACROS
#define BIT_READ(character, position, the_bit)	((*the_bit = *character & (1 << position)))	
#define BIT_SET(character, position) ((*character |= 1 << position))	
#define BIT_CLEAR(character, position) ((*character &= ~(1 << position)))
#define BIT_TOGGLE(character, position)	((*character ^= 1 << position))
#define BIT_CHECK(var,pos) !!((*var) & (1<<(pos)))

//define some type of access operations


#define _TYPE_INDEX 0
#define _TYPE_FILE 1

#define OP_SEARCH 2
#define OP_UPDATE 3


#define RECURSIVE_ACCESS 4
#define ODS_ACCESS 5


static unsigned char global_counter[ENCRYPT_BLOCK_SIZE];
static long long cc = 0;
