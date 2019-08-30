#include "gc.h"

unsigned char Globals::gc[ENCRYPT_BLOCK_SIZE] = {0};
unsigned long long Globals::timestamp[23] = {0};
unsigned int Globals::numAccess = 0 ;
