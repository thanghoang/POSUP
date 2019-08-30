#pragma once
#include "Block.h"
typedef struct Stash
{
	unsigned char ctr_meta[ENCRYPT_BLOCK_SIZE] = { '0' };
	unsigned char ctr_data[ENCRYPT_BLOCK_SIZE] = { '0' };

	Block* Block = NULL; 
	TYPE_ID* PathID = NULL;
	Stash()
	{
		this->Block = new BLOCK[STASH_SIZE];
		this->PathID = new TYPE_ID[STASH_SIZE];
	}
	~Stash()
	{

	}
	Stash(TYPE_ID data_size)
	{
		if (this->Block == NULL)
		{
			this->Block = new BLOCK[STASH_SIZE];
		}
		if (this->PathID == NULL)
		{
			this->PathID = new TYPE_ID[STASH_SIZE];
		}
		for (int i = 0; i < STASH_SIZE; i++)
		{
			Block[i].setData_size(data_size);
		}
	}
	void clear()
	{
		memset(ctr_meta, 0, ENCRYPT_BLOCK_SIZE);
		memset(ctr_data, 0, ENCRYPT_BLOCK_SIZE);
		for (int i = 0; i < STASH_SIZE; i++)
		{
			Block[i].clear();
			PathID[i] = -1;
		}
	}
	int getCurrentSize()
	{
		int c = 0;
		for (int i = 0; i < STASH_SIZE; i++)
			if (Block[i].ID != 0)
				c++;
		return c;
	}
	void setData_size(TYPE_ID data_size)
	{
		for (int i = 0; i < STASH_SIZE; i++)
		{
			Block[i].setData_size(data_size);
		}
	}
}STASH;