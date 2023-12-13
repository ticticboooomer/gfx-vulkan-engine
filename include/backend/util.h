#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
inline uint32_t clamp(uint32_t i, uint32_t min, uint32_t max) {
  const uint32_t t = i < min ? min : i;
  return t > max ? max : t;
}

typedef struct {
  char* data;
  size_t size;
} file_str_t;

inline file_str_t read_file(const char* fname) {
  printf("%s reading this file", fname);
  FILE* fd = fopen(fname, "rb");
  fseek(fd, 0, SEEK_END);
  const size_t sz = ftell(fd);
  rewind(fd);
  char* data = malloc(sizeof(char) * sz);
  
  size_t idx = 0;
  while (!feof(fd)) {
    data[idx] = fgetc(fd);
    idx++;
  }
  fclose(fd);
  file_str_t result;
  result.data = data;
  result.size = sz;
  return result;
}
