#pragma once

void		KeywordInit();
void		KeywordDestroy();
void		KeywordAdd(char* pKeyword, ULONG nLen);
void		KeywordClear();
BOOLEAN		KeywordFind(IN char* pData, IN int nLenData, OUT char** ppKeyWord, OUT int* pLenKeyWord);