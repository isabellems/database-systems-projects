#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "BF.h"
#include "column_store.h"


void CS_Init(){
	BF_Init();
}

int CS_CreateFiles(char **fieldNames, char *dbDirectory) {
	int i;
	DIR* dir = opendir(dbDirectory);
  	if (dir){
  		closedir(dir);
  		fprintf(stderr, "Directory already exists\n");
  		return -1;
    }
  	else{
	    char path[100];
	    ColumnFds *cfd = malloc(FIELDS * sizeof(ColumnFds));
	    FILE *csfile;
	    int desc;
	    void *block;
	    Info in;
	    mkdir(dbDirectory, 0777);
	    for(i=0 ; i < FIELDS ; i++){
        /* creating the file's path */
            sprintf(path, "%s/CSFile_%s" , dbDirectory , fieldNames[i]);
            /* check if files already exist */
            csfile = fopen(path,"r");
            if(csfile){ 
                fprintf(stderr, "File already exists.\n");
                fclose(csfile);
                return -1;
            }
            /* file doesn't exist */
            if(BF_CreateFile(path) < 0){
                BF_PrintError("Cannot create file.\n");
                return -1;
            }
            if((desc = BF_OpenFile(path)) < 0){
                BF_PrintError("Cannot open file.\n");
                return -1;
            }
            /* creating info block */
            if(BF_AllocateBlock(desc) < 0){
                BF_PrintError("Cannot allocate block.\n");
                BF_CloseFile(desc);
                return -1;
            }
            if(BF_ReadBlock(desc, 0, &block) < 0){
                BF_PrintError("Cannot read block.\n");
                return -1;
            }
            in.isCSfile = '1';
            in.recs = 0;
            if (strcmp(fieldNames[i],"id") == 0) {
                in.length = ID;
            }
            else if (strcmp(fieldNames[i],"name") == 0) {
                in.length = NAME;
            }
            else if (strcmp(fieldNames[i],"surname") == 0) {
                in.length = SURNAME;
            }
            else if (strcmp(fieldNames[i],"status") == 0) {
                in.length = STATUS;
            }
            else if (strcmp(fieldNames[i],"dateOfBirth") == 0) {
                in.length = DATE;
            }
            else if (strcmp(fieldNames[i],"salary") == 0) {
                in.length = SALARY;
            }
            memcpy(block,&in,sizeof(Info));
            if(BF_WriteBlock(desc,0) < 0){
                BF_PrintError("Cannot write block.\n");
                BF_CloseFile(desc);
                return -1;
            }
            if(BF_CloseFile(desc) < 0){
                BF_PrintError("Cannot close file.\n");
                return -1;
            }
            cfd[i].columnName = malloc(25*sizeof(char));
            sprintf(cfd[i].columnName,"CSFile_%s",fieldNames[i]);
            cfd[i].fd = -1;
        }
	    /* creating Header_Info file */
	    sprintf(path, "%s/Header_Info" , dbDirectory);
	    csfile = fopen(path,"r");
	    if(csfile){
            fprintf(stderr, "File already exists.\n");
            fclose(csfile);
            return -1;
	    }
	    if(BF_CreateFile(path) < 0){
            BF_PrintError("Cannot create file.\n");
            return -1;
	    }
	    if((desc = BF_OpenFile(path)) < 0){
            BF_PrintError("Cannot open file.\n");
            return -1;
	    }
	    if(BF_AllocateBlock(desc) < 0){
            BF_PrintError("Cannot allocate block.\n");
            BF_CloseFile(desc);
            return -1;
	    }
	    if(BF_ReadBlock(desc, 0, &block) < 0){
            BF_PrintError("Cannot read block.\n");
            return -1;
	    }
	    HeaderInfo hinfo;
	    hinfo.column_fds = malloc(sizeof(ColumnFds)* FIELDS);
	    hinfo.column_fds = cfd;
	    memcpy(block,&hinfo,sizeof(HeaderInfo));
	    if(BF_WriteBlock(desc,0) < 0){
            BF_PrintError("Cannot write block.\n");
            BF_CloseFile(desc);
            return -1;
	    }
	    if(BF_CloseFile(desc) < 0){
            BF_PrintError("Cannot close file.\n");
            return -1;
	    }
  	}
	return 0;
}


