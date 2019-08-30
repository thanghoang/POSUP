#ifndef __BLOCK_H__
#define __BLOCK_H__
#pragma once
#include "config.h"
typedef struct Block
{
	TYPE_ID ID = 0;
	//TYPE_ID PathID =0;
	unsigned char* DATA = NULL;
	unsigned int data_size = 0;
	

	TYPE_ID nextID = 0;
	TYPE_ID nextPathID = -1;

	Block()
	{
		memset(&ID, 0, sizeof(TYPE_ID));
	};
	void setData_size(unsigned int data_size)
	{
		if (DATA != NULL)
		{
			delete DATA;
		}
		DATA = new unsigned char[data_size];
		memset(DATA, 0, data_size);
		this->data_size = data_size;
	}
	Block(unsigned char* ctr, TYPE_ID id,  unsigned char* data, int data_size)
	{
		this->ID = ID;
		this->data_size = data_size;
		DATA = new unsigned char[data_size];
		memcpy(this->DATA, data, data_size);
	}
	Block(TYPE_ID ID, TYPE_ID PathID, unsigned char* data, unsigned int data_size)
	{
		this->ID = ID;
		this->data_size = data_size;
		DATA = new unsigned char[data_size];
		memcpy(this->DATA, data, data_size);
	}
	~Block()
	{
	}
	void clear()
	{
		this->ID = 0;
		memset(DATA, 0, data_size);

		this->nextID = 0;
		this->nextPathID = -1;

	}
	Block& operator=(const Block &obj)
	{
		this->ID = obj.ID;
		this->data_size = obj.data_size;
		memcpy(this->DATA, obj.DATA, data_size);

		this->nextID = obj.nextID;
		this->nextPathID = obj.nextPathID;

		return *this;
	};
}BLOCK;
#endif