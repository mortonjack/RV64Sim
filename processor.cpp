/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class members for processor

**************************************************************** */

#include <iostream>
#include <iomanip> 
#include "memory.h"
#include "processor.h"

enum Opcode : uint8_t {
    LUI     =   0x37, // 0b0110111, // LUI
    AUIPC   =   0x17, // 0b0010111, // AUIPIC
    JAL     =   0x6f, // 0b1101111, // JAL
    JALR    =   0x67, // 0b1100111, // JALR
    BRANCH  =   0x63, // 0b1100011, // BEQ, BNE, BLT, BGE, BLTU, BGEU
    LOAD    =   0x03, // 0b0000011, // LB, LH, LW, LBU, LHU | LWU, LD
    STORE   =   0x23, // 0b0100011, // SB, SH, SW | SD
    ARITH_I =   0x13, // 0b0010011, // ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI | SLLI, SRLI, SRAI
    ARITH   =   0x33, // 0b0110011, // ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
    FENCE   =   0x0f, // 0b0001111, // FENCE 
    E_INSTR =   0x73, // 0b1110011, // ECALL, EBREAK
    ARITH_W =   0x1b, // 0b0011011  // ADDIW, SLLIW, SRLIW, SRAIW, ADDW, SUBW, SLLW, SRLW, SRAW
};

constexpr uint64_t upper32(uint64_t doubleword) {
    return 0xFFFFFFFF00000000ULL & doubleword;
}

constexpr uint64_t lower32(uint64_t doubleword) {
    return 0xFFFFFFFFULL & doubleword;
}

constexpr int64_t uimmediate(uint32_t instruction) {
    // Casting a uint32_t to an int64_t won't sign extend
    return int32_t(instruction & 0xfffff000);
}

// Execute a number of instructions
void processor::execute(unsigned int num, bool breakpoint_check) {
    breakpoint_check = breakpoint_check && this->has_breakpoint;
    while (num-- && !(breakpoint_check && this->pc == this->breakpoint)) {
        // Fetch
        uint32_t instruction = this->fetch();
        uint64_t original_pc = this->pc;
        
        // Decode
        // Execute
        this->execute(instruction);

        // Increment program counter
        if (this->pc == original_pc) this->pc += 4;
    }
}

void processor::execute(uint32_t instruction) {
    Opcode opcode = static_cast<Opcode>(uint8_t(instruction & 0x7f));
    uint8_t funct3 = (instruction >> 12) & 0x7;
    size_t rd = (instruction >> 7) & 0x1f;
    size_t rs1 = (instruction >> 15) & 0x1f;
    size_t rs2 = (instruction >> 20) & 0x1f;
    int64_t immediate_signed = 0;
    uint64_t immediate_unsigned = 0;
    switch (opcode) {
        case Opcode::LUI: // LUI
            immediate_signed = uimmediate(instruction);
            this->set_reg(rd, immediate_signed);
            break;
        case Opcode::AUIPC: // AUIPIC
            immediate_signed = uimmediate(instruction);
            immediate_signed += this->pc;
            this->set_reg(rd, immediate_signed);
            break;
        case Opcode::JAL: // JAL
            break;
        case Opcode::JALR: // JALR
            break;
        case Opcode::BRANCH: // BEQ, BNE, BLT, BGE, BLTU, BGEU
            break;
        case Opcode::LOAD: // LB, LH, LW, LBU, LHU | LWU, LD
            break;
        case Opcode::STORE: // SB, SH, SW | SD
            break;
        case Opcode::ARITH_I: // ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI | SLLI, SRLI, SRAI
            break;
        case Opcode::ARITH: // ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
            break;
        case Opcode::ARITH_W: // ADDIW, SLLIW, SRLIW, SRAIW, ADDW, SUBW, SLLW, SRLW, SRAW
            break;
        case Opcode::FENCE: // FENCE
            break;
        case Opcode::E_INSTR: // ECALL, EBREAK
            break;
    }
}

uint32_t processor::fetch() {
    uint64_t doubleword = this->main_memory->read_doubleword(this->pc);
    return (this->pc & 0x4) ? upper32(doubleword) >> 32 : lower32(doubleword);
}

// Consructor
processor::processor (memory* main_memory, bool verbose, bool stage2): 
    verbose(verbose),
    instruction_count(0), 
    pc(0), 
    has_breakpoint(false),
    registers(),
    main_memory(main_memory)
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
void processor::show_prv() 
{}

// Set privilege level
// Empty implementation for stage 1, required for stage 2
void processor::set_prv(unsigned int prv_num) 
{}

// Display CSR value
// Empty implementation for stage 1, required for stage 2
void processor::show_csr(unsigned int csr_num)
{}

// Set CSR to new value
// Empty implementation for stage 1, required for stage 2
void processor::set_csr(unsigned int csr_num, uint64_t new_value) 
{}

uint64_t processor::get_instruction_count() {
    return instruction_count;
}

// Used for Postgraduate assignment. Undergraduate assignment can return 0.
uint64_t processor::get_cycle_count() {
    return 0;
}

