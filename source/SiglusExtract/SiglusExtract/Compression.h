#pragma once

void DecompressData(unsigned char *, unsigned char *, int);
unsigned char *CompressData(unsigned char *, int, int *, int);
int FindMatch(unsigned char *, short, int, int *, int);
void GenerateTable(unsigned char *, int);
int SearchData(unsigned char *, int, unsigned char *, int, int *);
