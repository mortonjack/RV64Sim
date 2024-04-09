/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class members for processor

**************************************************************** */

#include <iostream>
#include <iomanip> 
#include "memory.h"
#include "processor.h"

constexpr uint64_t upper32(uint64_t doubleword) {
    return 0xFFFFFFFF00000000ULL & doubleword;
}

constexpr uint64_t lower32(uint64_t doubleword) {
    return 0xFFFFFFFFULL & doubleword;
}

constexpr int64_t upper_immediate(uint32_t instruction) {
    return int32_t(instruction & 0xfffff000);
}

constexpr int64_t immediate_11_0(uint32_t instruction) {
    return int32_t(instruction & 0xfff00000) >> 20;
}

// Execute a number of instructions
void processor::execute(unsigned int num, bool breakpoint_check) {
    breakpoint_check = breakpoint_check && this->has_breakpoint;
    while (num--) {
        // Stop execution early
        if (breakpoint_check && this->pc == this->breakpoint) {
            std::cout << "Breakpoint reached at " << std::setw(16) << std::setfill('0') << std::hex << this->pc << std::endl;
            break;
        }
        if (this->pc & 0x3) {
            std::cout << "Error: misaligned pc" << std::endl;
            break;
        }

        // Fetch
        uint32_t instruction = this->fetch();
        uint64_t original_pc = this->pc;
        
        // Decode
        // Execute
        this->execute(instruction);

        // Increment program counter
        if (this->pc == original_pc) this->pc += 4;
        this->instruction_count += 1;
    }
}

bool take_branch(uint8_t funct3, uint64_t lval, uint64_t rval) {
    enum Branch_Type : uint8_t {
        BEQ     =   0x0, // 0b000
        BNE     =   0x1, // 0b001
        BLT     =   0x4, // 0b100
        BGE     =   0x5, // 0b101
        BLTU    =   0x6, // 0b110
        BGEU    =   0x7, // 0b111
    };

    switch (static_cast<Branch_Type>(funct3)) {
        case Branch_Type::BEQ:  return lval == rval;
        case Branch_Type::BNE:  return lval != rval;
        case Branch_Type::BLTU: return lval <  rval;
        case Branch_Type::BGEU: return lval >= rval;
        case Branch_Type::BLT:  return int64_t(lval) <  int64_t(rval);
        case Branch_Type::BGE:  return int64_t(lval) >= int64_t(rval);
        default: return false;
    }
}

void processor::load(uint8_t width, size_t dest, size_t base, int64_t offset) {
    bool has_sign = !(width & 0x4);
    int64_t address = int64_t(this->registers[base]) + offset;
    uint64_t doubleword = this->main_memory->read_doubleword(address);
    uint8_t shift = (uint64_t(address) % 8) * 8;
    switch (width & 0x3) {
        case 0x0: // LB, LBU (1 byte)
            doubleword &= 0x00000000000000ff << shift;
            if (has_sign) doubleword = int64_t(doubleword << (56 - shift)) >> (56 - shift);
            break;
        case 0x1: // LH, LHU (2 bytes)
            shift &= 0x18;
            doubleword &= 0x000000000000ffff << shift;
            if (has_sign) doubleword = int64_t(doubleword << (48 - shift)) >> (48 - shift);
            break;
        case 0x2: // LW, LWU (4 bytes)
            shift &= 0x10;
            doubleword &= 0x00000000ffffffff;
            if (has_sign) doubleword = int64_t(doubleword << (32 - shift)) >> (32 - shift);
            break;
        case 0x3: // LD (8 bytes)
            break;
    }
    this->set_reg(dest, doubleword);
}

void processor::store(uint8_t width, size_t src, size_t base, int64_t offset) {
    int64_t address = int64_t(this->registers[base]) + offset;
    uint64_t doubleword = this->registers[src];
    uint64_t mask = 0;
    uint8_t shift = (uint64_t(address) % 8) * 8;
    switch (width & 0x3) {
        case 0x0: // SB
            shift &= 0x1c;
            mask = 0x00000000000000ff;
            break;
        case 0x1: // SH
            shift &= 0x18;
            mask = 0x000000000000ffff;
            break;
        case 0x2: // SW
            shift &= 0x10;
            mask = 0x00000000ffffffff;
            break;
        case 0x3: // SD
            shift = 0;
            mask = 0xffffffffffffffff;
            break;
    }
    this->main_memory->write_doubleword(address, doubleword << shift, mask << shift);
}
            
