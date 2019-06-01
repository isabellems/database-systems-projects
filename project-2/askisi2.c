#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "AM.h"
#include "BF.h"
#include "defn.h"


void AM_Init(){
	int i;
	BF_Init();
	for (i=0; i < MAXOPENFILES; i++)
		Open[i] = NULL;
	for (i = 0; i < MAXSCANS; i++)
		Scan[i] = NULL;
	AM_errno = AME_OK;
}


int AM_CreateIndex(char *fileName, char attrType1, int attrLength1, char attrType2, int attrLength2){
	int file_id;
	void *block;
	Info in;
	strcpy(in.fileName, fileName);
	in.isBtree = '1';
	in.recs = 0;
	in.attrType1 = attrType1;
	in.attrLength1 = attrLength1;
	in.attrType2 = attrType2;
	in.attrLength2 = attrLength2;
	in.root = -1;
	FILE *outputFile = fopen("fileName", "r");
	if (outputFile != NULL){
		AM_errno = AME_FILE_EXISTED;
		fclose(outputFile);
		return AM_errno;
	}
	if (BF_CreateFile(fileName) < 0){
		AM_errno = AME_INDEX_NOT_CREATED;           /*Error in BF level means AM_CreateIndex failure*/
		BF_PrintError("Error in BF_CreateFile\n");
		return AM_errno;
	}
	if ((file_id = BF_OpenFile(fileName)) < 0){
		AM_errno = AME_INDEX_NOT_CREATED;
		BF_PrintError("Error in BF_OpenFile\n");
		BF_CloseFile(file_id);
		return AM_errno;
	}
	in.id = file_id;
	if (BF_AllocateBlock(file_id) < 0){
		AM_errno = AME_INDEX_NOT_CREATED;
		BF_PrintError("Error in BF_AllocateBlock\n");
		BF_CloseFile(file_id);
		return AM_errno;
	}
	if (BF_ReadBlock(file_id, 0, &block) < 0){
		AM_errno = AME_INDEX_NOT_CREATED;
		BF_PrintError("Error in BF_ReadBlock\n");
		BF_CloseFile(file_id);
		return AM_errno;
	}
	memcpy(block, &in, sizeof(Info));
	if (BF_WriteBlock(file_id, 0) < 0){
		AM_errno = AME_INDEX_NOT_CREATED;
		BF_PrintError("Error in BF_WriteBlock\n");
		BF_CloseFile(file_id);
		return AM_errno;
	}
	if (BF_CloseFile(file_id) < 0){
		AM_errno = AME_INDEX_NOT_CREATED;
		BF_PrintError("Error in BF_CloseFile\n");
		return AM_errno;
	}
	return AME_OK;
}


int AM_DestroyIndex(char *fileName){
	int i,rem;
	for(i=0 ; i<MAXOPENFILES ; i++){
		if (Open[i] != NULL) {
			if(!strcmp(Open[i]->fileName,fileName)){
				AM_errno = AME_OPEN_FILE;
				return AM_errno;
			}
		}
	}
	rem = remove(fileName);
	if(!rem)
		return AME_OK;
	else{
		AM_errno = AME_NOT_REMOVED;
		return AM_errno;
	}
}


int AM_OpenIndex (char *fileName){
	int file_id, i;
	void *block;
	Info *in = malloc(sizeof(Info));
	if((file_id = BF_OpenFile(fileName)) <0){
		AM_errno = AME_INDEX_NOT_OPENED; 
		BF_PrintError("Error in BF_OpenFile\n");
		BF_CloseFile(file_id);
		return AM_errno;
	}
	for(i = 0 ; i<MAXOPENFILES ; i++){
		if(Open[i]==NULL){
			if (BF_ReadBlock(file_id, 0, &block) < 0){
				AM_errno = AME_INDEX_NOT_OPENED;
				BF_PrintError("Error in BF_ReadBlock\n");
				BF_CloseFile(file_id);
				return AM_errno;
			}
			memcpy(in, block, sizeof(Info));
			Open[i] = malloc(sizeof(Info));
			Open[i] = in;
			return i;
		}
	}
	AM_errno = AME_MAX_OPEN;
	return AM_errno;
}


int AM_CloseIndex (int fileDesc){
	int i;
	int file_id = Open[fileDesc]->id;
	for(i=0 ; i<MAXSCANS ; i++){
		if (Scan[i] != NULL) {
			if(Scan[i]->id == file_id){ 
				AM_errno = AME_OPEN_SCAN;
				return AM_errno;
			}
		}
	}
	free(Open[fileDesc]);
	Open[fileDesc] = NULL;
	if (BF_CloseFile(file_id) < 0){
		AM_errno = AME_INDEX_NOT_CLOSED;
		BF_PrintError("Error in BF_CloseFile\n");
		return AM_errno;
	}
	return AME_OK;
}


int AM_OpenIndexScan(int fileDesc, int op, void *value){
	int i,s,rec;
	Info in;
	void *block;
	ScanCell *sc=malloc(sizeof(ScanCell));
	int file_id = Open[fileDesc]->id;
	if(BF_ReadBlock(Open[fileDesc]->id,0,&block)<0){
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in AM_FindNextEntry: ");
		BF_CloseFile(Open[fileDesc]->id);
		return AM_errno;
	}
	memcpy(&in,block,sizeof(Info));
	for (i=0; i<MAXSCANS; i++){
		if (Scan[i]==NULL){
			sc->id=file_id;
			sc->op=op;
			sc->value=value;
			if (op == LESS_THAN || op == LESS_THAN_OR_EQUAL) {   /* Go at the 1st data block */
				s = FirstDBlock(fileDesc);
				if(s<0){
					AM_errno = AME_FIRSTDBLOCK_FAILED;
					return AM_errno;
				}
			}
			else if(op==NOT_EQUAL){
				/* Call of AM_FirstNotEqual to ignore duplicates */
				s = AM_FirstNotEqual(fileDesc,(void*)value);
				if (s<0){
					AM_errno = AME_FIRSTNOTEQUAL_FAILED;
					return AM_errno;
				}
			}
			else {
				s = Search(file_id, in.root, (void*)value, Open[fileDesc]->attrLength1, NULL);
				if (s<0){
					AM_errno = AME_SEARCH_FAILED;
					return AM_errno;
				}
			}  
			rec = SearchDataBlock(file_id, s,(void*)value,op);
			if(rec>-1)
				sc->num_block = s;
			else
				sc->num_block = -1;
			sc->num_record = rec;
			Scan[i] = sc;
			return i;
		}
	}
	AM_errno = AME_MAX_SCAN;
	return AM_errno;
}


