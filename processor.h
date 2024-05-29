#ifndef PROCESSOR_H
#define PROCESSOR_H

/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class for processor

**************************************************************** */

#include "memory.h"
#include <array>

class processor {
public:
  enum class CSR : uint32_t {
    mvendorid   = 0xF11, // MRO
    marchid     = 0xF12,
    mimpid      = 0xF13,
    mhartid     = 0xF14,
    mstatus     = 0x300, // MRW
    misa        = 0x301,
    mie         = 0x304,
    mtvec       = 0x305,
    mscratch    = 0x340,
    mepc        = 0x341,
    mcause      = 0x342,
    mtval       = 0x343,
    mip         = 0x344,
  };

private:
  bool verbose;

  uint64_t instruction_count;
  uint64_t pc;
  // We are using C++11 so unfortunately I can't use optional.
  bool has_breakpoint;
  uint64_t breakpoint;
  std::array<uint64_t, 32> registers;

  // We do not have ownership over this object! Do not free it!
  memory *main_memory;

  // Control and Status Registers
  uint64_t mstatus;
  uint64_t mie;
  uint64_t mtvec;
  uint64_t mscratch;
  uint64_t mepc;
  uint64_t mcause;
  uint64_t mtval;
  uint64_t mip;

  // use uint16_t or cout will try to print a char
  enum class Privilege : uint16_t {
    User = 0,
    Machine = 3,
  };

  Privilege privilege;

  uint32_t fetch();
  void execute(uint32_t instruction);
  void load(uint8_t width, size_t dest, size_t base, int64_t offset);
  void store(uint8_t width, size_t src, size_t base, int64_t offset);
  bool system(uint32_t csr, size_t src, size_t dest, uint8_t funct3);
  void exception_handler();
  void update_privilege(bool mret);
  Privilege get_prv();
  uint64_t read_csr(uint32_t csr);
  void write_csr(CSR csr, uint64_t new_value);

public:
  // Consructor
  processor(memory *main_memory, bool verbose, bool stage2);

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