uint64_t op(uint8_t funct7, uint8_t funct3, uint64_t rs1, uint64_t rs2) {
    // Note: funct7 only has two possible values, 0b0100000 and 0
    enum Op_Type : uint8_t {
        ADD     =   0x00, // 0b00 ... 000
        SUB     =   0x20, // 0b01 ... 000
        SLL     =   0x01, // 0b00 ... 001
        SLT     =   0x02, // 0b00 ... 010
        SLTU    =   0x03, // 0b00 ... 011
        XOR     =   0x04, // 0b00 ... 100
        SRL     =   0x05, // 0b00 ... 101
        SRA     =   0x25, // 0b01 ... 101
        OR      =   0x06, // 0b00 ... 110
        AND     =   0x07, // 0b00 ... 111
    };
    uint8_t op_type = funct7 | funct3;
    int64_t ri1 = rs1, ri2 = rs2;
    switch (static_cast<Op_Type>(op_type)) {
        case Op_Type::ADD:  return rs1 + rs2;
        case Op_Type::SUB:  return rs1 - rs2;
        case Op_Type::XOR:  return rs1 ^ rs2;
        case Op_Type::OR:   return rs1 | rs2;
        case Op_Type::AND:  return rs1 & rs2;
        case Op_Type::SLT:  return rs1 < rs2 ? 1 : 0;
        case Op_Type::SLTU: return ri1 < ri2 ? 1 : 0;
        case Op_Type::SLL:  return rs1 << (rs2 & 0x1f);
        case Op_Type::SRL:  return rs1 >> (rs2 & 0x1f);
        case Op_Type::SRA:  return ri1 >> (rs2 & 0x1f);
        default: return 0;
    }
}
            
uint64_t op_imm(uint8_t funct3, uint64_t rs1, uint64_t immediate) {
    enum Op_Type : uint8_t {
        ADDI    =   0x00, // 0b000
        SLLI    =   0x01, // 0b001
        SLTI    =   0x02, // 0b010
        SLTIU   =   0x03, // 0b011
        XORI    =   0x04, // 0b100
        SRLI    =   0x05, // 0b101
        ORI     =   0x06, // 0b110
        ANDI    =   0x07, // 0b111
        SRAI    =   0x08, // custom - SRAI & immediate[10]
    };
    Op_Type op_type = static_cast<Op_Type>(funct3);
    uint8_t shamt = immediate & 0x3f;
    int64_t ri1 = rs1, i_imm = immediate;
    if (op_type == Op_Type::SRLI && (immediate & 0x400)) op_type = Op_Type::SRAI;
    switch (op_type) {
        case Op_Type::ADDI:  return rs1 +  immediate;
        case Op_Type::XORI:  return rs1 ^  immediate;
        case Op_Type::ORI:   return rs1 |  immediate;
        case Op_Type::ANDI:  return rs1 &  immediate;
        case Op_Type::SLLI:  return rs1 << shamt;
        case Op_Type::SRLI:  return rs1 >> shamt;
        case Op_Type::SRAI:  return ri1 >> shamt;
        case Op_Type::SLTI:  return ri1 <  i_imm     ? 1 : 0;
        case Op_Type::SLTIU: return rs1 <  immediate ? 1 : 0;
        default: return 0;
    }
}

uint64_t op_imm_32(uint8_t funct3, uint64_t rs1, uint64_t immediate) {
    enum Op_Type : uint8_t {
        ADDIW   =   0x00, // 0b000
        SLLIW   =   0x01, // 0b001
        SRLIW   =   0x05, // 0b101
        SRAIW   =   0x08, // custom - SRLIW & immediate[10]
    };
    Op_Type op_type = static_cast<Op_Type>(funct3);
    if (op_type == Op_Type::SRLIW && (immediate & 0x400)) op_type = Op_Type::SRAIW;
    uint8_t shamt = immediate & 0x1f;
    int32_t ri1 = rs1, i_imm = immediate;
    uint32_t ru1 = rs1;
    switch (op_type) {
        case Op_Type::ADDIW: return int64_t(int32_t(ri1 + i_imm));
        case Op_Type::SLLIW: return int64_t(int32_t(ri1 << shamt));
        case Op_Type::SRLIW: return int64_t(int32_t(ru1 >> shamt));
        case Op_Type::SRAIW: return int64_t(int32_t(ri1 >> shamt));
        default: return 0;
    }
}

uint64_t op_32(uint8_t funct7, uint8_t funct3, uint64_t rs1, uint64_t rs2) {
    enum Op_Type : uint8_t {
        ADDW    =   0x00, // 0b00 ... 000
        SUBW    =   0x20, // 0b01 ... 000
        SLLW    =   0x01, // 0b00 ... 001
        SRLW    =   0x05, // 0b00 ... 101
        SRAW    =   0x25, // 0b01 ... 101
    };
    Op_Type op_type = static_cast<Op_Type>(funct3 | funct7);
    int32_t ri1 = rs1;
    uint32_t ru1 = rs1, ru2 = rs2;
    uint8_t shamt = rs2 & 0x1f;
    switch (op_type) {
        case Op_Type::ADDW: return int64_t(int32_t(ru1 + ru2));
        case Op_Type::SUBW: return int64_t(int32_t(ru1 - ru2));
        case Op_Type::SLLW: return int64_t(int32_t(ri1 << shamt));
        case Op_Type::SRLW: return int64_t(int32_t(ru1 >> shamt));
        case Op_Type::SRAW: return int64_t(int32_t(ri1 >> shamt));
        default: return 0;
    }
}

