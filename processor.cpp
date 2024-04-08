/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class members for processor

**************************************************************** */

#include <iostream>
#include <iomanip> 
#include "memory.h"
#include "processor.h"

// Consructor
processor::processor (memory* main_memory, bool verbose, bool stage2): 
    instruction_count(0), 
    pc(0), 
    has_breakpoint(false),
    registers()
{}

// Display PC value
void processor::show_pc() {
    std::cout << std::setw(16) << std::setfill('0') << std::hex << pc << std::endl;
}

// Set PC to new value
void processor::set_pc(uint64_t new_pc) {
    pc = new_pc;
}

// Display register value
void processor::show_reg(unsigned int reg_num) {
    std::cout << std::setw(16) << std::setfill('0') << std::hex << registers[reg_num] << std::endl;
}

// Set register to new value
void processor::set_reg(unsigned int reg_num, uint64_t new_value) {
    registers[reg_num] = new_value;
    registers[0] = 0;
}

// Execute a number of instructions
void processor::execute(unsigned int num, bool breakpoint_check) {
}

// Clear breakpoint
void processor::clear_breakpoint() {
    has_breakpoint = false;
}

// Set breakpoint at an address
void processor::set_breakpoint(uint64_t address) {
    has_breakpoint = true;
    breakpoint = address;
}

// Show privilege level
// Empty implementation for stage 1, required for stage 2
void processor::show_prv() {
}

// Set privilege level
// Empty implementation for stage 1, required for stage 2
void processor::set_prv(unsigned int prv_num) {
}

// Display CSR value
// Empty implementation for stage 1, required for stage 2
void processor::show_csr(unsigned int csr_num) {
}

// Set CSR to new value
// Empty implementation for stage 1, required for stage 2
void processor::set_csr(unsigned int csr_num, uint64_t new_value) {
}

uint64_t processor::get_instruction_count() {
    return instruction_count;
}

// Used for Postgraduate assignment. Undergraduate assignment can return 0.
uint64_t processor::get_cycle_count() {
    return 0;
}

