#pragma once
#include "Block.h";
#include "config.h"
typedef struct Bucket
{
	unsigned char ctr_meta[ENCRYPT_BLOCK_SIZE] = { '0' };
	unsigned char ctr_data[ENCRYPT_BLOCK_SIZE] = { '0' };

	TYPE_ID PathID[BUCKET_SIZE] = { -1 };
	BLOCK blocks[BUCKET_SIZE];
	Bucket()
	{

	}
	Bucket(unsigned int data_size)
	{
		for (int i = 0; i < BUCKET_SIZE; i++)
		{
			blocks[i].setData_size(data_size);
		}
	}
	void setData_size(unsigned int data_size)
	{
		for (int i = 0; i < BUCKET_SIZE; i++)
		{
			blocks[i].setData_size(data_size);
		}
	}
	void clear()
	{
		memset(ctr_meta, 0, ENCRYPT_BLOCK_SIZE);
		memset(ctr_data, 0, ENCRYPT_BLOCK_SIZE);

		for (int i = 0; i < BUCKET_SIZE; i++)
		{
			blocks[i].clear();
			PathID[i] = -1;
		}
	}
	int getEmptySlot()
	{
		for (int i = 0; i < BUCKET_SIZE; i++)
		{
			if (PathID[i] == -1)
				return i;
		}
		return -1;
	}
	~Bucket()
	{
	}
}BUCKET;