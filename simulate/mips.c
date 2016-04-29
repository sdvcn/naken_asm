/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2016 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include "simulate/mips.h"
#include "disasm/mips32.h"

static int stop_running = 0;

static const char *reg_names[32] =
{
  "$0", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
  "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
  "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
  "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};


static void handle_signal(int sig)
{
  stop_running = 1;
  signal(SIGINT, SIG_DFL);
}

struct _simulate *simulate_init_mips(struct _memory *memory)
{
struct _simulate *simulate;

  simulate = (struct _simulate *)malloc(sizeof(struct _simulate_mips) +
                                        sizeof(struct _simulate));

  simulate->simulate_init = simulate_init_mips;
  simulate->simulate_free = simulate_free_mips;
  simulate->simulate_dumpram = simulate_dumpram_mips;
  simulate->simulate_push = simulate_push_mips;
  simulate->simulate_set_reg = simulate_set_reg_mips;
  simulate->simulate_get_reg = simulate_get_reg_mips;
  simulate->simulate_reset = simulate_reset_mips;
  simulate->simulate_dump_registers = simulate_dump_registers_mips;
  simulate->simulate_run = simulate_run_mips;

  simulate->memory = memory;
  simulate_reset_mips(simulate);
  simulate->usec = 1000000; // 1Hz
  simulate->step_mode = 0;
  simulate->show = 1;       // Show simulation
  simulate->auto_run = 0;   // Will this program stop on a ret from main
  return simulate;
}

void simulate_push_mips(struct _simulate *simulate, uint32_t value)
{
//struct _simulate_mips *simulate_mips = (struct _simulate_mips *)simulate->context;

}

int simulate_set_reg_mips(struct _simulate *simulate, char *reg_string, uint32_t value)
{
  struct _simulate_mips *simulate_mips = (struct _simulate_mips *)simulate->context;
  int reg, n;

  if (reg_string[0] != '$') { return -1; }

  if (reg_string[1] >= '0' && reg_string[1] <= '9' &&
      reg_string[2] >= '0' && reg_string[2] <= '9' &&
      reg_string[3] == 0)
  {
    reg = atoi(reg_string + 1);
    if (reg < 0 || reg > 31) { return -1; }

    simulate_mips->reg[reg] = value;
    return 0;
  }

  for (n = 0; n < 32; n++)
  {
    if (strcmp(reg_string, reg_names[n]) == 0)
    {
      simulate_mips->reg[n] = value;
      return 0;
    }
  }

  return -1;
}

uint32_t simulate_get_reg_mips(struct _simulate *simulate, char *reg_string)
{
//struct _simulate_mips *simulate_mips = (struct _simulate_mips *)simulate->context;

  return 0;
}

void simulate_reset_mips(struct _simulate *simulate)
{
  struct _simulate_mips *simulate_mips = (struct _simulate_mips *)simulate->context;

  simulate_mips->pc = simulate->memory->low_address;
  simulate_mips->reg[29] = 0x80000000;

  struct _memory *memory = simulate->memory;

  // PIC32 kind of hack.  Need to figure out a better way to do this
  // later.  Problem is PIC32 has virtual memory (where code addresses)
  // and physical memory (where the hex file says the code is).
  if (memory->low_address >= 0x1d000000 && memory->high_address <= 0x1d007fff)
  {
    uint32_t physical, virtual;
    uint32_t low_address, high_address;

    virtual = 0x9d000000 + (memory->low_address - 0x1d000000);

    printf("Copying physical 0x%x-0x%x to virtual 0x%x\n", memory->low_address, memory->high_address, virtual);

    simulate_mips->pc = virtual;
    low_address = memory->low_address;
    high_address = memory->high_address;

    for (physical = low_address; physical <= high_address; physical++)
    {
      memory_write_m(memory, virtual++, memory_read_m(memory, physical));
    }
  }
}

void simulate_free_mips(struct _simulate *simulate)
{
  free(simulate);
}

int simulate_dumpram_mips(struct _simulate *simulate, int start, int end)
{
  return -1;
}

void simulate_dump_registers_mips(struct _simulate *simulate)
{
  struct _simulate_mips *simulate_mips = (struct _simulate_mips *)simulate->context;
  int n;

  printf("\nSimulation Register Dump\n");
  printf("-------------------------------------------------------------------\n");
  printf(" PC: 0x%04x\n", simulate_mips->pc);

  for (n = 0; n < 32; n++)
  {
    printf("%c%3s: 0x%08x", (n & 0x3) == 0 ? '\n' : ' ',
                            reg_names[n],
                            simulate_mips->reg[n]);
  }

  printf("\n\n");
  printf("%d clock cycles have passed since last reset.\n\n", simulate->cycle_count);
}

static int simulate_execute_mips_shift(struct _simulate *simulate, uint32_t opcode)
{
  struct _simulate_mips *simulate_mips = (struct _simulate_mips *)simulate->context;

  int sa = (opcode >> 6) & 0x1f;
  int rd = (opcode >> 11) & 0x1f;
  int rt = (opcode >> 16) & 0x1f;

  switch(opcode & 0x3f)
  {
    case 0: // sll
      simulate_mips->reg[rd] = simulate_mips->reg[rt] << sa;
      break;
    case 2: // srl
      simulate_mips->reg[rd] = ((uint32_t)simulate_mips->reg[rt]) >> sa;
      break;
    case 3: // sra
      simulate_mips->reg[rd] = simulate_mips->reg[rt] >> sa;
      break;
    default:
      return -1;
  }

  //simulate_mips->reg[0] = 0;

  return 0;
}

static int simulate_execute_mips_r(struct _simulate *simulate, uint32_t opcode)
{
  struct _simulate_mips *simulate_mips = (struct _simulate_mips *)simulate->context;

  int rd = (opcode >> 11) & 0x1f;
  int rt = (opcode >> 16) & 0x1f;
  int rs = (opcode >> 21) & 0x1f;

  opcode = opcode & 0x3f;

  switch(opcode)
  {
    case 0x20: // add
      // FIXME - need to trap on overflow
      simulate_mips->reg[rd] = simulate_mips->reg[rs] + simulate_mips->reg[rt];
      break;
    case 0x21: // addu
      simulate_mips->reg[rd] = simulate_mips->reg[rs] + simulate_mips->reg[rt];
      break;
    case 0x22: // sub
      // FIXME - need to trap on overflow
      simulate_mips->reg[rd] = simulate_mips->reg[rs] - simulate_mips->reg[rt];
      break;
    case 0x23: // subu
      simulate_mips->reg[rd] = simulate_mips->reg[rs] - simulate_mips->reg[rt];
      break;
    case 0x25: // or
      simulate_mips->reg[rd] = simulate_mips->reg[rs] | simulate_mips->reg[rt];
      break;
    case 0x26: // xor
      simulate_mips->reg[rd] = simulate_mips->reg[rs] ^ simulate_mips->reg[rt];
      break;
    case 0x27: // nor
      simulate_mips->reg[rd] =
        ~(simulate_mips->reg[rs] | simulate_mips->reg[rt]);
      break;
    default:
      return -1;
  }

  //simulate_mips->reg[0] = 0;

  return 0;
}

static int simulate_execute_mips_i(struct _simulate *simulate, uint32_t opcode)
{
  struct _simulate_mips *simulate_mips = (struct _simulate_mips *)simulate->context;
  uint32_t address;

  int rs = (opcode >> 21) & 0x1f;
  int rt = (opcode >> 16) & 0x1f;

  switch(opcode >> 26)
  {
    case 0x08: // addi
      // FIXME - need to trap on overflow
      simulate_mips->reg[rt] = simulate_mips->reg[rs] + (int16_t)(opcode & 0xffff);
      break;
    case 0x09: // addiu
      simulate_mips->reg[rt] = simulate_mips->reg[rs] + (int16_t)(opcode & 0xffff);
      break;
    case 0x0d: // ori
      simulate_mips->reg[rt] = simulate_mips->reg[rs] | (opcode & 0xffff);
      break;
    case 0x0e: // xori
      simulate_mips->reg[rt] = simulate_mips->reg[rs] ^ (opcode & 0xffff);
      break;
    case 0x0f: // lui
      if (rs != 0) { return -1; }
      simulate_mips->reg[rt] = (opcode & 0xffff) << 16;
      break;
    case 0x20: // lb
      address = simulate_mips->reg[rs] + ((int16_t)(opcode & 0xffff));
      simulate_mips->reg[rt] = (int32_t)((int8_t)memory_read_m(simulate->memory, address));
    case 0x21: // lh
      address = simulate_mips->reg[rs] + ((int16_t)(opcode & 0xffff));
      if ((address & 1) != 0)
      {
        printf("Alignment error.  Reading address 0x%04x\n", address);
        return -2;
      }
      simulate_mips->reg[rt] = (int32_t)((int16_t)memory_read16_m(simulate->memory, address));
      break;
    case 0x23: // lw
      address = simulate_mips->reg[rs] + ((int16_t)(opcode & 0xffff));
      if ((address & 3) != 0)
      {
        printf("Alignment error.  Reading address 0x%04x\n", address);
        return -2;
      }
      simulate_mips->reg[rt] = memory_read32_m(simulate->memory, address);
      break;
    case 0x24: // lbu
      address = simulate_mips->reg[rs] + ((int16_t)(opcode & 0xffff));
      simulate_mips->reg[rt] = memory_read_m(simulate->memory, address);
      break;
    case 0x25: // lhu
      address = simulate_mips->reg[rs] + ((int16_t)(opcode & 0xffff));
      if ((address & 1) != 0)
      {
        printf("Alignment error.  Reading address 0x%04x\n", address);
        return -2;
      }
      simulate_mips->reg[rt] = (int32_t)((uint16_t)memory_read16_m(simulate->memory, address));
      break;
    case 0x28: // sb
      address = simulate_mips->reg[rs] + ((int16_t)(opcode & 0xffff));
      memory_write_m(simulate->memory, address, simulate_mips->reg[rt] & 0xff);
      break;
    case 0x29: // sh
      address = simulate_mips->reg[rs] + ((int16_t)(opcode & 0xffff));
      if ((address & 1) != 0)
      {
        printf("Alignment error.  Reading address 0x%04x\n", address);
        return -2;
      }
      memory_write16_m(simulate->memory, address, simulate_mips->reg[rt] & 0xffff);
      break;
    case 0x2b: // sw
      address = simulate_mips->reg[rs] + ((int16_t)(opcode & 0xffff));
      if ((address & 3) != 0)
      {
        printf("Alignment error.  Reading address 0x%04x\n", address);
        return -2;
      }
      memory_write32_m(simulate->memory, address, simulate_mips->reg[rt]);
      break;
    default:
      return -1;
  }

  return 0;
}

static int simulate_execute_mips(struct _simulate *simulate)
{
  struct _simulate_mips *simulate_mips = (struct _simulate_mips *)simulate->context;

  uint32_t opcode = memory_read32_m(simulate->memory, simulate_mips->pc);

  switch(opcode >> 26)
  {
    case 0x00:
      if (((opcode >> 6) & 0x1f) == 0)
      {
        if (simulate_execute_mips_r(simulate, opcode) == 0) { break; }
      }
        else
      if (((opcode >> 21) & 0x1f) == 0)
      {
        if (simulate_execute_mips_shift(simulate, opcode) == 0) { break; }
      }

      if (((opcode >> 11) & 0x3ff) == 0 && (opcode & 0x3f) == 0x08)
      {
        // jr
        simulate_mips->pc = simulate_mips->reg[(opcode >> 21) & 0x1f];
        return 0;
      }

      return -1;
    case 0x02:
#if 0
      offset = opcode & 0x3ffffff;
      if ((offset & 0x2000000) != 0) { offset |= 0xfc000000; }
      offset = offset << 2;
      simulate_mips->pc += offset;
#endif
      simulate_mips->pc &= 0xfc000000;
      simulate_mips->pc |= ((opcode & 0x3ffffff) << 2);
      return 0;
    case 0x03:
      simulate_mips->reg[31] = simulate_mips->pc + 8;
      simulate_mips->pc &= 0xfc000000;
      simulate_mips->pc |= ((opcode & 0x3ffffff) << 2);
      return 0;
    default:
      if (simulate_execute_mips_i(simulate, opcode) == 0) { break; }
      return -1;
      break;
  }

  simulate_mips->reg[0] = 0;
  simulate_mips->pc += 4;

  return 0;
}

int simulate_run_mips(struct _simulate *simulate, int max_cycles, int step)
{
  struct _simulate_mips *simulate_mips = (struct _simulate_mips *)simulate->context;
  char instruction[128];
  int pc;

  stop_running = 0;
  signal(SIGINT, handle_signal);

  while(stop_running == 0)
  {
    pc = simulate_mips->pc;

    int ret = simulate_execute_mips(simulate);

    if (ret == -1)
    {
      printf("Illegal instruction at address 0x%04x\n", simulate_mips->pc);
      return -1;
    }

    simulate->cycle_count++;

    if (simulate->show == 1)
    {
      simulate_dump_registers_mips(simulate);

      int n = 0;
      while(n < 6)
      {
        int cycles_min, cycles_max;

        uint32_t opcode = memory_read32_m(simulate->memory, pc);
        int count = disasm_mips32(simulate->memory, pc, instruction, &cycles_min, &cycles_max);

        if (count < 1) { break; }

        if (pc == simulate->break_point) { printf("*"); }
        else { printf(" "); }

        if (n == 0)
        { printf("! "); }
          else
        if (pc == simulate_mips->pc) { printf("> "); }
          else
        { printf("  "); }

        printf("0x%04x: 0x%08x %-40s %d\n", pc, opcode, instruction, cycles_min);

        pc += 4;
        n++;
      }
    }

    if (simulate->break_point == simulate_mips->pc)
    {
       printf("Breakpoint hit at 0x%04x\n", simulate->break_point);
      break;
    }

    if (simulate->usec == 0 || step == 1)
    {
      signal(SIGINT, SIG_DFL);
      return 0;
    }
  }

  signal(SIGINT, SIG_DFL);

  printf("Stopped.  PC=0x%04x.\n", simulate_mips->pc);
  printf("%d clock cycles have passed since last reset.\n", simulate->cycle_count);

  return 0;
}


