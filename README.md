# POSUP: Oblivious Search and Update Platform with SGX

Basic implementation of POSUP platform appeard in PETS'19. The full paper can be downloaded from [here](https://www.degruyter.com/downloadpdf/j/popets.2019.2019.issue-1/popets-2019-0010/popets-2019-0010.pdf). This project is mainly built on MS Visual Studio, but it can operate on Linux environment as well. 


# Prerequisites
1. Intel SGX (Linux version [here](https://github.com/intel/linux-sgx), Windows setup [here](https://downloadcenter.intel.com/download/28972/Intel-Software-Guard-Extensions-Intel-SGX-Driver-for-Windows-)

2. Intel AES-NI (Sample code was taken down from Intel website, though I provided it here in ``ext/aes-ni`` folder)

3. Libtomcrypt (download [here](https://github.com/libtom/libtomcrypt)

# Configuration
There are three configuration files in POSUP, including ``include/conf.h``, ``POSUP/config.h`` and ``POSUP/my_config.h`` with important parameters as follows

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




```


# Build & Compile

## Windows
Download the entire code and open the file ``POSUP.sln`` with Microsoft Visual Studio.

## Linux

Download the code and execute ``make`` in the project root folder, which will produce a binary executable file named ``app`` loated at the same directory.


# Further Information
For any inquiries, bugs, and assistance on building and running the code, please contact Thang Hoang (hoangmin@oregonstate.edu).

