#ifndef AM_H_
#define AM_H_

/* Error codes */
#include "stack.h" 

int AM_errno;

#define MAXOPENFILES 20
#define MAXSCANS 20

#define AME_OK 0
#define AME_EOF -1
#define AME_FILE_EXISTED -2
#define AME_INDEX_NOT_CREATED -3
#define AME_OPEN_FILE -4
#define AME_NOT_REMOVED -5
#define AME_INDEX_NOT_OPENED -6
#define AME_MAX_OPEN -7
#define AME_OPEN_SCAN -8
#define AME_INDEX_NOT_CLOSED -9
#define AME_MAX_SCAN -10
#define AME_NULL_INDEX -11
#define AME_BLOCK_NOT_CREATED -12
#define AME_CANNOT_READ_BLOCK -13
#define AME_CANNOT_WRITE_BLOCK -14
#define AME_CANNOT_SPLIT_DATABLOCK -15
#define AME_CANNOT_SPLIT_INDEXBLOCK -16
#define AME_CANNOT_INSERT_INDEX -17
#define AME_SEARCH_FAILED -18
#define AME_FIRSTNOTEQUAL_FAILED -19
#define AME_FIRSTDBLOCK_FAILED -20


#define EQUAL 1
#define NOT_EQUAL 2
#define LESS_THAN 3
#define GREATER_THAN 4
#define LESS_THAN_OR_EQUAL 5
#define GREATER_THAN_OR_EQUAL 6

typedef struct ScanCell{
  int id;           /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
  int num_block;    /* Arithmos block pio prosfatis eggrafis pou ikanopoiei sinthiki op */
  int num_record;   /* Thesi eggrafis sto block num_block */
  int op;           /* Telestis sigkrisis*/
  void *value;      /* I timi-kleidi pros anazitisi*/
}ScanCell;

typedef struct Info{  
  int id;
  char fileName[100];
  char isBtree;      /* 1 means B+ Tree */
  int recs;          /* Number of records in the file */
  char attrType1;    /* Type of first field: 'c' (string), 'i' (integer), 'f' (real) */
  int attrLength1;   /* Length of first field: 4 for 'i' or 'f', 1-255 for 'c' */
  char attrType2;    /* Type of second field: 'c' (string), 'i' (integer), 'f' (real) */
  int attrLength2;   /* Length of second field: 4 for 'i' or 'f', 1-255 for 'c' */
  int root;          /* Number of the root-block */
}Info;

Info *Open[MAXOPENFILES];
ScanCell *Scan[MAXSCANS];

void AM_Init( void );


int AM_CreateIndex(
  char *fileName, /* όνομα αρχείου */
  char attrType1, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength1, /* μήκος πρώτου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
  char attrType2, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength2 /* μήκος δεύτερου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
);


int AM_DestroyIndex(
  char *fileName /* όνομα αρχείου */
);


int AM_OpenIndex (
  char *fileName /* όνομα αρχείου */
);


int AM_CloseIndex (
  int fileDesc /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
);


int AM_InsertEntry(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  void *value1, /* τιμή του πεδίου-κλειδιού προς εισαγωγή */
  void *value2 /* τιμή του δεύτερου πεδίου της εγγραφής προς εισαγωγή */
);


int AM_OpenIndexScan(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  int op, /* τελεστής σύγκρισης */
  void *value /* τιμή του πεδίου-κλειδιού προς σύγκριση */
);


