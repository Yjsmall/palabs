#include "common.h"
#include "debug.h"
#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
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

static Elf32_Ehdr eh;
ftrace_entry     *ftrace_tab        = NULL;
int               ftrace_table_size = 0;
int               call_depth        = 0;

static void parse_elf_header(int fd, const char *elf_file) {
    Assert(lseek(fd, 0, SEEK_SET) == 0, "elf_file: %s lseek error.", elf_file);
    Assert(read(fd, (void *)&eh, sizeof(Elf32_Ehdr)) == sizeof(Elf32_Ehdr), "elf_file: %s lseek error.", elf_file);

    // 结束符号怎么表示？
    if (strncmp((const char *)eh.e_ident, "\177ELF", 4) != 0)
        panic("elf_file format error");
}

static void get_section_header(int fd, Elf32_Shdr *sh_tab) {
    lseek(fd, eh.e_shoff, SEEK_SET);
    Assert(lseek(fd, eh.e_shoff, SEEK_SET) == eh.e_shoff, "section header offset error.");

    for (int i = 0; i < eh.e_shnum; i++) {
        Assert(read(fd, (void *)&sh_tab[i], eh.e_shentsize) == eh.e_shentsize, "section header size error.");
    }
}

static void read_section(int fd, Elf32_Shdr sh, void *dst) {
    assert(dst != NULL);
    assert(lseek(fd, (off_t)sh.sh_offset, SEEK_SET) == (off_t)sh.sh_offset);
    assert(read(fd, dst, sh.sh_size) == sh.sh_size);
}

static void iterate_symbol_table(int fd, Elf32_Shdr *sh_tab, int sym_index) {
    /* symbol table data */
    Elf32_Sym sym_tab[sh_tab[sym_index].sh_size];
    int       sym_num = sh_tab[sym_index].sh_size / sizeof(Elf32_Sym);
    read_section(fd, sh_tab[sym_index], sym_tab);

    /* string table data:  */
    int  str_index = sh_tab[sym_index].sh_link;
    char str_tab[sh_tab[str_index].sh_size];
    read_section(fd, sh_tab[str_index], str_tab);

    /* store the ftrace */
    /* future optimize: just store type that is FUNC. */
    ftrace_tab        = (ftrace_entry *)malloc(sym_num * sizeof(ftrace_entry));
    ftrace_table_size = sym_num;
    // printf("sym_num or ftrace_table_size: %d\n", ftrace_table_size);
    for (int i = 0; i < ftrace_table_size; i++) {
        ftrace_tab[i].addr = sym_tab[i].st_value;
        ftrace_tab[i].info = sym_tab[i].st_info;
        ftrace_tab[i].size = sym_tab[i].st_size;
        memset(ftrace_tab[i].name, '\0', 32);
        strncpy(ftrace_tab[i].name, str_tab + sym_tab[i].st_name, 31);
        ftrace_tab[i].name[31] = '\0';
    }
}
static void get_symbols(int fd, Elf32_Shdr *sh_tab) {
    for (int i = 0; i < eh.e_shnum; i++) {
        switch (sh_tab[i].sh_type) {
            case SHT_SYMTAB:
            case SHT_DYNSYM: iterate_symbol_table(fd, sh_tab, i);
        }
    }
}

int find_symbol_func(vaddr_t target, bool is_call) {
    int i = 0;
    for (; i < ftrace_table_size; i++) {
        if (ELF32_ST_TYPE(ftrace_tab[i].info) == STT_FUNC) {
            if (is_call) {
                if (ftrace_tab[i].addr == target)
                    break;
            } else {
                if (ftrace_tab[i].addr <= target && target < ftrace_tab[i].addr + ftrace_tab[i].size)
                    break;
            }
        }
    }
    return i < ftrace_table_size ? i : -1;
}

void parse_elf(const char *elf_file) {
    if (!elf_file) {
        return;
    }

    printf("elf_file = %s\n", elf_file);

    int fd = open(elf_file, O_RDONLY | O_SYNC);
    Assert(fd != -1, "elf_file: %s open error", elf_file);
    parse_elf_header(fd, elf_file);

    Elf32_Shdr sh[eh.e_shentsize * eh.e_shnum];
    get_section_header(fd, sh);

    get_symbols(fd, sh);

    //init_tail_rec_list();

    close(fd);
}

#define CALL_DEPTH (((call_depth) * (2)) % 50)
void ftrace_func_call(vaddr_t pc, vaddr_t target, bool is_tail) {
    if (!ftrace_tab)
        return;

    call_depth++;
    int i = find_symbol_func(target, true);
    printf(FMT_WORD ":%*scall [%s@" FMT_WORD "]\n",
           //log_write(FMT_WORD ":%*scall [%s@" FMT_WORD "]\n",
           pc, CALL_DEPTH, " ",
           //call_depth, " ",
           i >= 0 ? ftrace_tab[i].name : "???", target);
    /*
    if (is_tail) {
		insert_tail_rec(pc, call_depth-1);
    }
    */
}

void ftrace_func_ret(vaddr_t pc) {
    if (!ftrace_tab)
        return;

    int i = find_symbol_func(pc, false);
    printf(FMT_WORD ":%*sret [%s]\n",
           //log_write(FMT_WORD ":%*sret [%s]\n",
           pc, CALL_DEPTH, " ",
           //call_depth, " ",
           i >= 0 ? ftrace_tab[i].name : "???");
    call_depth--;

    /*
    TailRecNode *node = tail_rec_head->next;
	if (node != NULL) {
		if (node->depth == call_depth) {
			paddr_t ret_target = node->pc;
			remove_tail_rec();
			ftrace_func_ret(ret_target);
		}
	}
    */
}
