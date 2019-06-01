#include <stdio.h>
#include <string.h>
#include "BF.h"
#include "heap.h"

typedef struct Fileinfo{
	int heap; //1 for heap, 0 for hash
	int rec; // Number of records in a block
	int num; //Number of records in a file
}Fileinfo;

int HP_CreateFile(char *fileName) {
  int fileDesc,blockFile;
  void *block;
  Fileinfo finfo;
  if(BF_CreateFile(fileName)<0){
  	BF_PrintError("File could not be created.\n");
  	return -1;
  }
  if((blockFile = BF_OpenFile(fileName))<0){
  	BF_PrintError("File could not be opened.\n");
  	BF_CloseFile(blockFile);
  	return -1;
  }
  if(BF_AllocateBlock(blockFile)<0){
  	BF_PrintError("Could not allocate block.\n");
  	BF_CloseFile(blockFile);
  	return -1;
  }
  finfo.heap = 1; //Heap file
  finfo.rec = BLOCK_SIZE/sizeof(Record);// Number of records that fit in a block
  finfo.num = 0;
  if(BF_ReadBlock(blockFile,0,&block)<0){
  	BF_PrintError("File could not be read.\n");
  	BF_CloseFile(blockFile);
  	return -1;
  }
  memcpy(block,&finfo,sizeof(Fileinfo));
  if(BF_WriteBlock(blockFile,0)<0){
  	BF_PrintError("File could not be written.\n");
  	return -1;
  }
  if(BF_CloseFile(blockFile)<0){
  	BF_PrintError("File could not be closed.\n");
  }
  return 0;
}

int HP_OpenFile(char *fileName) {
  int blockFile;
  void *block;
  Fileinfo finfo;
  if((blockFile=BF_OpenFile(fileName))<0){
  	 BF_PrintError("File could not be opened.\n");
  	 return -1;
  }
  if(BF_ReadBlock(blockFile,0,&block)<0){
  	 BF_PrintError("Block could not be read.\n");
  	 BF_CloseFile(blockFile);
  	 return -1;
  }
  finfo = *((Fileinfo *) block);
  if(finfo.heap!=1){//Checking if this is a heap file
  	BF_PrintError("This is not a heap file!\n");
  	BF_CloseFile(blockFile);
  	return -1;
  }
  return blockFile;
}

int HP_CloseFile(int fileDesc) {
  if(BF_CloseFile(fileDesc)<0){
  	BF_PrintError("File could not be closed.\n");
  	return -1;
  }
  printf("File was closed.\n");
  return 0;
}

int HP_InsertEntry(int fileDesc, Record record) {
	void *block, *cur;
	Fileinfo finfo;
	int c,en;
	if((c=BF_GetBlockCounter(fileDesc))<0){
		BF_PrintError("Could not get block counter.\n");
		return -1;
	}
	if(BF_ReadBlock(fileDesc,0,&block)<0){
			BF_PrintError("Block could not be read.\n");
			return -1;
		}
		finfo = *((Fileinfo *) block);
		en = finfo.num % finfo.rec;//Checking how many entries are in the last semi-filled block
	if(c==1){//First block contains only file info
		if(BF_AllocateBlock(fileDesc)<0){
			BF_PrintError("Block could not be allocated.\n");
			return -1;
		}
		if(BF_ReadBlock(fileDesc,1,&block)<0){
			BF_PrintError("Block could not be read.\n");
			return -1;
		}
		memcpy(block,&record,sizeof(Record));
		if(BF_WriteBlock(fileDesc,1)<0){
			BF_PrintError("Block could not be written.\n");
			return -1;
		}
	}
	else {//If there are more than one block
		if(en==0){//If all the blocks are full of entries,allocate new block
			if(BF_AllocateBlock(fileDesc)<0){
				BF_PrintError("Could not get block counter.\n");
				return -1;
			}
		}
		if((c=BF_GetBlockCounter(fileDesc))<0){
			BF_PrintError("Could not get block counter.\n");
			return -1;
		}
		if(BF_ReadBlock(fileDesc,c-1,&block)<0){
			BF_PrintError("Could not read block.\n");
			return -1;
		}
		cur = block+en*sizeof(Record);//Adding offset
		memcpy(cur,&record,sizeof(Record));
		if(BF_WriteBlock(fileDesc,c-1)<0){
			BF_PrintError("Could not write block.\n");
			return -1;
		}

	}
	if(BF_ReadBlock(fileDesc,0,&block)<0){
		BF_PrintError("Block could not be read.\n");
		return -1;
	}
	finfo = *((Fileinfo *) block);
	(finfo.num)++;
	memcpy(block,&finfo,sizeof(Record));
	if(BF_WriteBlock(fileDesc,0)<0){
		BF_PrintError("Block could not be written.\n");
		return -1;
	}
	return 0;
}