void* AM_FindNextEntry(int scanDesc){
	int block_num = Scan[scanDesc]->num_block;
	int file_id = Scan[scanDesc]->id;
	int rec_num = Scan[scanDesc]->num_record;
	int count_recs;
	int oper = Scan[scanDesc]->op;
	void *val = Scan[scanDesc]->value;
	void *block,*cur;              /* block points to the beggining of a block, cur to the current position */
	int len1,len2,flag=0,bid=0;    /* bid: id of block's brother */
	Info in;
	/* Reading the info block */
	if(BF_ReadBlock(file_id, 0 , &block) < 0){ 
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in AM_FindNextEntry: ");
		return NULL;
	}
	memcpy(&in, block , sizeof(Info));
	/* Getting attributes' lengths */
	len1 = in.attrLength1;      
	len2 = in.attrLength2;
	/* Reading the block id which is saved in ScanCell */
	if(BF_ReadBlock(file_id, block_num , &block) < 0){
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in AM_FindNextEntry: ");
		return NULL;
	}
	memcpy(&count_recs,block +sizeof(char)+2*sizeof(int),sizeof(int));
	if(in.attrType1!=STRING && in.attrType2!=STRING){
		void *val1,*val2;
		if(rec_num < count_recs){    /* Check for the next record into the block */
			cur = block+sizeof(char)+3*sizeof(int)+rec_num*(len1+len2); 
			memcpy(&val1,cur,len1);
			rec_num++;
		}
		else{    /* Or move to the brother */
			cur = block +sizeof(char)+sizeof(int); /* Getting id of brother */
			memcpy(&block_num,cur,sizeof(int)); 
			if(block_num==-1)   /* Last block */
				return AME_EOF;
			if (BF_ReadBlock(file_id, block_num , &block) < 0){
				AM_errno = AME_CANNOT_READ_BLOCK;
				BF_PrintError("Error in AM_FindNextEntry: ");
				return NULL; 
			}
			bid = block_num;
			rec_num = 1;
			cur = block+sizeof(char)+3*sizeof(int);
			memcpy(&val1,cur,len1);
		}
		if(oper!=NOT_EQUAL){
			if(!AM_Compare(file_id,(void*)val,(void*)val1,oper)){
				cur = cur + len1;
				memcpy(&val2,cur,len2);
				Scan[scanDesc]->num_record = rec_num;
				Scan[scanDesc]->num_block = block_num;
				return val2;
			}
			else
				return NULL;
		}
		else{
			while(flag==0){
				if(bid<0){
					flag=1;   /* If this is the last block break */
				}
				while(rec_num<=count_recs){
					memcpy(&val1,cur,len1);    /* Get first value */
					if(!AM_Compare(Scan[scanDesc]->id,(void*)val,(void*)val1,EQUAL)){
						cur = cur + len1 + len2;
						rec_num++;
					}
					else{
						cur = cur + len1;
						memcpy(&val2,cur,len2);
						Scan[scanDesc]->num_record = rec_num;
						Scan[scanDesc]->num_block = block_num;
						return val2;
					}
				}
				if(BF_ReadBlock(file_id, bid, &block) < 0){ /* Read the brother block */
					AM_errno = AME_CANNOT_READ_BLOCK;
					BF_PrintError("Error in AM_FirstNotEqual: ");
					return NULL;
				}
				rec_num = 1;
				block_num = bid; 
				cur = block+sizeof(char)+1*sizeof(int); 
				memcpy(&bid,cur,sizeof(int)); /* Get brother's id */
				cur = block+sizeof(char)+3*sizeof(int); 
			}
			return NULL;
		}
	}
	else if(in.attrType1!=STRING && in.attrType2==STRING){
		void *val1;
		char val2[len2];
		if(rec_num < count_recs){ /* Check for next record into the block */
			cur = block+sizeof(char)+3*sizeof(int)+rec_num*(len1+len2); 
			memcpy(&val1,cur,len1);
			rec_num++;
		}
		else{   /* Or move to the brother */
			cur = block +sizeof(char)+sizeof(int); 
			memcpy(&block_num,cur,sizeof(int)); /* Getting id of brother */
			if(block_num==-1) /* Last block */
				return AME_EOF;
			if (BF_ReadBlock(file_id, block_num , &block) < 0){
				AM_errno = AME_CANNOT_READ_BLOCK;
				BF_PrintError("Error in AM_FindNextEntry: ");
				return NULL; 
			}
			bid = block_num;
			rec_num = 1;
			cur = block+sizeof(char)+3*sizeof(int);
			memcpy(&val1,cur,len1);
		}
		if(oper!=NOT_EQUAL){
			if(!AM_Compare(file_id,(void*)val,(void*)val1,oper)){
				cur = cur + len1;
				memcpy(&val2,cur,len2);
				Scan[scanDesc]->num_record = rec_num;
				Scan[scanDesc]->num_block = block_num;
				return val2;
			}
			else
				return NULL;
		}
		else{
			while(flag==0){
				if(bid<0){
					flag=1; /* If this is the last block break */
				}
				while(rec_num<count_recs){
					memcpy(&val1,cur,len1);   /* Get first value */
					if(!AM_Compare(Scan[scanDesc]->id,(void*)val,(void*)val1,EQUAL)){
						cur = cur + len1 + len2;
						rec_num++;
					}
					else{
						cur = cur + len1;
						memcpy(&val2,cur,len2);
						Scan[scanDesc]->num_record = rec_num;
						Scan[scanDesc]->num_block = block_num;
						return val2;
					}
				}
				if(BF_ReadBlock(file_id, bid, &block) < 0){     /* Read brother block */
					AM_errno = AME_CANNOT_READ_BLOCK;
					BF_PrintError("Error in AM_FirstNotEqual: ");
					return NULL;
				}
				rec_num = 1;
				block_num = bid; 
				cur = block+sizeof(char)+1*sizeof(int); 
				memcpy(&bid,cur,sizeof(int));  /* Get brother's id */
				cur = block+sizeof(char)+3*sizeof(int); 
			}
			return NULL;
		}
	}
	else if(in.attrType1== STRING && in.attrType2!=STRING){
		char val1[len1];
		void *val2;
		if(rec_num < count_recs){    /* Check for next record into the block */
			cur = block+sizeof(char)+3*sizeof(int)+rec_num*(len1+len2); 
			memcpy(&val1,cur,len1);
			rec_num++;
		}
		else{     /* Or move to the brother */
			cur = block +sizeof(char)+sizeof(int); 
			memcpy(&block_num,cur,sizeof(int)); /* Getting id of brother */
			if(block_num==-1)     /* Last block */
				return AME_EOF;
			if (BF_ReadBlock(file_id, block_num , &block) < 0){
				AM_errno = AME_CANNOT_READ_BLOCK;
				BF_PrintError("Error in AM_FindNextEntry: ");
				return NULL; 
			}
			bid = block_num;
			rec_num = 1;
			cur = block+sizeof(char)+3*sizeof(int);
			memcpy(&val1,cur,len1);
		}
		if(oper!=NOT_EQUAL){
			if(!AM_Compare(file_id,(void*)val,(void*)val1,oper)){
				cur = cur + len1;
				memcpy(&val2,cur,len2);
				Scan[scanDesc]->num_record = rec_num;
				Scan[scanDesc]->num_block = block_num;
				return val2;
			}
			else
				return NULL;
		}
		else{
			while(flag==0){
				if(bid<0){
					flag=1; /* If this is the last block break */
				}
				while(rec_num<count_recs){
					memcpy(&val1,cur,len1); /* Get first value */
					if(!AM_Compare(Scan[scanDesc]->id,(void*)val,(void*)val1,EQUAL)){
						cur = cur + len1 + len2;
						rec_num++;
					}
					else{
						cur = cur + len1;
						memcpy(&val2,cur,len2);
						Scan[scanDesc]->num_record = rec_num;
						Scan[scanDesc]->num_block = block_num;
						return val2;
					}
				}
				if(BF_ReadBlock(file_id, bid, &block) < 0){    /* Read brother block */ 
					AM_errno = AME_CANNOT_READ_BLOCK; 
					BF_PrintError("Error in AM_FirstNotEqual: ");
					return NULL;
				}
				rec_num = 1;
				block_num = bid; 
				cur = block+sizeof(char)+1*sizeof(int); 
				memcpy(&bid,cur,sizeof(int));      /* Get brother's id */
				cur = block+sizeof(char)+3*sizeof(int); 
			}
			return NULL;
		}
	}
	else{
		char val1[len1],val2[len2];
		if(rec_num < count_recs){      /* Check for next record into the block */
			cur = block+sizeof(char)+3*sizeof(int)+rec_num*(len1+len2); 
			memcpy(&val1,cur,len1);
			rec_num++;
		}
		else{   /* Or move to the brother */
			cur = block +sizeof(char)+sizeof(int);     
			memcpy(&block_num,cur,sizeof(int));    /* Getting id of brother */
			if(block_num==-1)/*Last block*/
				return AME_EOF;
			if (BF_ReadBlock(file_id, block_num , &block) < 0){
				AM_errno = AME_CANNOT_READ_BLOCK;
				BF_PrintError("Error in AM_FindNextEntry: ");
				return NULL; 
			}
			bid = block_num;
			rec_num = 1;
			cur = block+sizeof(char)+3*sizeof(int);
			memcpy(&val1,&cur,len1);
		}
		if(oper!=NOT_EQUAL){
			if(!AM_Compare(file_id,(void*)val,(void*)val1,oper)){
				cur = cur + len1; 
				memcpy(&val2,cur,len2);
				Scan[scanDesc]->num_record = rec_num;
				Scan[scanDesc]->num_block = block_num;
				return val2;
			}
			else
				return NULL;
		}
		else{
			while(flag==0){
				if(bid<0){
					flag=1; /* If this is the last block break */
				}
				while(rec_num<count_recs){
					memcpy(&val1,cur,len1); /* Get first value */
					if(!AM_Compare(Scan[scanDesc]->id,(void*)val,(void*)val1,EQUAL)){
						cur = cur + len1 + len2;
						rec_num++;
					}
					else{
						cur = cur + len1;
						memcpy(&val2,cur,len2);
						Scan[scanDesc]->num_record = rec_num;
						Scan[scanDesc]->num_block = block_num;
						return val2;
					}
				}
				if(BF_ReadBlock(file_id, bid, &block) < 0){     /* Read brother block */
					AM_errno = AME_CANNOT_READ_BLOCK;  
					BF_PrintError("Error in AM_FirstNotEqual: ");
					return NULL;
				}
				rec_num = 1;
				block_num = bid; 
				cur = block+sizeof(char)+1*sizeof(int); 
				memcpy(&bid,cur,sizeof(int));         /* Get brother's id */
				cur = block+sizeof(char)+3*sizeof(int); 
			}
			return NULL;
		}
	}
}


