/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class members for processor

**************************************************************** */

#include <iostream>
#include <iomanip> 
#include <stdlib.h>

#include "memory.h"
#include "processor.h"

using namespace std;

// Consructor
processor::processor (memory* main_memory, bool verbose, bool stage2) {

	// TODO

}

// TODO

// Display PC value
void processor::show_pc() {
}

// Set PC to new value
void processor::set_pc(uint64_t new_pc) {
}

// Display register value
void processor::show_reg(unsigned int reg_num) {
}

// Set register to new value
void processor::set_reg(unsigned int reg_num, uint64_t new_value) {
}

// Execute a number of instructions
void processor::execute(unsigned int num, bool breakpoint_check) {
}

// Clear breakpoint
void processor::clear_breakpoint() {
}

// Set breakpoint at an address
void processor::set_breakpoint(uint64_t address) {
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
}

// Used for Postgraduate assignment. Undergraduate assignment can return 0.
uint64_t processor::get_cycle_count() {
}

