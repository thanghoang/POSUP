// CHECK RECURSIVE ORAM SIZE, which might not be like original SIZE / 2
#include "POSUP.h"
#include "TreeORAM.h"
#include "Utils.h"
#include "Bucket.h"
#include "gc.h"
#include "KeywordExtraction.h"

#include "tomcrypt.h"

#include <tuple>
using namespace std;

ORAM_INFO* POSUP::recursion_info_index = new ORAM_INFO[1];
ORAM_INFO* POSUP::recursion_info_file = new ORAM_INFO[(int)log2(FILE_NUM_BLOCKS) + 1];


POSUP::POSUP() {}
POSUP::~POSUP(){}


int POSUP::init()
{
	register_cipher(&rijndael_desc);
	this->masterKey = new unsigned char[ENCRYPT_BLOCK_SIZE];
	this->pos_map_index = new TYPE_POS_MAP[IDX_NUM_BLOCKS + 1];
	this->pos_map_file = new TYPE_POS_MAP[FILE_NUM_BLOCKS + 1];
	long long n; 
#if defined(K_TOP_CACHING)
	BUCKETS = new Bucket*[2];

	//Cache for Index-ORAM
	n = pow(2, CachedLevel[0]) - 1;
	BUCKETS[0] = new Bucket[n];
	for (TYPE_ID k = 0; k < n; k++)
	{
		BUCKETS[0][k].setData_size(recursion_info_index[0].DATA_SIZE);
	}
	//Cache for File-ORAM
	n = pow(2, CachedLevel[1]) - 1;
	BUCKETS[1] = new Bucket[n];
	for (TYPE_ID k = 0; k < n; k++)
	{
		BUCKETS[1][k].setData_size(recursion_info_file[0].DATA_SIZE);
	}
#endif
#if defined(STASH_CACHING)
	S = new STASH[2];
	S[0].setData_size(recursion_info_index[0].DATA_SIZE);
	S[1].setData_size(recursion_info_file[0].DATA_SIZE);
#endif
	file_pos_map_bucket = new Bucket*[recursion_info_file[0].N_LEVELS];
	file_pos_map_S = new STASH[recursion_info_file[0].N_LEVELS];

	for (int l = 1; l < recursion_info_file[0].N_LEVELS + 1; l++)
	{
		//Cache for File-ORAM
		n = pow(2, recursion_info_file[l].HEIGHT +1 ) - 1;
		file_pos_map_bucket[l-1] = new Bucket[n];
		for (long long k = 0; k < n; k++)
		{
			file_pos_map_bucket[l-1][k].setData_size(recursion_info_file[l].DATA_SIZE);
		}
		file_pos_map_S[l-1].setData_size(recursion_info_file[l].DATA_SIZE);
	}
	
	// Init SGX Enclave
	sgx_status_t ret;
	sgx_launch_token_t token = { 0 };
	int token_updated = 0;

	ret = sgx_create_enclave(ENCLAVE_FILE, true, &token, &token_updated, &recursion_info_file[0].eid, NULL);
	if (ret != SGX_SUCCESS) {
		std::cout << "sgx_create_enclave failed: " << ret << std::endl;
		return -1;

	}

	ret = sgx_create_enclave(ENCLAVE_FILE, true, &token, &token_updated, &recursion_info_index[0].eid, NULL);
	if (ret != SGX_SUCCESS) {
		std::cout << "sgx_create_enclave failed: " << ret << std::endl;
		return -1;

	}

	return 0;
}
int POSUP::initEnclave()
{
	ecall_prepareMemory(recursion_info_file[0].eid, recursion_info_file);

	ecall_prepareMemory(recursion_info_index[0].eid, recursion_info_index);

	return 0;
}