int AM_CloseIndexScan(int scanDesc){
	if(Scan[scanDesc]!=NULL){
		free(Scan[scanDesc]);
		Scan[scanDesc] = NULL;
		return AME_OK;
	}
	else{
		AM_errno = AME_NULL_INDEX;
		return AM_errno;
	}
}


void AM_PrintError(char *errString){
	fprintf(stderr, "%s\n", errString);
	switch(AM_errno){
		case AME_EOF:
			fprintf(stderr, "%s\n", "No more records with given value.\n");
			break;
		case AME_FILE_EXISTED:
			fprintf(stderr, "%s\n", "File already existed.\n");
			break;
		case AME_INDEX_NOT_CREATED:
			fprintf(stderr, "%s\n","Cannot create index because of a problem on BF level.\n" );
			break;
		case AME_OPEN_FILE:
			fprintf(stderr, "%s\n", "Cannot destroy index because there are open files.\n");
			break;
		case AME_NOT_REMOVED:
			fprintf(stderr, "%s\n", "Cannot remove file from disk.\n");
			break;
		case AME_INDEX_NOT_OPENED:
			fprintf(stderr, "%s\n","Cannot open index because of a problem on BF level.\n" );
			break;
		case AME_MAX_OPEN:
			fprintf(stderr, "%s\n", "Maximum number of open files reached.\n");
			break;
		case AME_OPEN_SCAN:
			fprintf(stderr, "%s\n","Cannot close index because there are active scans.\n" );
			break;
		case AME_INDEX_NOT_CLOSED:
			fprintf(stderr, "%s\n", "Cannot close index because of a problem on Bf level.\n");
			break;
		case AME_MAX_SCAN:
			fprintf(stderr, "%s\n", "Maximum number of active scans reached.\n");
			break;
		case AME_NULL_INDEX:
			fprintf(stderr, "%s\n", "No such scan active.\n" );
			break;
		case AME_BLOCK_NOT_CREATED:
			fprintf(stderr, "%s\n", "New could not be created.\n");
			break;
		case AME_CANNOT_READ_BLOCK:
			fprintf(stderr, "%s\n", "Block could not be read.\n");
			break;
		case AME_CANNOT_WRITE_BLOCK:
			fprintf(stderr, "%s\n", "Block could not be written.\n");
			break;
		case AME_CANNOT_SPLIT_DATABLOCK:
			fprintf(stderr, "%s\n", "Cannot split data block\n");
			break;
		case AME_CANNOT_SPLIT_INDEXBLOCK:
			fprintf(stderr, "%s\n", "Cannot split index block\n");
			break;
		case AME_CANNOT_INSERT_INDEX:
			fprintf(stderr, "%s\n", "Cannot insert index in index block\n");
			break;
		case AME_SEARCH_FAILED:
			fprintf(stderr, "%s\n", "Search Failed\n");
			break;
		case AME_FIRSTNOTEQUAL_FAILED:
			fprintf(stderr, "%s\n", "First_not_equal function failed\n");
			break;
		case AME_FIRSTDBLOCK_FAILED:
			fprintf(stderr, "%s\n", "First_data_block function failed\n");
			break;
	}
}


int AM_Compare(int file_id,void *val,void *val1,int op){
    Info in;
    void *block;
    int flag;
    if(BF_ReadBlock(file_id, 0 , &block) < 0){ 
        AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in AM_Compare: ");
        return 2;
    }
    memcpy(&in,block,sizeof(Info));
    if(in.attrType1== STRING){
        char v[in.attrLength1],v1[in.attrLength1];
        strcpy(v, (char*) val);
        strcpy(v1, (char*) val1);
        flag = strcmp(v,v1);
        if(op==EQUAL && flag==0)
            return 0;
        else if(op==NOT_EQUAL && flag!=0)
            return 0;
        else if(op==LESS_THAN && flag<0)
            return 0;
        else if(op==GREATER_THAN && flag>0)
            return 0;
        else if(op==LESS_THAN_OR_EQUAL && flag<=0)
            return 0;
        else if(op==GREATER_THAN_OR_EQUAL && flag>=0)
            return 0;
        else
            return -1;
    }
    else if(in.attrType1== INTEGER){
        int v = ((int*) val);   
        int v1 = ((int*) val1);
        if(op==EQUAL && v==v1)
            return 0;
        else if(op==NOT_EQUAL && v!=v1)
            return 0;
        else if(op==LESS_THAN && v<v1)
            return 0;
        else if(op==GREATER_THAN && v>v1)
            return 0;
        else if(op==LESS_THAN_OR_EQUAL && v<=v1)
            return 0;
        else if(op==GREATER_THAN_OR_EQUAL && v>=v1)
            return 0;
        else
            return -1;
    }
    else{
    	float v = *((float*) val);   
        float v1 = *((float*) val1);
        if(op==EQUAL && v==v1)
            return 0;
        else if(op==NOT_EQUAL && v!=v1)
            return 0;
        else if(op==LESS_THAN && v<v1)
            return 0;
        else if(op==GREATER_THAN && v>v1)
            return 0;
        else if(op==LESS_THAN_OR_EQUAL && v<=v1)
            return 0;
        else if(op==GREATER_THAN_OR_EQUAL && v>=v1)
            return 0;
        else
            return -1;
    }
}
 

