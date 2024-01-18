/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum
{
  TYPE_I,
  TYPE_U,
  TYPE_S,
  TYPE_N, // none

  // ----------
  TYPE_J, // 直接跳转
  // ----------
};

#define src1R()     \
  do                \
  {                 \
    *src1 = R(rs1); \
  } while (0)
#define src2R()     \
  do                \
  {                 \
    *src2 = R(rs2); \
  } while (0)
#define immI()                        \
  do                                  \
  {                                   \
    *imm = SEXT(BITS(i, 31, 20), 12); \
  } while (0)
#define immU()                              \
  do                                        \
  {                                         \
    *imm = SEXT(BITS(i, 31, 12), 20) << 12; \
  } while (0)
#define immS()                                               \
  do                                                         \
  {                                                          \
    *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); \
  } while (0)

// -------------- 我添加的内容
// 指令J 是先确定 1位 再确定8位, 再确定1位 再确定10位
#define immJ()                                                                                                     \
  do                                                                                                               \
  {                                                                                                                \
    *imm = ((((SEXT(BITS(i, 31, 30), 1) << 8) | BITS(i, 19, 12)) << 1 | BITS(i, 21, 20)) << 10 | BITS(i, 30, 21)); \
  } while (0)

// --------------

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type)
{
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd = BITS(i, 11, 7);
  switch (type)
  {
  case TYPE_I:
    src1R();
    immI();
    break;
  case TYPE_U:
    immU();
    break;
  case TYPE_S:
    src1R();
    src2R();
    immS();
    break;

  // ----------
  case TYPE_J:
    immJ();
    break;
    // ----------
  }
}

// 译码工作
static int decode_exec(Decode *s)
{
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)         \
  {                                                                  \
    decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
    __VA_ARGS__;                                                     \
  }

  INSTPAT_START();
  // ----添加的指令-----------------
  // li 伪指令 用addi指令实现  li rd,13 => addi rd,x0,13
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", li, I, R(rd) = R(0) + imm);

  // jal	ra,80000018   将 PC+4 的值保存到 rd 寄存器中，然后设置 PC = PC + offset  拿到的imm要左移一位
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, imm = imm << 1, R(rd) = s->pc + 4, s->dnpc = s->pc + imm);

  // 00112623    sw	ra,12(sp) 的含义是将寄存器 ra 中的值存储到地址 sp+12 的内存位置中。在这里，ra 是链接寄存器（link register），sp 是栈指针寄存器（stack pointer register） 存一个字
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(src1 + imm, 4, src2));

  // ------------------------------

  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, src2));

  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s)
{
  // 因为是riscv32 所以这里的传值是4
  s->isa.inst.val = inst_fetch(&s->snpc, 4); // 拿到当前pc指向内存的数据,一条指令
  return decode_exec(s);                     // 对指令译码
}
