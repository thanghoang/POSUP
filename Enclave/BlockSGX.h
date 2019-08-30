#pragma once
#include "conf.h"
#include "stdlib.h"
class BlockSGX
{
	unsigned int data_size = 0;
	TYPE_ID nextID = 0;
	TYPE_ID nextPathID = -1;

	TYPE_ID ID = 0;
	unsigned char DATA[];


public:
	BlockSGX();
	BlockSGX(TYPE_ID ID, unsigned char* data, unsigned int data_size);
	~BlockSGX();
	void clear();
	BlockSGX& operator=(const BlockSGX &obj);

	void setID(TYPE_ID id);
	TYPE_ID getID();
	void setDATA(unsigned char* DATA);
	unsigned char* getDATA();
	void setData_size(unsigned int data_size);
	unsigned int getData_size();

	void setNextID(TYPE_ID id);
	TYPE_ID getNextID();
	void setNextPathID(TYPE_ID pathID);
	TYPE_ID getNextPathID();
	unsigned char* getPointer(int recurLev);
};