int Block_Sort(int file_id,int block_num){
    void *block,*cur1,*cur2,*cur3,*cur4;
    Info in;
    char c;
    int i,j,countrec,attr2,len2;   /* In case of index block: attr2 = integer, len2=4 */
    if(BF_ReadBlock(file_id, 0 , &block) < 0){ 
    	AM_errno = AME_CANNOT_READ_BLOCK; 
        BF_PrintError("Error in Block_sort: ");  
        return AM_errno;
    }
    memcpy(&in,block,sizeof(Info));
    if(BF_ReadBlock(file_id, block_num , &block) < 0){ 
    	AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in Block_sort: ");
        return AM_errno;
    }
    memcpy(&c,block,sizeof(char));
    memcpy(&countrec,block +sizeof(char)+2*sizeof(int),sizeof(int)); /* Number of the records in the block */
    if(c=='d'){ /* data block */
    	cur1 = block+sizeof(char)+3*sizeof(int);   /* Moving to the first record */
    	attr2 = in.attrType2;
    	len2 = in.attrLength2;
    }
    else{ /* index block */
    	cur1 = block+sizeof(char)+4*sizeof(int);   /* Moving to the first record */
    	attr2 = INTEGER;
    	len2 = 4;
    }
    if(in.attrType1==STRING && (attr2==INTEGER || attr2==FLOAT)){ 
        char str1[in.attrLength1];
        char str2[in.attrLength1];
        char temp1[in.attrLength1];
        void *temp;
        memcpy(&str1,cur1,in.attrLength1);
        /* Sorting algorithm */
        for(i=0 ; i<countrec; i++){ 
            cur2=cur1;
            for(j=i+1 ; j<countrec ; j++){
                cur2 = cur2 + in.attrLength1 + len2;   /* cur2 always points to the next record */
                memcpy(&str2,cur2,in.attrLength1);
                cur3 = cur1 + in.attrLength1;
                cur4 = cur2 + in.attrLength1;
                if(strcmp(str1,str2)>0){
                	memcpy(&temp1,cur1,in.attrLength1);
                    memcpy(cur1,cur2,in.attrLength1);
                    memcpy(cur2,&temp1,in.attrLength1);
                    memcpy(&temp,cur3,len2);
                    memcpy(cur3,cur4,len2);
                    memcpy(cur4,&temp,len2);
                    memcpy(&str1,cur1,in.attrLength1);
                }
            }
            cur1 = cur1 + (in.attrLength1+len2);   /* Moving to the next record */
            memcpy(&str1,cur1,in.attrLength1);
        }
    }
   else if(in.attrType1==STRING && (attr2==STRING)){ 
        char str1[in.attrLength1];
        char str2[in.attrLength1];
        char temp1[in.attrLength1];
        char temp2[len2];
        memcpy(&str1,cur1,in.attrLength1);
        /* Sorting algorithm */
        for(i=0 ; i<countrec; i++){ 
            cur2=cur1;
            for(j=i+1 ; j<countrec ; j++){
                cur2 = cur2 + in.attrLength1 + len2;   /* cur2 always points to the next record */
                memcpy(&str2,cur2,in.attrLength1);
                cur3 = cur1 + in.attrLength1;
				cur4 = cur2 + in.attrLength1;
				memcpy(&temp1,cur3,len2);
				memcpy(&temp2,cur4,len2);
                if(strcmp(str1,str2)>0){
                   memcpy(&temp1,cur1,in.attrLength1);
                    memcpy(cur1,cur2,in.attrLength1);
                    memcpy(cur2,&temp1,in.attrLength1);
                    memcpy(&temp2,cur3,len2);
                    memcpy(cur3,cur4,len2);
                    memcpy(cur4,&temp2,len2);
                    memcpy(&str1,cur1,in.attrLength1);
                }
            }
            cur1 = cur1 + (in.attrLength1+len2);    /* Moving to the next record */
            memcpy(&str1,cur1,in.attrLength1);
        }
    }
   else if(in.attrType1==INTEGER && (attr2==INTEGER || attr2==FLOAT)){
        int first;
        int next;
        void *temp;
        memcpy(&first,cur1,in.attrLength1);
        /* Sorting algorithm */
        for(i=0 ; i<countrec ; i++){
            cur2=cur1;
            for(j=i+1; j<countrec ; j++){
                cur2 = cur2 + in.attrLength1 + len2;    /* Cur2 always points to the next record */
                memcpy(&next,cur2,in.attrLength1);
                cur3 = cur1 + in.attrLength1;
                cur4 = cur2 + in.attrLength1;
                if(first>next){
                    memcpy(&temp,cur1,in.attrLength1);
                    memcpy(cur1,cur2,in.attrLength1);
                    memcpy(cur2,&temp,in.attrLength1);
                    memcpy(&temp,cur3,len2);
                    memcpy(cur3,cur4,len2);
                    memcpy(cur4,&temp,len2);
                    memcpy(&first,cur1,in.attrLength1);
                }
            }
           cur1 = cur1 + (in.attrLength1+len2);       /* Moving to the next record */
           memcpy(&first,cur1,in.attrLength1);
        }
    }
    else if(in.attrType1==INTEGER && (attr2==STRING)){
        int first;
        int next;
        void *temp;
        char temp1[len2];
        memcpy(&first,cur1,in.attrLength1);
        /* Sorting algorithm */
        for(i=0 ; i<countrec ; i++){
            cur2=cur1;
            for(j=i+1; j<countrec ; j++){
                cur2 = cur2 + in.attrLength1 + len2; /* cur2 always points to the next record */
                memcpy(&next,cur2,in.attrLength1);
                cur3 = cur1 + in.attrLength1;
                cur4 = cur2 + in.attrLength1;
                if(first>next){
                    memcpy(&temp,cur1,in.attrLength1);
                    memcpy(cur1,cur2,in.attrLength1);
                    memcpy(cur2,&temp,in.attrLength1);
                    memcpy(&temp1,cur3,len2);
                    memcpy(cur3,cur4,len2);
                    memcpy(cur4,&temp1,len2);
                    memcpy(&first,cur1,in.attrLength1);
                }
            }
           cur1 = cur1 + (in.attrLength1+len2);   /* Moving to the next record */
           memcpy(&first,cur1,in.attrLength1);
        }
    }
    else if(in.attrType1==FLOAT && (attr2==INTEGER || attr2==FLOAT)){
        float first;
        float next;
        void *temp;
        memcpy(&first,cur1,in.attrLength1);
        /* Sorting algorithm */
        for(i=0 ; i<countrec ; i++){
            cur2=cur1;
            for(j=i+1; j<countrec ; j++){
                cur2 = cur2 + in.attrLength1 + len2;    /* cur2 always points to the next record */
                memcpy(&next,cur2,in.attrLength1);
                cur3 = cur1 + in.attrLength1;
                cur4 = cur2 + in.attrLength1;
                if(first>next){
                    memcpy(&temp,cur1,in.attrLength1);
                    memcpy(cur1,cur2,in.attrLength1);
                    memcpy(cur2,&temp,in.attrLength1);
                    memcpy(&temp,cur3,len2);
                    memcpy(cur3,cur4,len2);
                    memcpy(cur4,&temp,len2);
                    memcpy(&first,cur1,in.attrLength1);
                }
            }
           cur1 = cur1 + (in.attrLength1+len2);    /* Moving to the next record */
           memcpy(&first,cur1,in.attrLength1);
        }
    }
    else{ /* if first attrtype is float and second attr type is string */
        float first;
        float next;
        void *temp;
        char temp1[len2];
        memcpy(&first,cur1,in.attrLength1);
        /* Sorting algorithm */
        for(i=0 ; i<countrec ; i++){
            cur2=cur1;
            for(j=i+1; j<countrec ; j++){
                cur2 = cur2 + in.attrLength1 + len2;   /* Cur2 always points to the next record */
                memcpy(&next,cur2,in.attrLength1);
                cur3 = cur1 + in.attrLength1;
                cur4 = cur2 + in.attrLength1;
                if(first>next){
                    memcpy(&temp,cur1,in.attrLength1);
                    memcpy(cur1,cur2,in.attrLength1);
                    memcpy(cur2,&temp,in.attrLength1);
                    memcpy(&temp1,cur3,len2);
                    memcpy(cur3,cur4,len2);
                    memcpy(cur4,&temp1,len2);
                    memcpy(&first,cur1,in.attrLength1);
                }
            }
           cur1 = cur1 + (in.attrLength1+len2); /* Moving to the next record */
           memcpy(&first,cur1,in.attrLength1);
        }
    }
    if (BF_WriteBlock(file_id, block_num) < 0){
       BF_PrintError("Error in ΑΜ_BlockSort: ");
       return AM_errno;
    }
    return AME_OK;
}


int Search(int file_id, int block_num, void *value, int length, Stack *s) {
	void *block;
	char x;
	int i, nb, j, bl,comp;
	int flag = 0; 
	int ind_bl;
	Info in;
	if(BF_ReadBlock(file_id,0,&block)<0){
		AM_errno = AME_INDEX_NOT_CREATED;
		BF_PrintError("Error in Search \n");
		return AM_errno;
	}
    memcpy(&in,block,sizeof(Info));
	if(BF_ReadBlock(file_id, block_num , &block) < 0){ 
		AM_errno = AME_INDEX_NOT_CREATED;
		BF_PrintError("Error in Search \n");
		return AM_errno;
	}
	memcpy(&x,block,sizeof(char));
	if (x=='d') {                 /* Data block found */
		return block_num;
	}
	/* Index Block */
	if ( s!= NULL) 
        Stack_push(block_num,s);    /* in this way we keep the path through the index blocks */
	memcpy(&ind_bl,block +sizeof(char)+2*sizeof(int),sizeof(int));
	if(ind_bl>=1){
		if(in.attrType1!=STRING){
			void *key;
			bl = sizeof(char) + 4 *sizeof(int);
			for (j=0 ; j<ind_bl; j++) {
				memcpy(&key,block + bl,length);                /* Get the key */ 
				comp = AM_Compare(file_id,(void*)key,(void*)value,GREATER_THAN) ;
				if (comp == 0) {
					flag = 1 ;
					break;
				}
				bl = bl + length +  sizeof(int);
			}
			if (flag==1) {
				memcpy(&nb,block + bl - sizeof(int),sizeof(int));
				block_num=Search(file_id, nb,(void*)value,length,s);
			}
			else {            /* it's the last pointer */
				memcpy(&nb,block + bl + length,sizeof(int));
				block_num=Search(file_id, nb,(void*)value,length,s);
			}
			return block_num;
		}
		else{
			char key[length];
			bl = sizeof(char) + 4 *sizeof(int);
			for (j=0 ; j<ind_bl; j++) {
				memcpy(&key,block + bl,length);                /* Get the key */ 
				comp = AM_Compare(file_id,(void*)key,(void*)value,GREATER_THAN) ;
				if (comp == 0) {
					flag = 1 ;
					break;
				}
				bl = bl + length +  sizeof(int);
			}
			if (flag==1) {
				memcpy(&nb,block + bl - sizeof(int),sizeof(int));
				block_num=Search(file_id, nb,(void*)value,length,s);
			}
			else {            /* it's the last pointer */
				memcpy(&nb,block + bl + length,sizeof(int));
				block_num=Search(file_id, nb,(void*)value,length,s);
			}
			return block_num;
		}
	}
	else if(ind_bl==0){     /* there's only one data block in the tree */
		memcpy(&block_num,block+sizeof(char)+3*sizeof(int),sizeof(int));
		return block_num;
	}
}


int FirstDBlock(int fileDesc){
/* Returns the id of the first data block or error code */
	int root_id ;
	void *block;
	int next_block;
	char block_type;
	Info in;
	if(BF_ReadBlock(Open[fileDesc]->id, 0 ,&block) < 0 ){
		AM_errno = AME_CANNOT_READ_BLOCK;
		AM_PrintError("Error in FirstDBlock\n");
		return AM_errno;
	}
	memcpy(&in,block,sizeof(Info));
	root_id = in.root;
	if(BF_ReadBlock(Open[fileDesc]->id, root_id , &block) < 0){ 
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in FirstDBlock\n");
		return AM_errno;
	}
	memcpy(&block_type, block, sizeof(char));
	memcpy(&next_block, block+sizeof(char)+3*sizeof(int), sizeof(int));
	while(block_type!='d'){
		if(BF_ReadBlock(Open[fileDesc]->id, next_block , &block) < 0){ 
			AM_errno = AME_CANNOT_READ_BLOCK;
			BF_PrintError("Error in FirstDBlock\n");
			return AM_errno;
		}
		memcpy(&block_type, block, sizeof(char));
		if(block_type=='i')      /* If this is a data block there is not a child */
			memcpy(&next_block, block+sizeof(char)+3*sizeof(int), sizeof(int));
	}
	return next_block;
}