int CS_OpenFile(HeaderInfo* header_info, char *dbDirectory) {
	char path[100],filename[50];
	int desc,desc1,i;
	void *block,*block1; 
	Info finfo;
	sprintf(path, "%s/Header_Info" , dbDirectory); 
	if((desc = BF_OpenFile(path)) < 0){
		BF_PrintError("Cannot open file.\n");
		return -1;
	}
	if(BF_ReadBlock(desc, 0, &block) < 0){
		BF_PrintError("Cannot read block.\n");
		return -1;
	}
	/* Initialisation of head_info pointer */
	memcpy(header_info,block,sizeof(HeaderInfo));
	for(i=0 ; i < FIELDS ; i++){
		strcpy(filename,header_info->column_fds[i].columnName);
		sprintf(path, "%s/%s" , dbDirectory, filename);
		if((desc1 = BF_OpenFile(path)) < 0){
			BF_PrintError("Cannot open file.\n");
			return -1;
	    }
	    if(BF_ReadBlock(desc1, 0, &block1) < 0){
	      BF_PrintError("Cannot read block.\n");
	      return -1;
	    }
	    memcpy(&finfo,block1,sizeof(Info));
	    if(finfo.isCSfile!='1'){
	      fprintf(stderr, "This is not a CS file.\n");
	      return -1;
	    }
		header_info->column_fds[i].fd = desc1;
	}
	if(BF_CloseFile(desc) < 0){
		BF_PrintError("Cannot close file.\n");
		return -1;
	}
	return 0;
}


int CS_CloseFile(HeaderInfo* header_info) {
	int i,desc;
	for(i=0 ; i<FIELDS ; i++){
        desc = header_info->column_fds[i].fd;
        if(desc>0){
            if(BF_CloseFile(desc) < 0){
                BF_PrintError("Cannot close file.\n");
                return -1;
            }
            header_info->column_fds[i].fd = -1;  
        }
    }
    return 0;
}


int CS_InsertEntry(HeaderInfo* header_info, Record record) {
	if(InsertEntry(header_info->column_fds[0].fd, (void *) record.id) < 0){
	    fprintf(stderr, "Error in CS_InsertEntry\n");
      return -1;
  }
  if(InsertEntry(header_info->column_fds[1].fd, (void *) record.name) < 0){
     	fprintf(stderr, "Error in CS_InsertEntry\n");
      return -1;
  }
  if(InsertEntry(header_info->column_fds[2].fd, (void *) record.surname) < 0){
      fprintf(stderr, "Error in CS_InsertEntry\n");
     	return -1;
  }
  if(InsertEntry(header_info->column_fds[3].fd, (void *) record.status) < 0){
    	fprintf(stderr, "Error in CS_InsertEntry\n");
    	return -1;
  }
  if(InsertEntry(header_info->column_fds[4].fd, (void *) record.dateOfBirth) < 0){
      fprintf(stderr, "Error in CS_InsertEntry\n");
     	return -1;
  }
  if(InsertEntry(header_info->column_fds[5].fd, (void *) record.salary) < 0){
    	fprintf(stderr, "Error in CS_InsertEntry\n");
    	return -1;
  }
  return 0;
}


