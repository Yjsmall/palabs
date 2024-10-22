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

#include "common.h"
#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <stdint.h>
#include <stdio.h>

#define R(i) gpr(i)
#define Mr   vaddr_read
#define Mw   vaddr_write

enum {
    TYPE_I,
    TYPE_U,
    TYPE_S,
    TYPE_J,
    TYPE_R,
    TYPE_B,
    TYPE_N, // none
};

#define src1R()                                                                                                                                      \
    do {                                                                                                                                             \
        *src1 = R(rs1);                                                                                                                              \
    } while (0)

#define src2R()                                                                                                                                      \
    do {                                                                                                                                             \
        *src2 = R(rs2);                                                                                                                              \
    } while (0)

#define immI()                                                                                                                                       \
    do {                                                                                                                                             \
        *imm = SEXT(BITS(i, 31, 20), 12);                                                                                                            \
    } while (0)

#define immU()                                                                                                                                       \
    do {                                                                                                                                             \
        *imm = SEXT(BITS(i, 31, 12), 20) << 12;                                                                                                      \
    } while (0)

#define immS()                                                                                                                                       \
    do {                                                                                                                                             \
        *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7);                                                                                     \
    } while (0)


#define immJ() do { \
	*imm = SEXT(( \
            (BITS(i, 31, 31) << 19) | \
            (BITS(i, 19, 12) << 11) | \
            (BITS(i, 20, 20) << 10) | \
            (BITS(i, 30, 21)) \
            ) << 1, 21);  \
} while(0)

// TODO:


#define immB() do { \
	*imm = SEXT(( \
            (BITS(i, 31, 31) << 11) | \
            (BITS(i, 7, 7)   << 10) | \
            (BITS(i, 30, 25) << 4) | \
            (BITS(i, 11, 8)) \
            ) << 1, 13);  \
} while(0)


#define debug_print(x1, x2, x3) printf("src1-%08x src2-%08x imm-%08x\n", x1, x2, x3)
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
    uint32_t i   = s->isa.inst;
    int      rs1 = BITS(i, 19, 15);
    int      rs2 = BITS(i, 24, 20);
    *rd          = BITS(i, 11, 7);
    switch (type) {
        case TYPE_I:
            src1R();
            immI();
            break;
        case TYPE_U: immU(); break;
        case TYPE_S:
            src1R();
            src2R();
            immS();
            break;
        case TYPE_J: immJ(); break;
        case TYPE_R:
            src1R();
            src2R();
            break;
        case TYPE_B:
            src1R();
            src2R();
            immB();
            break;
        case TYPE_N: break;
        default: panic("unsupported type = %d", type);
    }
}

static int decode_exec(Decode *s) {
    s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)                                                                                         \
    {                                                                                                                                                \
        int    rd   = 0;                                                                                                                             \
        word_t src1 = 0, src2 = 0, imm = 0;                                                                                                          \
        decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type));                                                                             \
        __VA_ARGS__;                                                                                                                                 \
    }

    INSTPAT_START();
    // INSPAT(str, name, type, operantion)

    // TYPE_R: R-type instruction
    // logic operation
    INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, R(rd) = src1 + src2);
    INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, R(rd) = src1 - src2);
    INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, R, R(rd) = (src1 ^ src2));
    INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, R, R(rd) = (src1 | src2));
    INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and, R, R(rd) = src1 & src2);
    INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll, R, R(rd) = src1 << src2);
    INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl, R, R(rd) = src1 >> src2);
    INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra, R, R(rd) = (sword_t)src1 >> (sword_t)src2);
    INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt, R, R(rd) = ((sword_t)src1 < (sword_t)src2));
    INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, R, R(rd) = (src1 < src2));


    // TYPE_I: I-type instruction
    // ----------[imme]--------[rs1]-[3]-[rd]-[opcode]
    INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R(rd) = src1 + imm);
    INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori, I, R(rd) = src1 ^ imm);
    INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori, I, R(rd) = src1 | imm);
    INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I, R(rd) = src1 & imm);
    INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli, I, R(rd) = src1 << BITS(imm, 5, 0));
    INSTPAT("000000? ????? ????? 101 ????? 00100 11", srli, I, R(rd) = src1 >> BITS(imm, 5, 0));
    INSTPAT("010000? ????? ????? 101 ????? 00100 11", srai, I, R(rd) = (sword_t)src1 >> BITS(imm, 5, 0));
    // slti
    INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti, I, R(rd) = ((sword_t)src1 < (sword_t)imm));
    INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu, I, R(rd) = (src1 < (word_t)imm));

    /* load */
    INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, R(rd) = SEXT(Mr(src1 + imm, 1), 8));
    INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, R(rd) = SEXT(Mr(src1 + imm, 2), 16));
    INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, R(rd) = SEXT(Mr(src1 + imm, 4), 32));
    INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(rd) = Mr(src1 + imm, 1));
    INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, R(rd) = Mr(src1 + imm, 2));

    // TYPE_S: S-type instruction
    INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, src2));
    INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, Mw(src1 + imm, 2, src2));
    INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(src1 + imm, 4, src2));

    // TYPE_B: B-type instruction
    //                    rs2   rs1            opcode
    INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq, B, if (src1 == src2) s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne, B, if (src1 != src2) s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt, B, if ((sword_t)src1 < (sword_t)src2) s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B, if ((sword_t)src1 >= (sword_t)src2) s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu, B, if (src1 < src2) s->dnpc = s->pc + imm);
    INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B, if (src1 >= src2) s->dnpc = s->pc + imm);

    // TYPE_J: J-type instruction
    INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, R(rd) = s->pc + 4; s->dnpc = s->pc + imm;);
    INSTPAT("??????? ????? ????? 000 ????? 1100111", jalr, I, s->dnpc = src1 + imm; R(rd) = s->pc + 4;);

    // TYPE_U: U-type instruction
    INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);
    INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm;);

      /* RV32M Multiply Extension */
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1 * src2);
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   ,
          R, R(rd) = BITS(((int64_t)(int32_t)src1 * (int32_t)src2), 63,32));
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , 
          R, R(rd) = BITS(((uint64_t)(uint32_t)src1 * (uint32_t)src2), 63,32));
  // mulhsu
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = (sword_t)src1 / (sword_t)src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = src1 / src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = (sword_t)src1 % (sword_t)src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = src1 % src2);

    // TYPE_N: No operand instruction
    INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
    INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));

    INSTPAT_END();

    R(0) = 0; // reset $zero to 0

    return 0;
}

int isa_exec_once(Decode *s) {
    s->isa.inst = inst_fetch(&s->snpc, 4);
    return decode_exec(s);
}
