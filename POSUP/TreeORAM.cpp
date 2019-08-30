
#include "TreeORAM.h"
#include "Block.h"
#include "Utils.h"
#include "config.h"
#include "Stash.h"
#include "gc.h"
TreeORAM::TreeORAM()
{
}

TreeORAM::~TreeORAM()
{
}

int TreeORAM::build(vector<TYPE_ID> &blockIDs, unsigned char* masterKey, unsigned long long& counter, ORAM_INFO* oram_info)
{
	BUCKET bucket = BUCKET(oram_info[0].DATA_SIZE);
	
	cout << "=================================================================" << endl;
	cout << "[ORAMTree] Creating ORAM Tree Buckets on Disk" << endl;

	//generate random blocks in leaf-buckets
	TYPE_ID iter = 0;
	Block b;
	b.setData_size(oram_info[0].DATA_SIZE);
	
	//non-leaf buckets are all empty
	for (TYPE_ID i = 0; i < oram_info[0].NUM_NODES / 2; i++)
	{
		bucket.clear();
		this->enc_decBucket(bucket, 0, oram_info);
		this->writeBucket_to_file(bucket, i, oram_info[0].DATA_SIZE, 0, oram_info);
	}

	for (TYPE_ID i = oram_info[0].NUM_NODES / 2; i < oram_info[0].NUM_NODES; i++)
	{
		bucket.clear();
		if (iter >= oram_info[0].NUM_BLOCKS)
			break;
		b.clear();
		readBlock_from_file(b, to_string(blockIDs[iter]), oram_info[0].BlockPath);
		bucket.blocks[0]= b;
			
		//bucket.blocks[0].ID =  blockIDs[iter];
		bucket.PathID[0] = i;
		iter++;
		//write bucket to file
		this->enc_decBucket(bucket, 0, oram_info);
		this->writeBucket_to_file(bucket, i, oram_info[0].DATA_SIZE,0,oram_info);
	}
	cout << "=================================================================" << endl;
	return 0;
}

int TreeORAM::buildRecursiveORAM(TYPE_POS_MAP* pos_map, int pos_map_size, ORAM_INFO* oram_info)
{
	//construct the small ORAMs for recuresively storing the position maps
	vector<TYPE_ID> blockIDs;

	for (int l = 1; l < oram_info[0].N_LEVELS+1; l++)
	{
		TYPE_POS_MAP* tmp_pos_map = new TYPE_POS_MAP[pos_map_size / COMPRESSION_RATIO];

		BUCKET bucket(oram_info[l].DATA_SIZE);
		//generate bucket ID pools
		blockIDs.clear();
		for (TYPE_ID i = 0; i < oram_info[l].NUM_BLOCKS; i++)
		{
			blockIDs.push_back(i + 1);
		}

		for (TYPE_ID i = 0; i < oram_info[l].NUM_NODES / 2; i++)
		{
			bucket.clear();
			this->enc_decBucket(bucket, l, oram_info);
			this->writeBucket_to_file(bucket, i, oram_info[l].DATA_SIZE, l, oram_info);
		}

		//random permutation using a built-in function (changed later to a crypto-secure PRP)
		std::random_shuffle(blockIDs.begin(), blockIDs.end());

		//generate real blocks in leaf-buckets
		TYPE_ID iter = 0;
		int num_bits = ceil(log2(pos_map_size)) + 1;
		for (TYPE_ID i = oram_info[l].NUM_NODES / 2; i < oram_info[l].NUM_NODES; i++)
		{
			bucket.clear();
			int ii = 0;

			if (iter >= oram_info[l].NUM_BLOCKS)
				break;
			bucket.blocks[ii].ID = blockIDs[iter];
			if (i == 0)// for the smallest level
				bucket.PathID[ii] = 1;
			else
			{
				bucket.PathID[ii] = i;
			}
			//bucket.blocks[ii].PathID = i;
			// package pos map into data of this bucket
			int endIdx = blockIDs[iter] * COMPRESSION_RATIO;
			int startIdx = endIdx - COMPRESSION_RATIO;
			int pos = 0;
			for (int j = startIdx; j < endIdx; j++)
			{
				TYPE_ID curPathID = pos_map[j].pathID;
				for (int bit_pos = 0; bit_pos < num_bits; bit_pos++)
				{
					if (BIT_CHECK(&curPathID, bit_pos))
					{
						BIT_SET(&bucket.blocks[ii].DATA[pos / BYTE_SIZE], pos%BYTE_SIZE);
					}
					pos++;
				}
			}
			tmp_pos_map[blockIDs[iter] - 1].pathID = i;
			iter++;
			//encrypt & write bucket to file
			this->enc_decBucket(bucket, l, oram_info);
			this->writeBucket_to_file(bucket, i, oram_info[l].DATA_SIZE, l, oram_info);
		}

		pos_map_size = pos_map_size / COMPRESSION_RATIO;
		delete[] pos_map;
		pos_map = tmp_pos_map;

		cout << "=================================================================" << endl;
	}


	return 0;
}

