#include "common.h"
#include "utils.h"
void display_pread(paddr_t addr, int len) {
    log_write("[mtrace] "
              "pread at" FMT_PADDR " len=%d\n",
              addr, len);
}

void display_pwrite(paddr_t addr, int len, word_t data) {
    log_write("[mtrace] "
              "pwrite at" FMT_PADDR " len=%d, data=" FMT_PADDR "\n",
              addr, len, data);
}