int SearchDataBlock(int file_id, int block_num, void *value,int op) {
	Info in;
	void *block;
	int i,bl,comp;
	int r = 1 ;                /* Record's position */
	int countrec;
	int flag = 0 ;
	if(BF_ReadBlock(file_id, 0, &block) < 0){   
		AM_errno = AME_CANNOT_READ_BLOCK;   
		BF_PrintError("Error in SearchDataBlock \n");
		return AM_errno;
	}
	memcpy(&in, block, sizeof(Info));
	if(BF_ReadBlock(file_id, block_num , &block) < 0){ 
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in SearchDataBlock \n");
		return AM_errno;
	}
	memcpy(&countrec,block +sizeof(char)+2*sizeof(int),sizeof(int));   /*Number of the records in the block*/
	bl = sizeof(char) + 3 *sizeof(int);
	if(in.attrType1!=STRING){
		void *key;
		for (i=0;i<countrec;i++) {
			memcpy(&key,block + bl,in.attrLength1);
			comp = AM_Compare(file_id,(void*)value,(void*)key,op);
			if (comp== 0) {
				flag = 1;
				break;
			}
			r++ ;
			bl = bl + in.attrLength1 + in.attrLength2;
		}
		if (flag==0) 
			r = -1 ;
		return r;
	}
	else{
		char key[in.attrLength1];
		for (i=0;i<countrec;i++) {
			memcpy(&key,block + bl,in.attrLength1);
			comp = AM_Compare(file_id,(void*)value,(void*)key,op);
			if (comp== 0) {
				flag = 1;
				break;
			}
			r++ ;
			bl = bl + in.attrLength1 + in.attrLength2;
		}
		if (flag==0) 
			r = -1 ;
		return r; 
	}
}


int AM_FirstNotEqual(int fileDesc,void *val){
	void *block,*cur;
	Info in;
	int atr1,atr2,countrec,id,flag=0;
	int bid,j;
	if(BF_ReadBlock(Open[fileDesc]->id, 0, &block) < 0){    
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in AM_FirstNotEqual: ");
		return AM_errno;
	}
	memcpy(&in,block,sizeof(Info));
	atr1 = in.attrLength1;
	atr2 = in.attrLength2;
	/* Get id of first data block */
	id = FirstDBlock(fileDesc); 
	if(BF_ReadBlock(Open[fileDesc]->id, id, &block) < 0){  
		AM_errno = AME_CANNOT_READ_BLOCK;    
		BF_PrintError("Error in AM_FirstNotEqual: ");
		return AM_errno;
	}
	memcpy(&countrec,block +sizeof(char)+2*sizeof(int), sizeof(int)); /* Number of records in the block */
	cur = block+sizeof(char)+sizeof(int); 
	memcpy(&bid,cur,sizeof(int)); /* Get brother's id */
	cur = block+sizeof(char)+3*sizeof(int); /* Set cur to the first record of the block */
	if(in.attrType1!=STRING){
		void *val1;
		while(flag==0){
			if(bid<0){
				flag=1; /* If this is the last block break */
			}
			for(j=0 ; j<countrec ; j++){
				memcpy(&val1,cur,atr1); /* Get first value */
				if(!AM_Compare(Open[fileDesc]->id,(void*)val,(void*)val1,NOT_EQUAL))
					return id; /* if not equal return id */
				else
					cur = cur + atr1 + atr2; /* move to the next record */
				j++;
			}
			if(bid>=0){
				if(BF_ReadBlock(Open[fileDesc]->id, bid, &block) < 0){ /* Read brother block */
					AM_errno = AME_CANNOT_READ_BLOCK;
					BF_PrintError("Error in AM_FirstNotEqual: ");
					return AM_errno;
				}
				memcpy(&countrec,block +sizeof(char)+2*sizeof(int), sizeof(int));
				id = bid; 
				cur = block+sizeof(char)+1*sizeof(int); 
				memcpy(&bid,cur,sizeof(int)); /* Get brother's id */
				cur = block+sizeof(char)+3*sizeof(int);
			} 
			else break;
		}
    }
    else{
    	char val1[in.attrLength1];
    	while(flag==0){
			if(bid<0){
				flag=1; /* If this is the last block break */
			}
			for(j=0 ; j<countrec ; j++){
				memcpy(&val1,cur,atr1); /*Get first value*/
				if(!AM_Compare(Open[fileDesc]->id,(void*)val,(void*)val1,NOT_EQUAL))
					return id; /* If not equal return id */
				else
					cur = cur + atr1 + atr2; /* move to the next record */
				j++;
			}
			if(bid>=0){
				if(BF_ReadBlock(Open[fileDesc]->id, bid, &block) < 0){ /* Read brother block */
					AM_errno = AME_CANNOT_READ_BLOCK;
					BF_PrintError("Error in AM_FirstNotEqual: ");
					return AM_errno;
				}
				memcpy(&countrec,block +sizeof(char)+2*sizeof(int), sizeof(int));
				id = bid; 
				cur = block+sizeof(char)+1*sizeof(int); 
				memcpy(&bid,cur,sizeof(int)); /* Get brother's id */
				cur = block+sizeof(char)+3*sizeof(int);
			} 
			else break;
		}
    }
    return id;
}


int AM_CreateBlock(char type, int fileDesc) {  
/* Returns id of new block (index or data) or error code */
	void *block;
	Info info ;
	int c = BF_GetBlockCounter(fileDesc) ;
	int brother = -1;
	int rec = 0;
	if (BF_AllocateBlock(fileDesc) < 0){
		AM_errno = AME_BLOCK_NOT_CREATED;
		BF_PrintError("Error in AM_CreateBlock \n");
		return AM_errno;
	}
	if (BF_ReadBlock(fileDesc, c, &block) < 0){
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in AM_CreateBlock \n");
		return AM_errno;
	}
	memcpy(block, &type, sizeof(char));     /* the type of the block : d or i */
	memcpy(block + sizeof(char), &c, sizeof(int));         /* the number-id of the block */
	memcpy(block + sizeof(char) + sizeof(int), &brother, sizeof(int));    /* the 'pointer' to the brother*/
	memcpy(block + sizeof(char) + 2 *sizeof(int), &rec, sizeof(int));    /* the number of records*/
	if (BF_WriteBlock(fileDesc, c) < 0){
		AM_errno = AME_CANNOT_WRITE_BLOCK;
		BF_PrintError("Error in ΑΜ_CreateBlock \n");
		return AM_errno;
	}
	return c;
}


int AM_NumberOfRecords(int file_id) {
/* Returns the number of records that fit in the data block or error code*/
	void *block;
	Info info ;
	int nrb ;
	if (BF_ReadBlock(file_id,0, &block) < 0) {
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in AM_NumberOfRecords \n");
		return AM_errno; 
	}
	memcpy(&info,block, sizeof(Info));
	nrb = (BLOCK_SIZE - 2 * sizeof(int) - sizeof(char)) / (info.attrLength1 + info.attrLength2) ;
	return nrb ;
}


int AM_NumberOfKeys(int file_id) {
/* Returns number of pairs (key-'pointer') */
	void *block;
	Info info ;
	int n ;
	if (BF_ReadBlock(file_id,0, &block) < 0) {
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in AM_NumberOfKeys \n");
		return AM_errno;
	}
	memcpy(&info,block, sizeof(Info));
	n = (BLOCK_SIZE - sizeof(char) - 4 * sizeof(int)) / ( sizeof(int) + info.attrLength1) ;
	return n;
}


int AM_FindMiddle(int file_id, int block_id){
	void *block;
	char block_type;
	int  count;
	int  middle;
	if (BF_ReadBlock(file_id, block_id, &block) < 0) {
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in AM_FindMiddle \n");
		return AM_errno;
	}
	memcpy(&block_type, block, sizeof(char));
	if (block_type=='d')
		count = AM_NumberOfRecords(file_id);
	else
		count = AM_NumberOfKeys(file_id);
	if (count%2 == 0)
		middle = count / 2;
	else
		middle = count / 2 + 1;
	return AME_OK;
}