/**
* Function Name: getFullPathIdx
*
* Description: Creates array of the indexes of the buckets that are on the given path
*
* @param fullPath: (output) The array of the indexes of buckets that are on given path
* @param pathID: (input) The leaf ID based on the index of the bucket in ORAM tree.
* @return 0 if successful
*/
int TreeORAM::getFullPathIdx(TYPE_ID* fullPath, TYPE_ID pathID, int Height)
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

int TreeORAM::enc_decBucket(BUCKET &bucket, int recurLev, ORAM_INFO* oram_info)
{

	int numCipherBlock = ceil((double)BUCKET_SIZE * sizeof(TYPE_ID) / ENCRYPT_BLOCK_SIZE);

	unsigned char* pt_meta = new unsigned char[numCipherBlock * ENCRYPT_BLOCK_SIZE];
	unsigned char* ct_meta = new unsigned char[numCipherBlock * ENCRYPT_BLOCK_SIZE];


	for (int i = 0; i < BUCKET_SIZE; i++)
	{
		memcpy(&pt_meta[i * sizeof(TYPE_ID)], &bucket.PathID[i], sizeof(TYPE_ID));
	}
	unsigned char ctr[ENCRYPT_BLOCK_SIZE];
	memcpy(ctr,Globals::gc,ENCRYPT_BLOCK_SIZE);
	memcpy(bucket.ctr_meta, (unsigned char*)ctr, ENCRYPT_BLOCK_SIZE);
	intel_AES_encdec128_CTR(pt_meta, ct_meta, (unsigned char*) master_key, numCipherBlock, (unsigned char*)ctr);

	cc += ceil((double)(BUCKET_SIZE * sizeof(TYPE_ID)) / ENCRYPT_BLOCK_SIZE);
	for (int ii = 0; ii < BUCKET_SIZE; ii++)
	{
		memcpy(&bucket.PathID[ii], &ct_meta[ii * sizeof(TYPE_ID)], sizeof(TYPE_ID));
	}
	
	numCipherBlock = ceil((double)oram_info[recurLev].BLOCK_SIZE / ENCRYPT_BLOCK_SIZE);

	unsigned char*	pt_data = new unsigned char[numCipherBlock *  ENCRYPT_BLOCK_SIZE];
	unsigned char*	ct_data = new unsigned char[numCipherBlock * ENCRYPT_BLOCK_SIZE];
	

	unsigned long pos = 0;
	
	memcpy(bucket.ctr_data, (unsigned char*)ctr, ENCRYPT_BLOCK_SIZE);

	for (int ii = 0; ii < BUCKET_SIZE; ii++)
	{
		pos = 0;
		if (recurLev == 0)
		{
			memcpy(&pt_data[pos], &bucket.blocks[ii].nextID, sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
			memcpy(&pt_data[pos], &bucket.blocks[ii].nextPathID, sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
		}

		memcpy( &pt_data[pos], &bucket.blocks[ii].ID, sizeof(TYPE_ID));
		pos += sizeof(TYPE_ID);
		memcpy( &pt_data[pos], bucket.blocks[ii].DATA,  oram_info[recurLev].DATA_SIZE);
		pos += oram_info[recurLev].DATA_SIZE;
	
		intel_AES_encdec128_CTR(pt_data, ct_data, (unsigned char*)master_key, numCipherBlock, (unsigned char*)ctr);
		
		pos = 0;
		if (recurLev == 0)
		{
			memcpy(&bucket.blocks[ii].nextID, &ct_data[pos], sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
			memcpy(&bucket.blocks[ii].nextPathID, &ct_data[pos], sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
		}

		memcpy( &bucket.blocks[ii].ID, &ct_data[pos], sizeof(TYPE_ID));
		pos += sizeof(TYPE_ID);

		memcpy(bucket.blocks[ii].DATA, &ct_data[pos], oram_info[recurLev].DATA_SIZE);
		pos += oram_info[recurLev].DATA_SIZE;

	
		
	}
	
	memcpy(Globals::gc, ctr,ENCRYPT_BLOCK_SIZE);

	delete[] pt_meta;
	delete[] pt_data;
	delete[] ct_data;
	delete[] ct_meta;


	return 0;
}

int TreeORAM::enc_decStash(STASH &S, int recurLev, ORAM_INFO* oram_info) //CHECK this
{
	int numCipherBlock = ceil((double)STASH_SIZE * sizeof(TYPE_ID) / ENCRYPT_BLOCK_SIZE);

	unsigned char*pt_meta = new unsigned char[numCipherBlock * ENCRYPT_BLOCK_SIZE];
	unsigned char* 	ct_meta = new unsigned char[ numCipherBlock * ENCRYPT_BLOCK_SIZE];

	//enc meta
	for (int i = 0; i < STASH_SIZE; i++)
	{
		memcpy(&pt_meta[i * sizeof(TYPE_ID)], &S.PathID[i], sizeof(TYPE_ID));
	}
	unsigned char* ctr = new unsigned char[ENCRYPT_BLOCK_SIZE];
	memset(ctr, 0, ENCRYPT_BLOCK_SIZE);
	memcpy(S.ctr_meta, (unsigned char*)ctr, ENCRYPT_BLOCK_SIZE);

	intel_AES_encdec128_CTR(pt_meta, ct_meta, (unsigned char*)master_key, numCipherBlock, (unsigned char*)ctr);
	for (int ii = 0; ii < STASH_SIZE; ii++)
	{
		memcpy(&S.PathID[ii], &ct_meta[ii * sizeof(TYPE_ID)], sizeof(TYPE_ID));
	}


	//enc block

	numCipherBlock = ceil((double)oram_info[recurLev].BLOCK_SIZE / ENCRYPT_BLOCK_SIZE);

	unsigned char*  pt_data = new unsigned char[numCipherBlock*ENCRYPT_BLOCK_SIZE];
	unsigned char* ct_data = new unsigned char[numCipherBlock*ENCRYPT_BLOCK_SIZE];
	
	unsigned long pos = 0;

	memcpy(S.ctr_data, (unsigned char*)ctr, ENCRYPT_BLOCK_SIZE);

	for (int ii = 0; ii < STASH_SIZE; ii++)
	{
		pos = 0;
		if (recurLev == 0)
		{
			memcpy(&pt_data[pos], &S.Block[ii].nextID, sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
			memcpy(&pt_data[pos], &S.Block[ii].nextPathID, sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
		}

		memcpy(&pt_data[pos], &S.Block[ii].ID, sizeof(TYPE_ID));
		pos += sizeof(TYPE_ID);
		memcpy(&pt_data[pos], S.Block[ii].DATA, oram_info[recurLev].DATA_SIZE);
		pos += oram_info[recurLev].DATA_SIZE;

		intel_AES_encdec128_CTR(pt_data, ct_data, (unsigned char*)master_key, numCipherBlock, (unsigned char*)ctr);

		pos = 0;

		if (recurLev == 0)
		{
			memcpy(&S.Block[ii].nextID, &ct_data[pos], sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
			memcpy(&S.Block[ii].nextPathID, &ct_data[pos], sizeof(TYPE_ID));
			pos += sizeof(TYPE_ID);
		}

		memcpy(&S.Block[ii].ID, &ct_data[pos], sizeof(TYPE_ID));
		pos += sizeof(TYPE_ID);
		memcpy(S.Block[ii].DATA, &ct_data[pos], oram_info[recurLev].DATA_SIZE);
		pos += oram_info[recurLev].DATA_SIZE;

		
		
	}

	memcpy(Globals::gc, ctr, ENCRYPT_BLOCK_SIZE);
	
	delete[] pt_meta;
	delete[] ct_meta;
	delete[] pt_data;
	delete[] ct_data;
	return 0;
}
int TreeORAM::readBucket_from_file(BUCKET &bucket, TYPE_ID bucketID,  TYPE_ID data_size, int recurLev, ORAM_INFO* oram_info)
{
	int PathLevel = floor(log2(bucketID + 1));
	long long order = (bucketID+1) - pow(2, PathLevel);
	long long location;
	string path = oram_info[0].ORAMPath + to_string(recurLev);
	path += "/" + metaPath + to_string(PathLevel) + "_meta";
	FILE* file_in = NULL;
	file_in = fopen(path.c_str(), "rb");
	location = order*(BUCKET_SIZE * sizeof(TYPE_ID) +ENCRYPT_BLOCK_SIZE*2);
	fseek(file_in, location , SEEK_SET);

	fread(bucket.ctr_meta, 1, ENCRYPT_BLOCK_SIZE, file_in);
	fread(bucket.ctr_data, 1, ENCRYPT_BLOCK_SIZE, file_in);
	for (int ii = 0; ii < BUCKET_SIZE; ii++)
	{
		fread(&bucket.PathID[ii], 1, sizeof(TYPE_ID), file_in);
	}
	fclose(file_in);

	if (data_size >0)
	{
		path = oram_info[0].ORAMPath + to_string(recurLev);
		path += +"/" + bucketPath + to_string(PathLevel);
		file_in = fopen(path.c_str(), "rb");
		location = order*oram_info[recurLev].BLOCK_SIZE*BUCKET_SIZE;
		fseek(file_in, location, SEEK_SET);

		for (int ii = 0; ii < BUCKET_SIZE; ii++)
		{
			if (recurLev == 0)
			{
				fread(&bucket.blocks[ii].nextID, 1, sizeof(TYPE_ID), file_in);
				fread(&bucket.blocks[ii].nextPathID, 1, sizeof(TYPE_ID), file_in);
			}

			fread(&bucket.blocks[ii].ID, 1, sizeof(TYPE_ID), file_in);
			fread(bucket.blocks[ii].DATA, 1, data_size, file_in);

			
		}
		fclose(file_in);
	}
	return 0;
}

int TreeORAM::writeBucket_to_file(BUCKET &bucket, TYPE_ID bucketID,  TYPE_ID data_size, int recurLev, ORAM_INFO* oram_info)
{
	int PathLevel = floor(log2(bucketID + 1));
	long long order = (bucketID+1) - pow(2, PathLevel);
	long long location;
	string path = oram_info[0].ORAMPath + to_string(recurLev);
	path+= "/" + metaPath + to_string(PathLevel) + "_meta";
	FILE* file_out = NULL;
	file_out = fopen(path.c_str(), "r+b");
	location = order*(BUCKET_SIZE * sizeof(TYPE_ID) + ENCRYPT_BLOCK_SIZE * 2);
	fseek(file_out, location, SEEK_SET);

	fwrite(bucket.ctr_meta, 1, ENCRYPT_BLOCK_SIZE, file_out);
	fwrite(bucket.ctr_data, 1, ENCRYPT_BLOCK_SIZE, file_out);
	for (int ii = 0; ii < BUCKET_SIZE; ii++)
	{
		fwrite(&bucket.PathID[ii], 1, sizeof(TYPE_ID), file_out);
	}
	fclose(file_out);

	if (data_size >0)
	{
		path = oram_info[0].ORAMPath + to_string(recurLev);;
		path +="/" + bucketPath + to_string(PathLevel);
		file_out = fopen(path.c_str(), "r+b");
		location =  order*oram_info[recurLev].BLOCK_SIZE*BUCKET_SIZE;
		fseek(file_out,location, SEEK_SET);

		for (int ii = 0; ii < BUCKET_SIZE; ii++)
		{
			if (recurLev == 0)
			{
				fwrite(&bucket.blocks[ii].nextID, 1, sizeof(TYPE_ID), file_out);
				fwrite(&bucket.blocks[ii].nextPathID, 1, sizeof(TYPE_ID), file_out);
			}

			fwrite(&bucket.blocks[ii].ID, 1, sizeof(TYPE_ID), file_out);
			fwrite(bucket.blocks[ii].DATA, 1, data_size, file_out);
			

		}
		fclose(file_out);
	}
	return 0;
}

int TreeORAM::readBucket_from_file(unsigned char* metaBucket, unsigned char* blocksBucket, TYPE_ID bucketID, TYPE_ID data_size, int recurLev, ORAM_INFO* oram_info)
{
	int PathLevel = floor(log2(bucketID + 1));
	long long order = (bucketID+1)- pow(2, PathLevel);
	long long location ; 	
	long long pos = 0;
	string path = oram_info[0].ORAMPath + to_string(recurLev);
	path += "/" + metaPath + to_string(PathLevel) + "_meta";
	FILE* file_in = NULL;
	file_in = fopen(path.c_str(), "rb");
	location = order*(BUCKET_SIZE * sizeof(TYPE_ID) + ENCRYPT_BLOCK_SIZE * 2);
	fseek(file_in, location, SEEK_SET);

	fread(&metaBucket[pos], 1, ENCRYPT_BLOCK_SIZE, file_in);
	pos += ENCRYPT_BLOCK_SIZE;
	fread(&metaBucket[pos], 1, ENCRYPT_BLOCK_SIZE, file_in);
	pos += ENCRYPT_BLOCK_SIZE;
	for (int ii = 0; ii < BUCKET_SIZE; ii++)
	{
		fread(&metaBucket[pos], 1, sizeof(TYPE_ID), file_in);
		pos += sizeof(TYPE_ID);
	}
	fclose(file_in);

	if (data_size >0)
	{
		pos = 0;
		path = oram_info[0].ORAMPath + to_string(recurLev);
		path +="/" + bucketPath + to_string(PathLevel);
		file_in = fopen(path.c_str(), "rb");
		location = order*oram_info[recurLev].BLOCK_SIZE*BUCKET_SIZE;
		fseek(file_in, location, SEEK_SET);

		for (int ii = 0; ii < BUCKET_SIZE; ii++)
		{
			if (recurLev == 0)
			{
				fread(&blocksBucket[pos], 1, sizeof(TYPE_ID), file_in);
				pos += sizeof(TYPE_ID);
				fread(&blocksBucket[pos], 1, sizeof(TYPE_ID), file_in);
				pos += sizeof(TYPE_ID);
			}

			fread(&blocksBucket[pos], 1, sizeof(TYPE_ID), file_in);
			pos += sizeof(TYPE_ID);
			fread(&blocksBucket[pos], 1, data_size, file_in);
			pos += data_size;
			
			

		}
		fclose(file_in);
	}
	return 0;
}

int TreeORAM::writeBucket_to_file(unsigned char* metaBucket, unsigned char* blocksBucket, TYPE_ID bucketID, TYPE_ID data_size, int recurLev, ORAM_INFO* oram_info)
{
	int PathLevel = floor(log2(bucketID + 1));
	long long order = (bucketID+1) - pow(2, PathLevel);
	long long location;
	int pos = 0;
	string path = oram_info[0].ORAMPath + to_string(recurLev);
	path +="/" + metaPath + to_string(PathLevel) + "_meta";
	FILE* file_out = NULL;
	file_out = fopen(path.c_str(), "r+b");
	location = order*(BUCKET_SIZE * sizeof(TYPE_ID) + ENCRYPT_BLOCK_SIZE * 2);
	fseek(file_out, location, SEEK_SET);

	fwrite(&metaBucket[pos], 1, ENCRYPT_BLOCK_SIZE, file_out);
	pos += ENCRYPT_BLOCK_SIZE;
	fwrite(&metaBucket[pos], 1, ENCRYPT_BLOCK_SIZE, file_out);
	pos += ENCRYPT_BLOCK_SIZE;

	for (int ii = 0; ii < BUCKET_SIZE; ii++)
	{
		fwrite(&metaBucket[pos], 1, sizeof(TYPE_ID), file_out);
		pos += sizeof(TYPE_ID);
	}
	fclose(file_out);

	if (data_size >0)
	{
		pos = 0;
		path = oram_info[0].ORAMPath + to_string(recurLev);
		path +="/" + bucketPath + to_string(PathLevel);
		file_out = fopen(path.c_str(), "r+b");
		location =  order*oram_info[recurLev].BLOCK_SIZE*BUCKET_SIZE;
		fseek(file_out, location, SEEK_SET);

		for (int ii = 0; ii < BUCKET_SIZE; ii++)
		{
			if (recurLev == 0)
			{
				fwrite(&blocksBucket[pos], 1, sizeof(TYPE_ID), file_out);
				pos += sizeof(TYPE_ID);
				fwrite(&blocksBucket[pos], 1, sizeof(TYPE_ID), file_out);
				pos += sizeof(TYPE_ID);
			}

			fwrite(&blocksBucket[pos], 1, sizeof(TYPE_ID), file_out);
			pos += sizeof(TYPE_ID);
			fwrite(&blocksBucket[pos], 1, data_size, file_out);
			pos += data_size;

			
		}
		fclose(file_out);
	}
	return 0;
}

int TreeORAM::readStash_from_file(unsigned char* metaStash, unsigned char* blocksStash, TYPE_ID data_size, int recurLev, ORAM_INFO* oram_info)
{
	string path = oram_info[0].ORAMPath + to_string(recurLev);
	path += "/" + stashPath + "Stash" + "_meta";
	FILE* file_in = NULL;
	file_in = fopen(path.c_str(), "rb");
	int pos = 0;
	fread(&metaStash[pos], 1, ENCRYPT_BLOCK_SIZE, file_in);
	pos += ENCRYPT_BLOCK_SIZE;
	fread(&metaStash[pos], 1, ENCRYPT_BLOCK_SIZE, file_in);
	pos += ENCRYPT_BLOCK_SIZE;

	for (int ii = 0; ii < STASH_SIZE; ii++)
	{
		fread(&metaStash[pos], 1, sizeof(TYPE_ID), file_in);
		pos += sizeof(TYPE_ID);
	}
	fclose(file_in);
	if (data_size > 0)
	{
		pos = 0;
		path = oram_info[0].ORAMPath + to_string(recurLev);
		path += "/" + stashPath + "Stash";
		file_in = fopen(path.c_str(), "rb");
		for (int ii = 0; ii < STASH_SIZE; ii++)
		{
			if (recurLev == 0)
			{
				fread(&blocksStash[pos], 1, sizeof(TYPE_ID), file_in);
				pos += sizeof(TYPE_ID);
				fread(&blocksStash[pos], 1, sizeof(TYPE_ID), file_in);
				pos += sizeof(TYPE_ID);
			}

			fread(&blocksStash[pos], 1, sizeof(TYPE_ID), file_in);
			pos += sizeof(TYPE_ID);
			fread(&blocksStash[pos], 1, data_size, file_in);
			pos += data_size;

			
		}
		fclose(file_in);
	}
	return 0;
}

int TreeORAM::writeStash_to_file(unsigned char* metaStash, unsigned char* blocksStash, TYPE_ID data_size, int recurLev, ORAM_INFO* oram_info)
{
	FILE* file_out = NULL;
	string path = oram_info[0].ORAMPath + to_string(recurLev);
	path += "/" + stashPath + "Stash" + "_meta";
	file_out = fopen(path.c_str(), "wb+");
	int pos = 0;
	fwrite(&metaStash[pos], 1, ENCRYPT_BLOCK_SIZE, file_out);
	pos += ENCRYPT_BLOCK_SIZE;
	fwrite(&metaStash[pos], 1, ENCRYPT_BLOCK_SIZE, file_out);
	pos += ENCRYPT_BLOCK_SIZE;


	for (int ii = 0; ii < STASH_SIZE; ii++)
	{
		fwrite(&metaStash[pos], 1, sizeof(TYPE_ID), file_out);
		pos += sizeof(TYPE_ID);
	}
	fclose(file_out);
	if (data_size > 0)
	{
		pos = 0;
		path = oram_info[0].ORAMPath + to_string(recurLev);
		path += "/" + stashPath + "Stash";
		file_out = fopen(path.c_str(), "wb+");
		for (int ii = 0; ii < STASH_SIZE; ii++)
		{
			if (recurLev == 0)
			{
				fwrite(&blocksStash[pos], 1, sizeof(TYPE_ID), file_out);
				pos += sizeof(TYPE_ID);
				fwrite(&blocksStash[pos], 1, sizeof(TYPE_ID), file_out);
				pos += sizeof(TYPE_ID);
			}

			fwrite(&blocksStash[pos], 1, sizeof(TYPE_ID), file_out);
			pos += sizeof(TYPE_ID);
			fwrite(&blocksStash[pos], 1, data_size, file_out);
			pos += data_size;
		}
		fclose(file_out);
	}
	return 0;
}

int TreeORAM::readStash_from_file(STASH &S, TYPE_ID data_size, int recurLevel, ORAM_INFO* oram_info)
{
	string path = oram_info[0].ORAMPath + to_string(recurLevel);
	path += "/" + stashPath + "Stash" + "_meta";
	FILE* file_in = NULL;
	file_in = fopen(path.c_str(), "rb");
	fread(S.ctr_meta, 1, ENCRYPT_BLOCK_SIZE, file_in);
	fread(S.ctr_data, 1, ENCRYPT_BLOCK_SIZE, file_in);
	for (int ii = 0; ii < STASH_SIZE; ii++)
	{
		fread(&S.PathID[ii], 1, sizeof(TYPE_ID), file_in);
	}
	fclose(file_in);
	if (data_size > 0)
	{
		path = oram_info[0].ORAMPath + to_string(recurLevel);;
		path += "/" + stashPath + "Stash";
		file_in = fopen(path.c_str(), "rb");
		for (int ii = 0; ii < STASH_SIZE; ii++)
		{
			if (recurLevel == 0)
			{
				fread(&S.Block[ii].nextID, 1, sizeof(TYPE_ID), file_in);
				fread(&S.Block[ii].nextPathID, 1, sizeof(TYPE_ID), file_in);
			}

			fread(&S.Block[ii].ID, 1, sizeof(TYPE_ID), file_in);
			fread(S.Block[ii].DATA, 1, data_size, file_in);

			
		}

		fclose(file_in);
	}
	return 0;
}

int TreeORAM::writeStash_to_file(STASH S, TYPE_ID data_size, int recurLev, ORAM_INFO* oram_info)
{
	FILE* file_out = NULL;
	string path = oram_info[0].ORAMPath + to_string(recurLev);
	path += "/" + stashPath + "Stash" + "_meta";
	file_out = fopen(path.c_str(), "wb+");
	fwrite(S.ctr_meta, 1, ENCRYPT_BLOCK_SIZE, file_out);
	fwrite(S.ctr_data, 1, ENCRYPT_BLOCK_SIZE, file_out);
	for (int ii = 0; ii < STASH_SIZE; ii++)
	{
		fwrite(&S.PathID[ii], 1, sizeof(TYPE_ID), file_out);
	}
	fclose(file_out);
	if (data_size > 0)
	{
		path = oram_info[0].ORAMPath + to_string(recurLev);
		path += "/" + stashPath + "Stash";
		file_out = fopen(path.c_str(), "wb+");
		for (int ii = 0; ii < STASH_SIZE; ii++)
		{
			if (recurLev == 0)
			{
				fwrite(&S.Block[ii].nextID, 1, sizeof(TYPE_ID), file_out);
				fwrite(&S.Block[ii].nextPathID, 1, sizeof(TYPE_ID), file_out);
			}

			fwrite(&S.Block[ii].ID, 1, sizeof(TYPE_ID), file_out);
			fwrite(S.Block[ii].DATA, 1, data_size, file_out);

			
		}
		fclose(file_out);
	}
	return 0;
}

/**
* Function Name: getFullEvictPathIdx
*
* Description: Determine the indices of source bucket, destination bucket and sibling bucket
* residing on the eviction path
*
* @param srcIdx: (output) source bucket index array
* @param destIdx: (output) destination bucket index array
* @param siblIdx: (output) sibling bucket index array
* @param str_evict: (input) eviction edges calculated by binary ReverseOrder of the eviction number
* @return 0 if successful
*/
int TreeORAM::getFullEvictPathIdx(TYPE_ID *fullPathIdx, string str_evict, int Height)
{
	fullPathIdx[0] = 0;
	for (int i = 0; i < Height; i++)
	{
		if (str_evict[i] - '0' == 1)
		{
			fullPathIdx[i + 1] = fullPathIdx[i] * 2 + 2;
		}
		else
		{
			fullPathIdx[i + 1] = fullPathIdx[i] * 2 + 1;
		}
	}
	return 0;
}

/**
* Function Name: getEvictString
*
* Description: Generates the path for eviction acc. to eviction number based on reverse
* lexicographical order.
* [For details refer to 'Optimizing ORAM and using it efficiently for secure computation']
*
* @param n_evict: (input) The eviction number
* @return Bit sequence of reverse lexicographical eviction order
*/
string TreeORAM::getEvictString(TYPE_ID n_evict, int Height)
{
	string s = std::bitset<sizeof(TYPE_ID)*BYTE_SIZE>(n_evict).to_string();
	reverse(s.begin(), s.end());
	s.resize(Height);
	return s;
}

int TreeORAM::writeCurrentState_to_file( ORAM_INFO* oram_info)
{
	FILE* file_out = NULL;
	int pos = 0;
	string path(oram_info[0].ORAMPath);
	path+= +"/State";
	file_out = fopen(path.c_str(), "wb+");
	for (int i = 0; i < oram_info[0].N_LEVELS+1; i++)
	{
		fwrite(&oram_info[i].evict_order, 1, sizeof(TYPE_ID), file_out);
	}

	fclose(file_out);
	return 0;
}

int TreeORAM::readCurrentState_from_file(ORAM_INFO* oram_info)
{
	string path(oram_info[0].ORAMPath);
	path += +"/State";
	FILE* file_in = NULL;
	file_in = fopen(path.c_str(), "rb");
	for (int i = 0; i < oram_info[0].N_LEVELS; i++)
	{
		fread(&oram_info[i].evict_order, 1, sizeof(TYPE_ID), file_in);
	}
	return 0;
}

void TreeORAM::reserveORAM_disk(ORAM_INFO* oram_info)
{
	for (int n = 0; n < oram_info[0].N_LEVELS + 1; n++)
	{
		Bucket B;
		B.setData_size(oram_info[n].DATA_SIZE);
		B.clear();
		for (int i = 0; i < oram_info[n].HEIGHT + 1; i++)
		{
			//reserve bucket meta
			string path = oram_info[0].ORAMPath + to_string(n);
			path += "/" + metaPath + to_string(i) + "_meta";
			FILE* file_out = NULL;
			file_out = fopen(path.c_str(), "wb+");
			for (long j = 0; j < pow(2, i); j++)
			{
				fwrite(B.ctr_meta, 1, ENCRYPT_BLOCK_SIZE, file_out);
				fwrite(B.ctr_data, 1, ENCRYPT_BLOCK_SIZE, file_out);
				for (int k = 0; k < BUCKET_SIZE; k++)
				{
					fwrite(&B.PathID[k], 1, sizeof(TYPE_ID), file_out);
				}

			}
			fclose(file_out);

			//reserve bucket data
			path = oram_info[0].ORAMPath + to_string(n);
			path += "/" + bucketPath + to_string(i);
			file_out = NULL;
			file_out = fopen(path.c_str(), "wb+");
			for (long j = 0; j < pow(2, i); j++)
			{
				for (int k = 0; k < BUCKET_SIZE; k++)
				{
					fwrite(&B.blocks[k].ID, 1, sizeof(TYPE_ID), file_out);
					fwrite(B.blocks[k].DATA, 1, oram_info[n].DATA_SIZE, file_out);

					if (n == 0)
					{
						fwrite(&B.blocks[k].nextID, 1, sizeof(TYPE_ID), file_out);
						fwrite(&B.blocks[k].nextPathID, 1, sizeof(TYPE_ID), file_out);
					}
				}

			}
			fclose(file_out);
		}
	}
	
}
