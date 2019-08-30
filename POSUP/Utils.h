#ifndef __UTILS_H__
#define __UTILS_H__
#pragma once
#include "config.h"
#include "Block.h"
#include "gc.h"
constexpr char hexmap[] = { '0', '1', '2', '3', '4', '5', '6', '7',
'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
std::string hexStr(unsigned char *data, int len);
void print_ucharstring(const unsigned char *pInput, int in_len);
class Utils
{
public:
	Utils();
	~Utils();


	static inline unsigned long long _LongRand()
	{
		unsigned char MyBytes[4];
		unsigned long long MyNumber = 0;
		unsigned char * ptr = (unsigned char *)&MyNumber;

		MyBytes[0] = rand() % 256; //0-255
		MyBytes[1] = rand() % 256; //256 - 65535
		MyBytes[2] = rand() % 256; //65535 -
		MyBytes[3] = rand() % 256; //16777216

		memcpy(ptr + 0, &MyBytes[0], 1);
		memcpy(ptr + 1, &MyBytes[1], 1);
		memcpy(ptr + 2, &MyBytes[2], 1);
		memcpy(ptr + 3, &MyBytes[3], 1);

		return(MyNumber);
	}

	static inline unsigned long long RandBound(unsigned long long bound)
	{
		return _LongRand() % bound;
	}
};
string trim(const string& str);
int readBlock_from_file(Block &b, string filename, string dirPath);
int writeBlock_to_file(Block b, string filename, string dirPath);

int readMap_from_file(map <string, string> &map, string filename, string dirPath);
int writeMap_to_file(map <string, string> map, string filename, string dirPath);



int writeMap_to_file(map <TYPE_ID, pair<TYPE_ID, TYPE_ID>> map, string filename, string dirPath);
int readMap_from_file(map <TYPE_ID, pair<TYPE_ID, TYPE_ID>> &map,  string filename, string dirPath);

int readMap_from_file(map <string, pair<TYPE_ID, TYPE_ID>> &map, string filename, string dirPath);
int writeMap_to_file(map <string, pair<TYPE_ID, TYPE_ID>> map, string filename, string dirPath);

std::vector<std::string> split(const std::string &text, char sep);
int getAllFiles(string path, vector<string> &filename, vector<string> &filePath);


int writeVector_to_file(vector<TYPE_ID> blockIDs, string filename, string dirPath);
int readVector_from_file(vector<TYPE_ID>&blockIDs, string filename,string dirPath);


int readKW_map_from_file(vector <pair<string, string>> &map, unsigned char* ctr, string filename, string dirPath);
int writeString_to_file(string &s, string filename, string dirPath);




void inc_dec_ctr(unsigned char* input, unsigned long long number, bool isInc);




int writeByte_array_to_file(unsigned char* byte_arr, long long len, string filename, string dirPath);
int readByte_array_from_file(unsigned char* byte_arr, long long &len, string filename, string dirPath);



int reverseByteArray(unsigned char* data, int len, int elem_size);

void showStats(int isRecursive);
void clearStats();
#endif