int AM_InsertEntry(int fileDesc,void *value1,void *value2){
    void *block,*cur;
    Info in;
    int r,len1,len2;
    char type1,type2,c;
    Stack *s = Stack_Create();
    if(BF_ReadBlock(Open[fileDesc]->id, 0, &block) < 0){
    	AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in AM_InsertEntry: ");
        return AM_errno;
    }
    memcpy(&in,block,sizeof(Info)); 
    r = in.root;
    type1 = in.attrType1;
    type2 = in.attrType2;
    len1 = in.attrLength1;
    len2 = in.attrLength2;
    if(r == -1){ /* Tree is empty */
        int id1 = AM_CreateBlock('i',Open[fileDesc]->id);    /* Allocate root (index block) */
        int id2 = AM_CreateBlock('d',Open[fileDesc]->id);     /* Allocate first data block */
        if(BF_ReadBlock(Open[fileDesc]->id, id1, &block) < 0){   /* Reading first block */
            AM_errno = AME_CANNOT_READ_BLOCK;
            BF_PrintError("Error in AM_InsertEntry: ");
            return AM_errno;
        }
        int count = 0;   /* Even when the index block has one child count is 0 */
        cur = block + sizeof(char) + 2*sizeof(int); /* Move to the third int (number of records in the block) */
        memcpy(cur,&count,sizeof(int)); /* Change count */
        cur = cur + sizeof(int); /* Move to the fourth int (Child's id) */
        memcpy(cur,&id2,sizeof(int)); 
        count = 1;
        if(BF_ReadBlock(Open[fileDesc]->id, id2, &block) < 0){
           AM_errno = AME_CANNOT_READ_BLOCK;
           BF_PrintError("Error in AM_InsertEntry: ");
           return AM_errno;
        }
        cur = block + sizeof(char);
        memcpy(cur,&id2,sizeof(int));
        cur = cur + 2*sizeof(int);
        memcpy(cur,&count,sizeof(int));
        cur = block + sizeof(char) +3*sizeof(int); /* Move to the key value */
		memcpy(cur,value1,len1);
		cur = cur + in.attrLength1; /*Move to second value*/
		memcpy(cur,value2,len2);
        if(BF_WriteBlock(Open[fileDesc]->id,id1)){ /* Writing first index block */
        	AM_errno = AME_CANNOT_WRITE_BLOCK;
            BF_PrintError("Error in AM_InsertEntry: ");
            return AM_errno;
        }
        if(BF_WriteBlock(Open[fileDesc]->id,id2)){ /* Writing second data block */
            AM_errno = AME_CANNOT_WRITE_BLOCK;
            BF_PrintError("Error in AM_InsertEntry: ");
            return AM_errno;
        }
        if(BF_ReadBlock(Open[fileDesc]->id, 0, &block) < 0){ /* Reading first block */   
        	AM_errno = AME_CANNOT_READ_BLOCK;
        	BF_PrintError("Error in AM_InsertEntry: ");
            return AM_errno;
        }
        memcpy(&in,block,sizeof(Info));
        in.root = id1;
        memcpy(block,&in,sizeof(Info)); /* Writing root to Info block */
        if(BF_WriteBlock(Open[fileDesc]->id,0)){ /* Writing first data block */
        	AM_errno = AME_CANNOT_WRITE_BLOCK;
            BF_PrintError("Error in AM_InsertEntry: ");
            return AM_errno;
        }
    }
    else{ /* Tree is not empty */
        int id = Search(Open[fileDesc]->id, in.root,(void*)value1,len1,NULL); /* Get the right block for inserting value */
    	if(id<0){
    		AM_errno = AME_SEARCH_FAILED;
    		return AM_errno;
    	}
        if (BF_ReadBlock(Open[fileDesc]->id, id, &block) < 0){ /* Read that block */
        	AM_errno = AME_CANNOT_READ_BLOCK;
            BF_PrintError("Error in BF_ReadBlock\n");
            return AM_errno;
        }
        cur = block + sizeof(char) + 2*sizeof(int); /* Move to the third int */
        int count;
        memcpy(&count,cur,sizeof(int)); /* Get number of records in the block */
        cur = cur + sizeof(int);   /* Move to First record */
        if(count < AM_NumberOfRecords(Open[fileDesc]->id)){ /* if the record fits this block */
            cur = cur + count*(len1+len2); /* Go to the first avalaible bit */
	   		memcpy(cur,value1,len1);
            cur = cur + len1;
            memcpy(cur,value2,len2);
            count++;
            cur = block + sizeof(char) + 2*sizeof(int);
            memcpy(cur,&count,sizeof(int));
            if(BF_WriteBlock(Open[fileDesc]->id,id)){ /* Writing first data block */
                AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_InsertEntry: ");
                return AM_errno;
            }
            Block_Sort(Open[fileDesc]->id,id); /* Sort records inside the block */
        }
        else{
		    int addr;
		    int did;
		    addr = AM_SplitData(Open[fileDesc]->id,id,value1,value2,len1,len2,&did);/* Returns first key of new (sorted) block */
		    if(addr<0){
		    	AM_errno = AME_CANNOT_SPLIT_DATABLOCK;
		    	return AM_errno;
		    }
		    if(BF_ReadBlock(Open[fileDesc]->id,addr,&block) < 0){ /* Read that block */
		    	AM_errno = AME_CANNOT_READ_BLOCK;
		        BF_PrintError("Error in AM_InsertEntry\n");
		        return AM_errno;
			}
			if(type1!=STRING){
				void *min;
				memcpy(&min,addr,len1);
				AM_Insert_Index(fileDesc,s,did,(void*)min);
			}
	    	else{
	    		char min[len1];
	    		memcpy(&min,addr,len1);
	    		AM_Insert_Index(fileDesc,s,did,(void*)min);
	    	}
		}
    }
    Stack_Destroy(s);
}


int AM_Insert_Index(int fileDesc,Stack *s,int did,void *min){
    void *block,*block1;
    int cur,count,c, index, i_id, flag;
    int file_id = Open[fileDesc]->id;
    Info in;
    if(BF_ReadBlock(file_id,0,&block)){ /* Reading info block */
    	AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in AM_Insert_Index: ");
        return AM_errno;
    }
    memcpy(&in,block,sizeof(Info));
    /* Get the index block from the stack */
    index = Stack_pop(s);
    if(BF_ReadBlock(file_id,index,&block1)){
    	AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in AM_Insert_Index: ");
        return AM_errno;
    }
    /* Get the count */
    cur = sizeof(char) + 2*sizeof(int);
    memcpy(&count,block1 + cur,sizeof(int));
    if(count<AM_NumberOfKeys(file_id)){
        cur = cur + 2*sizeof(int) + count * (in.attrLength1+sizeof(int));
        memcpy(block1 + cur,min,in.attrLength1);
        memcpy(block1 + cur + in.attrLength1,&did,sizeof(int));
        /* Change the count */
        count++;
        memcpy(block1 + 2 * sizeof(int),&count,sizeof(int));
        if (BF_WriteBlock(file_id, index)<0){
            AM_errno = AME_CANNOT_WRITE_BLOCK;
            BF_PrintError("Error in AM_Insert_Index: \n");
            return AM_errno;
        }
        Block_Sort(file_id,index);
        return AME_OK;
    }
    else{
        int addr;
        void *block_;
        addr = AM_SplitIndex(file_id,index,min,did,in.attrLength1,&i_id);
        if(addr<0){
        	AM_errno = AME_CANNOT_INSERT_INDEX;
        	return AM_errno;
        }
        if(s->count==0){    /* Create a new root */
            void *block2;
            int cur1 ;
            int rid = AM_CreateBlock('i',file_id);
            if(rid<0){
            	AM_errno = AME_BLOCK_NOT_CREATED;
            	return AM_errno;
            }
            int old_root = in.root;
            in.root = rid;         /* Change root's id in Info block */
            memcpy(block,&in,sizeof(Info));     /* Write info back to block 0 */
            if(BF_WriteBlock(file_id,0)){
            	AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_Insert_Index: ");
                return AM_errno;
            }
            if(BF_ReadBlock(file_id,rid,&block2)){    /* Reading the new index block (root) */
                AM_errno = AME_CANNOT_READ_BLOCK;
                BF_PrintError("Error in AM_Insert_Index: ");
                return AM_errno;
            }
            /* Change the 1st pointer */
            memcpy(block2+sizeof(char)+3*sizeof(int),&old_root,sizeof(int));
            cur1 =sizeof(char) + 4 * sizeof(int);
            if(in.attrType1!=STRING){
                void *sp;
                memcpy(&sp,addr,in.attrLength1);
                memcpy(block2 + cur,sp,in.attrLength1);
            }
            else{
                char sp[in.attrLength1];
                memcpy(&sp,addr,in.attrLength1);
                memcpy(block2 + cur,sp,in.attrLength1);
            }
            memcpy(block2 + cur + in.attrLength1,&i_id,sizeof(int));
            /* Change the count */
            int c = 1;
            memcpy(block2+sizeof(char)+2*sizeof(int),&c,sizeof(int));
            if (BF_WriteBlock(file_id, rid)<0){
                AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_Insert_Index: \n");
                return AM_errno;
            }
            Block_Sort(file_id,rid);  
            return AME_OK;
        }
	    else{
	        index = Stack_pop(s);
	        if(in.attrType1!=STRING){
	            void *sp;
	            memcpy(&sp,addr,in.attrLength1);
	            flag = AM_Insert_Index(file_id,s,i_id,(void*)sp);
	        }
	        else{
	            char sp[in.attrLength1];
	            memcpy(&sp,addr,in.attrLength1);
	            flag = AM_Insert_Index(file_id,s,i_id,(void*)sp);
	        }
	        return flag ;
	    }
    }
} 


