/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2013 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disasm_common.h"
#include "disasm_68hc08.h"
#include "table_68hc08.h"

#define READ_RAM(a) memory_read_m(memory, a)
#define READ_RAM16(a) (memory_read_m(memory, a)<<8)|memory_read_m(memory, a+1)

extern struct _m68hc08_table m68hc08_table[];
extern struct _m68hc08_16_table m68hc08_16_table[];

int get_cycle_count_68hc08(unsigned short int opcode)
{
  return -1;
}

int disasm_68hc08(struct _memory *memory, int address, char *instruction, int *cycles_min, int *cycles_max)
{
//int bit_instr;
int opcode;
int size=1;
int n;

  strcpy(instruction, "???");

  *cycles_min=-1;
  *cycles_max=-1;

  opcode=READ_RAM(address);

  if (m68hc08_table[opcode].instr==NULL)
  {
    opcode=READ_RAM16(address);

    n=0;
    while(m68hc08_16_table[n].instr!=NULL)
    {
      if (m68hc08_16_table[n].opcode==opcode)
      {
        switch(m68hc08_16_table[n].operand_type)
        {
          case CPU08_OP_OPR8_SP:
            sprintf(instruction, "%s $%02x,SP", m68hc08_16_table[n].instr, READ_RAM(address+2));
            size=3;
            break;
          case CPU08_OP_OPR8_SP_REL:
            sprintf(instruction, "%s $%02x,SP,$%04x", m68hc08_16_table[n].instr, READ_RAM(address+2), (address+4)+((char)READ_RAM(address+3)));
            size=4;
            break;
          case CPU08_OP_OPR16_SP:
            sprintf(instruction, "%s $%04x,SP", m68hc08_16_table[n].instr, READ_RAM16(address+2));
            size=4;
            break;
        }

        *cycles_min=m68hc08_16_table[n].cycles;
        *cycles_max=m68hc08_16_table[n].cycles;

        break;
      }
      n++;
    }

    return size;
  }

  *cycles_min=m68hc08_table[opcode].cycles;
  *cycles_max=m68hc08_table[opcode].cycles;

  switch(m68hc08_table[opcode].operand_type)
  {
    case CPU08_OP_NONE:
      sprintf(instruction, "%s", m68hc08_table[opcode].instr);
      break;
    case CPU08_OP_NUM16:
      sprintf(instruction, "%s #$%04x", m68hc08_table[opcode].instr, READ_RAM16(address+1));
      size=3;
      break;
    case CPU08_OP_NUM8:
      sprintf(instruction, "%s #$%02x", m68hc08_table[opcode].instr, READ_RAM(address+1));
      size=2;
      break;
    case CPU08_OP_NUM8_OPR8:
      sprintf(instruction, "%s #$%02x,$%02x", m68hc08_table[opcode].instr, READ_RAM(address+1), READ_RAM(address+2));
      size=3;
      break;
    case CPU08_OP_NUM8_REL:
      sprintf(instruction, "%s #$%02x, $%04x (%d)", m68hc08_table[opcode].instr, READ_RAM(address+1), (address+3)+((char)READ_RAM(address+2)), (char)READ_RAM(address+2));
      size=3;
      break;
    case CPU08_OP_OPR16:
      sprintf(instruction, "%s $%04x", m68hc08_table[opcode].instr, READ_RAM16(address+1));
      size=3;
      break;
    case CPU08_OP_OPR16_X:
      sprintf(instruction, "%s $%04x,X", m68hc08_table[opcode].instr, READ_RAM16(address+1));
      size=3;
      break;
    case CPU08_OP_OPR8:
      sprintf(instruction, "%s $%02x", m68hc08_table[opcode].instr, READ_RAM(address+1));
      size=2;
      break;
    case CPU08_OP_OPR8_OPR8:
      sprintf(instruction, "%s $%02x,$%02x", m68hc08_table[opcode].instr, READ_RAM(address+1), READ_RAM(address+2));
      size=3;
      break;
    case CPU08_OP_OPR8_REL:
      sprintf(instruction, "%s $%02x,$%04x (%d)", m68hc08_table[opcode].instr, READ_RAM(address+1), ((address+3)+(char)READ_RAM(address+2)), (char)READ_RAM(address+2));
      size=3;
      break;
    case CPU08_OP_OPR8_X:
      sprintf(instruction, "%s %02x,X", m68hc08_table[opcode].instr, READ_RAM(address+1));
      size=2;
      break;
    case CPU08_OP_OPR8_X_PLUS:
      sprintf(instruction, "%s $%02x,X+", m68hc08_table[opcode].instr, READ_RAM(address+1));
      size=2;
      break;
    case CPU08_OP_OPR8_X_PLUS_REL:
      sprintf(instruction, "%s $%02x,X+,$%04x (%d)", m68hc08_table[opcode].instr, READ_RAM(address+1), (address+3)+((char)READ_RAM(address+2)), (char)READ_RAM(address+2));
      size=3;
      break;
    case CPU08_OP_OPR8_X_REL:
      sprintf(instruction, "%s $%02x,X,$%04x (%d)", m68hc08_table[opcode].instr, READ_RAM(address+1), (address+3)+((char)READ_RAM(address+2)), (char)READ_RAM(address+2));
      size=3;
      break;
    case CPU08_OP_REL:
      sprintf(instruction, "%s $%04x (%d)", m68hc08_table[opcode].instr, (address+2)+((char)READ_RAM(address+1)), (char)READ_RAM(address+1));
      size=2;
      break;
    case CPU08_OP_COMMA_X:
      sprintf(instruction, "%s ,X", m68hc08_table[opcode].instr);
      break;
    case CPU08_OP_X:
      sprintf(instruction, "%s X", m68hc08_table[opcode].instr);
      break;
    case CPU08_OP_X_PLUS_OPR8:
      sprintf(instruction, "%s ,X+,$%02x", m68hc08_table[opcode].instr, READ_RAM(address+1));
      size=2;
      break;
    case CPU08_OP_X_PLUS_REL:
      sprintf(instruction, "%s ,X+,$%04x (%d)", m68hc08_table[opcode].instr, (address+2)+((char)READ_RAM(address+1)), (char)READ_RAM(address+1));
      size=2;
      break;
    case CPU08_OP_X_REL:
      sprintf(instruction, "%s ,X,$%04x (%d)", m68hc08_table[opcode].instr, (address+2)+((char)READ_RAM(address+1)), (char)READ_RAM(address+1));
      size=2;
      break;
    case CPU08_OP_0_COMMA_OPR:
    case CPU08_OP_1_COMMA_OPR:
    case CPU08_OP_2_COMMA_OPR:
    case CPU08_OP_3_COMMA_OPR:
    case CPU08_OP_4_COMMA_OPR:
    case CPU08_OP_5_COMMA_OPR:
    case CPU08_OP_6_COMMA_OPR:
    case CPU08_OP_7_COMMA_OPR:
      sprintf(instruction, "%s %d,$%02x", m68hc08_table[opcode].instr, m68hc08_table[opcode].operand_type-CPU08_OP_0_COMMA_OPR, READ_RAM(address+1));
      size=2;
      break;
    case CPU08_OP_0_COMMA_OPR_REL:
    case CPU08_OP_1_COMMA_OPR_REL:
    case CPU08_OP_2_COMMA_OPR_REL:
    case CPU08_OP_3_COMMA_OPR_REL:
    case CPU08_OP_4_COMMA_OPR_REL:
    case CPU08_OP_5_COMMA_OPR_REL:
    case CPU08_OP_6_COMMA_OPR_REL:
    case CPU08_OP_7_COMMA_OPR_REL:
      sprintf(instruction, "%s %d,$%02x,$%04x (%d)", m68hc08_table[opcode].instr, m68hc08_table[opcode].operand_type-CPU08_OP_0_COMMA_OPR_REL, READ_RAM(address+1), (address+3)+(char)READ_RAM(address+2), (char)READ_RAM(address+2));
      size=3;
      break;
  }

  return size;
}

