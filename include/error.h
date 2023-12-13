
#include <stdio.h>
#include <stdlib.h>

inline void ptia_panic(char* msg) {
  printf("%s", msg);
  exit(EXIT_FAILURE);
}