int POSUP::buildIndexORAM()
{
	vector<TYPE_ID> blockIDs;
	TreeORAM ORAM;
	//readVector_from_file(blockIDs,"blockIDIndex",clientDataDir); //debug purpose
	printf("Building ORAM for index...\n");

	KeywordExtraction ke;
	map <string, string> invertedIdx;
	ke.createInvertedIndex(dbPath,invertedIdx);

	//build the position map
	for (TYPE_ID i = 0; i < recursion_info_index[0].NUM_BLOCKS; i++)
	{
		blockIDs.push_back(i + 1);
	}
	//random permutation using a built-in function (changed later to a crypto-secure PRP)
	std::random_shuffle(blockIDs.begin(), blockIDs.end());


	printf("Writing shuffled blockIDs to file..");
	writeVector_to_file(blockIDs,"blockIDIndex",clientDataDir);
	printf("OK!\n");
	printf("Reserving disk for ORAM structures...");
	ORAM.reserveORAM_disk(recursion_info_index);
	printf("OK!\n");

	TYPE_ID PathID = recursion_info_index[0].NUM_NODES / 2;
	for (int i = 0; i < blockIDs.size(); i++)
	{
		pos_map_index[blockIDs[i] - 1].pathID = PathID;
		PathID++;
	}

	//map <string, pair<TYPE_ID, TYPE_ID>> keywordBlockID;
	map <string, string> keywordBlockID;
	// map keyword with file's ORAM block ID
	
	//read file BlockID map
	map <TYPE_ID, pair<TYPE_ID,TYPE_ID>> fileBlockID;
	readMap_from_file(fileBlockID,  filename_fileBlockIDMap, clientDataDir);


	TYPE_ID ValueBlockID = 1;
	Block b;
	b.setData_size(INDEX_DATA_SIZE);
	long total = 0;
	string kwMap = "";

	unsigned char iv[16];
	memset(iv, 0, 16);
	kwMap.assign((char*)iv, 16);
	int ct = 0;

	for (std::map<string, string>::const_iterator it = invertedIdx.begin(); it != invertedIdx.end(); it++)
	{
		//create ORAM block for values (file IDs) for each keyword
		string filenames = it->second;
		
		std::vector<std::string> filename_arr = split(filenames, ',');
		filename_arr.pop_back(); //remove the , at the end
		unsigned char* BlockID = new unsigned char[filename_arr.size() * (sizeof(TYPE_ID))];
		memset(BlockID, 0, filename_arr.size() * (sizeof(TYPE_ID)));

		for (int i = 0; i < filename_arr.size(); i++)
		{
			std::memcpy(&BlockID[i * (sizeof(TYPE_ID))], &fileBlockID[stol(filename_arr[i])].first, sizeof(TYPE_ID));
		}
		int num_oram_blocks =  ceil((double)filename_arr.size() * (sizeof(TYPE_ID)) / INDEX_DATA_SIZE);


		// compute keyed hash of keyword
		long numFileIDs_first_block = INDEX_DATA_SIZE / sizeof(TYPE_ID);
		if (filename_arr.size() < INDEX_DATA_SIZE / sizeof(TYPE_ID))
			numFileIDs_first_block = filename_arr.size();
		int idx, err;
		omac_state omac;
		unsigned char tag[16];
		unsigned long tag_len = 16;

		omac_init(&omac, find_cipher("rijndael"), (unsigned char*)master_key, 16);
		omac_process(&omac, (unsigned char*)it->first.c_str(), it->first.size());

		omac_done(&omac, tag, &tag_len);

		string tmp;
		tmp.assign((char*)tag, 16);
		kwMap += tmp;

		unsigned char uchar_val[32];
		unsigned char encVal[32];
		memset(uchar_val, 0, 32);
		memset(encVal, 0, 32);
		std::memcpy(&uchar_val[0], &ValueBlockID, sizeof(ValueBlockID));
		std::memcpy(&uchar_val[8], &pos_map_index[ValueBlockID - 1].pathID, sizeof(pos_map_index[ValueBlockID - 1].pathID));
		std::memcpy(&uchar_val[16], &numFileIDs_first_block, sizeof(numFileIDs_first_block));

		intel_AES_encdec128_CTR(uchar_val, encVal, (unsigned char*)master_key, 2, iv);

		tmp.assign((char*)encVal, 32);
		kwMap += tmp;


		for (int i = 0, j=0; i < num_oram_blocks; i++, j+= INDEX_DATA_SIZE)
		{
			b.clear();
			b.ID = ValueBlockID;

			if (i < num_oram_blocks - 1)
			{
				b.nextID = ValueBlockID + 1;
				b.nextPathID = pos_map_index[ValueBlockID].pathID;
			}

			if ((j + INDEX_DATA_SIZE) < filename_arr.size() * sizeof(TYPE_ID))
			{
				std::memcpy(b.DATA, &BlockID[j], INDEX_DATA_SIZE);
			}
			else
			{
				std::memcpy(b.DATA, &BlockID[j], (filename_arr.size() * sizeof(TYPE_ID)) -j);	
			}
			reverseByteArray(b.DATA, INDEX_DATA_SIZE, sizeof(TYPE_ID));



			//save block to files
			writeBlock_to_file(b, to_string(ValueBlockID), IdxBlockPath);
			ValueBlockID++;
		}
		ct++;
	}
	//store a list of empty blocks as well as its path
	
	numEmptyBlocks = recursion_info_index[0].NUM_BLOCKS - ValueBlockID + 1;
	empty_block_arr_len = numEmptyBlocks * sizeof(TYPE_ID) * 2;
	empty_block_arr = new unsigned char[empty_block_arr_len];


	for (TYPE_ID i = ValueBlockID, j = 0 ; i <= recursion_info_index[0].NUM_BLOCKS; i++, j+=sizeof(TYPE_ID)*2)
	{
		b.clear();
		b.ID = i;
		writeBlock_to_file(b, to_string(i), IdxBlockPath);
		memcpy(&empty_block_arr[j], &i, sizeof(i));
		memcpy(&empty_block_arr[j + sizeof(TYPE_ID)], &pos_map_index[i - 1].pathID, sizeof(TYPE_ID));
	}
	writeByte_array_to_file(empty_block_arr, empty_block_arr_len, filename_emptyIdxBlock,clientDataDir);
	
	//write len of empty block array to file
	FILE* file_out = NULL;
	string path = clientDataDir + filename_emptyIdxBlock_len;
	file_out = fopen(path.c_str(), "wb+");
	fwrite(&empty_block_arr_len, sizeof(long long), 1, file_out);
	fclose(file_out);


	
	cout << "Total number of full blocks: " << ValueBlockID << endl;
	cout << "Total number of empty blocks: " << numEmptyBlocks << endl;

	writeString_to_file(kwMap, filename_kwBlockIDMap, clientDataDir);

	writeMap_to_file(invertedIdx, filename_invertedIdx, clientDataDir);


	ORAM.build(blockIDs, this->masterKey, this->counter,recursion_info_index);

	STASH S(INDEX_DATA_SIZE);
	S.clear();
	ORAM.enc_decStash(S,0,recursion_info_index);
	
	ORAM.writeStash_to_file(S, INDEX_DATA_SIZE, 0, recursion_info_index);

	//write the state to file
	ORAM.writeCurrentState_to_file(recursion_info_index);
	return 0;
}
int POSUP::buildFilesORAM()
{
	TreeORAM ORAM;
	vector<TYPE_ID> blockIDs;
	//readVector_from_file(blockIDs,"blockIDFile",clientDataDir); //debug purpose
	printf("Building ORAM for files...\n");

	vector<string> filename, filePath;
	filename.clear();
	filePath.clear();
	getAllFiles(dbPath, filename, filePath);
	struct stat file_stat;
	TYPE_ID ValueBlockID = 1;
	Block b;
	b.setData_size(FILE_DATA_SIZE);

	//build the position map
	map <TYPE_ID, pair<TYPE_ID,TYPE_ID>> fileBlockID;

	for (TYPE_ID i = 0; i <recursion_info_file[0].NUM_BLOCKS; i++)
	{
		blockIDs.push_back(i + 1);
	}
	//random permutation using a built-in function (changed later to a crypto-secure PRP)
	std::random_shuffle(blockIDs.begin(), blockIDs.end());

	printf("Writing shuffled blockIDs to file..");
	writeVector_to_file(blockIDs,"blockIDFile",clientDataDir);
	printf("OK!\n");

	printf("Reserving disk for ORAM structures...");
	ORAM.reserveORAM_disk(recursion_info_file);
	printf("OK!\n");

	TYPE_ID PathID = recursion_info_file[0].NUM_NODES / 2;
	for (int i = 0; i < blockIDs.size(); i++)
	{
		pos_map_file[blockIDs[i] - 1].pathID = PathID;
		PathID++;
	}

	for (int i = 0; i < filename.size(); i++)
	{
		string filename_with_path = filePath[i] + filename[i];
		stat(filename_with_path.c_str(), &file_stat);
		int num_oram_blocks = ceil((double)file_stat.st_size / FILE_DATA_SIZE);

		pair<TYPE_ID, TYPE_ID> val(ValueBlockID, pos_map_file[ValueBlockID - 1].pathID);

		pair<TYPE_ID, pair<TYPE_ID, TYPE_ID>> p(stol(filename[i]), val);

		fileBlockID.insert(p);

		FILE* file_in = NULL;
		if ((file_in = fopen(filename_with_path.c_str(), "rb")) == NULL) {
			cout << "[Utils] File Cannot be Opened!!" << endl;
			exit(0);
		}
		for (int ii = 0, j = 0; ii < num_oram_blocks; ii++, j += FILE_DATA_SIZE)
		{
			b.clear();
			b.ID = ValueBlockID;

			if (ii < num_oram_blocks - 1)
			{
				b.nextID = ValueBlockID + 1;
				b.nextPathID = pos_map_file[ValueBlockID].pathID;
			}

			if (j + FILE_DATA_SIZE < file_stat.st_size)
			{
				fread(b.DATA,1, FILE_DATA_SIZE,file_in);
			}
			else
			{
				fread(b.DATA, 1, file_stat.st_size - j,file_in);
			}
			//save block to files
			writeBlock_to_file(b, to_string(ValueBlockID), FileBlockPath);
			ValueBlockID++;
		}
		fclose(file_in);

	}
	for (int i = ValueBlockID; i <= recursion_info_file[0].NUM_BLOCKS; i++)
	{
		b.clear();
		writeBlock_to_file(b, to_string(i), FileBlockPath);
	}
	cout << "Total real num blocks " << ValueBlockID << endl;
	writeMap_to_file(fileBlockID, filename_fileBlockIDMap, clientDataDir);

	// Construct data ORAM
	ORAM.build(blockIDs, this->masterKey, this->counter,recursion_info_file);
	
	// Construct recursive ORAM
	ORAM.buildRecursiveORAM(this->pos_map_file, recursion_info_file[0].NUM_BLOCKS,  recursion_info_file);

	// initialize the stash for data ORAM and recursive ORAM
	for (int i = 0; i < recursion_info_file[0].N_LEVELS + 1; i++)
	{
		STASH S(recursion_info_file[i].DATA_SIZE);
		S.clear();
		ORAM.enc_decStash(S, i, recursion_info_file);
		ORAM.writeStash_to_file(S, recursion_info_file[i].DATA_SIZE, i, recursion_info_file);
	}

	//write current state to file
	ORAM.writeCurrentState_to_file(recursion_info_file);
	cout << "Done!\n";
	return 0;
}
int POSUP::buildRecursiveORAMInfo()
{

	recursion_info_index[0].NUM_BLOCKS = IDX_NUM_BLOCKS;
	recursion_info_index[0].HEIGHT = log2(IDX_NUM_BLOCKS);
	recursion_info_index[0].NUM_LEAVES = IDX_NUM_BLOCKS;
	recursion_info_index[0].NUM_NODES = (int)(pow(2, recursion_info_index[0].HEIGHT + 1) - 1);;
	recursion_info_index[0].DATA_SIZE = INDEX_DATA_SIZE;
	recursion_info_index[0].ORAMPath = new char[IdxORAMPath.size() + 1];
	recursion_info_index[0].BlockPath = new char[IdxBlockPath.size() + 1];

	memset(recursion_info_index[0].ORAMPath, 0, IdxORAMPath.size() + 1);
	memset(recursion_info_index[0].BlockPath, 0, IdxBlockPath.size() + 1);

	strncpy(recursion_info_index[0].ORAMPath, IdxORAMPath.c_str(), IdxORAMPath.size());
	strncpy(recursion_info_index[0].BlockPath, IdxBlockPath.c_str(), IdxBlockPath.size());

	recursion_info_index[0].type = _TYPE_INDEX;
	recursion_info_index[0].evict_order = 0;
	recursion_info_index[0].BLOCK_SIZE = recursion_info_index[0].DATA_SIZE + sizeof(TYPE_ID);

	recursion_info_index[0].BLOCK_SIZE += sizeof(TYPE_ID) + sizeof(TYPE_ID);


	recursion_info_index[0].N_LEVELS = 0;



	recursion_info_file[0].NUM_BLOCKS = FILE_NUM_BLOCKS;
	recursion_info_file[0].HEIGHT = log2(FILE_NUM_BLOCKS);
	recursion_info_file[0].NUM_LEAVES = FILE_NUM_BLOCKS;
	recursion_info_file[0].NUM_NODES = (int)(pow(2, recursion_info_file[0].HEIGHT + 1) - 1);;
	recursion_info_file[0].DATA_SIZE = FILE_DATA_SIZE;


	recursion_info_file[0].ORAMPath = new char[FileORAMPath.size() + 1];
	recursion_info_file[0].BlockPath = new char[FileBlockPath.size() + 1];
	memset(recursion_info_file[0].ORAMPath, 0, FileORAMPath.size() + 1);
	memset(recursion_info_file[0].BlockPath, 0, FileBlockPath.size() + 1);

	strncpy(recursion_info_file[0].ORAMPath, FileORAMPath.c_str(), FileORAMPath.size());
	strncpy(recursion_info_file[0].BlockPath, FileBlockPath.c_str(), FileBlockPath.size());

	recursion_info_file[0].type = _TYPE_FILE;
	recursion_info_file[0].evict_order = 0;
	recursion_info_file[0].BLOCK_SIZE = recursion_info_file[0].DATA_SIZE + sizeof(TYPE_ID);

	recursion_info_file[0].BLOCK_SIZE += sizeof(TYPE_ID) + sizeof(TYPE_ID);



	int FILE_N_LEVELS = (ceil(log(FILE_NUM_BLOCKS) / log(COMPRESSION_RATIO)));
	int curHEIGHT = recursion_info_file[0].HEIGHT;
	recursion_info_file[0].N_LEVELS = FILE_N_LEVELS;
	for (int i = 1; i <= FILE_N_LEVELS; i++)
	{
		recursion_info_file[i].N_LEVELS = FILE_N_LEVELS;
		recursion_info_file[i].ORAMPath = new char[FileORAMPath.size() + 1];
		recursion_info_file[i].BlockPath = new char[FileBlockPath.size() + 1];
		memset(recursion_info_file[i].ORAMPath, 0, FileORAMPath.size() + 1);
		memset(recursion_info_file[i].BlockPath, 0, FileBlockPath.size() + 1);

		strncpy(recursion_info_file[i].ORAMPath, FileORAMPath.c_str(), FileORAMPath.size());
		strncpy(recursion_info_file[i].BlockPath, FileBlockPath.c_str(), FileBlockPath.size());

		curHEIGHT--; //!? change this later for compression ratio > 2
		recursion_info_file[i].HEIGHT = curHEIGHT;
		recursion_info_file[i].NUM_NODES = (TYPE_ID)(pow(2, curHEIGHT + 1) - 1);
		recursion_info_file[i].NUM_LEAVES = pow(2, curHEIGHT);
		recursion_info_file[i].NUM_BLOCKS = recursion_info_file[i].NUM_LEAVES;
		recursion_info_file[i].DATA_SIZE = ceil((log2(recursion_info_file[i - 1].NUM_BLOCKS) + 1) * COMPRESSION_RATIO / BYTE_SIZE);

		recursion_info_file[i].type = _TYPE_FILE;
		recursion_info_file[i].evict_order = 0;

		recursion_info_file[i].BLOCK_SIZE = recursion_info_file[i].DATA_SIZE + sizeof(TYPE_ID);
	}


	return 0;
}


