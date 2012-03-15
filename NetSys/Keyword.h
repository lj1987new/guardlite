#pragma once

void		keyword_Init();
void		keyword_Release();
void		keyword_Add(char* pKeyword, ULONG nLen);
void		keyword_Clear();
BOOLEAN		keyword_Find(IN char* pData, IN int nLenData, OUT char** ppKeyWord, OUT int* pLenKeyWord);