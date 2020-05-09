#include "9cc.h"

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}