int POSUP::loadState()
{
	//load state into mem (if any) including Cache (if defined K_TOP_CACHING) and Eviction order (if defined DETERMINISTIC EVICTION)
	TreeORAM ORAM;
#if defined(DETERMINISTIC_EVICTION)
	ORAM.readCurrentState_from_file(recursion_info_file);
	ORAM.readCurrentState_from_file(recursion_info_index);
#endif
#if defined(STASH_CACHING)
	ORAM.readStash_from_file(S[_TYPE_INDEX], recursion_info_index[0].DATA_SIZE, 0, recursion_info_index);
	ORAM.readStash_from_file(S[_TYPE_FILE], recursion_info_file[0].DATA_SIZE, 0, recursion_info_file);
#endif
#if defined(K_TOP_CACHING)
	this->loadCache_from_disk(recursion_info_file);
	this->loadCache_from_disk(recursion_info_index);
#endif

	for (int l = 1; l < recursion_info_file[0].N_LEVELS + 1; l++)
	{
		ORAM.readStash_from_file(file_pos_map_S[l - 1], recursion_info_file[l].DATA_SIZE, l, recursion_info_file);
	}
	loadPosMapCache_from_disk(recursion_info_file);

	//load len of empty block array from file
	FILE* file_in = NULL;
	string path = clientDataDir + filename_emptyIdxBlock_len;
	file_in = fopen(path.c_str(), "rb");
	fread(&empty_block_arr_len, sizeof(long long), 1, file_in);
	fclose(file_in);
	
	if (empty_block_arr == NULL)
	{
		empty_block_arr = new unsigned char[empty_block_arr_len];
		memset(empty_block_arr, 0, empty_block_arr_len);
	}

	return 0;
}
int POSUP::saveState()
{
	//save state into disk (if any) including Cache (if defined K_TOP_CACHING) and Eviction order (if defined DETERMINISTIC EVICTION)
	TreeORAM ORAM;
#if defined(DETERMINISTIC_EVICTION)
	ORAM.writeCurrentState_to_file(recursion_info_file);
	ORAM.writeCurrentState_to_file(recursion_info_index);
#endif
#if defined(STASH_CACHING)
	ORAM.writeStash_to_file(S[_TYPE_INDEX], recursion_info_index[0].DATA_SIZE, 0, recursion_info_index);
	ORAM.writeStash_to_file(S[_TYPE_FILE], recursion_info_file[0].DATA_SIZE, 0, recursion_info_file);
#endif
#if defined(K_TOP_CACHING)
	this->saveCache_to_disk(recursion_info_file);
	this->saveCache_to_disk(recursion_info_index);
#endif

	for (int l = 1; l < recursion_info_file[0].N_LEVELS + 1; l++)
	{
		ORAM.writeStash_to_file(file_pos_map_S[l - 1], recursion_info_file[l].DATA_SIZE, l, recursion_info_file);
	}
	savePosMapCache_to_disk(recursion_info_file);

	//write len of empty block array to file
	FILE* file_out = NULL;
	string path = clientDataDir + filename_emptyIdxBlock_len;
	file_out = fopen(path.c_str(), "wb+");
	fwrite(&empty_block_arr_len, sizeof(long long), 1, file_out);
	fclose(file_out);
	
	return 0;
}


