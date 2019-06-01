#include <stdio.h>
#include <stdlib.h>

#include "BF.h"

#define FILENAME "hello"

int main(int argc, char** argv) {
	int bf, blockCounter, i, j;
	unsigned char* block;

	BF_Init();
	if ((bf = BF_OpenFile(FILENAME)) < 0) {
		BF_PrintError("Error opening file");
		exit(EXIT_FAILURE);
	}
	if ((blockCounter = BF_GetBlockCounter(bf)) < 0) {
		BF_PrintError("Error getting the number of blocks");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < blockCounter; i++) {
		if (BF_ReadBlock(bf, i, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			exit(EXIT_FAILURE);
		}
		for (j = 0; j < BLOCK_SIZE; j++)
			printf("%c", block[j]);
	}
	if (BF_CloseFile(bf) < 0) {
		BF_PrintError("Error closing file");
		exit(EXIT_FAILURE);
	}
	getchar();
	return 0;
}

