/* the number of fields in the struct Record */
#define FIELDS 6   

/* the length of each field */
#define ID 4
#define NAME 15
#define SURNAME 20
#define STATUS 10
#define DATE 11
#define SALARY 4


typedef struct Record {
	int id;
	char name[15];
	char surname[20];
	char status[10];
	char dateOfBirth[11];
	int salary;
} Record;

typedef struct ColumnFds {
	char *columnName;
	int fd;
} ColumnFds;

typedef struct HeaderInfo {
	ColumnFds *column_fds;		// Πίνακας με τους FDs για όλα τα column files
} HeaderInfo;

typedef struct Info{
	char isCSfile;  /* '1' means it is a CSfile */
	int recs;       /* total number of records in the file */
	int length;     /*record's length in this file*/
}Info;

void CS_Init();
/*
 * Η συνάρτηση CS_CreateFiles χρησιμοποιείται για τη δημιουργία και 
 * κατάλληλη αρχικοποίηση άδειων αρχείων, ένα για κάθε πεδίο που 
 * προσδιορίζεται από τον πίνακα fieldNames (και πρόκειται ουσιαστικά 
 * για τα πεδία της δομής Record). Τα αρχεία θα δημιουργούνται στο 
 * φάκελο με μονοπάτι dbDirectory. Σε περίπτωση που εκτελεστεί 
 * επιτυχώς η συνάρτηση, επιστρέφεται 0, αλλιώς ‐1.
 */
int CS_CreateFiles(char **fieldNames, char *dbDirectory);

/*
 * Η συνάρτηση CS_OpenFile ανοίγει το αρχείο “header_info” που βρίσκεται 
 * στο dbDirectory και επιστρέφει στο struct HeaderInfo τον αναγνωριστικό 
 * αριθμό ανοίγματος κάθε αρχείου, όπως αυτός επιστράφηκε από το επίπεδο 
 * διαχείρισης μπλοκ. Η συνάρτηση επιστρέφει ‐1 σε περίπτωση σφάλματος 0 αλλιώς.
 */
int CS_OpenFile(HeaderInfo* header_info, char *dbDirectory);

/*
 * Η συνάρτηση CS_CloseFile βρίσκει τα fileDesc των αρχείων που είναι
 * ανοικτά και τα κλείνει. Σε περίπτωση που εκτελεστεί επιτυχώς, 
 * επιστρέφεται 0, ενώ σε διαφορετική περίπτωση ‐1.
 */
int CS_CloseFile(HeaderInfo* header_info);


/*
 * Η συνάρτηση CS_InsertEntry χρησιμοποιείται για την εισαγωγή μίας εγγραφής 
 * στα αρχεία column‐store. Η πληροφορία για τα αρχεία CS διαβάζεται από τη 
 * δομή HeaderInfo, ενώ η εγγραφή προς εισαγωγή προσδιορίζεται από τη δομή 
 * Record. Η τιμή του κάθε πεδίου προστίθεται στο τέλος του αντίστοιχου αρχείου, 
 * μετά την τρέχουσα τελευταία τιμή που υπήρχε. Σε περίπτωση που εκτελεστεί 
 * επιτυχώς, επιστρέφεται 0, ενώ σε διαφορετική περίπτωση ‐1.
 */
int CS_InsertEntry(HeaderInfo* header_info, Record record);

/*
 * Η συνάρτηση InsertEntry χρησιμοποιείται για την εισαγωγή ενός πεδίου
 * στο αντίστοιχο αρχείο column‐store. Δέχεται τον αναγνωριστικό αριθμό του
 * αρχείου αυτού (fileDesc )καθώς και τη τιμή του πεδίου που θέλουμε να
 * εισάγουμε (value). Σε περίπτωση που εκτελεστεί επιτυχώς επιστρέφεται 0,
 * ενώ σε διαφορετική περίπτωση ‐1.
 */
int InsertEntry(int fileDesc, void *value);

/*
 * Η συνάρτηση αυτή χρησιμοποιείται για την εκτύπωση όλων των εγγραφών οι οποίες
 * έχουν τιμή στο πεδίο με όνομα fieldName ίση με value. Οι αναγνωριστικοί αριθμοί
 * ανοίγματος αρχείων δίνονται στη δομή HeaderInfo.  Η  παράμετρος  fieldName  
 * μπορεί  να  πάρει  για  τιμή  κάποιο  από τα ονόματα των πεδίων της εγγραφής. 
 * Σε περίπτωση που η τιμή του fieldName και του value είναι ίση με NULL, να
 * εκτυπώνονται όλες οι εγγραφές. Να εκτυπώνεται επίσης το πλήθος των μπλοκ 
 * που διαβάστηκαν.
 */
void CS_GetAllEntries(HeaderInfo* header_info, 
                      char *fieldName, 
                      void *value,
                      char **fieldNames,
                      int n);
 

/*
 * Η συνάρτηση αυτή χρησιμοποιείται για την εκτύπωση της εγγραφής με αριθμό rec
 * του αρχείου με αναγνωριστικό αριθμό desc, ο τύπος της οποίας προσδιορίζεται
 * από το όρισμα field. Το όρισμα nb είναι ο μετρητής των μπλοκ που διαβάστηκαν 
 * και εμφανίζεται από τη CS_GetAllEntries.
 */
void PrintEntry(int desc,int rec,char *field,int *nb);


/*
 * Η συνάρτηση αυτή βρίσκει και επιστρέφει τον αριθμό των εγγραφών
 * που χωρούν σε κάθε μπλοκ του αρχείου με αναγνωριστικό αριθμό 
 * file_id. Σε περίπτωση σφάλματος επιστρέφει -1. 
 */
int NumberOfRecords(int file_id);