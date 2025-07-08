#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char* port;
    char* logfile;
    char* db_filename;
} Config;

void config_load(char* filename, Config* config);

#endif
