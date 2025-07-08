#ifndef DATABASE_H
#define DATABASE_H

typedef struct {
    char* name;
    char* phone;
    char* email;
} Contact;

typedef struct {
    Contact** contacts;
    int count;
    int capacity;
    char* filename;
} Database;

Database* database_new(const char* filename);
void database_close(Database* db);
void database_save(Database* db);
void database_import(Database* db, const char* filepath);
void database_export(Database* db, const char* filepath);
int database_add_contact(Database* db, Contact* contact);
Contact* database_get_contact(Database* db, const char* name);
int database_del_contact(Database* db, const char* name);
Contact** database_list_contacts(Database* db, int* count);

#endif
