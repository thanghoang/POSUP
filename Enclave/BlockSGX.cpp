#include "BlockSGX.h"

BlockSGX::BlockSGX()
{
	memset(&ID, 0, sizeof(TYPE_ID));
};

BlockSGX::BlockSGX(TYPE_ID ID, unsigned char* data, unsigned int data_size)
{
	this->ID = ID;
	this->data_size = data_size;
	memcpy(this->DATA, data, data_size);
}
BlockSGX::~BlockSGX()
{
}
void BlockSGX::clear()
{
	this->ID = 0;
	memset(this->DATA, 0, data_size);

	this->nextID = 0;
	this->nextPathID = -1;

}
BlockSGX& BlockSGX::operator=(const BlockSGX &obj)
{
	this->ID = obj.ID;
	this->data_size = obj.data_size;
	memcpy(this->DATA, obj.DATA, data_size);

	this->nextID = obj.nextID;
	this->nextPathID = obj.nextPathID;

	return *this;
}

void BlockSGX::setID(TYPE_ID id)
{
	this->ID = id;
}
TYPE_ID BlockSGX::getID()
{
	return this->ID;
}
void BlockSGX::setDATA(unsigned char* DATA)
{
	memcpy(this->DATA, DATA, data_size);
}
unsigned char* BlockSGX::getDATA()
{
	return this->DATA;
}
void BlockSGX::setData_size(unsigned int data_size)
{
	this->data_size = data_size;
}
unsigned int BlockSGX::getData_size()
{
	return this->data_size;
}

void BlockSGX::setNextID(TYPE_ID id)
{
	this->nextID = id;
}
TYPE_ID BlockSGX::getNextID()
{
	return this->nextID;
}
void BlockSGX::setNextPathID(TYPE_ID pathID)
{
	this->nextPathID = pathID;
}
TYPE_ID BlockSGX::getNextPathID()
{
	return this->nextPathID;
}
unsigned char* BlockSGX::getPointer(int recurLev)
{
	if(recurLev == 0)
		return (unsigned char*)&(this->nextID);
	return (unsigned char*)&(this->ID);
}

