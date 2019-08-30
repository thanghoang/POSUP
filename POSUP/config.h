#pragma once
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>



#ifdef __linux__
#define ENCLAVE_FILE "enclave.signed.so"
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#elif _WIN32
#include "dirent_win.h"
#include <tchar.h>
#define ENCLAVE_FILE _T("Enclave.signed.dll")
#else

#endif


#include <climits>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>
#include <bitset>
#include <algorithm>
#include <map>
#include <chrono>
#include <string>
#include <sstream>
#include <bitset>
#include <limits>
#include <set>
#include <iterator>     // std::istream_iterator
#include <sstream>
#include <chrono>

#include "iaes_asm_interface.h"
#include "iaesni.h"
#include "conf.h"
using namespace std;


template <typename T>
static inline std::string to_string(T value)
{
	std::ostringstream os;
	os << value;
	return os.str();

}

//=== SOCKET COMMAND =========================================
#define CMD_WRITE_PATH		          	0x00000E
#define CMD_READ_PATH		              0x000050
#define CMD_SUCCESS                   "OK"

typedef set<string> TYPE_KEYWORD_DICTIONARY;

// Delimiter separating unique keywords from files
const char* const delimiter = "`-=[]\\;\',./~!@#$%^&*()+{}|:\"<>? \n\t\v\b\r\f\a\00123456789";

#define time_now std::chrono::high_resolution_clock::now()


#include "my_config.h"