int InsertEntry(int fileDesc, void * value) {
	  void *block;
  	Info info;
  	int bc = BF_GetBlockCounter(fileDesc);
  	if (bc < 0) {
    	return -1;
  	}
  	int nrb = NumberOfRecords(fileDesc);    /* the number of fields that fit in a block */
  	if (BF_ReadBlock(fileDesc,0, &block) < 0) {
    	BF_PrintError("Error in ReadBlock (InsertEntry).\n");
    	return -1;
  	}
  	memcpy(&info,block,sizeof(Info));
  	if ((bc == 1) || ((info.recs % nrb) == 0)) {   /* there is only the info block or the block is full,allocate a new one */
    	if (BF_AllocateBlock(fileDesc) < 0) {
      		BF_PrintError("Error in AllocateBlock (InsertEntry).\n");
      		return -1;
    	}
    	if (BF_ReadBlock(fileDesc,bc, &block) < 0 ) {
    		BF_PrintError("Error in ReadBlock (InsertEntry).\n");
      		return -1;
    	}
    	if ((info.length == ID) || (info.length == SALARY)) {
      		int val = *(int *) (&value);
      		memcpy(block,&val,info.length);
    	}
    	else {
    		char v[25];
    		strcpy(v,(char*) value);
    		memcpy(block,&v,info.length);
    	}
    	if (BF_WriteBlock(fileDesc, bc) < 0) {
    		BF_PrintError("Error in WriteBlock (InsertEntry).\n");
    		return -1;
    	}
  	}
  	else {   /* we are in the same block */
    	if (BF_ReadBlock(fileDesc,bc-1, &block) < 0) {
      		BF_PrintError("Error in ReadBlock (InsertEntry).\n");
      		return -1;
    	}
    	if ((info.length == ID) || (info.length == SALARY)) {
      		int val = *(int *) (&value);
      		memcpy(block + (info.recs % nrb) * info.length,&val,info.length);
    	}
    	else {
      		char v[25];
      		strcpy(v,(char*) value);
      		memcpy(block + (info.recs % nrb) * info.length,&v,info.length);
    	}
    	if (BF_WriteBlock(fileDesc, bc-1) < 0) {
      		BF_PrintError("Error in WriteBlock (InsertEntry).\n");
      		return -1;
    	}
  	}
    /* change the counter of total records in file's info block */
  	if (BF_ReadBlock(fileDesc,0, &block) < 0) {
      	BF_PrintError("Error in ReadBlock (InsertEntry).\n");  
      	return -1;
  	}
  	memcpy(&info,block,sizeof(Info));
  	info.recs ++ ;  
  	memcpy(block,&info,sizeof(Info));
  	if (BF_WriteBlock(fileDesc, 0) < 0) {
      	BF_PrintError("Error in WriteBlock (InsertEntry).\n");
      	return -1;
  	} 
  	return 0;
}


void PrintEntry(int desc,int rec,char *field,int *nb){
  	Info in;
  	void *block,*cur;
  	int num,full,ent;
  	/* reading info block */
  	if(BF_ReadBlock(desc, 0, &block) < 0){ 
        BF_PrintError("Cannot read block.\n");
    	return ;
  	}
  	memcpy(&in,block,sizeof(Info));
  	num = NumberOfRecords(desc); /* number of fields that fit in a block */
  	full = rec/num;              /* number of full blocks before record i */
  	ent = rec%num;               /* number of entries in the last semi-filled block */
    if(ent!=0){                  /* i isn't the last record in its block */
        if(BF_ReadBlock(desc, full+1 , &block) < 0){   /* full+1 is the block that contains rec i */
            BF_PrintError("Cannot read block.\n");
            return ;
        }
        (*nb)++;
        cur = block + (ent-1)*in.length;
    }
    else{ /* i is the last record in its block */
        if(BF_ReadBlock(desc, full , &block) < 0){  /* full is the block that contains rec i */
            BF_PrintError("Cannot read block.\n");
            return ;
        }
        (*nb)++;
        cur = block + (num-1)*in.length;
    }
    if(!strcmp(field,"CSFile_salary") || !strcmp(field,"CSFile_id")){  /* field contains an integer */
        int val;
        memcpy(&val,cur,in.length);
        printf("%d ", val);
    }
  	else{  /* field contains a string */
        char buffer[25];
        memcpy(&buffer,cur,in.length);
        printf("%s ", buffer );
    }
}