void HP_GetAllEntries(int fileDesc, char* fieldName, void *value) {
	void *block,*cur;
	Record r;
	Fileinfo finfo;
	int num,rec,c,i,j,m,d,bln=0;
	/* num:number of entries,rec:number of records per block,c:block counter,bln:number of read blocks*/
	if(BF_ReadBlock(fileDesc,0,&block)<0){
		BF_PrintError("Block could not be read.\n");
		return ;
	}
	if((c=BF_GetBlockCounter(fileDesc))<0){
		BF_PrintError("Could not get block counter.\n");
		return ;
	}
	finfo = *((Fileinfo *) block);
	num = finfo.num;
	rec = finfo.rec;
	d = num/rec; //Number of full blocks
	m = num % rec;//Number of entries in the last semi-filled block
	for (i = 1; i <= d ; i++)
	{	
		bln++;
		for(j=0; j<rec ; j++){
			if(BF_ReadBlock(fileDesc,i,&block)<0){
				BF_PrintError("Block could not be read.\n");
				return ;
			}
			cur = block+j*sizeof(Record);//Adding offset
			r = *((Record *) cur );
			if(!strcmp(fieldName,"all"))
				printf("%d,%s,%s,%s \n",r.id,r.name,r.surname,r.city);
			else if(!strcmp(fieldName,"id")){
				int id = atoi(value);
				if(id==r.id)
					printf("%d,%s,%s,%s \n",r.id,r.name,r.surname,r.city);
			}
			else if(!strcmp(fieldName,"name")){
				if(!strcmp(value,r.name))
					printf("%d,%s,%s,%s \n",r.id,r.name,r.surname,r.city);
			}
			else if(!strcmp(fieldName,"surname")){
				if(!strcmp(value,r.surname))
					printf("%d,%s,%s,%s \n",r.id,r.name,r.surname,r.city);
			}
			else if(!strcmp(fieldName,"city")){
				if(!strcmp(value,r.city))
					printf("%d,%s,%s,%s \n",r.id,r.name,r.surname,r.city);
			}
			else{
				printf("FieldName is not valid.\n");
				return;
			}
		}
	}
	if(m!=0){
		bln++;
		for(j=0; j<m; j++){
			if(BF_ReadBlock(fileDesc,i,&block)<0){
				BF_PrintError("Block could not be read.\n");
				return ;
			}
			cur = block+j*sizeof(Record);//Adding offset
			r = *((Record *) cur);
			if(!strcmp(fieldName,"all"))
				printf("%d,%s,%s,%s \n",r.id,r.name,r.surname,r.city);
			else if(!strcmp(fieldName,"id")){
				int id = atoi(value);
				if(id==r.id)
					printf("%d,%s,%s,%s \n",r.id,r.name,r.surname,r.city);
			}
			else if(!strcmp(fieldName,"name")){
				if(!strcmp(value,r.name))
					printf("%d,%s,%s,%s \n",r.id,r.name,r.surname,r.city);
			}
			else if(!strcmp(fieldName,"surname")){
				if(!strcmp(value,r.surname))
					printf("%d,%s,%s,%s \n",r.id,r.name,r.surname,r.city);
			}
			else if(!strcmp(fieldName,"city")){
				if(!strcmp(value,r.city))
					printf("%d,%s,%s,%s \n",r.id,r.name,r.surname,r.city);
			}
			else{
				printf("FieldName is not valid.\n");
				return;
			}
		}
	}
	printf("Number of blocks that were read: %d \n",bln );
}