int AM_SplitIndex(int file_id, int block_id, void *newkey, int newpointer, int attrLength1, int *i_id) {
    Info in;
    void *block1, *block2,*mid, *block;
    int l,r,i,cur1,cur2,bro,m_p,m ;
    int max = AM_NumberOfKeys(file_id);
    int new_block = AM_CreateBlock('i', file_id);
    if (new_block < 0){
        AM_errno = AME_CANNOT_SPLIT_INDEXBLOCK;
        return AM_errno;    
    }
    int middle =  AM_FindMiddle(file_id, block_id);
    if (BF_ReadBlock(file_id, block_id, &block1) < 0) {
        AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in AM_SplitIndex: \n");
        return AM_errno;
    }
    if (BF_ReadBlock(file_id, new_block, &block2) < 0) {
        AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in AM_SplitIndex: \n");
        return AM_errno;
    }
    /* Get the middle key */
    memcpy(mid,block1+sizeof(char) + 4*sizeof(int)+ (middle-1) * (sizeof(int) +attrLength1),attrLength1);
    /* Get the pointer - pair of the middle key */
    memcpy(&m_p,block1+sizeof(char) + 4*sizeof(int)+ (middle-1) * (sizeof(int) +attrLength1) + attrLength1,sizeof(int));
    if (AM_Compare(file_id,(void*)newkey,(void*)mid, LESS_THAN) == 0) {      /* key < middle */
    	m = block1+sizeof(char) + 4*sizeof(int)+ (middle-1) * (sizeof(int) +attrLength1);
        l = middle - 1 ;     /* number of 'couples' staying at this index block */
        r = max - middle ;   /* number of 'couples' moving to the new index block */
        /* Moving */
        cur1 =sizeof(char) + 4*sizeof(int) + middle * (sizeof(int) +attrLength1);
        cur2 =sizeof(char) + 4*sizeof(int);
        for (i=0 ; i<r ; i++){
            memswap(file_id,block1+cur1,block2+cur2,'i');
            cur1 = cur1 + sizeof(int) + attrLength1;
            cur2 = cur2 + sizeof(int) + attrLength1;
        }
        /* Put the new key to the old block */
        memcpy(block1+sizeof(char) + 4*sizeof(int) + l * (sizeof(int) +attrLength1),newkey,attrLength1);
        memcpy(block1+sizeof(char) + 4*sizeof(int) + l * (sizeof(int) +attrLength1) + attrLength1,&newpointer,sizeof(int));
        /* Fixing the counts */
        memcpy(block1+sizeof(char) + 2*sizeof(int),&middle,sizeof(int));
        memcpy(block2+sizeof(char) + 2*sizeof(int),&r,sizeof(int));
    }
    else if (AM_Compare(file_id,(void*)newkey,(void*)mid, GREATER_THAN) == 0) {
        middle++; 
        m = block1+sizeof(char) + 4*sizeof(int)+ (middle-1) * (sizeof(int) +attrLength1);
        l = middle - 1 ; /* number of 'couples' staying at this index block */
        r = max - middle ; /* number of 'couples' moving to the new index block */
        /* Moving */
        cur1 =sizeof(char) + 4*sizeof(int) + middle * (sizeof(int) +attrLength1);
        cur2 =sizeof(char) + 4*sizeof(int);
        for (i=0 ; i<r ; i++){
            memswap(file_id,block1+cur1,block2+cur2,'i');
            cur1 = cur1 + sizeof(int) + attrLength1;
            cur2 = cur2 + sizeof(int) + attrLength1;
        }
        /* Put the new key to the old block */
        memcpy(block2+sizeof(char) + 4*sizeof(int) + r * (sizeof(int) +attrLength1),newkey,attrLength1);
        memcpy(block2+sizeof(char) + 4*sizeof(int) + r * (sizeof(int) +attrLength1) + attrLength1,&newpointer,sizeof(int));
        /* Fixing the counts */
        memcpy(block1+sizeof(char) + 2*sizeof(int),&l,sizeof(int));
        r++;  /* because of the new key */
        memcpy(block2+sizeof(char) + 2*sizeof(int),&r,sizeof(int));
    }
    else {
        AM_errno = AME_CANNOT_SPLIT_INDEXBLOCK;
        return AM_errno;
    }
    /* Fixing the pointers of the brothers */
    memcpy(&bro,block1+sizeof(char) + sizeof(int),sizeof(int));
    memcpy(block1+sizeof(char) + sizeof(int),&new_block,sizeof(int));
    memcpy(block2+sizeof(char) + sizeof(int),&bro,sizeof(int));
 
    /* Fixing the 1st 'pointer' of the new block */
    memcpy(block2+sizeof(char) + 3 * sizeof(int),&m_p,sizeof(int));
    if (BF_WriteBlock(file_id, block_id)<0){
        AM_errno = AME_CANNOT_WRITE_BLOCK;
        BF_PrintError("Error in AM_SplitIndex: \n");
        return AM_errno;
    }
    if (BF_WriteBlock(file_id, new_block)<0){
        AM_errno = AME_CANNOT_WRITE_BLOCK;
        BF_PrintError("Error in AM_SplitIndex: \n");
        return AM_errno;
    }
    Block_Sort(file_id, new_block);
    Block_Sort(file_id, block_id);
    /* Return id of new block*/
    *i_id = new_block;
    /* Return the offset of the middle */
    return m;
}


