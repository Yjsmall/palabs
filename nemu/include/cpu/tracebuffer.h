#include <stdio.h>
#include <stdlib.h>

char *buffer[16];
int   cur  = -1;
int   size = 0;

void add_inst(char *inst) {
    if (size >= 16) {
        free(buffer[cur]);
        buffer[cur] = NULL; // Set to NULL after freeing
        buffer[cur] = inst;
        cur         = (cur + 1) % 16;
    } else {
        cur         = (cur + 1) % 16;
        buffer[cur] = inst;
    }

    if (size < 16) {
        size++;
    }
}

void print_buffer() {
    printf("instructions \n");
    for (int i = 0; i < 16; i++) {
        if (buffer[i] != NULL) {
            printf("%s\n", buffer[i]);
        }
    }
}

void destory_buffer() {
    for (int i = 0; i < 16; i++) {
        if (buffer[i] != NULL) {
            free(buffer[i]);
            buffer[i] = NULL; // Set to NULL after freeing
        }
    }
    cur = -1;
}