void processor::execute(uint32_t instruction) {
    enum Opcode : uint8_t {
        LUI      =  0x37, // 0b0110111, // LUI
        AUIPC    =  0x17, // 0b0010111, // AUIPIC
        JAL      =  0x6f, // 0b1101111, // JAL
        JALR     =  0x67, // 0b1100111, // JALR
        BRANCH   =  0x63, // 0b1100011, // BEQ, BNE, BLT, BGE, BLTU, BGEU
        LOAD     =  0x03, // 0b0000011, // LB, LH, LW, LBU, LHU | LWU, LD
        STORE    =  0x23, // 0b0100011, // SB, SH, SW | SD
        OP_IMM   =  0x13, // 0b0010011, // ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI | SLLI, SRLI, SRAI
        OP       =  0x33, // 0b0110011, // ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
        MISC_MEM =  0x0f, // 0b0001111, // FENCE 
        SYSTEM   =  0x73, // 0b1110011, // ECALL, EBREAK
        OP_IMM32 =  0x1b, // 0b0011011  // ADDIW, SLLIW, SRLIW, SRAIW
        OP_32    =  0x3b, // 0b0111011  // ADDW, SUBW, SLLW, SRLW, SRAW
    };

    Opcode opcode = static_cast<Opcode>(uint8_t(instruction & 0x7f));
    uint8_t funct3 = (instruction >> 12) & 0x7;
    uint8_t funct7 = (instruction >> 25) & 0x7f;
    size_t rd = (instruction >> 7) & 0x1f;
    size_t rs1 = (instruction >> 15) & 0x1f;
    size_t rs2 = (instruction >> 20) & 0x1f;
    int64_t immediate = 0;
    switch (opcode) {
        case Opcode::LUI: // LUI
            immediate = upper_immediate(instruction);
            this->set_reg(rd, immediate);
            break;
        case Opcode::AUIPC: // AUIPIC
            immediate = upper_immediate(instruction);
            immediate += this->pc;
            this->set_reg(rd, immediate);
            break;
        case Opcode::JAL: // JAL
            // Weird immediate encoding needed! 20|10:1|11|19:12
            immediate  = int32_t (instruction & 0x80000000) >> 11; 
            immediate |= int32_t (instruction & 0x7fe00000) >> 20;
            immediate |= int32_t (instruction & 0x00100000) >> 9;
            immediate |= int32_t (instruction & 0x000ff000);
            // Store return address in rd & update PC
            this->set_reg(rd, this->pc+4);
            this->pc += immediate;
            break;
        case Opcode::JALR: // JALR
            immediate = immediate_11_0(instruction);
            immediate += this->registers[rs1];
            // Store return address in rd & update PC
            this->set_reg(rd, this->pc+4);
            this->pc = uint64_t(immediate) & 0xfffffffffffffffeULL;
            break;
        case Opcode::BRANCH: // BEQ, BNE, BLT, BGE, BLTU, BGEU
            // Weird immediate encoding needed! 12|10:5, 4:1|11
            immediate  = int32_t (instruction & 0x80000000) >> 19;
            immediate |= int32_t (instruction & 0x7e000000) >> 20;
            immediate |= int32_t (instruction & 0x00000f00) >> 7;
            immediate |= int32_t (instruction & 0x00000080) << 4;
            if (take_branch(funct3, this->registers[rs1], this->registers[rs2])) {
                this->pc += uint64_t(immediate);
            }
            break;
        case Opcode::LOAD: // LB, LH, LW, LBU, LHU | LWU, LD
            immediate  = int32_t (instruction & 0xfff00000) >> 20;
            this->load(funct3, rd, rs1, immediate);
            break;
        case Opcode::STORE: // SB, SH, SW | SD
            immediate  = int32_t (instruction & 0xfe000000) >> 20;
            immediate |= int32_t (instruction & 0x00000f80) >> 7;
            this->store(funct3, rs2, rs1, immediate);
            break;
        case Opcode::OP_IMM : // ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI | SLLI, SRLI, SRAI
            immediate  = int32_t (instruction & 0xfff00000) >> 20;
            this->registers[rd] = op_imm(funct3, this->registers[rs1], immediate);
            break;
        case Opcode::OP: // ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
            this->registers[rd] = op(funct7, funct3, this->registers[rs1], this->registers[rs2]);
            break;
        case Opcode::OP_IMM32: // ADDIW, SLLIW, SRLIW, SRAIW
            immediate  = int32_t (instruction & 0xfff00000) >> 20;
            this->registers[rd] = op_imm_32(funct3, this->registers[rs1], immediate);
            break;
        case Opcode::OP_32: // ADDW, SUBW, SLLW, SRLW, SRAW
            this->registers[rd] = op_32(funct7, funct3, this->registers[rs1], this->registers[rs2]);
            break;
        case Opcode::MISC_MEM: // FENCE
            break;
        case Opcode::SYSTEM: // ECALL, EBREAK
            std::cout << "Error! System instructions are currently unimplemented." << std::endl;
            break;
        default:
            if (this->verbose) {
                std::cout << "Malformed opcode 0x" << std::setw(2) << std::setfill('0') << std::hex << opcode << std::endl;
            }
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

