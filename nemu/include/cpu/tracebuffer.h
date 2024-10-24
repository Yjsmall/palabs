#include <stdio.h>
#include <stdlib.h>
char * buffer[16];
int cur = -1;
int size = 0;

void add_inst(char *inst) {
  if (size >= 16) {
    free(buffer[cur]);
  }
    cur = (cur + 1) % 16;
    buffer[cur] = inst;
    size++;
}

void print_buffer() {
    for (int i = 0; i < 16; i++) {
        printf("%s\n", buffer[i]);
    }
}

void destory_buffer() {
    for (int i = 0; i < 16; i++) {
      free(buffer[i]);
    }
    cur = -1;
}