void* AM_FindNextEntry(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


int AM_CloseIndexScan(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


void AM_PrintError(
  char *errString /* κείμενο για εκτύπωση */
);



/*
 * I sunartisi AM_Compare sugkrinei tis times val, val1 
 * analoga me ton telesti op.
 * An i sugkrisi auti isxuei i sunartisi epistrefei 0, allios -1.
 * Se periptosi sfalmatos epistrefei 2.
 */
int AM_Compare(
  int file_id ,  /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
  void *val,     /* Ι proti timi pros sugkrisi */
  void *val1,    /* Ι deuteri timi pros sugkrsi */
  int op         /* Ο telestis sugkrisis */
);


/*
 * I sunartisi Block_Sort taksinomei to block me id block_num, 
 * topothetontas ta pedia-kleidia kata auksousa seira.
 * Se periptosi epituxias epistrefei AME_OK.
 * Se periptosi sfalmatos epistrefetai o antistoixos kodikos.
 */
int Block_Sort(
  int file_id,    /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
  int block_num   /* O arithmos pou antistoixei sto block (id) */
);

/*
 * Briskei tin proti eggrafi pou den einai idia me to val ksekinontas apo to 1o data block
 * Prospernaei ta diplotipa 
 * Epistrefei id tou block pou vrisketai i eggrafi / error code
 */ 
int AM_FirstNotEqual(
  int fileDesc,     /* O arithmos pou antistoixei sto anoikto arxeio */
  void * val        
);


/*
 * I sunartisi FirstDBlock entopizei kai epistrefei  
 * ton arithmo (id) tou protou data block tou dentrou.
 * Se periptosi sfalmatos epistrefetai o antistoixos kodikos.
 */
int FirstDBlock(
  int fileDesc    /* O arithmos pou antistoixei sto anoikto arxeio */
);


/*
 * I sunartisi Search anazita mesa sto dentro to block sto opoio 
 * vrisketai i timi value kai epistrefei ton arithmo tou (id). 
 * Otan kaleitai sti sunartisi InsertEntry o arithmos tou block pou
 * epistrefetai einai autos sto opoio kanonika i timi value prepei
 * na eisaxthei.
 * Se periptosi sfalmatos epistrefetai o antistoixos kodikos.
 */
int Search(
  int file_id,     /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
  int block_num,   /* O arithmos pou antistoixei sto block (id) */
  void * value,    /* I timi-kleidi pros anazitisi */
  int length,      /* To megethos tis timis*/
  Stack *s         /* I stoiva stin opoia tha kratithei i diadromi anamesa sta index block
                    * Se periptosi pou den theloume na tin xrisimopoiisoume dinoume os orisma NULL 
                    */
);


/*
 * I sunartisi SearchDataBlock anazita mesa sto block to prwto
 * pedio-kleidi gia to opoio isxuei i sugkrisi pou orizei o
 * telestis op me to pedio-kleidi value.
 * Se periptosi epituxias epistrefetai o arithmos pou antistoixei
 * stin eggrafi auti.
 * Se periptosi sfalmatos epistrefetai o antistoixos kodikos.
 */
int SearchDataBlock(
  int file_id,     /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
  int block_num,   /* O arithmos pou antistoixei sto block (id) */
  void * value,    /* I timi-kleidi pros anazitisi */
  int op           /* Ο telestis sugkrisis */
);


/*
 * I sunartisi AM_CreateBlock dimiourgei sto arxeio (me file_id) ena block
 * eite tupou index eite tupou data, analoga me to orisma pou exei
 * dothei. Epistrefei to id tou neou block h se periptosi sfalmatos
 * ton antistoixo kodiko.
 */
int AM_CreateBlock(
  char type,      /* O tupos tou block pou theloume na dimiourgisoume : 'd' gia data block, 'i' gia index block */
  int file_id    /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
);


/*
 * I sunartisi AM_NumberOfRecords vriskei kai epistrefei 
 * ton arithmo ton eggrafon pou xwroun mesa se ena block tupou data.
 * Se periptosi sfalmatos epistrefei ton antistoixo kodiko.
 */
int AM_NumberOfRecords(
  int file_id      /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
);


/*
 * I sunartisi AM_NumberOfKeys vriskei kai epistrefei ton arithmo 
 * ton zeugarion pedio-kleidi kai 'deiktis' pou xoroun mesa se ena 
 * block tupou index.
 * Se periptosi sfalmatos epistrefei ton antistoixo kodiko.
 */
int AM_NumberOfKeys(
  int file_id     /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
);


int AM_FindMiddle(
  int file_id,   /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
  int block_id    /* O arithmos pou antistoixei sto block (id) */
);

/*
 * I sunartisi AM_Insert_Index topothetei sto antistoixo index block 
 * to zeugari timi-'deiktis' pou exei prokupsei apo tin AM_SplitIndex.
 * An den xwraei se auto, kaleitai anadromika.
 * An xreiastei dimiourgei ek neou ena index block - riza.
 * Se periptosi epituxias epistrefei AME_OK.
 * Se periptosi sfalmatos epistrefei ton antistoixo kodiko.
 */
int AM_Insert_Index(
  int fileDesc,  /* O arithmos pou antistoixei sto anoikto arxeio */
  Stack *s,      /* I stoiva stin opoia tha kratithei i diadromi anamesa sta index block */ 
  int did,       /* O 'deiktis'-zeugari tis timis-kleidi */
  void *min      /* I timi-kleidi pou theloume na eisaxthei */ 
);


/*
 * I sunartisi AM_SplitIndex kanei to split otan exoume index block 
 * (den xoroun ta nea stoixeia),dimiourgontas to neo block 
 * kai metakinontas se auto ta antistoixa stoixeia,
 * exontas to zeugari timi-'deiktis' pou exei prokupsei apo tin AM_SplitData.
 * Epistrefei tin dieuthinsi tis timis-kleidi pou tha perasei stin AM_Insert_Index.
 * Se periptosi sfalmatos epistrefei ton antistoixo kodiko.
 */
int AM_SplitIndex(
  int file_id,      /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
  int block_id,     /* O arithmos pou antistoixei sto block (id) */
  void *newkey,     /* I timi-kleidi pou theloume na eisaxthei */
  int newpointer,   /* O 'deiktis'-zeugari tis timis-kleidi */
  int attrLength1,  /* To megethos tis timis-kleidi*/
  int *i_id         /* To id tou neou index block */
);


/*
 * I sunartisi AM_SplitData kanei to split otan exoume data block, 
 * (den xora i nea eggrafi), dimiourgontas to neo block kai 
 * metakinontas se auto ta antistoixa stoixeia.
 * Epistrefei ti dieuthinsi tis protis eggrafis tou neou block.
 * Se periptosi sfalmatos epistrefei ton antistoixo kodiko.
 */
int AM_SplitData(
  int file_id,     /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
  int block_id,    /* O arithmos pou antistoixei sto block (id) */
  void *value1,    /* I timi-kleidi pou theloume na eisaxthei */
  void *value2,    /* I timi tou deuterou pediou tis eggrafis */
  int attrLength1, /* To megethos tou protou pediou */
  int attrLength2, /* To megethos tou deuterou pediou */
  int *did         /* /* To id tou neou data block */
);

/*
 * Briskei ta diplotipa pou ksekinane apo to "position", an i eggrafi tis thesis position einai monadiki
 * epistrefei 1 alliws error code
 */
int FindDuplicates(
  int file_id,      /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
  int block_id,     /* O arithmos pou antistoixei sto block (id) */
  void *position,   /* I thesi tis eggrafis pou tha eleksoume gia diplotupa */
  int length1,      /* To megethos tou protou pediou tis eggrafis */
  int length2       /* To megethos tou deuterou pediou tis eggrafis */
);


/*
 * Antallassei 2 records pou vriskontai stis theseis mem1, mem2
 * Xrisimopoiei to block_type gia na kathorisei sosta offsets kata ti metafora
 */
void memswap(
  int file_id,     /* Anangoristikos arithmos anoigmatos arxeiou epipedou block */
  void *mem1,      /* I thesi tis 1is eggrafis pou tha metaferthei*/
  void *mem2,      /* I thesi tis 2is eggrafis pou tha metaferthei*/
  char block_type  /* O tipos tou block pou tha ginei metafora*/
);


#endif /* AM_H_ */
