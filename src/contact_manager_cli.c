#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "config.h"
#include "database.h"

char* commands[] = {"add", "get", "del", "list", "help", "exit", NULL};

char* command_generator(const char* text, int state) {
    static int list_index, len;
    char* name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = commands[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL;
}

char** command_completion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_generator);
}

void print_contact(Contact* contact) {
    if (contact) {
        printf("  Name:  %s\n", contact->name);
        printf("  Phone: %s\n", contact->phone);
        printf("  Email: %s\n", contact->email);
    }
}

void handle_command(char* line, Database* db) {
    if (line == NULL) {
        return;
    }
    add_history(line);
    char* command = strtok(line, " \n");
    if (command == NULL) {
        return;
    }

    if (strcmp(command, "add") == 0) {
        char* name = strtok(NULL, " \n");
        char* phone = strtok(NULL, " \n");
        char* email = strtok(NULL, " \n");
        if (name && phone && email) {
            Contact* contact = malloc(sizeof(Contact));
            contact->name = strdup(name);
            contact->phone = strdup(phone);
            contact->email = strdup(email);
            database_add_contact(db, contact);
            printf("Contact added.\n");
        } else {
            printf("Usage: add <name> <phone> <email>\n");
        }
    } else if (strcmp(command, "get") == 0) {
        char* name = strtok(NULL, " \n");
        if (name) {
            Contact* contact = database_get_contact(db, name);
            if (contact) {
                print_contact(contact);
            } else {
                printf("Contact not found.\n");
            }
        } else {
            printf("Usage: get <name>\n");
        }
    } else if (strcmp(command, "del") == 0) {
        char* name = strtok(NULL, " \n");
        if (name) {
            if (database_del_contact(db, name)) {
                printf("Contact deleted.\n");
            } else {
                printf("Contact not found.\n");
            }
        } else {
            printf("Usage: del <name>\n");
        }
    } else if (strcmp(command, "list") == 0) {
        int count;
        Contact** contacts = database_list_contacts(db, &count);
        for (int i = 0; i < count; i++) {
            printf("Contact #%d:\n", i + 1);
            print_contact(contacts[i]);
            printf("\n");
        }
        if (count == 0) {
            printf("No contacts found.\n");
        }
    } else if (strcmp(command, "help") == 0) {
        printf("Available commands:\n");
        printf("  add <name> <phone> <email> - Add a new contact\n");
        printf("  get <name>                  - Get a contact by name\n");
        printf("  del <name>                  - Delete a contact by name\n");
        printf("  list                        - List all contacts\n");
        printf("  exit                        - Exit the program\n");
    } else if (strcmp(command, "exit") == 0) {
        free(line);
        database_close(db);
        exit(0);
    } else {
        printf("Unknown command. Type 'help' for a list of commands.\n");
    }

    free(line);
}

int main(int argc, char* argv[]) {\
    Config config;\
    config_load("contact_manager_gtk.conf", &config);\
\
    Database* db = database_new(config.db_filename);\
    if (db == NULL) {\
        return 1;
    }

    rl_attempted_completion_function = command_completion;

    char* line;
    while ((line = readline("> ")) != NULL) {
        handle_command(line, db);
    }

    database_close(db);
    return 0;
}