
#include <iostream>
#include "POSUP.h"
#include "config.h"
#include "Utils.h"
#include "TreeORAM.h"

#include <string>
#include <iostream>
#include "KeywordExtraction.h"


#include "Enclave_u.h"

#include "sgx_urts.h"
#include "gc.h";

using namespace std;
void printMenu();

struct thang { int n; double d[]; };
int main(int argc, char **argv)
{
	TreeORAM ORAM;
	TYPE_ID* k;
	POSUP* p = new POSUP();
	p->buildRecursiveORAMInfo();
	p->init();
	p->initEnclave();
	Block b_index;
	string str_keyword;
	int search_result;
	int ntime = 0;
	TYPE_ID fID;
	while (1)
	{
		int selection = -1;
		do
		{
			printMenu();
			cout << "Select your choice: ";
			while (!(cin >> selection))
			{
				cin.clear();
				cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
				cout << "Invalid input. Try again: ";
			}
		} while (selection < 0 && selection >4);
		switch (selection)
		{
		case 0:
			p->loadState();
			break;
		case 1:
			p->buildFilesORAM();
			p->buildIndexORAM();
			p->loadState();
			break;
		case 2:
			//cout << "Keyword search: ";
			//cin >> str_keyword;
			//std::transform(str_keyword.begin(), str_keyword.end(), str_keyword.begin(), ::tolower);
			p->loadState();
			str_keyword = "the";
			p->search(str_keyword, search_result);
			p->saveState();
			cout << " Keyword *" << str_keyword << "* appeared in " << search_result << " files" << endl;
			break;
		case 3:
			//cout << "Keyword to be updated: ";
			//cin >> str_keyword;
			//std::transform(str_keyword.begin(), str_keyword.end(), str_keyword.begin(), ::tolower);
			//cout << "File ID to be added (positive) or deleted (negative): ";
			//cin >> fID;
			p->loadState();
			str_keyword = "the";
			fID = -2;
			p->update(str_keyword, fID);
			p->saveState();
			break;
		case 4:
			p->saveState();
			exit(1);
			break;
	
		default:
			break;
		}
	}
	return 0;
}




/* for testing of recursive ORAMs
Block b_index;
b_index.setData_size(ClientORAM::recursion_info_file[0].DATA_SIZE);
for (int i = 0; i < ClientORAM::recursion_info_file[0].NUM_BLOCKS; i++)
{
client->recursiveCircuitORAM_access(i+1, b_index, ClientORAM::recursion_info_file);
cout << i << endl;
//cin.get();
}
cin.get();
*/


void printMenu()
{
	cout << "---------------" << endl << endl;
	cout << "0. Load previous state" << endl;
	cout << "1. (Re)build ORAMs" << endl;
	cout << "2. Keyword search: " << endl;
	cout << "3. Update keyword-file pair " << endl << endl;;
	cout << "4. Save current state and Exit" << endl << endl;;
	cout << "---------------" << endl;
}



/* OCall functions */
void ocall_print_string(const char *str, int len)
{
	/* Proxy/Bridge will check the length and null-terminate
	 * the input string to prevent buffer overflow.
	 */
	//printf("%s\n", str);
	int indx;
	if (str != NULL && len > 0)
	{
		cout << "0x";
		for (indx = 0; indx < len; indx++)
		{
			if ((indx % 72 == 0) && (indx != 0))
			{
				cout << "\n";
			}
			printf("%.2x", str[indx]);
		}
		printf("\n");
	}
	else
	{
		cout << "Input string is NULL" << endl;
	}


}


void ocall_print_value(long long val)
{
	/* Proxy/Bridge will check the length and null-terminate
	 * the input string to prevent buffer overflow.
	 */
	cout << val << endl;
}

