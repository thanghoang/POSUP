
#include "Utils.h"


Utils::Utils()
{
}


Utils::~Utils()
{
}


std::string hexStr(unsigned char *data, int len)
{
	std::string s(len * 2, ' ');
	for (int i = 0; i < len; ++i) {
		s[2 * i] = hexmap[(data[i] & 0xF0) >> 4];
		s[2 * i + 1] = hexmap[data[i] & 0x0F];
	}
	return s;
}

void print_ucharstring(const unsigned char *pInput, int in_len)
{
	int indx;

	if (pInput != NULL && in_len>0)
	{
		cout << "0x";
		for (indx = 0; indx<in_len; indx++)
		{
			if ((indx % 72 == 0) && (indx != 0))
			{
				cout << "\n";
			}
			printf("%.2x", pInput[indx]);
		}
	}
	else
	{
		cout << "Input string is NULL" << endl;
	}
}

string trim(const string& str)
{
	size_t first = str.find_first_not_of(' ');
	if (string::npos == first)
	{
		return str;
	}
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

int writeByte_array_to_file(unsigned char* byte_arr, long long len, string filename, string dirPath)
{
	FILE* file_out = NULL;
	string path = dirPath + filename;
	if ((file_out = fopen(path.c_str(), "wb+")) == NULL)
	{
		cout << "[Utils] File Cannot be Opened!!" << endl;
		exit(0);
	}
	fwrite(byte_arr, 1, len, file_out);
	fclose(file_out);
	return 0;
}


int readByte_array_from_file(unsigned char *byte_arr, long long &len, string filename, string dirPath)
{
	FILE* file_in = NULL;
	string path = dirPath + filename;

	if ((file_in = fopen(path.c_str(), "rb")) == NULL)
	{
		cout << "[Utils]" <<path<<" Cannot be Opened!!" << endl;
		exit(0);
	}
	fread(byte_arr, 1, len, file_in);
	fclose(file_in);
	return 0;
}

int readBlock_from_file(Block &b, string filename, string dirPath)
{
	string path = dirPath + filename;
	FILE* file_in = NULL;
	if ((file_in = fopen(path.c_str(), "rb")) == NULL) {
		cout << "[Utils] File Cannot be Opened!!" << endl;
		exit(0);
	}
	fread(&b.ID, 1, sizeof(TYPE_ID), file_in);
	fread(b.DATA,1,b.data_size,file_in);

	fread(&b.nextID, 1, sizeof(TYPE_ID), file_in);
	fread(&b.nextPathID, 1, sizeof(TYPE_ID), file_in);

	fclose(file_in);
	return 0;
}

int writeBlock_to_file(Block b, string filename, string dirPath)
{
	FILE* file_out = NULL;
	string path = dirPath + filename;
	if ((file_out = fopen(path.c_str(), "wb+")) == NULL)
	{
		cout << "[Utils] File Cannot be Opened!!" << endl;
		exit(0);
	}
	fwrite(&b.ID, 1, sizeof(TYPE_ID),file_out);
	fwrite(b.DATA, 1, b.data_size, file_out);

	fwrite(&b.nextID, 1, sizeof(TYPE_ID), file_out);
	fwrite(&b.nextPathID, 1, sizeof(TYPE_ID), file_out);

	fclose(file_out);

	return 0;
}

std::vector<std::string> split(const std::string &text, char sep) {
	std::vector<std::string> tokens;
	std::size_t start = 0, end = 0;
	while ((end = text.find(sep, start)) != std::string::npos) {
		tokens.push_back(text.substr(start, end - start));
		start = end + 1;
	}
	tokens.push_back(text.substr(start));
	return tokens;
}

int getAllFiles(string path, vector<string> &filename, vector<string> &filePath)
{
	DIR *pDir;
	struct dirent *pEntry;
	struct stat file_stat;
	string file_name, file_name_with_path;
  std::cout << "Get from where: " << path << endl;
	try
	{
		if ((pDir = opendir(path.c_str())) != NULL)
		{
			while ((pEntry = readdir(pDir)) != NULL)
			{
				file_name = pEntry->d_name;
				// look into pEntry
				if (!file_name.compare(".") || !file_name.compare(".."))
				{
					continue;
				}
				else
				{
					file_name_with_path = path + pEntry->d_name;
					// If the file is a directory (or is in some way invalid) we'll skip it
					if (stat(file_name_with_path.c_str(), &file_stat))
						continue;
					if (S_ISDIR(file_stat.st_mode))
					{
						file_name_with_path.append("/");
						getAllFiles(file_name_with_path, filename, filePath);
						continue;
					}
					if (file_name_with_path.size() > 0)
					{
						filename.push_back(file_name);
						filePath.push_back(path);
					}
					else
						printf("File name is empty\n");
				}
			}
			closedir(pDir);
		}
		else
		{
			printf("Could not locate the directory: %s\n", path.c_str());
		}
	}
	catch (exception &e)
	{
		printf("Error!!\n");
		exit(1);
	}
	return 0;
}

int readMap_from_file(map <string, string> &map, string filename, string dirPath)
{
	ifstream in(dirPath + filename);
	std::string first, second;
	while (std::getline(in, first))
	{
		std::getline(in, second);
		pair<string, string> s(first, second);
		map.insert(s);
	}
	in.close();
	return 0;
}

int writeMap_to_file(map <string, string> map, string filename, string dirPath)
{
	FILE* file_out = NULL;
	std::ofstream out(dirPath + filename);
	for (std::map<string, string>::const_iterator it = map.begin(); it != map.end(); it++)
	{
		out << it->first << "\n";
		out << it->second << "\n";
	}
	out.close();
	return 0;
}

int readMap_from_file(map <TYPE_ID, pair<TYPE_ID, TYPE_ID>> &map,  string filename, string dirPath)
{
	ifstream in(dirPath + filename);
	TYPE_ID first;
	while (1)
	{
		pair<TYPE_ID, TYPE_ID> p;
		in >> first >> p.first >> p.second;

		pair<TYPE_ID, pair<TYPE_ID, TYPE_ID>> s(first, p);
		map.insert(s);
		if (in.eof())
			break;
	}
	in.close();
	return 0;
}

int writeMap_to_file(map <TYPE_ID, pair<TYPE_ID,TYPE_ID>> map, string filename, string dirPath)
{
	FILE* file_out = NULL;
	std::ofstream out(dirPath + filename);
	for (std::map<TYPE_ID, pair<TYPE_ID, TYPE_ID>>::const_iterator it = map.begin(); it != map.end(); it++)
	{
		out << it->first << " " << it->second.first << " " << it->second.second << "\n";
	}
	out.close();
	return 0;
}

int readKW_map_from_file(vector <pair<string, string>> &map, unsigned char* ctr, string filename, string dirPath)
{
	FILE* file_in = NULL;
	string file_with_path = dirPath + filename;
	file_in = fopen(file_with_path.c_str(), "rb");
	unsigned char buffer[16 + KWMAP_VALUE_SIZE];
	//read ctr first
	fread(ctr, 1, 16, file_in);

	while (fread(buffer, 1, (16+KWMAP_VALUE_SIZE), file_in))
	{
		string s1 = "";
		s1.assign((char*)&buffer[0],16);

		string s2 = "";
		s2.assign((char*)&buffer[16], KWMAP_VALUE_SIZE);
		pair<string, string> p(s1, s2);
		map.push_back(p);
	}
	fclose(file_in);
	return 0;
}

int writeString_to_file(string &s, string filename, string dirPath)
{
	
	FILE* file_out = NULL;
	string file_with_path = dirPath + filename;
	file_out = fopen(file_with_path.c_str(), "wb+");
	fwrite(s.c_str(), 1, s.size(), file_out);
	//std::ofstream out(dirPath + filename);
	//out << s.c_str();
	//out.close();
	fclose(file_out);
	return 0;
}

int readMap_from_file(map <string, pair<TYPE_ID, TYPE_ID>> &map, string filename, string dirPath)
{
	ifstream in(dirPath + filename);
	std::string first;
	while (1)
	{
		pair<TYPE_ID, TYPE_ID> p;
		in >> first >>  p.first>> p.second;

		pair<string, pair<TYPE_ID, TYPE_ID>> s(first, p);
		map.insert(s);
		if (in.eof())
			break;
	}
	in.close();
	return 0;
}

int writeMap_to_file(map <string, pair<TYPE_ID, TYPE_ID>> map, string filename, string dirPath)
{
	FILE* file_out = NULL;
	std::ofstream out(dirPath + filename);
	for (std::map<string, pair<TYPE_ID, TYPE_ID>>::const_iterator it = map.begin(); it != map.end(); it++)
	{
		out << it->first << " " << it->second.first << " " << it->second.second << "\n";
	}
	out.close();
	return 0;
}

int writeVector_to_file(vector<TYPE_ID> blockIDs, string filename, string dirPath)
{
	FILE* file_out = NULL;
	std::ofstream out(dirPath + filename);
	for(int i = 0 ; i < blockIDs.size(); i++)
	{
		out << blockIDs[i]<<endl;
	}
	out.close();
	return 0;

}

int readVector_from_file(vector<TYPE_ID>&blockIDs, string filename,string dirPath)
{
	ifstream in(dirPath + filename);
	std::string first;
	while (1)
	{
		TYPE_ID blockID;
		in >>  blockID;
		blockIDs.push_back(blockID);
		if (in.eof())
			break;
	}
	in.close();
	return 0;
}


void clearStats()
{
	for (int i = 0; i < 23; i++)
	{
		Globals::timestamp[i] = 0;
	}
	Globals::numAccess = 0;
}

void showStats(int isRecursive)
{
	unsigned long long sum = 0;

	if (isRecursive)
	{
		for (int i = 21; i < 23; i++)
		{
			sum += Globals::timestamp[i];
		}
		for (int i = 0; i < 23; i++)
		{
			if (Globals::numAccess != 0)
			{
				Globals::timestamp[i] /= Globals::numAccess;
			}
		}
	}
	else
	{
		for (int i = 0; i < 5; i++)
		{
			sum += Globals::timestamp[i];
		}
		for (int i = 0; i < 23; i++)
		{
			if (Globals::numAccess != 0)
			{
				Globals::timestamp[i] /= Globals::numAccess;
			}
		}
	}

	
	cout << endl << endl << endl << endl << endl << endl<< endl << endl;


	if (!isRecursive)
	{
		cout << "TOTAL ODS COST:\t\t\t" << sum << " us"<<endl;
		cout << "\# ORAM ACCESSES:\t\t\t\t" << Globals::numAccess << endl;
		cout << "AVG COST PER ODS ACCESS:\t\t\t" << sum/Globals::numAccess << " us" << endl << endl << endl;;;;
		
		cout << "----------------------------" << endl;
		cout << "[SGX] Prepare Access: \t\t\t" << Globals::timestamp[0] << " us" << endl;
		cout << "[SGX] ORAM Read / access: \t\t\t" << Globals::timestamp[1] << " us" << endl;
		cout << "[SGX] Get 1 Block / access: \t\t\t" << Globals::timestamp[2] << " us" << endl;
		cout << "[no-SGX] prepare Eviction:\t\t\t" << Globals::timestamp[3] << " us" << endl;
		cout << "[SGX] Eviction (ORAM write) / access:\t\t" << Globals::timestamp[4] << " us" << endl;
		cout << "[SGX] Get Next Pointer / access:\t\t" << Globals::timestamp[5] << " us" << endl  << endl << endl;
		cout << "----------------------------" << endl;
	}
	else
	{
		cout << "TOTAL RECURSION COST:\t\t\t\t\t" << sum << "us" << endl <<endl << endl;;;
		cout << "\# RECURSION ACCESSES:\t\t\t\t" << Globals::numAccess << endl;
		cout << "AVG COST PER RECURSION ACCESS:\t\t\t" << sum / Globals::numAccess << " us" << endl << endl << endl;;;;
		cout << "----------------------------" << endl;
		cout << "[SGX] Prepare Access: \t\t\t" << Globals::timestamp[21] << " us" << endl;
		cout << "[SGX] Recursive Access: \t\t\t" << Globals::timestamp[22] << " us" << endl << endl << endl;
		cout << "----------------------------" << endl;
	}
	cout << "DETAIL READ / access:" << endl;

	cout << "[no-SGX] prepare memory(outside):\t\t" << Globals::timestamp[6] << " us" << endl;
	cout << "[no-SGX] I / O access (Disk or RAM):\t\t" << Globals::timestamp[7] << " us" << endl;
	cout << "[SGX] load Meta data into SGX:\t\t\t" << Globals::timestamp[8] << " us" << endl;
	cout << "[SGX] Load Bucket to SGX:\t\t\t" << Globals::timestamp[9] << " us" << endl;
	cout << "[SGX] Load Stash to SGX:\t\t\t" << Globals::timestamp[10] << " us" << endl;
	cout << "[SGX] get Stash Meta to outside:\t\t" << Globals::timestamp[11] << " us" << endl;
	cout << "[SGX] Get Path Meta to outside:\t\t\t" << Globals::timestamp[12] << " us" << endl;
	cout << "[no-SGX] I / O access (Disk or RAM):\t\t" << Globals::timestamp[13] << " us" << endl <<endl << endl;;
	cout << "----------------------------" << endl;

	cout << "DETAIL WRITE / access: " << endl;

	cout << "[no-SGX] prepare memory(outside):\t\t" << Globals::timestamp[14] << " us" << endl;
	cout << "[no-SGX] I / O access (Disk or RAM):\t\t" << Globals::timestamp[15] << " us" << endl;
	cout << "[SGX] load Meta data into SGX:\t\t\t" << Globals::timestamp[16] << " us" << endl;
	cout << "[SGX] Evict:\t\t\t\t\t" << Globals::timestamp[17] << " us" << endl;
	cout << "[SGX] get Stash Meta to outside:\t\t" << Globals::timestamp[18] << " us" << endl;
	cout << "[SGX] Get Path Meta to outside:\t\t\t" << Globals::timestamp[19] << " us" << endl;
	cout << "[no-SGX] I / O access (Disk or RAM):\t\t" << Globals::timestamp[20] << " us" << endl  << endl << endl;;;
	cout << "----------------------------" << endl;

	



}

void inc_dec_ctr(unsigned char* input, unsigned long long number, bool isInc)
{
	//current implementation supports counter to be upto 64-bits

	//reverse ctr first
	unsigned char tmp[8];
	for (int i = 0; i < 8; i++)
	{
		tmp[i] = input[15 - i];
	}
	unsigned long long* ctr = (unsigned long long*)&tmp;

	if (isInc)
	{
		*ctr += number;
	}
	else
	{
		*ctr -= number;
	}
	//reverse it back
	for (int i = 0; i < 8; i++)
	{
		input[15 - i] = tmp[i];
	}
}

int reverseByteArray(unsigned char* data, int len, int elem_size)
{
	unsigned char* tmp = new unsigned char[len];
	memcpy(tmp, data, len);
	for (int i = 0, j = len - sizeof(TYPE_ID); i < len; i += elem_size, j -= elem_size)
	{
		memcpy(&data[i], &tmp[j], elem_size);
	}
	delete[] tmp;
	return 0;
}