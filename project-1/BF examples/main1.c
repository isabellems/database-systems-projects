#include <stdio.h>
#include <stdlib.h>

#include "BF.h"

#define FILENAME "hello"
#define BLOCK_COUNTER 20

int main(int argc, char** argv) {
	int bf, i, j;
	unsigned char* block;

	BF_Init();
	if (BF_CreateFile(FILENAME) < 0) {
		BF_PrintError("Error creating file");
		exit(EXIT_FAILURE);
	}
	if ((bf = BF_OpenFile(FILENAME)) < 0) {
		BF_PrintError("Error opening file");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < BLOCK_COUNTER; i++) {
		if (BF_AllocateBlock(bf) < 0) {
			BF_PrintError("Error allocating block");
			exit(EXIT_FAILURE);
		}
		if (BF_ReadBlock(bf, i, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			exit(EXIT_FAILURE);
		}
		for (j = 0; j < BLOCK_SIZE; j++)
			block[j] = i + j;
	}
	if (BF_CloseFile(bf) < 0) {
		BF_PrintError("Error closing file");
		exit(EXIT_FAILURE);
	}
	return 0;	
}