int AM_SplitData(int file_id, int block_id, void *value1, void *value2, int attrLength1, int attrLength2, int *did){
    Info in;
    void *block1, *block2, *block;
    int i, position, middle, new_count, old_count, max, moved, temp, left, right;
    int duplicates = 0;
    int new_block = AM_CreateBlock('d', file_id);
    if (new_block < 0){
        AM_errno = AME_CANNOT_SPLIT_DATABLOCK;
        return AM_errno;    
    }
    if (BF_ReadBlock(file_id, block_id, &block1) < 0) {
        AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in AM_SplitData \n");
        return AM_errno;
    }
    if (BF_ReadBlock(file_id, new_block, &block2) < 0) {
        AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in AM_SplitData \n");
        return AM_errno;
    }
    /* Change brothers of old and new block */
    memcpy(&temp, block1 + sizeof(char) + sizeof(int), sizeof(int));
    memcpy(block1 + sizeof(char) + sizeof(int), &new_block, sizeof(int));
    memcpy(block2 + sizeof(char) + sizeof(int), &temp, sizeof(int));
    max = AM_NumberOfRecords(file_id);
    if(max%2==0)
        middle = max/2;
    else
        middle = max/2+1;
    void *mid = malloc(attrLength1);
    memcpy(mid, block1+sizeof(char)+3*sizeof(int)+(middle-1)*(attrLength1+attrLength2), attrLength1);
    int start = SearchDataBlock(file_id, block_id, mid, EQUAL); /*the start of potential duplicates*/
    duplicates = FindDuplicates(file_id, block_id, block1+sizeof(char)+3*sizeof(int)+(start-1)*(attrLength1+attrLength2), attrLength1, attrLength2);
    moved=0;
    if(!AM_Compare(file_id,(void*)value1,(void*)mid, LESS_THAN)){
        left = middle - start;
        right = duplicates - left;
        block1 += sizeof(char)+3*sizeof(int);
        block2 += sizeof(char)+3*sizeof(int);
        /* Move records to the side with the highest number of duplicates*/
        if(right>=left){  /* Move to the right, new record goes to old block*/
            for(i=start; i<=max; i++){
                memswap(file_id, block1+(i-1)*(attrLength1+attrLength2), block2+moved*(attrLength1+attrLength2), 'd');
                moved++;
            }
            memcpy(block1+(start-1)*(attrLength1+attrLength2), value1, attrLength1);
            memcpy(block1+(start-1)*(attrLength1+attrLength2)+attrLength1, value2, attrLength2);
            if (BF_WriteBlock(file_id, block_id)<0){
                AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_SplitData \n");
                return AM_errno;
            }
            if (BF_WriteBlock(file_id, new_block)<0){
                AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_SplitData \n");
                return AM_errno;
            }
        }
        else{
            for(i=start+duplicates; i<=max; i++){
                memswap(file_id, block1+(i-1)*(attrLength1+attrLength2), block2+moved*(attrLength1+attrLength2), 'd');
                moved++;
            }
            memcpy(block1+(start+duplicates-1)*(attrLength1+attrLength2), value1, attrLength1);
            memcpy(block1+(start+duplicates-1)*(attrLength1+attrLength2)+attrLength1, value2, attrLength2);
            if (BF_WriteBlock(file_id, block_id)<0){
                AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_SplitData \n");
                return AM_errno;
            }
            if (BF_WriteBlock(file_id, new_block)<0){
                AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_SplitData \n");
                return AM_errno;
            }
        }
        Block_Sort(file_id, block_id);
        /* Fix counts in both blocks */
        BF_ReadBlock(file_id, block_id, &block1);
        BF_ReadBlock(file_id, new_block, &block2);
        new_count = moved;
        memcpy(block2+sizeof(char)+2*sizeof(int), &new_count, sizeof(int));
        old_count = max - moved + 1;
        memcpy(block1+sizeof(char)+2*sizeof(int), &old_count, sizeof(int));
    }
    else if(!AM_Compare(file_id,(void*)value1,(void*)mid, GREATER_THAN)){
        left = middle - start + 1;
        right = duplicates - left;
        if(right>=left){ /* Move to the right */
            for(i=start; i<=max; i++){
                memswap(file_id, block1+(i-1)*(attrLength1+attrLength2), block2+moved*(attrLength1+attrLength2), 'd');
                moved++;
            }
        }
        else{
            for(i=start+duplicates; i<=max; i++){
                memswap(file_id, block1+(i-1)*(attrLength1+attrLength2), block2+moved*(attrLength1+attrLength2), 'd');
                moved++;
            }       
        }
        /* Place record to the 2nd block */
        memcpy(block2+moved*(attrLength1+attrLength2), value1, attrLength1);
        memcpy(block2+moved*(attrLength1+attrLength2)+attrLength1, value2, attrLength2);
        if (BF_WriteBlock(file_id, block_id)<0){
            AM_errno = AME_CANNOT_WRITE_BLOCK;
            BF_PrintError("Error in AM_SplitData \n");
            return AM_errno;
        }
        if (BF_WriteBlock(file_id, new_block)<0){
            AM_errno = AME_CANNOT_WRITE_BLOCK;
            BF_PrintError("Error in AM_SplitData \n");
            return AM_errno;
        }
        Block_Sort(file_id, new_block);
        /* Fix counts in both blocks */
        BF_ReadBlock(file_id, block_id, &block1);
        BF_ReadBlock(file_id, new_block, &block2);
        new_count = moved + 1;
        memcpy(block2+sizeof(char)+2*sizeof(int), &new_count, sizeof(int));
        old_count = max - moved;
        memcpy(block1+sizeof(char)+2*sizeof(int), &old_count, sizeof(int));
    }
    else{/* if(!AM_Compare(file_id, value1, mid, EQUAL)*/
        left = middle - start + 1;
        right = duplicates - left;
        /* New record goes to the side of the duplicates */
        if(right>=left){ /*Move to the right*/
            for(i=start; i<=max; i++){
                memswap(file_id, block1+(i-1)*(attrLength1+attrLength2), block2+moved*(attrLength1+attrLength2), 'd');
                moved++;
            }           
            /* New record to 2nd block */
            memcpy(block2+moved*(attrLength1+attrLength2), value1, attrLength1);
            memcpy(block2+moved*(attrLength1+attrLength2)+attrLength1, value2, attrLength2);
            if (BF_WriteBlock(file_id, block_id)<0){
                AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_SplitData \n");
                return AM_errno;
            }
            if (BF_WriteBlock(file_id, new_block)<0){
                AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_SplitData \n");
                return AM_errno;
            }
            Block_Sort(file_id, new_block);
            /* Fix counts in both blocks */
            BF_ReadBlock(file_id, block_id, &block1);
            BF_ReadBlock(file_id, new_block, &block2);
            new_count = moved + 1;
            memcpy(block2+sizeof(char)+2*sizeof(int), &new_count, sizeof(int));
            old_count = max - moved;
            memcpy(block1+sizeof(char)+2*sizeof(int), &old_count, sizeof(int));
        }
        else{
            for(i=start+duplicates; i<=max; i++){
                memswap(file_id, block1+(i-1)*(attrLength1+attrLength2), block2+moved*(attrLength1+attrLength2), 'd');
                moved++;
            }       
            /* New record to 1st block */
            memcpy(block1+(start+duplicates-1)*(attrLength1+attrLength2), value1, attrLength1);
            memcpy(block1+(start+duplicates-1)*(attrLength1+attrLength2)+attrLength1, value2, attrLength2);
            if (BF_WriteBlock(file_id, block_id)<0){
                AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_SplitData \n");
                return AM_errno;
            }
            if (BF_WriteBlock(file_id, new_block)<0){
                AM_errno = AME_CANNOT_WRITE_BLOCK;
                BF_PrintError("Error in AM_SplitData \n");
                return AM_errno;
            }
            Block_Sort(file_id, block_id);
            /* Fix counts in both blocks */
            BF_ReadBlock(file_id, block_id, &block1);
            BF_ReadBlock(file_id, new_block, &block2);
            new_count = moved;
            memcpy(block2+sizeof(char)+2*sizeof(int), &new_count, sizeof(int));
            old_count = max - moved + 1;
            memcpy(block1+sizeof(char)+2*sizeof(int), &old_count, sizeof(int));     
        }
    }
    /* Return id of new block*/
    *did = new_block;
    /* Return first (minimum) key of the new block */
    if (BF_ReadBlock(file_id, 0, &block) < 0) {
        AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in AM_SplitData \n");
        return AM_errno;
    }
    if (BF_ReadBlock(file_id, new_block, &block2) < 0) {
        AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in AM_SplitData \n");
        return AM_errno;
    }
    if (BF_WriteBlock(file_id, block_id)<0){
        AM_errno = AME_CANNOT_WRITE_BLOCK;
        BF_PrintError("Error in AM_SplitData \n");
        return AM_errno;
    }
    if (BF_WriteBlock(file_id, new_block)<0){
        AM_errno = AME_CANNOT_WRITE_BLOCK;
        BF_PrintError("Error in AM_SplitData \n");
        return AM_errno;
    }
    free(mid);
    memcpy(&in, block, sizeof(Info));
    /* Return the address of new block's first record */
    return block2+sizeof(char)+3*sizeof(int);
}

int FindDuplicates(int file_id, int block_id, void *position, int length1, int length2){
/* Returns error code or number of duplicates (starting from "position") - If there are none, returns 1 */
	void *block;
	void *value = malloc(length1);
	void *next = malloc(length1);
	int count=1;
	if(BF_ReadBlock(file_id, block_id, &block)<0){
		AM_errno = AME_CANNOT_READ_BLOCK;
		BF_PrintError("Error in FindDuplicates: \n");
		return AM_errno; 
	}
	memcpy(value, position, length1);
	memcpy(next, position+length1+length2, length1);
	while(!AM_Compare(file_id, (void*)value, (void*)next, EQUAL)){
		count++;
		position+=length1+length2;
		memcpy(next, position+length1+length2, length1);
	}
	free(value);
	free(next);
	return count;
}


void memswap(int file_id,void *mem1,void *mem2, char block_type){
    void *block,*mem3,*mem4,*temp1,*temp2;
    Info in;
    if(BF_ReadBlock(file_id,0,&block)){
        AM_errno = AME_CANNOT_READ_BLOCK;
        BF_PrintError("Error in memswap \n");
    }
    memcpy(&in,block,sizeof(Info));
    temp1 = malloc(in.attrLength1);
    if (block_type == 'd') {
        temp2 = malloc(in.attrLength2);
        mem3 = mem1 + in.attrLength1;
        mem4 = mem2 + in.attrLength1;
        memcpy(temp1,mem1,in.attrLength1);
        memcpy(mem1,mem2,in.attrLength1);
        memcpy(mem2,temp1,in.attrLength1);
        memcpy(temp2,mem3,in.attrLength2);
        memcpy(mem3,mem4,in.attrLength2);
        memcpy(mem4,temp2,in.attrLength2);
        free(temp1);
	    free(temp2);
    }
    else {
        temp2 = malloc(sizeof(int)) ;
        mem3 = mem1 + in.attrLength1;
        mem4 = mem2 + in.attrLength1;
        memcpy(temp1,mem1,in.attrLength1);
        memcpy(mem1,mem2,in.attrLength1);
        memcpy(mem2,temp1,in.attrLength1);
        memcpy(temp2,mem3,sizeof(int));
        memcpy(mem3,mem4,sizeof(int));
        memcpy(mem4,temp2,sizeof(int));
        free(temp1);
	    free(temp2);
    }
}