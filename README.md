# POSUP: Oblivious Search and Update Platform with SGX

Basic implementation of POSUP platform appeard in PETS'19. The full paper can be downloaded from [here](https://www.degruyter.com/downloadpdf/j/popets.2019.2019.issue-1/popets-2019-0010/popets-2019-0010.pdf). This project is mainly built on MS Visual Studio, but it can operate on Linux environment as well. 


# Prerequisites
1. Intel SGX (Linux version [here](https://github.com/intel/linux-sgx), Windows setup [here](https://downloadcenter.intel.com/download/28972/Intel-Software-Guard-Extensions-Intel-SGX-Driver-for-Windows-))

2. Intel AES-NI (Sample code was taken down from Intel website, though it can be found on ``ext/aes-ni`` folder)

3. Libtomcrypt (download [here](https://github.com/libtom/libtomcrypt))

# Configuration
There are TWO main configuration files in POSUP, including ``include/conf.h`` and ``POSUP/my_config.h`` with important parameters as follows:

## File ``include/conf.h``: Contain basic ORAM parameters

```
#define PATH_ORAM               -> Enable this (and disable the below) to use Path-ORAM controller
#define CIRCUIT_ORAM            -> Enable this (and disable the above) to use Circuit-ORAM controller


#define STASH_CACHING           -> Enable this to cache the Stash on RAM
#define K_TOP_CACHING           -> Enable this to cache the k-top tree on RAM

#if defined(K_TOP_CACHING)
	const int CachedLevel[2] = { 3,4 };       -> Indicate the k-value to be cached of the index tree and the file tree, respectively 
#endif

#define POSITION_MAP_CACHING    -> Enable this to cache the file position map on RAM


#define STASH_SIZE 40		-> Declare the size of the stash (preferably 80)


#define INDEX_DATA_SIZE 1012 	-> Declare the payload size of each ORAM block in the index tree
#define FILE_DATA_SIZE 3060 	-> Declare the payload size of each the ORAM block in the file tree
// The total block size = |payload| + |block_ID| + |next block ID | + |next_path_ID|, which should be a multiple of 1024.

#define IDX_NUM_BLOCKS 512 	-> Declare number of blocks in the index tree
#define FILE_NUM_BLOCKS 32 	-> Declare number of blocks in the file tree

#define NUMBLOCK_PATH_LOAD BUCKET_SIZE	-> Declare number of blocks in the tree path to be processed by SGX each time
#define NUMBLOCK_STASH_LOAD 40		-> Declare number of blocks in the stash to be processed by SGX each time

#define NUM_EMPTY_BLOCK_LOAD 		-> Declare number of blocks in the empty block array to be processed by SGX each time
#define NUM_KWMAP_ENTRIES_LOAD 20	->  Declare number of entries in the keyword map to be processed by SGX each time

```


## File ``POSUP/my_config.h``: Contain dataset and storage location. Unzip the file data.zip to better understanding the structure.

```

const std::string rootPath = "C:/data/";		-> Root path of the DB input

const std::string clientLocalDir = rootPath + "client_local/";
const std::string clientDataDir = rootPath + "client/";
const std::string IdxORAMPath = rootPath + "server/idx/";
const std::string FileORAMPath = rootPath + "server/file/";

const std::string IdxBlockPath = rootPath + "idxBlock/";
const std::string FileBlockPath = rootPath + "fileBlock/";

```



# Build & Compile

## Windows
Download the entire code and open the file ``POSUP.sln`` with Microsoft Visual Studio.

## Linux

Download the code and execute ``make`` in the project root folder, which will produce a binary executable file named ``app`` loated at the same directory.

## Execution

Run the program and it will display the menu showing the main functionalities of POSUP, including setup, search, update. Follow the instruction in the menu and enjoy! ;)


## Important Note:
The implementation is just for the proof of concept. There are several places in the code that was implemented INSECURELY for the sake of code readibility and understanding. We are not responsible for any damages if the code is used for commercial products.


# Further Information
For any inquiries, bugs, and assistance on building and running the code, please contact Thang Hoang (hoangm@mail.usf.edu).

