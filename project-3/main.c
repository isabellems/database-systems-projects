#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "column_store.h"

int main(int argc, char *argv[]){
	int i, j, val, n;
	char c;
	FILE *inputFile;
	char dir_path[100], str[200], value[25];
	char *fieldnames[FIELDS];
	char *tok;
	char **printFields;
	HeaderInfo* header_info = malloc(sizeof(HeaderInfo));
	Record rec;
	if (argc != 2){
		fprintf(stderr, "Wrong number of command line arguments. Please give name of input file.\n");
		exit(EXIT_FAILURE);
	}
	if ((inputFile = fopen(argv[1],"r"))==NULL){
		fprintf(stderr, "Cannot Open Input File\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stdout, "Please enter a directory path.\n");
	scanf("%99s", dir_path);
	for(i=0 ; i<FIELDS ; i++)
		fieldnames[i] = malloc(50*sizeof(char));
	strcpy(fieldnames[0], "id");
	strcpy(fieldnames[1], "name");
	strcpy(fieldnames[2], "surname");
	strcpy(fieldnames[3], "status");
	strcpy(fieldnames[4], "dateOfBirth");
	strcpy(fieldnames[5], "salary");
	CS_Init();
	if (CS_CreateFiles(fieldnames, dir_path) < 0){
		fprintf(stderr, "Error Creating Files\n");
		exit(EXIT_FAILURE);
	}
	if (CS_OpenFile(header_info, dir_path) < 0){
		fprintf(stderr, "Error Opening Files\n");
		exit(EXIT_FAILURE);
	}
	while(fgets(str,200,inputFile) != NULL){
		i = 0;
		tok = strtok(str,",\"");
  		rec.id = atoi(tok);
      	for(i=1 ; i<=5 ; i++){
      		tok = strtok(NULL,",\"");
      		/* Assigning each string to the right record member */
      		if(i==1)
          		strcpy(rec.name,tok);
        	else if(i==2)
          		strcpy(rec.surname,tok);
        	else if(i==3)
          		strcpy(rec.status,tok);
        	else if(i==4)
        		strcpy(rec.dateOfBirth,tok);
        	else if(i==5)
        		rec.salary=atoi(tok);
		}
		if(CS_InsertEntry(header_info, rec) < 0){
      		fprintf(stderr, "Could not insert entries.\n");
      		CS_CloseFile(header_info);
			exit(EXIT_FAILURE);
      	}
    }
    do {
		fprintf(stdout, "\nSelect 1 to search for ID, 2 for Name, 3 for Surname, 4 for Status, 5 for Date of Birth, 6 for salary, 7 for all fields, 0 to quit\n");
		fscanf(stdin, "%d", &i);
		/* Clear stdin from newline char */
	    while ((c = getchar()) != '\n');
		if (i==0)
			break;
		else{
			/* Create printFields array */
			fprintf(stdout, "Please, give the number of fields you want print.\n");
			fscanf(stdin, "%d", &n);
			printFields = malloc(n * sizeof(char *));
			fprintf(stdout, "Please, give the fieldnames you want to print (Seperate them with spaces or new line).\n");
			for (j=0; j<n; j++){
				printFields[j] = malloc (25*sizeof(char));
				fscanf(stdin, "%24s", printFields[j]);
			}
			/* Clear stdin from newline char */
		    	while ((c = getchar()) != '\n');
			if (i==7)
	 			CS_GetAllEntries(header_info, NULL, NULL, printFields, n);
			if (i<0 || i>7) {
				fprintf(stdout, "Try again with a number between 0-5\n");
				exit(EXIT_FAILURE);
			}
			if(i==1){
				fprintf(stdout, "Please give value for the selected field\n");
				fscanf(stdin, "%d", &val);
				fprintf(stdout, "\n");
				CS_GetAllEntries(header_info, "id", (void *)val, printFields, n);
			}
			else if(i==2){
				fprintf(stdout, "Please give value for the selected field\n");
				fscanf(stdin, "%24s", value);
				fprintf(stdout, "\n");
				CS_GetAllEntries(header_info, "name", (void *)value, printFields, n);
			}
			else if(i==3){
				fprintf(stdout, "Please give value for the selected field\n");
				fscanf(stdin, "%24s", value);
				fprintf(stdout, "\n");
				CS_GetAllEntries(header_info, "surname", (void *)value, printFields, n);
			}
			else if(i==4){
				fprintf(stdout, "Please give value for the selected field\n");
				fscanf(stdin, "%24s", value);
				fprintf(stdout, "\n");
				CS_GetAllEntries(header_info, "status", (void *)value, printFields, n);
			}
			else if(i==5){
				fprintf(stdout, "Please give value for the selected field\n");
				fscanf(stdin, "%24s", value);
				fprintf(stdout, "\n");
				CS_GetAllEntries(header_info, "dateOfBirth", (void *)value, printFields, n);
			}
			else if(i==6){
				fprintf(stdout, "Please give value for the selected field\n");
				fscanf(stdin, "%d", &val);
				fprintf(stdout, "\n");
				CS_GetAllEntries(header_info, "salary", (void *)val, printFields, n);
			}
			for (j=0; j<n; j++)
				free(printFields[j]);
			free(printFields);
		}
	} while (i);
	if (CS_CloseFile(header_info) < 0){
		fprintf(stderr, "Error Closing Files\n");
		exit(EXIT_FAILURE);
	}
	fclose(inputFile);
	return 0;
}
