#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BF.h"

#define FILENAME1 "hello"
#define FILENAME2 "world"

int main(int argc, char** argv) {
	int bf1, bf2, blockCounter, i;
	void* block1;
	void* block2;

	BF_Init();
	if (BF_CreateFile(FILENAME2) < 0) {
		BF_PrintError("Error creating file");
		exit(EXIT_FAILURE);
	}
	if((bf1 = BF_OpenFile(FILENAME1)) < 0) {
		BF_PrintError("Error opening file");
		exit(EXIT_FAILURE);
	}
	if ((bf2 = BF_OpenFile(FILENAME2)) < 0) {
		BF_PrintError("Error opening file");
		BF_CloseFile(bf1);
		exit(EXIT_FAILURE);
	}
	if ((blockCounter = BF_GetBlockCounter(bf1)) < 0) {
		BF_PrintError("Error getting block counter");
		BF_CloseFile(bf1);
		BF_CloseFile(bf2);
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < blockCounter; i++) {
		if (BF_ReadBlock(bf1, blockCounter -i - 1, &block1) < 0) {
			BF_PrintError("Error getting block");
			BF_CloseFile(bf1);
			BF_CloseFile(bf2);
			exit(EXIT_FAILURE);
		}
		if (BF_AllocateBlock(bf2) < 0) {
			BF_PrintError("Error allocating block");
			BF_CloseFile(bf1);
			BF_CloseFile(bf2);
			exit(EXIT_FAILURE);
		}
		if (BF_ReadBlock(bf2, i, &block2) < 0) {
			BF_PrintError("Error getting block");
			BF_CloseFile(bf1);
			BF_CloseFile(bf2);
			exit(EXIT_FAILURE);
		}
		if (block1 == block2) {
			fprintf(stderr, "Block 2 evicted block 1, giving up...\n");
			BF_CloseFile(bf1);
			BF_CloseFile(bf2);
			exit(EXIT_FAILURE);
		}
		memmove(block2, block1, BLOCK_SIZE);
		printf("Copied file %s block %d to file %s block %d\n", FILENAME1, blockCounter - i -1, FILENAME2, i);
	}
	if (BF_CloseFile(bf1) < 0) {
		BF_PrintError("Error closing file");
		BF_CloseFile(bf2);
		exit(EXIT_FAILURE);
	}
	if (BF_CloseFile(bf2) < 0) {
		BF_PrintError("Error closing file");
		exit(EXIT_FAILURE);
	}
	return 0;
}