void list_output_68hc08(struct _asm_context *asm_context, int address)
{
int cycles_min,cycles_max;
char instruction[128];
char bytes[14];
int count;
int n;
//unsigned int opcode=memory_read_m(&asm_context->memory, address);

  fprintf(asm_context->list, "\n");
  count=disasm_68hc08(&asm_context->memory, address, instruction, &cycles_min, &cycles_max);

  bytes[0]=0;
  for (n=0; n<count; n++)
  {
    char temp[4];
    sprintf(temp, "%02x ", memory_read_m(&asm_context->memory, address+n));
    strcat(bytes, temp);
  }

  fprintf(asm_context->list, "0x%04x: %-12s %-40s cycles: ", address, bytes, instruction);

  if (cycles_min==cycles_max)
  { fprintf(asm_context->list, "%d\n", cycles_min); }
    else
  { fprintf(asm_context->list, "%d-%d\n", cycles_min, cycles_max); }
}

void disasm_range_68hc08(struct _memory *memory, int start, int end)
{
char instruction[128];
char bytes[14];
int cycles_min=0,cycles_max=0;
int count;
int n;

  printf("\n");

  printf("%-7s %-5s %-40s Cycles\n", "Addr", "Opcode", "Instruction");
  printf("------- ------ ----------------------------------       ------\n");

  while(start<=end)
  {
    count=disasm_68hc08(memory, start, instruction, &cycles_min, &cycles_max);

    bytes[0]=0;
    for (n=0; n<count; n++)
    {
      char temp[4];
      sprintf(temp, "%02x ", READ_RAM(start+n));
      strcat(bytes, temp);
    }

    if (cycles_min<1)
    {
      printf("0x%04x: %-12s %-40s ?\n", start, bytes, instruction);
    }
      else
    if (cycles_min==cycles_max)
    {
      printf("0x%04x: %-12s %-40s %d\n", start, bytes, instruction, cycles_min);
    }
      else
    {
      printf("0x%04x: %-12s %-40s %d-%d\n", start, bytes, instruction, cycles_min, cycles_max);
    }

    start=start+count;
  }
}


