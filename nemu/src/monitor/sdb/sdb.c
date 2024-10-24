/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include "common.h"
#include "memory/paddr.h"
#include "watchpoint.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets() {
    static char *line_read = NULL;

    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

    line_read = readline("(nemu) ");

    if (line_read && *line_read) {
        add_history(line_read);
    }

    return line_read;
}

static int cmd_c(char *args) {
    cpu_exec(-1);
    return 0;
}

static int cmd_q(char *args) {
    nemu_state.state = NEMU_QUIT;
    return -1;
}

static int cmd_si(char *args) {
    if (args == NULL) {
        cpu_exec(1);
    }

    char  *endptr;
    word_t n = (word_t)strtol(args, &endptr, 10);

    if (*endptr == '\0') {
        cpu_exec(n);
    }
    return -1;
}

static int cmd_info(char *args) {
    if (*args == 'r') {
        isa_reg_display();
        return 0;
    } else if (*args == 'w') {
        info_watchpoints();
        return 0;
    }
    return -1;
}

static int cmd_x(char *args) {
    int    bytes;
    word_t start;
    int    result = sscanf(args, "%d %x", &bytes, &start);
    if (result == 2) {
        for (word_t offset = 0; offset < bytes; ++offset) {
            word_t address = start + offset * 4;
            printf(FMT_WORD " ", address);
            for (word_t byte = 0; byte < 4; ++byte) {
                uint8_t *value = guest_to_host(address + byte);
                printf("%02x ", *value);
            }
            printf("\n");
        }
        return 1;
    }
    return -1;
}

static int cmd_p(char *args) {
    bool   success;
    word_t res = expr(args, &success);
    if (!success) {
        Assert(true, "error");
    } else {
        printf("%d\n", res);
    }
    return 0;
}

static int cmd_w(char *args) {
    WP *wp = new_wp();
    strcpy(wp->expr, args);
    bool success  = false;
    wp->old_value = expr(wp->expr, &success);
    if (!success) {
        printf("The expr of watch is error\n");
        return -1;
    }
    return 0;
}

static int cmd_d(char *args) {
    if (args == NULL) {
        printf("No args. \n");
    } else {
        delete_watchpoint(atoi(args));
    }
    return 0;
}

static int cmd_help(char *args);

static struct {
    const char *name;
    const char *description;
    int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c",    "Continue the execution of the program",            cmd_c   },
    {"q",    "Exit NEMU",                                        cmd_q   },
    {"si",   "Step by step",                                     cmd_si  },
    {"info", "Display program status information",               cmd_info},
    {"x",    "Display memory information",                       cmd_x   },
    {"p",    "Evaluate expression",                              cmd_p   },
    {"w",    "Set a watchpoint",                                 cmd_w   },
    {"d",    "Delete a watchpoint",                              cmd_d   },
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    int   i;

    if (arg == NULL) {
        /* no argument given */
        for (i = 0; i < NR_CMD; i++) {
            printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    } else {
        for (i = 0; i < NR_CMD; i++) {
            if (strcmp(arg, cmd_table[i].name) == 0) {
                printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
                return 0;
            }
        }
        printf("Unknown command '%s'\n", arg);
    }
    return 0;
}

void sdb_set_batch_mode() {
    is_batch_mode = true;
}

void sdb_mainloop() {
    if (is_batch_mode) {
        cmd_c(NULL);
        return;
    }

    for (char *str; (str = rl_gets()) != NULL;) {
        char *str_end = str + strlen(str);

        /* extract the first token as the command */
        char *cmd = strtok(str, " ");
        if (cmd == NULL) {
            continue;
        }

        /* treat the remaining string as the arguments,
     * which may need further parsing
     */
        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end) {
            args = NULL;
        }

#ifdef CONFIG_DEVICE
        extern void sdl_clear_event_queue();
        sdl_clear_event_queue();
#endif

        int i;
        for (i = 0; i < NR_CMD; i++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
                if (cmd_table[i].handler(args) < 0) {
                    return;
                }
                break;
            }
        }

        if (i == NR_CMD) {
            printf("Unknown command '%s'\n", cmd);
        }
    }
}

void test_expr() {
    char *nemu_pth = getenv("NEMU_HOME");
    if (nemu_pth == NULL) {
        perror("NEMU_HOME environment variable not set");
        return;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/tools/gen-expr/input", nemu_pth);

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        perror("test_expr error");

        // 执行 make gen-expr
        char make_command[256];
        snprintf(make_command, sizeof(make_command), "cd %s/tools/gen-expr && make gen-expr", nemu_pth);
        int make_result = system(make_command);
        if (make_result != 0) {
            perror("make gen-expr failed");
            return;
        }

        // 执行 ./gen-expr 1000 > input
        char gen_expr_command[256];
        snprintf(gen_expr_command, sizeof(gen_expr_command), "cd %s/tools/gen-expr && ./gen-expr 1000 > input", nemu_pth);
        int gen_expr_result = system(gen_expr_command);
        if (gen_expr_result != 0) {
            perror("./gen-expr failed");
            return;
        }

        // 重新尝试打开文件
        fp = fopen(filepath, "r");
        if (fp == NULL) {
            perror("Failed to open input file after generating it");
            return;
        }
    }

    char   *e = NULL;
    word_t  correct_res;
    size_t  len = 0;
    ssize_t read;
    bool    success   = false;
    word_t  which_one = 0;

    while (true) {
        which_one++;
        if (fscanf(fp, "%u ", &correct_res) == -1) break;
        read        = getline(&e, &len, fp);
        e[read - 1] = '\0';

        word_t res = expr(e, &success);

        assert(success);
        if (res != correct_res) {
            puts(e);
            printf("The %u expected: %u, got: %u\n", which_one, correct_res, res);
            assert(0);
        }
    }

    fclose(fp);
    if (e) free(e);

    Log("expr test pass");
}

void init_sdb() {
    /* Compile the regular expressions. */
    init_regex();

    test_expr();

    /* Initialize the watchpoint pool. */
    init_wp_pool();
}
