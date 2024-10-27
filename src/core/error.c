#include "error.h"

void ptia_panic(const char* msg) {
    printf("%s", msg);
    exit(EXIT_FAILURE);
}