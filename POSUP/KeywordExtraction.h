#pragma once
#include "config.h"
class KeywordExtraction
{
public:
	KeywordExtraction();
	~KeywordExtraction();

	int extractKeywords(TYPE_KEYWORD_DICTIONARY &rKeywordsDictionary, string file_name, string path);
	int extractWords_using_find_first_of(TYPE_KEYWORD_DICTIONARY &rKeywordsDictionary, unsigned long int *pKeywordNum, ifstream &rFin);
	int createInvertedIndex(string path, map <string, string> &invertedIdx);
};

