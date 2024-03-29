#ifndef PROCESSOR_H
#define PROCESSOR_H

/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class for processor

**************************************************************** */

#include "memory.h"
#include <array>

using namespace std;

class processor {

 private:

  uint64_t instruction_count;
  uint64_t pc;
  // We are using C++11 so unfortunately I can't use optional.
  bool has_breakpoint;
  uint64_t breakpoint;
  array<uint64_t, 32> registers;
  
  // Raw pointer - this is just a view! This exists for the lifetime of the program!
  // We do not have ownership over this object! Do not free it!
  //
  // Oh and we do not clean this up in our main function. We are bad programmers :3
  // Srsly why not just allocate on the stack??
  // Okay I might clean this up myself....
  memory* main_memory;

 public:

  // Consructor
  processor(memory* main_memory, bool verbose, bool stage2);

  // Display PC value
  void show_pc();

  // Set PC to new value
  void set_pc(uint64_t new_pc);

  // Display register value
  void show_reg(unsigned int reg_num);

  // Set register to new value
  void set_reg(unsigned int reg_num, uint64_t new_value);

  // Execute a number of instructions
  void execute(unsigned int num, bool breakpoint_check);

  // Clear breakpoint
  void clear_breakpoint();

  // Set breakpoint at an address
  void set_breakpoint(uint64_t address);

  // Show privilege level
  // Empty implementation for stage 1, required for stage 2
  void show_prv();

  // Set privilege level
  // Empty implementation for stage 1, required for stage 2
  void set_prv(unsigned int prv_num);

  // Display CSR value
  // Empty implementation for stage 1, required for stage 2
  void show_csr(unsigned int csr_num);

  // Set CSR to new value
  // Empty implementation for stage 1, required for stage 2
  void set_csr(unsigned int csr_num, uint64_t new_value);

  uint64_t get_instruction_count();

  // Used for Postgraduate assignment. Undergraduate assignment can return 0.
  uint64_t get_cycle_count();

};

#endif
