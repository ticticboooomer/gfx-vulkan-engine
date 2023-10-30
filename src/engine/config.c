#include "config.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct ConfigEntry ConfigEntry;

ConfigEntry *root;

void init_config() {}

void load_config(char *config_path) {
  FILE *fptr;

  fopen_s(&fptr, config_path, "rw");

  char ch;
  long sz;
  fseek(fptr, 0L, SEEK_END);
  sz = ftell(fptr);
  rewind(fptr);

  char *whole_doc = malloc(sz * sizeof(char));
  int cpos = 0;
  while (!feof(fptr)) {
    ch = fgetc(fptr);
    whole_doc[cpos] = ch;
    cpos++; 
  }

}
