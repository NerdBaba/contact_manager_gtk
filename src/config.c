#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

#define MAX_LINE_SIZE 1024

void config_load(char* filename, Config* config) {
    config->port = strdup("1234");
    config->logfile = strdup("contact_manager_gtk.log");
    config->db_filename = strdup("contact_manager_gtk.db");

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return;
    }

    char line[MAX_LINE_SIZE];
    while (fgets(line, sizeof(line), file) != NULL) {
        char* key = strtok(line, " \n");
        char* value = strtok(NULL, " \n");
        if (key != NULL && value != NULL) {
            if (strcmp(key, "port") == 0) {
                free(config->port);
                config->port = strdup(value);
            } else if (strcmp(key, "logfile") == 0) {
                free(config->logfile);
                config->logfile = strdup(value);
            } else if (strcmp(key, "db_filename") == 0) {
                free(config->db_filename);
                config->db_filename = strdup(value);
            }
        }
    }

    fclose(file);
}
