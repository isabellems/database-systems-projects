#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "heap.h"

int main() {
	//Add your test code here
	int id,i; 
	Record rec;
  void *value;
	char c,filename[20],str[100],str1[20],str2[20],*tok;
  FILE *tty = fopen("/dev/tty", "r"); //Reading from keyboard
  if (tty==NULL) {
    printf("Error in reading from keyboard.\n");
    return -1;
  }
  printf("Type a file name.\n");
  fscanf(tty,"%s",filename);
  fclose(tty);
  BF_Init();
  if(HP_CreateFile(filename)==0)
    printf("File %s was successfully created.\n",filename);
  else{
    printf("File could not be created\n");
    return 0;
  }
  if(id=HP_OpenFile(filename)<0){
    printf("File could not be opened.\n");
    return 0;
  }
  else
    printf("File with id %d was opened.\n",id);
	while (1) {
    i=0;
    if (fgets(str,100,stdin) == NULL) break;
      tok = strtok(str,",\"");//Splitting string between commas and quotation marks
      rec.id = atoi(tok);
      while(tok!=NULL){
        if(i==1){
          strcpy(rec.name,tok);//Asigning each string to the right record member
        }
        else if(i==2){
          strcpy(rec.surname,tok);
        }
        else if(i==3){
          strcpy(rec.city,tok);
        }
        tok = strtok(NULL,",\"");
        i++;
      }
      if(HP_InsertEntry(id,rec)<0){//Inserting record into file
        printf("Could not insert entries.\n");
        HP_CloseFile(id);
        return -1;
      }
  }
  printf("Entries were successfully inserted.\n");
  if (!freopen("/dev/tty", "r", stdin)) {
    printf("Error in reading from keyboard.\n");//Stdin is no longer the given file but the keyboard
    return -1;
  }
  while(1){
    printf("Type a field name,or q to exit.\n");
    scanf("%s", str1);//Reads field name 
    if(!strcmp(str1,"q")) break;
    if(strcmp(str1,"all")!=0){//Reads a value
      printf("Type a value.\n");
      while ((c = getchar()) != '\n');//Clearing stdin from newline character
      fgets(str2,20,stdin);
      tok = strtok(str2,"\n"); // Stores string ignoring newline character
    }
    tok = str2;
    HP_GetAllEntries(id,str1,tok);
  } 
  HP_CloseFile(id);
}

