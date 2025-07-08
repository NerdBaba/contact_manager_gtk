#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "database.h"

static void database_load(Database* db) {
    FILE* file = fopen(db->filename, "r");
    if (file == NULL) {
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        char* name = strtok(line, ",");
        char* phone = strtok(NULL, ",");
        char* email = strtok(NULL, "\n");

        if (name && phone && email) {
            Contact* contact = malloc(sizeof(Contact));
            contact->name = strdup(name);
            contact->phone = strdup(phone);
            contact->email = strdup(email);
            database_add_contact(db, contact);
        }
    }
    fclose(file);
}

void database_save(Database* db) {
    FILE* file = fopen(db->filename, "w");
    if (file == NULL) {
        return;
    }

    for (int i = 0; i < db->count; i++) {
        fprintf(file, "%s,%s,%s\n", db->contacts[i]->name, db->contacts[i]->phone, db->contacts[i]->email);
    }
    fclose(file);
}

void database_import(Database* db, const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Error opening import file");
        return;
    }

    char line[1024];
    Contact* current_contact = NULL;

    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline or carriage return
        line[strcspn(line, "\r\n")] = 0;

        if (strcmp(line, "BEGIN:VCARD") == 0) {
            current_contact = malloc(sizeof(Contact));
            current_contact->name = NULL;
            current_contact->phone = NULL;
            current_contact->email = NULL;
        } else if (current_contact != NULL && strncmp(line, "FN:", 3) == 0) {
            if (current_contact->name) free(current_contact->name);
            current_contact->name = strdup(line + 3);
        } else if (current_contact != NULL && strncmp(line, "TEL:", 4) == 0) {
            if (current_contact->phone) free(current_contact->phone);
            current_contact->phone = strdup(line + 4);
        } else if (current_contact != NULL && strncmp(line, "EMAIL:", 6) == 0) {
            if (current_contact->email) free(current_contact->email);
            current_contact->email = strdup(line + 6);
        } else if (strcmp(line, "END:VCARD") == 0) {
            if (current_contact != NULL && current_contact->name != NULL) {
                // Ensure all fields are non-NULL before adding, use empty string if missing
                if (current_contact->phone == NULL) current_contact->phone = strdup("");
                if (current_contact->email == NULL) current_contact->email = strdup("");
                database_add_contact(db, current_contact);
            } else {
                // If contact is incomplete, free allocated memory
                if (current_contact->name) free(current_contact->name);
                if (current_contact->phone) free(current_contact->phone);
                if (current_contact->email) free(current_contact->email);
                free(current_contact);
            }
            current_contact = NULL;
        }
    }
    fclose(file);
    database_save(db); // Save after import
}

void database_export(Database* db, const char* filepath) {
    FILE* file = fopen(filepath, "w");
    if (file == NULL) {
        perror("Error opening export file");
        return;
    }

    for (int i = 0; i < db->count; i++) {
        fprintf(file, "%s,%s,%s\n", db->contacts[i]->name, db->contacts[i]->phone, db->contacts[i]->email);
    }
    fclose(file);
}

Database* database_new(const char* filename) {
    Database* db = malloc(sizeof(Database));
    db->filename = strdup(filename);
    db->count = 0;
    db->capacity = 10;
    db->contacts = malloc(sizeof(Contact*) * db->capacity);
    database_load(db);
    return db;
}

void database_close(Database* db) {
    database_save(db);
    for (int i = 0; i < db->count; i++) {
        free(db->contacts[i]->name);
        free(db->contacts[i]->phone);
        free(db->contacts[i]->email);
        free(db->contacts[i]);
    }
    free(db->contacts);
    free(db->filename);
    free(db);
}

int database_add_contact(Database* db, Contact* contact) {
    if (db->count == db->capacity) {
        db->capacity *= 2;
        db->contacts = realloc(db->contacts, sizeof(Contact*) * db->capacity);
    }
    db->contacts[db->count++] = contact;
    return 1;
}

Contact* database_get_contact(Database* db, const char* name) {
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->contacts[i]->name, name) == 0) {
            return db->contacts[i];
        }
    }
    return NULL;
}

int database_del_contact(Database* db, const char* name) {
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->contacts[i]->name, name) == 0) {
            free(db->contacts[i]->name);
            free(db->contacts[i]->phone);
            free(db->contacts[i]->email);
            free(db->contacts[i]);
            db->count--;
            if (i < db->count) {
                db->contacts[i] = db->contacts[db->count];
            }
            return 1;
        }
    }
    return 0;
}

Contact** database_list_contacts(Database* db, int* count) {
    *count = db->count;
    return db->contacts;
}