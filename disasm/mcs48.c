/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPLv3
 *
 * Copyright 2010-2019 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disasm/mcs48.h"
#include "table/mcs48.h"

#define READ_RAM(a) memory_read_m(memory, a)

int get_cycle_count_mcs48(unsigned short int opcode)
{
  return -1;
}

int disasm_mcs48(struct _memory *memory, uint32_t address, char *instruction, int *cycles_min, int *cycles_max)
{
  int opcode;
  char temp[32];
  int value, length;
  int n, r;

  opcode = memory_read_m(memory, address);

  n = 0;

  while(table_mcs48[n].name != NULL)
  { 
    if (table_mcs48[n].opcode == (opcode & table_mcs48[n].mask))
    {
      sprintf(instruction, "%s", table_mcs48[n].name);

      *cycles_min = table_mcs48[n].cycles;
      *cycles_max = table_mcs48[n].cycles;

      if (table_mcs48[n].operand_count == 0)
      {
        return 1;
      }

      length = 1;

      strcat(instruction, " ");

      for (r = 0; r < table_mcs48[n].operand_count; r++)
      {
        if (r == 1) { strcat(instruction, ", "); }

        int type = (r == 0) ? table_mcs48[n].operand_1 :
                              table_mcs48[n].operand_2;

        switch (type)
        {
          case OP_A:
            strcat(instruction, "A");
            break;
          case OP_C:
            strcat(instruction, "C");
            break;
          case OP_I:
            strcat(instruction, "I");
            break;
          case OP_T:
            strcat(instruction, "T");
            break;
          case OP_F0:
            strcat(instruction, "F0");
            break;
          case OP_F1:
            strcat(instruction, "F1");
            break;
          case OP_BUS:
            strcat(instruction, "BUS");
            break;
          case OP_CLK:
            strcat(instruction, "CLK");
            break;
          case OP_CNT:
            strcat(instruction, "CNT");
            break;
          case OP_MBK:
            strcat(instruction, "MBK");
            break;
          case OP_RBK:
            strcat(instruction, "RBK");
            break;
          case OP_PSW:
            strcat(instruction, "PSW");
            break;
          case OP_TCNT:
            strcat(instruction, "TCNT");
            break;
          case OP_TCNTI:
            strcat(instruction, "TCNTI");
            break;
          case OP_AT_A:
            strcat(instruction, "@A");
            break;
          case OP_PP:
            sprintf(temp, "p%d", opcode & 0x3);
            strcat(instruction, temp);
            break;
          case OP_RR:
            sprintf(temp, "r%d", opcode & 0x7);
            strcat(instruction, temp);
            break;
          case OP_AT_R:
            sprintf(temp, "@r%d", opcode & 0x1);
            strcat(instruction, temp);
            break;
          case OP_NUM:
            value = memory_read_m(memory, address + 1);
            sprintf(temp, "#0x%02x", value);
            strcat(instruction, temp);
            length = 2;
            break;
          case OP_ADDR:
            value = memory_read_m(memory, address + 1);
            sprintf(temp, "#0x%02x", ((opcode & 0xe000) >> 5) | value);
            strcat(instruction, temp);
            length = 2;
            break;
          case OP_PADDR:
            value = memory_read_m(memory, address + 1);
            sprintf(temp, "#0x%02x", (address & 0xff00) | value);
            strcat(instruction, temp);
            length = 2;
            break;
        }
      }

      return length;
    }

    n++;
  }

  return 0;
}

void list_output_mcs48(struct _asm_context *asm_context, uint32_t start, uint32_t end)
{
  int cycles_min = -1, cycles_max = -1, count;
  char instruction[128];
  char temp[32];
  char temp2[4];
  int n;

  while(start < end)
  {
    count = disasm_mcs48(&asm_context->memory, start, instruction, &cycles_min, &cycles_max);

    temp[0] = 0;

    for (n = 0; n < count; n++)
    {
      sprintf(temp2, "%02x ", memory_read_m(&asm_context->memory, start + n));
      strcat(temp, temp2);
    }

    fprintf(asm_context->list, "0x%04x: %-10s %-40s cycles: %d\n",
      start, temp, instruction, cycles_min);

    start += count;
  }
}

void disasm_range_mcs48(struct _memory *memory, uint32_t flags, uint32_t start, uint32_t end)
{
  char instruction[128];
  char temp[32];
  char temp2[4];
  int cycles_min = 0, cycles_max = 0;
  int count;
  int n;

  printf("\n");

  printf("%-7s %-5s %-40s Cycles\n", "Addr", "Opcode", "Instruction");
  printf("------- ------ ----------------------------------       ------\n");

  while(start <= end)
  {
    count = disasm_mcs48(memory, start, instruction, &cycles_min, &cycles_max);

    temp[0] = 0;
    for (n = 0; n < count; n++)
    {
      sprintf(temp2, "%02x ", memory_read_m(memory, start+n));
      strcat(temp, temp2);
    }

    printf("0x%04x: %-10s %-40s %d\n", start, temp, instruction, cycles_min);

    start += count;
  }
}