int POSUP::retrieve_PORAM(TYPE_ID &pathID, int recurLevel, ORAM_INFO* oram_info)
{
	auto start = time_now;
	auto end = time_now;
	unsigned long long elapsed = 0;
	start = time_now;
	TreeORAM ORAM;
	int H = oram_info[recurLevel].HEIGHT;
	TYPE_ID* fullPathIdx = new TYPE_ID[H + 1];
	ORAM.getFullPathIdx(fullPathIdx, pathID, H);

	unsigned long int bbSize = (H + 1)*BUCKET_SIZE*(oram_info[recurLevel].BLOCK_SIZE);

	unsigned char* serialized_blockBucket = new unsigned char[bbSize];
	memset(serialized_blockBucket, 0, bbSize);
	unsigned char* serialized_metaBucket = new unsigned char[(H + 1)*BUCKET_META_SIZE];
	memset(serialized_metaBucket, 0, (H + 1)*BUCKET_META_SIZE);

	unsigned long int bsSize = STASH_SIZE*(oram_info[recurLevel].BLOCK_SIZE);
	unsigned char* serialized_stash = new unsigned char[bsSize];
	memset(serialized_stash, 0, bsSize);

	unsigned char* serialized_metaStash = new unsigned char[STASH_META_SIZE];
	memset(serialized_metaStash, 0, STASH_META_SIZE);

	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[6] += elapsed;

	start = time_now;
	// Read all blocks along the path
	int pos = 0, pos_meta = 0;
	int n = 0;
	if (recurLevel == 0)
	{
		// Read the stash
#if defined(STASH_CACHING)
		this->loadStash_from_cache(serialized_metaStash, serialized_stash, oram_info);
#else
		ORAM.readStash_from_file(serialized_metaStash, serialized_stash, oram_info[0].DATA_SIZE, 0, oram_info);
#endif
#if defined(K_TOP_CACHING)
		n = CachedLevel[oram_info[recurLevel].type];
		this->loadBucket_from_cache(serialized_metaBucket, &pos_meta, serialized_blockBucket, &pos, oram_info, fullPathIdx, n);
#endif
	}
	else
	{
#if defined(POSITION_MAP_CACHING)
		this->loadPosMapBucket_from_cache(serialized_metaBucket,  serialized_blockBucket, oram_info, fullPathIdx, recurLevel);
		n = H + 1;
		this->loadPosMapStash_from_cache(serialized_metaStash, serialized_stash, oram_info, recurLevel);
#else
		ORAM.readStash_from_file(serialized_metaStash, serialized_stash, oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
#endif
	}
	for (int i = n; i < (H + 1); i++)
	{
		ORAM.readBucket_from_file(&serialized_metaBucket[pos_meta], &serialized_blockBucket[pos], fullPathIdx[i], oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
		pos += (BUCKET_SIZE*(oram_info[recurLevel].BLOCK_SIZE));
		pos_meta += BUCKET_META_SIZE;
	}

	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[7] += elapsed;
	// put blocks into stash via SGX Enclave
	int step_block = NUMBLOCK_PATH_LOAD*oram_info[recurLevel].BLOCK_SIZE;
	int step_stash = NUMBLOCK_STASH_LOAD*oram_info[recurLevel].BLOCK_SIZE;

	start = time_now;
	ecall_readMeta(oram_info[0].eid, serialized_metaBucket, serialized_metaStash, oram_info, recurLevel);
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[8] += elapsed;

	start = time_now;
	for (int i = 0, ii = 0; i < bbSize; i += step_block, ii += NUMBLOCK_PATH_LOAD)
	{
		for (int j = 0, jj = 0; j < bsSize; j += step_stash, jj += NUMBLOCK_STASH_LOAD)
		{
			ecall_readPathData_PORAM(oram_info[0].eid, &serialized_blockBucket[i], ii, &serialized_stash[j], jj, oram_info[recurLevel].DATA_SIZE, oram_info, recurLevel);
		}
	}
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[9] += elapsed;

	start = time_now;
	ecall_getStash_meta(oram_info[0].eid, serialized_metaStash, oram_info);
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[11] += elapsed;
	// write stash into file
	start = time_now;
	if (recurLevel == 0)
	{
#if defined(STASH_CACHING)
		this->saveStash_to_cache(serialized_metaStash, serialized_stash, oram_info);
#else
		ORAM.writeStash_to_file(serialized_metaStash, serialized_stash, oram_info[0].DATA_SIZE, 0, oram_info);
#endif
	}
	else
	{
#if defined(POSITION_MAP_CACHING)
		this->savePosMapStash_to_cache(serialized_metaStash, serialized_stash, oram_info, recurLevel);
#else
		ORAM.writeStash_to_file(serialized_metaStash, serialized_stash, oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
#endif
	}
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[13] += elapsed;

	delete[] fullPathIdx;
	delete[] serialized_blockBucket;
	delete[] serialized_metaBucket;


	delete[] serialized_stash;
	delete[] serialized_metaStash;

	return 1;
}
int POSUP::evict_PORAM(TYPE_ID pathID, int recurLevel, ORAM_INFO* oram_info)
{
	auto start = time_now;
	auto end = time_now;
	unsigned long long elapsed = 0;

	start = time_now;
	TreeORAM ORAM;
	int H = oram_info[recurLevel].HEIGHT;
	TYPE_ID* fullPathIdx = new TYPE_ID[H + 1];
	ORAM.getFullPathIdx(fullPathIdx, pathID, H);

	unsigned long int bbSize = (H + 1)*BUCKET_SIZE*(oram_info[recurLevel].BLOCK_SIZE);

	unsigned char* serialized_blockBucket = new unsigned char[bbSize];
	memset(serialized_blockBucket, 0, bbSize);

	unsigned char* serialized_metaBucket = new unsigned char[(H + 1)*BUCKET_META_SIZE];
	memset(serialized_metaBucket, 0, (H + 1)*BUCKET_META_SIZE);

	unsigned long int bsSize = STASH_SIZE*(oram_info[recurLevel].BLOCK_SIZE);
	unsigned char* serialized_stash = new unsigned char[bsSize];
	memset(serialized_stash, 0, bsSize);

	unsigned char* serialized_metaStash = new unsigned char[STASH_META_SIZE];
	memset(serialized_metaStash, 0, STASH_META_SIZE);
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[14] += elapsed;

	start = time_now;

	if (recurLevel == 0)
	{
		// Read the stash
#if defined(STASH_CACHING)	//CHECK THIS
		this->loadStash_from_cache(serialized_metaStash, serialized_stash, oram_info);
#else
		ORAM.readStash_from_file(serialized_metaStash, serialized_stash, oram_info[0].DATA_SIZE, 0, oram_info);
#endif
	}
	else
	{
#if defined(POSITION_MAP_CACHING)
		this->loadPosMapStash_from_cache(serialized_metaStash, serialized_stash, oram_info, recurLevel);
#else	
		ORAM.readStash_from_file(serialized_metaStash, serialized_stash, oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
#endif
	}

	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[15] += elapsed;


	// put blocks into stash via SGX Enclave
	int step_block = NUMBLOCK_PATH_LOAD*oram_info[recurLevel].BLOCK_SIZE;
	int step_stash = NUMBLOCK_STASH_LOAD*oram_info[recurLevel].BLOCK_SIZE;
	
	// 5. Flush data from Stash back to the read path
	/********************************************SGX *********************************/
	start = time_now;
	//IMPORTANT NOTE: serialized_blockBucket is in reversed order (ii is in reversed order);
	for (int i = 0, ii = (H + 1)*BUCKET_SIZE - 1; i < bbSize; i += step_block, ii -= NUMBLOCK_PATH_LOAD)
	{
		for (int j = 0, jj = 0; j < bsSize; j += step_stash, jj += NUMBLOCK_STASH_LOAD)
		{
			ecall_writePathData_PORAM(oram_info[0].eid, &serialized_blockBucket[i], ii, &serialized_stash[j], jj, oram_info[recurLevel].DATA_SIZE, oram_info, recurLevel, pathID);
		}
	}

	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[17] += elapsed;
	

	start = time_now;
	ecall_getStash_meta(oram_info[0].eid, serialized_metaStash, oram_info);
	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[18] += elapsed;

	start = time_now;
	ecall_getPath_meta(oram_info[0].eid, serialized_metaBucket, oram_info, recurLevel);

	//reverse the path of blocks
	unsigned char* reversed_serialized_blockBucket = new unsigned char[bbSize];
	memset(reversed_serialized_blockBucket, 0, bbSize);
	int numBlock_in_path = (H + 1)*BUCKET_SIZE;

	for (int i = 0, j = numBlock_in_path - 1; i < numBlock_in_path; i++, j--)
	{
		std::memcpy(&reversed_serialized_blockBucket[i*oram_info[recurLevel].BLOCK_SIZE], &serialized_blockBucket[j*oram_info[recurLevel].BLOCK_SIZE], oram_info[recurLevel].BLOCK_SIZE);
	}
	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[19] += elapsed;



	//write to file
	start = time_now;
	int pos = 0, pos_meta = 0;
	int n = 0;
	if (recurLevel == 0)
	{
		// write stash into file
#if defined(STASH_CACHING)
		this->saveStash_to_cache(serialized_metaStash, serialized_stash, oram_info);
#else
		ORAM.writeStash_to_file(serialized_metaStash, serialized_stash, oram_info[0].DATA_SIZE, 0, oram_info);
#endif
#if defined(K_TOP_CACHING)
		n = CachedLevel[oram_info[0].type];
		this->saveBucket_to_cache(serialized_metaBucket, &pos_meta, reversed_serialized_blockBucket, &pos, oram_info, fullPathIdx, n);

#endif
	}
	else
	{
#if defined(POSITION_MAP_CACHING)
		this->savePosMapBucket_to_cache(serialized_metaBucket, reversed_serialized_blockBucket, oram_info, fullPathIdx, recurLevel);
		n = H + 1;
		this->savePosMapStash_to_cache(serialized_metaStash, serialized_stash, oram_info, recurLevel);
#else
		ORAM.writeStash_to_file(serialized_metaStash, serialized_stash, oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
#endif
	}
	for (int i = n; i < (H + 1); i++)
	{
		ORAM.writeBucket_to_file(&serialized_metaBucket[pos_meta], &reversed_serialized_blockBucket[pos], fullPathIdx[i],  oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
		pos += BUCKET_SIZE*(oram_info[recurLevel].BLOCK_SIZE);
		pos_meta += BUCKET_META_SIZE;
	}


	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[20] += elapsed;


	delete[] fullPathIdx;

	delete[] serialized_blockBucket;
	delete[] serialized_metaBucket;

	delete[] serialized_stash;
	delete[] serialized_metaStash;
	delete[] reversed_serialized_blockBucket;

	return 1;


}
int POSUP::recursive_PORAM(TYPE_ID& pid_output, ORAM_INFO* oram_info)
{
	auto start = time_now;
	//prepareAccess_SGX(oram_info[0].eid, oram_info, blockID, 0);
	auto end = time_now;
	double elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[21] += elapsed;
	TreeORAM ORAM;
	// 1. get the recursive blockIDs of the block of interest
	TYPE_ID pathID = 1;
	start = time_now;
	for (int i = oram_info[0].N_LEVELS; i >= 1; i--)
	{
		//cout << "level:" << i << "PathID: " << pathID << endl;
		this->retrieve_PORAM(pathID, i, oram_info);

		this->evict_PORAM(pathID, i, oram_info);
		//call SGX to get next PathID;
		ecall_getNextPathID(oram_info[0].eid, oram_info, i, &pathID);
	}
	pid_output = pathID;
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[22] += elapsed;
	return 1;
}
int POSUP::ODS_PORAM(TYPE_ID pathID, ORAM_INFO* oram_info)
{
	auto start = time_now;
	auto end = time_now;
	unsigned long long elapsed = 0;
	start = time_now;
	//prepareAccess_SGX(oram_info[0].eid, oram_info, blockID, new_curPathID);
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[0] += elapsed;

	TreeORAM ORAM;
	unsigned char* tmp = new unsigned char[oram_info[0].DATA_SIZE];
	int numFileIDs_per_block = ceil((double)INDEX_DATA_SIZE / (sizeof(TYPE_ID) + sizeof(TYPE_ID)));
	unsigned char* pathID_files = new unsigned char[numFileIDs_per_block * sizeof(TYPE_ID)];
	int k = 0;
	do
	{
		start = time_now;
		retrieve_PORAM(pathID, 0, oram_info);
		end = time_now;
		elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		Globals::timestamp[1] += elapsed;
		
		start = time_now;
		evict_PORAM(pathID, 0, oram_info);
		end = time_now;
		elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		Globals::timestamp[4] += elapsed;

		start = time_now;
		ecall_getNextPathID(oram_info[0].eid, oram_info, 0, &pathID);
		end = time_now;
		elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		Globals::timestamp[5] += elapsed;

		//cout << k << "..." << endl;
		Globals::numAccess++;
	} while (pathID > 0);
	delete[] pathID_files;
	delete[] tmp;

	return 1;
}


int POSUP::retrieve_CORAM(TYPE_ID pathID, int recurLevel,  ORAM_INFO* oram_info)
{
	TreeORAM ORAM;
	auto start = time_now;
	auto end = time_now;
	unsigned long long elapsed = 0;

	start = time_now;
	int H = oram_info[recurLevel].HEIGHT;
	TYPE_ID* fullPathIdx = new TYPE_ID[H + 1];
	ORAM.getFullPathIdx(fullPathIdx, pathID, H);
	unsigned long int bbSize = (H + 1)*BUCKET_SIZE*(oram_info[recurLevel].BLOCK_SIZE);
	unsigned char* tmp_ctr = new unsigned char[ENCRYPT_BLOCK_SIZE*(H + 1)];
	unsigned char* serialized_blockBucket = new unsigned char[bbSize];
	memset(serialized_blockBucket, 0, bbSize);
	unsigned char* serialized_metaBucket = new unsigned char[(H + 1)*BUCKET_META_SIZE];
	memset(serialized_metaBucket, 0, (H + 1)*BUCKET_META_SIZE);

	unsigned long int bsSize = STASH_SIZE*(oram_info[recurLevel].BLOCK_SIZE);
	unsigned char* serialized_stash = new unsigned char[bsSize];
	memset(serialized_stash, 0, bsSize);

	unsigned char* serialized_metaStash = new unsigned char[STASH_META_SIZE];
	memset(serialized_metaStash, 0, STASH_META_SIZE);
	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[6] += elapsed;

	// Read all blocks along the path
	start = time_now;
	int pos = 0, pos_meta = 0;
	int n = 0;
	if (recurLevel == 0)
	{
		// Read the stash
#if defined(STASH_CACHING)   
		this->loadStash_from_cache(serialized_metaStash, serialized_stash, oram_info);
#else
		ORAM.readStash_from_file(serialized_metaStash, serialized_stash, oram_info[0].DATA_SIZE, 0, oram_info);
#endif
#if defined(K_TOP_CACHING)
		n = CachedLevel[oram_info[recurLevel].type];
		this->loadBucket_from_cache(serialized_metaBucket, &pos_meta, serialized_blockBucket, &pos, oram_info, fullPathIdx, n);
#endif
	}
	else
	{
#if defined(POSITION_MAP_CACHING)
		this->loadPosMapBucket_from_cache(serialized_metaBucket, serialized_blockBucket, oram_info, fullPathIdx, recurLevel);
		n = H + 1;
		this->loadPosMapStash_from_cache(serialized_metaStash, serialized_stash, oram_info, recurLevel);
#else
		ORAM.readStash_from_file(serialized_metaStash, serialized_stash, oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
#endif
	}
	for (int i = n; i < (H + 1); i++)
	{
		ORAM.readBucket_from_file(&serialized_metaBucket[pos_meta], &serialized_blockBucket[pos], fullPathIdx[i], oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
		pos += (BUCKET_SIZE*(oram_info[recurLevel].BLOCK_SIZE));
		pos_meta += BUCKET_META_SIZE;
	}
	
	//store encrypt counter of block data because we only read it (no rewrite)

	for (int i = 0; i < H + 1; i++)
	{
		std::memcpy(&tmp_ctr[i*ENCRYPT_BLOCK_SIZE], &serialized_metaBucket[i*BUCKET_META_SIZE + ENCRYPT_BLOCK_SIZE], ENCRYPT_BLOCK_SIZE);
	}
	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[7] += elapsed;

	// read 1 block of interest and put into stash via SGX Enclave
	int step_block = NUMBLOCK_PATH_LOAD*oram_info[recurLevel].BLOCK_SIZE;
	int step_stash = NUMBLOCK_STASH_LOAD*oram_info[recurLevel].BLOCK_SIZE;

	start = time_now;
	ecall_readMeta(oram_info[0].eid, serialized_metaBucket,serialized_metaStash, oram_info, recurLevel);
	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[8] += elapsed;

	start = time_now;
	for (int i = 0, ii = 0; i < bbSize; i += step_block, ii += NUMBLOCK_PATH_LOAD)
	{
		ecall_readPathData_CORAM(oram_info[0].eid, &serialized_blockBucket[i], ii, oram_info, recurLevel);
	}
	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[9] += elapsed;

	start = time_now;
	for (int j = 0, jj = 0; j < bsSize; j += step_stash, jj += NUMBLOCK_STASH_LOAD)
	{
		ecall_readStashData_CORAM(oram_info[0].eid, &serialized_stash[j],  jj,  oram_info, recurLevel);
	}

	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[10] += elapsed;

	// write stash into file
	start = time_now;
	ecall_getStash_meta(oram_info[0].eid, serialized_metaStash, oram_info);

	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[11] += elapsed;

	start = time_now;
	// write meta path into file
	ecall_getPath_meta(oram_info[0].eid, serialized_metaBucket, oram_info, recurLevel);
	//recover the enc counter of block data;
	for (int i = 0; i < H + 1; i++)
	{
		std::memcpy(&serialized_metaBucket[i*BUCKET_META_SIZE + ENCRYPT_BLOCK_SIZE], &tmp_ctr[i*ENCRYPT_BLOCK_SIZE], ENCRYPT_BLOCK_SIZE);
	}
	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[12] += elapsed;

	start = time_now;

	pos_meta = 0;
	n = 0;
	if (recurLevel == 0)
	{
#if defined(STASH_CACHING)
		 this->saveStash_to_cache(serialized_metaStash, serialized_stash, oram_info);
#else
		 ORAM.writeStash_to_file(serialized_metaStash, serialized_stash, oram_info[0].DATA_SIZE, 0, oram_info);
#endif

#if defined(K_TOP_CACHING)
		 n = CachedLevel[oram_info[0].type];
		 this->saveBucket_to_cache(serialized_metaBucket, &pos_meta, NULL, NULL, oram_info, fullPathIdx, n);
#endif
	}
	else
	{
#if defined(POSITION_MAP_CACHING)
		this->savePosMapBucket_to_cache(serialized_metaBucket, NULL, oram_info, fullPathIdx, recurLevel);
		n = H + 1;
		this->savePosMapStash_to_cache(serialized_metaStash, serialized_stash, oram_info, recurLevel);
#else
		ORAM.writeStash_to_file(serialized_metaStash, serialized_stash, oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
#endif
	}
	for (int i = n; i < (H + 1); i++)
	{
		ORAM.writeBucket_to_file(&serialized_metaBucket[pos_meta], NULL, fullPathIdx[i], 0, recurLevel, oram_info);
		pos_meta += BUCKET_META_SIZE;
	}
	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[13] += elapsed;

	/*STASH S(oram_info->DATA_SIZE);
	ORAM.readStash_from_file(S, oram_info->DATA_SIZE, oram_info);
	cout << "Stash size after read (i): " << S.getCurrentSize() << endl;*/


	delete[] serialized_metaBucket;
	delete[] serialized_metaStash;
	delete[] serialized_stash;
	delete[] serialized_blockBucket;

	delete[] fullPathIdx;
	delete[] tmp_ctr;
	return 1;
}
int POSUP::evict_CORAM(TYPE_ID evictPath, int recurLevel, ORAM_INFO* oram_info)
{
	auto start = time_now;
	auto end = time_now;
	unsigned long long elapsed = 0;

	start = time_now;
	TreeORAM ORAM;
	int H = oram_info[recurLevel].HEIGHT;
	TYPE_ID* fullEvictIdx = new TYPE_ID[oram_info[recurLevel].HEIGHT + 1];
	ORAM.getFullPathIdx(fullEvictIdx, evictPath, oram_info[recurLevel].HEIGHT);

	unsigned long int bbSize = (H + 1)*BUCKET_SIZE*(oram_info[recurLevel].BLOCK_SIZE);

	unsigned char* serialized_blockBucket = new unsigned char[bbSize];
	memset(serialized_blockBucket, 0, bbSize);
	unsigned char* serialized_metaBucket = new unsigned char[(H + 1)*BUCKET_META_SIZE];
	memset(serialized_metaBucket, 0, (H + 1)*BUCKET_META_SIZE);

	unsigned long int bsSize = STASH_SIZE*(oram_info[recurLevel].BLOCK_SIZE);
	unsigned char* serialized_stash = new unsigned char[bsSize];
	memset(serialized_stash, 0, bsSize);

	unsigned char* serialized_metaStash = new unsigned char[STASH_META_SIZE];
	memset(serialized_metaStash, 0, STASH_META_SIZE);

	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[14] += elapsed;

	int pos = 0, pos_meta = 0;
	int n = 0;
	start = time_now;
	if (recurLevel == 0)
	{
#if defined(STASH_CACHING)
		this->loadStash_from_cache(serialized_metaStash, serialized_stash, oram_info);
#else
		ORAM.readStash_from_file(serialized_metaStash, serialized_stash, oram_info[0].DATA_SIZE, 0, oram_info);
#endif
#if defined(K_TOP_CACHING)
		n = CachedLevel[oram_info[recurLevel].type];
		this->loadBucket_from_cache(serialized_metaBucket, &pos_meta, serialized_blockBucket, &pos, oram_info, fullEvictIdx, n);

#endif
	}
	else
	{
#if defined(POSITION_MAP_CACHING)
		this->loadPosMapBucket_from_cache(serialized_metaBucket, serialized_blockBucket, oram_info, fullEvictIdx, recurLevel);
		n = H + 1;
		this->loadPosMapStash_from_cache(serialized_metaStash, serialized_stash, oram_info, recurLevel);
#else
		ORAM.readStash_from_file(serialized_metaStash, serialized_stash, oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
#endif
	}
	for (int i = n; i < (H + 1); i++)
	{
		ORAM.readBucket_from_file(&serialized_metaBucket[pos_meta], &serialized_blockBucket[pos], fullEvictIdx[i], oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
		pos += (BUCKET_SIZE*(oram_info[recurLevel].BLOCK_SIZE));
		pos_meta += BUCKET_META_SIZE;
	}

	//read the stash

	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[15] += elapsed;

	start = time_now;
	ecall_prepareEviction_CORAM(oram_info[0].eid, serialized_metaBucket, serialized_metaStash, evictPath, oram_info, recurLevel);
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[16] += elapsed;

	int step_block = NUMBLOCK_PATH_LOAD*oram_info[recurLevel].BLOCK_SIZE;
	int step_stash = NUMBLOCK_STASH_LOAD*oram_info[recurLevel].BLOCK_SIZE;

	//current implemtation only works if number of blocks smaller than BUCKET_SIZE is loaded into SGX per time.
	start = time_now;
	for (int j = 0, jj = 0; j < bsSize; j += step_stash, jj += NUMBLOCK_STASH_LOAD)
	{
		ecall_evictCORAM(oram_info[0].eid, &serialized_stash[j], jj, 0, oram_info, recurLevel);
	}

	for (int i = 0, ii = 0; i < bbSize; i += step_block, ii += NUMBLOCK_PATH_LOAD)
	{
		ecall_evictCORAM(oram_info[0].eid, &serialized_blockBucket[i], ii, (ii / BUCKET_SIZE) + 1, oram_info, recurLevel);
	}
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[17] += elapsed;

	start = time_now;

	ecall_getStash_meta(oram_info[0].eid, serialized_metaStash, oram_info);

	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[18] += elapsed;

	start = time_now;
	// write stash into file

	// write meta path into file
	start = time_now;
	ecall_getPath_meta(oram_info[0].eid, serialized_metaBucket, oram_info, recurLevel);
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[19] += elapsed;

	pos = 0; pos_meta = 0;
	n = 0;
	start = time_now;
	if (recurLevel == 0)
	{
#if defined(STASH_CACHING)
		this->saveStash_to_cache(serialized_metaStash, serialized_stash, oram_info);
#else
		ORAM.writeStash_to_file(serialized_metaStash, serialized_stash, oram_info[0].DATA_SIZE, 0, oram_info);
#endif
#if defined(K_TOP_CACHING)
		n = CachedLevel[oram_info[recurLevel].type];
		this->saveBucket_to_cache(serialized_metaBucket, &pos_meta, serialized_blockBucket, &pos, oram_info, fullEvictIdx, n);

#endif
	}
	else
	{
#if defined(POSITION_MAP_CACHING)
		this->savePosMapBucket_to_cache(serialized_metaBucket, serialized_blockBucket, oram_info, fullEvictIdx, recurLevel);
		n = H + 1;
		this->savePosMapStash_to_cache(serialized_metaStash, serialized_stash, oram_info, recurLevel);
#else
		ORAM.writeStash_to_file(serialized_metaStash, serialized_stash, oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
#endif
	}

	for (int i = n; i < (H + 1); i++)
	{
		ORAM.writeBucket_to_file(&serialized_metaBucket[pos_meta], &serialized_blockBucket[pos], fullEvictIdx[i], oram_info[recurLevel].DATA_SIZE, recurLevel, oram_info);
		pos_meta += BUCKET_META_SIZE;
		pos += BUCKET_SIZE*(oram_info[recurLevel].BLOCK_SIZE);
	}

	end =  time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[20] += elapsed;
	//STASH S(oram_info->DATA_SIZE);
	//ORAM.readStash_from_file(S, oram_info->DATA_SIZE, recurLevel, oram_info);
	//cout << "Stash size after write (i): " << S.getCurrentSize() << endl;

	delete[] fullEvictIdx;
	delete[] serialized_blockBucket;
	delete[] serialized_metaBucket;

	delete[] serialized_stash;

	delete[] serialized_metaStash;

	return 0;
}
int POSUP::recursive_CORAM(TYPE_ID& pid_output, ORAM_INFO* oram_info)
{
	auto start = time_now;
//	prepareAccess_SGX(oram_info[0].eid, oram_info, blockID, 0);
	auto end = time_now;
	double elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[21] += elapsed;

	TreeORAM ORAM;
	TYPE_ID* evictPathIDs = new TYPE_ID[(oram_info[0].N_LEVELS + 1) * 2];

	for (int i = 0; i < oram_info[0].N_LEVELS; i++)
	{
#if defined(DETERMINISTIC_EVICTION)
		for (int j = 0; j < 2; j++)
		{
			string str_evict_sequence = ORAM.getEvictString(oram_info[i].evict_order, oram_info[i].HEIGHT);
			TYPE_ID* fullEvictPathIDs = new TYPE_ID[oram_info[i].HEIGHT + 1];
			ORAM.getFullEvictPathIdx(fullEvictPathIDs, str_evict_sequence, oram_info[i].HEIGHT);
			evictPathIDs[i * 2 + j] = fullEvictPathIDs[oram_info[i].HEIGHT];
			oram_info[i].evict_order = (oram_info[i].evict_order + 1) % oram_info[i].NUM_LEAVES;
			delete[] fullEvictPathIDs;
		}
#else
		evictPathIDs[i * 2] = Utils::RandBound(oram_info[i].NUM_LEAVES / 2) + (oram_info[i].NUM_LEAVES - 1);
		evictPathIDs[i * 2 + 1] = Utils::RandBound(oram_info[i].NUM_LEAVES / 2) + oram_info[i].NUM_LEAVES / 2 + (oram_info[i].NUM_LEAVES - 1);
#endif

	}
	evictPathIDs[(int)oram_info[0].N_LEVELS * 2] = 1;
	evictPathIDs[(int)oram_info[0].N_LEVELS * 2 + 1] = 1;

	// 1. get the recursive blockIDs of the block of interest
	TYPE_ID pathID = 1;
	start = time_now;
	for (int i = oram_info[0].N_LEVELS; i >= 1; i--)
	{
		cout << "level:" << i << "PathID: " << pathID << endl;
		this->retrieve_CORAM(pathID, i, oram_info);

		//retrieve the pathID of next level
		ecall_getNextPathID(oram_info[0].eid, oram_info, i, &pathID);

		//perform eviction
		for (int j = 0; j < 2; j++)
		{
			this->evict_CORAM(evictPathIDs[i * 2 + j], i, oram_info);
		}
	}
	//get new path ID at level 0 (insecure) shoud be inside SGX
	pid_output = pathID;
	//getNewPathID_recurLev_SGX(oram_info[0].eid, oram_info, 0, &newPathID);
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[22] += elapsed;

	delete[] evictPathIDs;
	cout << "DONE!" << endl;
	return 1;
}
int POSUP::ODS_CORAM( TYPE_ID pathID,  ORAM_INFO* oram_info)
{
	auto start = time_now;
	auto end = time_now;
	unsigned long long elapsed = 0;

	TYPE_ID evictPathIDs[2];

	unsigned char* tmp = new unsigned char[oram_info[0].DATA_SIZE];
	int numFileIDs_per_block = ceil((double)INDEX_DATA_SIZE / (sizeof(TYPE_ID) + sizeof(TYPE_ID)));
	unsigned char* pathID_files = new unsigned char[numFileIDs_per_block * sizeof(TYPE_ID)];
	TYPE_ID* fullEvictPathIDs = new TYPE_ID[oram_info[0].HEIGHT + 1];
	end = time_now;
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	Globals::timestamp[0] += elapsed;

	TreeORAM ORAM;
	int k = 0;
	do
	{
		k++;
		
		//cout << pathID;
		start = time_now;
		retrieve_CORAM(pathID, 0, oram_info);
		end = time_now;

		elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		Globals::timestamp[1] += elapsed;

		//Eviction
		start = time_now;
#if defined(DETERMINISTIC_EVICTION)
		start = time_now;
		for (int j = 0; j < 2; j++)
		{
			string str_evict_sequence = ORAM.getEvictString(oram_info[0].evict_order, oram_info[0].HEIGHT);
			ORAM.getFullEvictPathIdx(fullEvictPathIDs, str_evict_sequence, oram_info[0].HEIGHT);
			evictPathIDs[j] = fullEvictPathIDs[oram_info[0].HEIGHT];
			oram_info[j].evict_order = (oram_info[0].evict_order + 1) % oram_info[0].NUM_LEAVES;
		}
#else
		evictPathIDs[0] = Utils::RandBound(oram_info[0].NUM_LEAVES / 2) + (oram_info[0].NUM_LEAVES - 1);
		evictPathIDs[1] = Utils::RandBound(oram_info[0].NUM_LEAVES / 2) + (oram_info[0].NUM_LEAVES / 2) + (oram_info[0].NUM_LEAVES - 1);
#endif
		end = time_now;
		elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		Globals::timestamp[3] += elapsed;
		for (int j = 0; j < 2; j++)
		{
			start = time_now;
			this->evict_CORAM(evictPathIDs[j], 0, oram_info);
			end = time_now;
			elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
			Globals::timestamp[4] += elapsed;
		}
		start = time_now;
		ecall_getNextPathID(oram_info[0].eid, oram_info, 0, &pathID);
		end = time_now;
		elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		Globals::timestamp[5] += elapsed;

		Globals::numAccess++;
	} while (pathID>0);
	if (k > 1)
	{
		cout << k <<endl;
	}
	delete[] tmp;
	delete[] pathID_files;
	delete[] fullEvictPathIDs;
	return 1;
}





int POSUP::search(string keyword, int &num)
{
	unsigned char ctr[16];
	unsigned char ctr_reencrypt[16];
	string str_kwMap = "";
	
	vector <pair<string, string>> keywordBlockID;
	
	readKW_map_from_file(keywordBlockID, ctr, filename_kwBlockIDMap, clientDataDir);
	int kwmap_size = keywordBlockID.size();

	memcpy(ctr_reencrypt, ctr, 16);
	inc_dec_ctr(ctr_reencrypt, ceil((double)kwmap_size*KWMAP_VALUE_SIZE / ENCRYPT_BLOCK_SIZE), true);
	str_kwMap.assign((char*)ctr_reencrypt, 16);

	
	//convert to byte array
	unsigned char* KWMap_byte_arr = new unsigned char[keywordBlockID.size()*48];
	long j = 0;
	for (int i = 0; i < keywordBlockID.size(); i++)
	{
		memcpy(&KWMap_byte_arr[j], keywordBlockID[i].first.c_str(), 16);
		j += 16;
		memcpy(&KWMap_byte_arr[j], keywordBlockID[i].second.c_str(), KWMAP_VALUE_SIZE);
		j += 32;
	}
	ecall_setSearchKeyword(recursion_info_index[0].eid, (unsigned char*)keyword.c_str(), keyword.size(),&kwmap_size, ctr);
	
	//for (int i = 0; i < 32* ; i+=)
	//{
		ecall_scanKWMap(recursion_info_index[0].eid, KWMap_byte_arr, keywordBlockID.size() * 48);

	//}
	j = 0;
	for (int i = 0; i < keywordBlockID.size(); i++)
	{
		string tmp;
		tmp.assign((char*)&KWMap_byte_arr[j], 16);
		str_kwMap += tmp;
		j += 16;

		tmp.assign((char*)&KWMap_byte_arr[j], KWMAP_VALUE_SIZE);
		str_kwMap += tmp;
		j += 32;
	}

	TYPE_ID pathID;
	ecall_getPathID(recursion_info_index[0].eid, &pathID);


	try
	{
		//TYPE_ID new_curPathID = Utils::RandBound(recursion_info_index[0].NUM_LEAVES) + (recursion_info_index[0].NUM_LEAVES - 1);

		ecall_prepareAccess(recursion_info_index[0].eid, recursion_info_index, ODS_ACCESS);
#if defined(PATH_ORAM)
		this->ODS_PORAM(pathID, recursion_info_index);
#elif defined(CIRCUIT_ORAM)
		this->ODS_CORAM(pathID, recursion_info_index);
#endif
		showStats(false);
		clearStats();

		
		ecall_getNumFileIDs(recursion_info_index[0].eid, &num);
		unsigned char *fid_arr = new unsigned char[num * sizeof(TYPE_ID)];
		memset(fid_arr, 0, num * sizeof(TYPE_ID));
		int len = 0;

		ecall_getFileIDs(recursion_info_index[0].eid, fid_arr); // this should be encrypted inside sgx prior to sending outside, will update later

		ecall_setFileIDs(recursion_info_file[0].eid,fid_arr,num); // this should be decrypted inside sgx after parsing into, will update later

		for (int i = 0; i < num; i++)
		{
			TYPE_ID FilePathIdx;

			//Recursive Access to get Path of File
			clearStats();
			ecall_prepareAccess(recursion_info_file[0].eid, recursion_info_file, RECURSIVE_ACCESS);

#if defined(PATH_ORAM)
			recursive_PORAM(FilePathIdx, recursion_info_file);
#elif defined(CIRCUIT_ORAM)
			recursive_CORAM(FilePathIdx, recursion_info_file);
#endif
			clearStats();
			ecall_prepareAccess(recursion_info_file[0].eid, recursion_info_file, ODS_ACCESS);
#if defined(PATH_ORAM)
			ODS_PORAM(FilePathIdx, recursion_info_file);
#elif defined(CIRCUIT_ORAM)
			ODS_CORAM(FilePathIdx,  recursion_info_file);
#endif
		}
		cout << " Total NUM: " << num << endl;
		writeString_to_file(str_kwMap, filename_kwBlockIDMap, clientDataDir);
	}
	catch (exception e)
	{
		cout << " ERROR!" << endl;
	}
	return 0;
}

int POSUP::update(string keyword, TYPE_ID fileID)
{
	unsigned char ctr[16];
	unsigned char ctr_reencrypt[16];
	string str_kwMap = "";

	vector <pair<string, string>> keywordBlockID;

	readKW_map_from_file(keywordBlockID, ctr, filename_kwBlockIDMap, clientDataDir);
	int kwmap_size = keywordBlockID.size();

	memcpy(ctr_reencrypt, ctr, 16);
	inc_dec_ctr(ctr_reencrypt, ceil((double)kwmap_size*KWMAP_VALUE_SIZE / ENCRYPT_BLOCK_SIZE), true);
	str_kwMap.assign((char*)ctr_reencrypt, 16);

	ecall_setUpdateKeyword(recursion_info_index[0].eid, (unsigned char*)keyword.c_str(), keyword.size(), fileID, &kwmap_size, ctr);
	

	readByte_array_from_file(empty_block_arr, empty_block_arr_len, filename_emptyIdxBlock, clientDataDir);

	for (long long i = 0 ; i < empty_block_arr_len;  i+= NUM_EMPTY_BLOCK_LOAD*(16+KWMAP_VALUE_SIZE))
	{
		if (i + NUM_EMPTY_BLOCK_LOAD * (16 + KWMAP_VALUE_SIZE) > empty_block_arr_len)
			ecall_scanEmptyBlock(recursion_info_index[0].eid, &empty_block_arr[i], empty_block_arr_len - i);
		else
			ecall_scanEmptyBlock(recursion_info_index[0].eid, &empty_block_arr[i], NUM_EMPTY_BLOCK_LOAD*(16 + KWMAP_VALUE_SIZE));
	}
	writeByte_array_to_file(empty_block_arr, empty_block_arr_len, filename_emptyIdxBlock, clientDataDir);
	

	//convert to byte array
	unsigned char* KWMap_byte_arr = new unsigned char[keywordBlockID.size() * (16 + KWMAP_VALUE_SIZE)];
	long long j = 0;
	for (int i = 0; i < keywordBlockID.size(); i++)
	{
		memcpy(&KWMap_byte_arr[j], keywordBlockID[i].first.c_str(), 16);
		j += 16;
		memcpy(&KWMap_byte_arr[j], keywordBlockID[i].second.c_str(), KWMAP_VALUE_SIZE);
		j += KWMAP_VALUE_SIZE;
	}
	for (long long i = 0; i < keywordBlockID.size()* (16 + KWMAP_VALUE_SIZE); i += (NUM_KWMAP_ENTRIES_LOAD* (16 + KWMAP_VALUE_SIZE)))
	{
		if( i + (NUM_KWMAP_ENTRIES_LOAD* (16 + KWMAP_VALUE_SIZE)) > keywordBlockID.size()* (16 + KWMAP_VALUE_SIZE) )
			ecall_scanKWMap(recursion_info_index[0].eid, &KWMap_byte_arr[i], (keywordBlockID.size()*(16 + KWMAP_VALUE_SIZE)) - i);
		else
			ecall_scanKWMap(recursion_info_index[0].eid, &KWMap_byte_arr[i], (NUM_KWMAP_ENTRIES_LOAD* (16 + KWMAP_VALUE_SIZE)));
	}
	j = 0;
	for (int i = 0; i < keywordBlockID.size(); i++)
	{
		string tmp;
		tmp.assign((char*)&KWMap_byte_arr[j], 16);
		str_kwMap += tmp;
		j += 16;

		tmp.assign((char*)&KWMap_byte_arr[j], KWMAP_VALUE_SIZE);
		str_kwMap += tmp;
		j += KWMAP_VALUE_SIZE;
	}

	TYPE_ID pathID;
	ecall_getPathID(recursion_info_index[0].eid, &pathID);
	try
	{
		ecall_prepareAccess(recursion_info_index[0].eid, recursion_info_index, ODS_ACCESS);
#if defined(PATH_ORAM)
		this->ODS_PORAM(pathID, recursion_info_index);
#elif defined(CIRCUIT_ORAM)
		this->ODS_CORAM(pathID, recursion_info_index);
#endif
		showStats(false);
		clearStats();

		unsigned char *fid_arr = new unsigned char[sizeof(TYPE_ID)];
		memcpy(fid_arr, &fileID, sizeof(TYPE_ID));
		int len = 0;
		
		ecall_setFileIDs(recursion_info_file[0].eid, fid_arr, 1); 

		TYPE_ID FilePathIdx;

		//Recursive Access to get Path of File
		clearStats();
		ecall_prepareAccess(recursion_info_file[0].eid, recursion_info_file, RECURSIVE_ACCESS);

#if defined(PATH_ORAM)
		recursive_PORAM(FilePathIdx, recursion_info_file);
#elif defined(CIRCUIT_ORAM)
		recursive_CORAM(FilePathIdx, recursion_info_file);
#endif
		clearStats();
		ecall_prepareAccess(recursion_info_file[0].eid, recursion_info_file, ODS_ACCESS);

#if defined(PATH_ORAM)
		this->ODS_PORAM(FilePathIdx, recursion_info_file);
#elif defined(CIRCUIT_ORAM)		
		ODS_CORAM(FilePathIdx, recursion_info_file);
#endif
		writeString_to_file(str_kwMap, filename_kwBlockIDMap, clientDataDir);
	}
	catch (exception e)
	{
		cout << " ERROR!" << endl;
	}
	return 0;

}















































#if defined(K_TOP_CACHING) // For data ORAM_0 (non-recursive)
int POSUP::loadCache_from_disk(ORAM_INFO* oram_info)
{
	TreeORAM ORAM;
	TYPE_ID n = pow(2, CachedLevel[oram_info[0].type]) - 1;
	for (TYPE_ID j = 0; j < n; j++)
	{
			ORAM.readBucket_from_file(BUCKETS[oram_info[0].type][j], j, oram_info[0].DATA_SIZE, 0,  oram_info);
	}
	return 1;
}
int POSUP::saveCache_to_disk(ORAM_INFO* oram_info) //only for ODS rightnow
{
	TreeORAM ORAM;
	TYPE_ID n = pow(2, CachedLevel[oram_info[0].type]) - 1;
	for (TYPE_ID j = 0; j < n; j++)
	{
			ORAM.writeBucket_to_file(BUCKETS[oram_info[0].type][j], j, oram_info[0].DATA_SIZE, 0, oram_info);
	}
	return 1;
}
#endif

#if defined(K_TOP_CACHING) // For data ORAM (non-recursive)
int POSUP::loadBucket_from_cache(unsigned char* serializedMetaBucket, int* pos_meta, unsigned char* serializedBlockBucket, int* pos, ORAM_INFO* oram_info, TYPE_ID* fullPathIdx, int PathLevel)
{
	for (int i = 0; i < PathLevel; i++)
	{
		std::memcpy(&serializedMetaBucket[*pos_meta], BUCKETS[oram_info[0].type][fullPathIdx[i]].ctr_meta, ENCRYPT_BLOCK_SIZE);
		*pos_meta += ENCRYPT_BLOCK_SIZE;

		std::memcpy(&serializedMetaBucket[*pos_meta], BUCKETS[oram_info[0].type][fullPathIdx[i]].ctr_data, ENCRYPT_BLOCK_SIZE);
		*pos_meta += ENCRYPT_BLOCK_SIZE;


		for (int j = 0; j < BUCKET_SIZE; j++)
		{
			std::memcpy(&serializedMetaBucket[*pos_meta], &BUCKETS[oram_info[0].type][fullPathIdx[i]].PathID[j], sizeof(TYPE_ID));
			*pos_meta += sizeof(TYPE_ID);
			if (pos !=NULL && serializedBlockBucket != NULL)
			{
				std::memcpy(&serializedBlockBucket[*pos], &BUCKETS[oram_info[0].type][fullPathIdx[i]].blocks[j].nextID, sizeof(TYPE_ID));
				*pos += sizeof(TYPE_ID);
				std::memcpy(&serializedBlockBucket[*pos], &BUCKETS[oram_info[0].type][fullPathIdx[i]].blocks[j].nextPathID, sizeof(TYPE_ID));
				*pos += sizeof(TYPE_ID);

				std::memcpy(&serializedBlockBucket[*pos], &BUCKETS[oram_info[0].type][fullPathIdx[i]].blocks[j].ID, sizeof(TYPE_ID));
				*pos += sizeof(TYPE_ID);
				std::memcpy(&serializedBlockBucket[*pos], BUCKETS[oram_info[0].type][fullPathIdx[i]].blocks[j].DATA, oram_info[0].DATA_SIZE);
				*pos += oram_info[0].DATA_SIZE;



			}
		}
	}
	return 0;
}
int POSUP::saveBucket_to_cache(unsigned char* serializedMetaBucket, int* pos_meta, unsigned char* serializedBlockBucket, int *pos, ORAM_INFO* oram_info, TYPE_ID* fullPathIdx, int PathLevel)
{
	for (int i = 0; i < PathLevel; i++)
	{
		std::memcpy(BUCKETS[oram_info[0].type][fullPathIdx[i]].ctr_meta, &serializedMetaBucket[*pos_meta], ENCRYPT_BLOCK_SIZE);
		*pos_meta += ENCRYPT_BLOCK_SIZE;

		std::memcpy(BUCKETS[oram_info[0].type][fullPathIdx[i]].ctr_data, &serializedMetaBucket[*pos_meta], ENCRYPT_BLOCK_SIZE);
		*pos_meta += ENCRYPT_BLOCK_SIZE;

		for (int j = 0; j < BUCKET_SIZE; j++)
		{
			memcpy(&BUCKETS[oram_info[0].type][fullPathIdx[i]].PathID[j], &serializedMetaBucket[*pos_meta], sizeof(TYPE_ID));
			*pos_meta += sizeof(TYPE_ID);
			if (pos !=NULL && serializedBlockBucket != NULL)
			{
				memcpy(&BUCKETS[oram_info[0].type][fullPathIdx[i]].blocks[j].nextID, &serializedBlockBucket[*pos], sizeof(TYPE_ID));
				*pos += sizeof(TYPE_ID);
				memcpy(&BUCKETS[oram_info[0].type][fullPathIdx[i]].blocks[j].nextPathID, &serializedBlockBucket[*pos], sizeof(TYPE_ID));
				*pos += sizeof(TYPE_ID);

				memcpy(&BUCKETS[oram_info[0].type][fullPathIdx[i]].blocks[j].ID, &serializedBlockBucket[*pos], sizeof(TYPE_ID));
				*pos += sizeof(TYPE_ID);
				memcpy(BUCKETS[oram_info[0].type][fullPathIdx[i]].blocks[j].DATA, &serializedBlockBucket[*pos], oram_info[0].DATA_SIZE);
				*pos += oram_info[0].DATA_SIZE;

			
			}
		}
	}
	return 0;
}
#endif

#if defined(STASH_CACHING)
int POSUP::loadStash_from_cache(unsigned char* serializedMetaStash, unsigned char* serializedBlockStash, ORAM_INFO* oram_info)
{
	int pos_meta = 0, pos = 0;
	memcpy(&serializedMetaStash[pos_meta], this->S[oram_info[0].type].ctr_meta, ENCRYPT_BLOCK_SIZE);
	pos_meta += ENCRYPT_BLOCK_SIZE;
	memcpy(&serializedMetaStash[pos_meta], this->S[oram_info[0].type].ctr_data, ENCRYPT_BLOCK_SIZE);
	pos_meta += ENCRYPT_BLOCK_SIZE;
	for (int j = 0; j < STASH_SIZE; j++)
	{
		memcpy(&serializedMetaStash[pos_meta], &this->S[oram_info[0].type].PathID[j], sizeof(TYPE_ID));
		pos_meta += sizeof(TYPE_ID);
		if (serializedBlockStash != NULL)
		{
			memcpy(&serializedBlockStash[pos], &S[oram_info[0].type].Block[j].nextID, sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
			memcpy(&serializedBlockStash[pos], &S[oram_info[0].type].Block[j].nextPathID, sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);


			memcpy(&serializedBlockStash[pos], &S[oram_info[0].type].Block[j].ID, sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
			memcpy(&serializedBlockStash[pos], S[oram_info[0].type].Block[j].DATA, oram_info[0].DATA_SIZE);
			pos += oram_info[0].DATA_SIZE;

		

		}
	}
	return 0;
}
int POSUP::saveStash_to_cache(unsigned char* serializedMetaStash, unsigned char* serializedBlockStash, ORAM_INFO* oram_info)
{
	int pos = 0, pos_meta = 0;
	memcpy( this->S[oram_info[0].type].ctr_meta, &serializedMetaStash[pos_meta], ENCRYPT_BLOCK_SIZE);
	pos_meta += ENCRYPT_BLOCK_SIZE;
	memcpy( this->S[oram_info[0].type].ctr_data, &serializedMetaStash[pos_meta], ENCRYPT_BLOCK_SIZE);
	pos_meta += ENCRYPT_BLOCK_SIZE;
	for (int j = 0; j < STASH_SIZE; j++)
	{
		memcpy(&S[oram_info[0].type].PathID[j], &serializedMetaStash[pos_meta], sizeof(TYPE_ID));
		pos_meta += sizeof(TYPE_ID);
		if (serializedBlockStash != NULL)
		{
			memcpy(&S[oram_info[0].type].Block[j].nextID, &serializedBlockStash[pos], sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
			memcpy(&S[oram_info[0].type].Block[j].nextPathID, &serializedBlockStash[pos], sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);

			memcpy(&S[oram_info[0].type].Block[j].ID, &serializedBlockStash[pos], sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
			memcpy(S[oram_info[0].type].Block[j].DATA, &serializedBlockStash[pos], oram_info[0].DATA_SIZE);
			pos += oram_info[0].DATA_SIZE;

		

		}
	}
	return 0;
}
#endif

int POSUP::loadPosMapStash_from_cache(unsigned char* serializedMetaStash, unsigned char* serializedBlockStash, ORAM_INFO* oram_info, int recurLev)
{
	int pos_meta = 0, pos = 0;
	memcpy(&serializedMetaStash[pos_meta], this->file_pos_map_S[recurLev-1].ctr_meta, ENCRYPT_BLOCK_SIZE);
	pos_meta += ENCRYPT_BLOCK_SIZE;
	memcpy(&serializedMetaStash[pos_meta], this->file_pos_map_S[recurLev-1].ctr_data, ENCRYPT_BLOCK_SIZE);
	pos_meta += ENCRYPT_BLOCK_SIZE;
	for (int j = 0; j < STASH_SIZE; j++)
	{
		memcpy(&serializedMetaStash[pos_meta], &this->file_pos_map_S[recurLev-1].PathID[j], sizeof(TYPE_ID));
		pos_meta += sizeof(TYPE_ID);
		if (serializedBlockStash != NULL)
		{
			memcpy(&serializedBlockStash[pos], &file_pos_map_S[recurLev-1].Block[j].ID, sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
			memcpy(&serializedBlockStash[pos], file_pos_map_S[recurLev-1].Block[j].DATA, oram_info[recurLev].DATA_SIZE);
			pos += oram_info[recurLev].DATA_SIZE;
		}
	}
	return 0;
}
int POSUP::savePosMapStash_to_cache(unsigned char* serializedMetaStash, unsigned char* serializedBlockStash, ORAM_INFO* oram_info, int recurLev)
{
	int pos = 0, pos_meta = 0;
	memcpy(this->file_pos_map_S[recurLev-1].ctr_meta, &serializedMetaStash[pos_meta], ENCRYPT_BLOCK_SIZE);
	pos_meta += ENCRYPT_BLOCK_SIZE;
	memcpy(this->file_pos_map_S[recurLev-1].ctr_data, &serializedMetaStash[pos_meta], ENCRYPT_BLOCK_SIZE);
	pos_meta += ENCRYPT_BLOCK_SIZE;
	for (int j = 0; j < STASH_SIZE; j++)
	{
		memcpy(&this->file_pos_map_S[recurLev-1].PathID[j], &serializedMetaStash[pos_meta], sizeof(TYPE_ID));
		pos_meta += sizeof(TYPE_ID);
		if (serializedBlockStash != NULL)
		{
			memcpy(&this->file_pos_map_S[recurLev-1].Block[j].ID, &serializedBlockStash[pos], sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
			memcpy(this->file_pos_map_S[recurLev-1].Block[j].DATA, &serializedBlockStash[pos], oram_info[recurLev].DATA_SIZE);
			pos += oram_info[recurLev].DATA_SIZE;
		}
	}
	return 0;
}

int POSUP::loadPosMapBucket_from_cache(unsigned char* serializedMetaBucket, unsigned char* serializedBlockBucket,  ORAM_INFO* oram_info, TYPE_ID* fullPathIdx,  int recurLev)
{
	long long pos_meta = 0, pos = 0;
	for (int i = 0; i < oram_info[recurLev].HEIGHT+1; i++)
	{
		memcpy(&serializedMetaBucket[pos_meta], file_pos_map_bucket[recurLev-1][fullPathIdx[i]].ctr_meta, ENCRYPT_BLOCK_SIZE);
		pos_meta += ENCRYPT_BLOCK_SIZE;
		
		memcpy(&serializedMetaBucket[pos_meta], file_pos_map_bucket[recurLev-1][fullPathIdx[i]].ctr_data, ENCRYPT_BLOCK_SIZE);
		pos_meta += ENCRYPT_BLOCK_SIZE;


		for (int j = 0; j < BUCKET_SIZE; j++)
		{
			memcpy(&serializedMetaBucket[pos_meta], &file_pos_map_bucket[recurLev-1][fullPathIdx[i]].PathID[j], sizeof(TYPE_ID));
			pos_meta += sizeof(TYPE_ID);
			if (serializedBlockBucket != NULL)
			{
				memcpy(&serializedBlockBucket[pos], &file_pos_map_bucket[recurLev-1][fullPathIdx[i]].blocks[j].ID, sizeof(TYPE_ID));
				pos += sizeof(TYPE_ID);
				memcpy(&serializedBlockBucket[pos], file_pos_map_bucket[recurLev-1][fullPathIdx[i]].blocks[j].DATA, oram_info[recurLev].DATA_SIZE);
				pos += oram_info[recurLev].DATA_SIZE;
			}
		}
	}
	return 0;
}
int POSUP::savePosMapBucket_to_cache(unsigned char* serializedMetaBucket, unsigned char* serializedBlockBucket,  ORAM_INFO* oram_info, TYPE_ID* fullPathIdx, int recurLev)
{
	long long pos_meta = 0, pos = 0;
	for (int i = 0; i < oram_info[recurLev].HEIGHT+1; i++)
	{
		memcpy(file_pos_map_bucket[recurLev-1][fullPathIdx[i]].ctr_meta, &serializedMetaBucket[pos_meta], ENCRYPT_BLOCK_SIZE);
		pos_meta += ENCRYPT_BLOCK_SIZE;

		memcpy(file_pos_map_bucket[recurLev-1][fullPathIdx[i]].ctr_data, &serializedMetaBucket[pos_meta], ENCRYPT_BLOCK_SIZE);
		pos_meta += ENCRYPT_BLOCK_SIZE;

		for (int j = 0; j < BUCKET_SIZE; j++)
		{
			memcpy(&file_pos_map_bucket[recurLev-1][fullPathIdx[i]].PathID[j], &serializedMetaBucket[pos_meta], sizeof(TYPE_ID));
			pos_meta += sizeof(TYPE_ID);
			if (serializedBlockBucket != NULL)
			{
				memcpy(&file_pos_map_bucket[recurLev-1][fullPathIdx[i]].blocks[j].ID, &serializedBlockBucket[pos], sizeof(TYPE_ID));
				pos += sizeof(TYPE_ID);
				memcpy(file_pos_map_bucket[recurLev-1][fullPathIdx[i]].blocks[j].DATA, &serializedBlockBucket[pos], oram_info[recurLev].DATA_SIZE);
				pos += oram_info[recurLev].DATA_SIZE;
			}
		}
	}
	return 0;
}

int POSUP::loadPosMapCache_from_disk(ORAM_INFO* oram_info)
{
	TreeORAM ORAM;
	long long n;
	for (int l = 1; l < oram_info[0].N_LEVELS+1; l++)
	{
		n = pow(2, oram_info[l].HEIGHT + 1) - 1;
		for (long long j = 0; j < n; j++)
		{
			ORAM.readBucket_from_file(file_pos_map_bucket[l-1][j], j, oram_info[l].DATA_SIZE, l, oram_info);
		}
	}
	
	return 1;
}
int POSUP::savePosMapCache_to_disk(ORAM_INFO* oram_info) //only for ODS rightnow
{
	TreeORAM ORAM;
	long long n;
	for (int l = 1; l < oram_info[0].N_LEVELS + 1; l++)
	{
		n = pow(2, oram_info[l].HEIGHT + 1) - 1;
		for (long long j = 0; j < n; j++)
		{
			ORAM.writeBucket_to_file(file_pos_map_bucket[l - 1][j], j, oram_info[l].DATA_SIZE, l, oram_info);
		}
	}
	return 1;
}