void CS_GetAllEntries(HeaderInfo* header_info,char *fieldName,void *value,char **fieldNames,int n){
    int i,j,k,desc,num,full,ent,rec,recnum,nb;
	char name[50];
	Info in;
	void *block,*cur;
    /* reading the first CSfile to know how many records there are */
	if(BF_ReadBlock(header_info->column_fds[0].fd, 0, &block) < 0){ 
    	BF_PrintError("Cannot read block.\n");
    	return ;
 	}
    memcpy(&in,block,sizeof(Info));
    rec = in.recs;                   /* number of records in each file */
    if(fieldName==NULL && value==NULL){
    /* for each record open the files one by one to print the right field */
        for(i=1 ; i<=rec ; i++){
            for(j=0 ; j<FIELDS ; j++){
                for(k=0 ; k<n ; k++){      /* print as many fields as the user asked for */
                    char comp[25];
                    sprintf(comp,"CSFile_%s",fieldNames[k]);
                    /* read this file only if its name is the fieldNames array */
                    if(!strcmp(header_info->column_fds[j].columnName,comp))
                    PrintEntry(header_info->column_fds[j].fd,i,comp,&nb);
                }
            }
            printf("\n");
        }
        printf("Number of blocks that were read : %d\n",nb );
        return;
    }
    /* print specific records and fields depending on user's arguments */
    sprintf(name,"CSFile_%s",fieldName);
    /* getting the file_id of the file named 'fieldname' */
    for(i=0 ; i<FIELDS ; i++){ 
        if(!strcmp(name,header_info->column_fds[i].columnName)){
            desc = header_info->column_fds[i].fd;
            break;
        }
    }
    /* read info block */
    if(BF_ReadBlock(desc, 0, &block) < 0){ 
        BF_PrintError("Cannot read block.\n");
        return ;
    }
    memcpy(&in,block,sizeof(Info));
    num = NumberOfRecords(desc);      /* number of fields that fit in a block */
    /* check each record of the file to know if it matches user's value */
    for(i=1 ; i<=rec ; i++){ 
        full = i/num;      /* the number of full blocks before record i */
        ent = i%num;       /* the number of entries in the last semi-filled block */
        if(ent!=0){        /* i isn't the last record in its block */
            if(BF_ReadBlock(desc, full+1 , &block) < 0){    /* full+1 is the block that contains rec i */
                BF_PrintError("Cannot read block.\n");
                return ;
            }
            nb++;
        cur = block + (ent-1)*in.length;
        recnum = i;
        }
        else{    /* i is the last record in its block */
            if(BF_ReadBlock(desc, full , &block) < 0){  /* full is the block that contains rec i */
                BF_PrintError("Cannot read block.\n");
                return ;
            }
            nb++;
    	    cur = block + (num-1)*in.length;
    	    recnum = i;
        }
        if(!strcmp(name,"CSFile_salary") || !strcmp(name,"CSFile_id")){  /* field contains an integer */
            int val;
            int value_;
            value_ =*(int*)(&value);
            memcpy(&val,cur,in.length);
            if(val==value_){            /*  field matches value */
                for(j=0 ; j<6 ; j++){     /* search every file */
                    for(k=0 ; k<n ; k++){ /* print as many fields as the user asked for */
                        char comp[25];
                        sprintf(comp,"CSFile_%s",fieldNames[k]);
                        /* read this file only if its name is the fieldNames array */
                        if(!strcmp(header_info->column_fds[j].columnName,comp))
                            PrintEntry(header_info->column_fds[j].fd,recnum,comp,&nb);
                    }
                }
                printf("\n");
            }
        }
        else{   /* field contains a string */
            char val[25];
            char value_[25];
         	strcpy(value_,(char*)value);
         	memcpy(&val,cur,in.length);
            if(!strcmp(val,value_)){        /* field matches value */
                for(j=0 ; j<6 ; j++){   /* search every file */
          		    for(k=0 ; k<n ; k++){    /* print as many fields as the user asked for */
                        char comp[25];
                        sprintf(comp,"CSFile_%s",fieldNames[k]);
                        /* read this file only if its name is the fieldNames array */
                        if(!strcmp(header_info->column_fds[j].columnName,comp))
                            PrintEntry(header_info->column_fds[j].fd,recnum,comp,&nb);
          		    }
                }
                printf("\n");
            }
        }
    }
    printf("\nNumber of blocks that were read : %d\n",nb );
}
 
int NumberOfRecords(int file_id) {
	void *block;
	Info info ;
	int nrb ;
	if (BF_ReadBlock(file_id,0, &block) < 0) {
    	BF_PrintError("Error in NumberOfRecords, cannot read block. \n");
    	return -1; 
  	}
  	memcpy(&info,block, sizeof(Info));
  	nrb = BLOCK_SIZE / info.length;
  	return nrb ;
}
