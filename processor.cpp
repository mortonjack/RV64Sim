/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class members for processor

**************************************************************** */

#include <iostream>
#include <iomanip> 
#include <array>
#include "memory.h"
#include "processor.h"

using CSR = processor::CSR;

constexpr uint64_t upper32(uint64_t doubleword) {
    return 0xffffffff00000000ULL & doubleword;
}

constexpr uint64_t lower32(uint64_t doubleword) {
    return 0xffffffffULL & doubleword;
}

constexpr int64_t upper_immediate(uint32_t instruction) {
    return static_cast<int32_t>(instruction & 0xfffff000);
}

constexpr int64_t immediate_11_0(uint32_t instruction) {
    return static_cast<int32_t>(instruction & 0xfff00000) >> 20;
}

bool valid_csr(uint32_t csr_num) {
    switch (static_cast<CSR>(csr_num)) {
        case CSR::mvendorid:
        case CSR::marchid:
        case CSR::mimpid:
        case CSR::mhartid:
        case CSR::mstatus:
        case CSR::misa:
        case CSR::mie:
        case CSR::mtvec:
        case CSR::mscratch:
        case CSR::mepc:
        case CSR::mcause:
        case CSR::mtval:
        case CSR::mip:
            return true;
        default:
            return false;
    }
}

// Execute a number of instructions
void processor::execute(unsigned int num, bool breakpoint_check) {
    breakpoint_check = breakpoint_check && this->has_breakpoint;
    while (num--) {
        // Stop execution early
        //if (this->verbose) std::cout << "Running instruction at 0x" << std::setw(16) << std::setfill('0') << std::hex << this->pc << std::endl;
        if (breakpoint_check && this->pc == this->breakpoint) {
            std::cout << "Breakpoint reached at " << std::setw(16) << std::setfill('0') << std::hex << this->pc << std::endl;
            break;
        }
        
        // Check interrupts
        if ((this->mstatus & 0x8) || this->privilege == Privilege::User) {
            // mip.usip && mie.usie -> cause code 0, bitfield 0
            // mip.msip && mie.msie -> cause code 3, bitfield 3
            // mip.utip && mie.utie -> cause code 4, bitfield 4
            // mip.mtip && mie.mtie -> cause code 7, bitfield 7
            // mip.ueip && mie.ueie -> cause code 8, bitfield 8
            // mip.meip && mie.meie -> cause code 11, bitfield 11
            const std::array<uint8_t, 6> interrupt_bits = {11, 3, 7, 8, 0, 4};
            for (uint64_t bit: interrupt_bits) {
                if (((this->mip >> bit) & 1ULL) && ((this->mie >> bit) & 1ULL)) {
                    uint64_t cause = (1ULL << 63ULL) | bit;
                    this->write_csr(CSR::mtval, 0);
                    this->write_csr(CSR::mcause, cause);
                    this->exception_handler();
                    break;
                }
            }
        }

        // Fetch
        if (this->pc & 0x3) {
            this->write_csr(CSR::mtval, this->pc);
            this->write_csr(CSR::mcause, 0);
            this->exception_handler();
            continue;
        }
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

bool take_branch(uint8_t funct3, uint64_t lval, uint64_t rval, bool& illegal_instruction) {
    enum class Branch_Type : uint8_t {
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
        case Branch_Type::BLT:  return static_cast<int64_t>(lval) <  static_cast<int64_t>(rval);
        case Branch_Type::BGE:  return static_cast<int64_t>(lval) >= static_cast<int64_t>(rval);
        default: 
            illegal_instruction = true;
            return false;
    }
}

void processor::load(uint8_t width, size_t dest, size_t base, int64_t offset) {
    bool has_sign = !(width & 0x4);
    int64_t address = static_cast<int64_t>(this->registers[base]) + offset;
    uint64_t doubleword = this->main_memory->read_doubleword(address);
    uint8_t shift = (static_cast<uint64_t>(address) % 8) * 8;
    bool misaligned = false;
    switch (width & 0x3) {
        case 0x0: // LB, LBU (1 byte)
            doubleword &= 0x00000000000000ffULL << shift;
            doubleword >>= shift;
            if (has_sign) doubleword = static_cast<int64_t>(doubleword << 56) >> 56;
            break;
        case 0x1: // LH, LHU (2 bytes)
            misaligned = shift != (shift & 48);
            doubleword &= 0x000000000000ffffULL << shift;
            doubleword >>= shift;
            if (has_sign) doubleword = static_cast<int64_t>(doubleword << 48) >> 48;
            break;
        case 0x2: // LW, LWU (4 bytes)
            misaligned = shift != (shift & 32);
            doubleword &= 0x00000000ffffffffULL << shift;
            doubleword >>= shift;
            if (has_sign) doubleword = static_cast<int64_t>(doubleword << 32) >> 32;
            break;
        case 0x3: // LD (8 bytes)
            misaligned = shift != 0;
            break;
    }
    if (misaligned) {
        this->write_csr(CSR::mtval, address);
        this->write_csr(CSR::mcause, 4);
        --this->instruction_count;
        this->exception_handler();
    } else {
        this->set_reg(dest, doubleword);
    }
}

void processor::store(uint8_t width, size_t src, size_t base, int64_t offset) {
    int64_t address = static_cast<int64_t>(this->registers[base]) + offset;
    uint64_t doubleword = this->registers[src];
    uint64_t mask = 0;
    uint8_t shift = (static_cast<uint64_t>(address) % 8) * 8;
    bool misaligned = false;
    switch (width & 0x3) {
        case 0x0: // SB
            mask = 0x00000000000000ffULL;
            break;
        case 0x1: // SH
            misaligned = shift != (shift & 0x30);
            mask = 0x000000000000ffffULL;
            break;
        case 0x2: // SW
            misaligned = shift != (shift & 0x20);
            mask = 0x00000000ffffffffULL;
            break;
        case 0x3: // SD
            misaligned = shift != 0;
            mask = 0xffffffffffffffffULL;
            break;
    }
    if (misaligned) {
        this->write_csr(CSR::mtval, address);
        this->write_csr(CSR::mcause, 6);
        --this->instruction_count;
        this->exception_handler();
    } else {
        doubleword <<= shift;
        mask <<= shift;
        this->main_memory->write_doubleword(address, doubleword, mask);
    }
}
            
uint64_t op(uint8_t funct7, uint8_t funct3, uint64_t rs1, uint64_t rs2, bool& illegal_instruction) {
    // Note: funct7 only has two possible values, 0b0100000 and 0
    if (funct7 != 0 && funct7 != 0x20) illegal_instruction = true;
    enum class Op_Type : uint8_t {
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
        case Op_Type::SLT:  return ri1 < ri2 ? 1 : 0;
        case Op_Type::SLTU: return rs1 < rs2 ? 1 : 0;
        case Op_Type::SLL:  return rs1 << (rs2 & 0x3f);
        case Op_Type::SRL:  return rs1 >> (rs2 & 0x3f);
        case Op_Type::SRA:  return ri1 >> (rs2 & 0x3f);
        default: 
            illegal_instruction = true;
            return 0;
    }
}
            
uint64_t op_imm(uint8_t funct3, uint64_t rs1, uint64_t immediate, bool& illegal_instruction) {
    enum class Op_Type : uint8_t {
        ADDI    =   0x00, // 0b000
        SLLI    =   0x01, // 0b001
        SLTI    =   0x02, // 0b010
        SLTIU   =   0x03, // 0b011
        XORI    =   0x04, // 0b100
        SRLI    =   0x05, // 0b101
        ORI     =   0x06, // 0b110
        ANDI    =   0x07, // 0b111
        SRAI    =   0x08, // custom - SRLI & immediate[10]
    };
    Op_Type op_type = static_cast<Op_Type>(funct3);
    uint8_t shamt = immediate & 0x3f;
    int64_t ri1 = rs1, i_imm = immediate;
    if (op_type == Op_Type::SRLI && (immediate >> 6) == 0x10) {
        op_type = Op_Type::SRAI;
    }
    switch (op_type) {
        case Op_Type::ADDI:
            return rs1 + immediate;
        case Op_Type::XORI:
            return rs1 ^ immediate;
        case Op_Type::ORI:
            return rs1 | immediate;
        case Op_Type::ANDI:
            return rs1 & immediate;
        case Op_Type::SLLI:
            illegal_instruction = (immediate >> 6) != 0;
            return rs1 << shamt;
        case Op_Type::SRLI:
            illegal_instruction = (immediate >> 6) != 0;
            return rs1 >> shamt;
        case Op_Type::SRAI:
            illegal_instruction = (immediate >> 6) != 0x10;
            return ri1 >> shamt;
        case Op_Type::SLTI:
            return ri1 < i_imm ? 1 : 0;
        case Op_Type::SLTIU:
            return rs1 < immediate ? 1 : 0;
        default: 
            illegal_instruction = true;
            return 0;
    }
}

uint64_t op_imm_32(uint8_t funct3, uint64_t rs1, uint64_t immediate, bool& illegal_instruction) {
    enum class Op_Type : uint8_t {
        ADDIW   =   0x00, // 0b000
        SLLIW   =   0x01, // 0b001
        SRLIW   =   0x05, // 0b101
        SRAIW   =   0x08, // custom - SRLIW & immediate[10]
    };
    Op_Type op_type = static_cast<Op_Type>(funct3);
    if (op_type == Op_Type::SRLIW && (immediate >> 5) == 0x20) {
        op_type = Op_Type::SRAIW;
    }
    uint8_t shamt = immediate & 0x1f;
    int32_t ri1 = rs1, i_imm = immediate;
    uint32_t ru1 = rs1;
    switch (op_type) {
        case Op_Type::ADDIW:
            return static_cast<int64_t>(static_cast<int32_t>(ri1 + i_imm));
        case Op_Type::SLLIW:
            illegal_instruction = (immediate >> 5) != 0;
            return static_cast<int64_t>(static_cast<int32_t>(ri1 << shamt));
        case Op_Type::SRLIW:
            illegal_instruction = (immediate >> 5) != 0;
            return static_cast<int64_t>(static_cast<int32_t>(ru1 >> shamt));
        case Op_Type::SRAIW:
            illegal_instruction = (immediate >> 5) != 0x20;
            return static_cast<int64_t>(static_cast<int32_t>(ri1 >> shamt));
        default: 
            illegal_instruction = true;
            return 0;
    }
}

uint64_t op_32(uint8_t funct7, uint8_t funct3, uint64_t rs1, uint64_t rs2, bool& illegal_instruction) {
    enum class Op_Type : uint8_t {
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
        case Op_Type::ADDW: return static_cast<int64_t>(static_cast<int32_t>(ru1 + ru2));
        case Op_Type::SUBW: return static_cast<int64_t>(static_cast<int32_t>(ru1 - ru2));
        case Op_Type::SLLW: return static_cast<int64_t>(static_cast<int32_t>(ri1 << shamt));
        case Op_Type::SRLW: return static_cast<int64_t>(static_cast<int32_t>(ru1 >> shamt));
        case Op_Type::SRAW: return static_cast<int64_t>(static_cast<int32_t>(ri1 >> shamt));
        default:
            illegal_instruction = true;
            return 0;
    }
}

void processor::execute(uint32_t instruction) {
    enum class Opcode : uint8_t {
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
        SYSTEM   =  0x73, // 0b1110011, // ECALL, EBREAK, CSR*
        OP_IMM32 =  0x1b, // 0b0011011  // ADDIW, SLLIW, SRLIW, SRAIW
        OP_32    =  0x3b, // 0b0111011  // ADDW, SUBW, SLLW, SRLW, SRAW
    };

    /*
    auto read_opcode = [](Opcode opcode){
        switch (opcode) {
            case Opcode::LUI:
                return "LUI";
            case Opcode::AUIPC:
                return "AUIPC";
            case Opcode::JAL:
                return "JAL";
            case Opcode::JALR:
                return "JALR";
            case Opcode::BRANCH:
                return "BRANCH";
            case Opcode::LOAD:
                return "LOAD";
            case Opcode::STORE:
                return "STORE";
            case Opcode::OP_IMM:
                return "OP_IMM";
            case Opcode::OP:
                return "OP";
            case Opcode::MISC_MEM:
                return "MISC_MEM";
            case Opcode::SYSTEM:
                return "SYSTEM";
            case Opcode::OP_IMM32:
                return "OP_IMM32";
            case Opcode::OP_32:
                return "OP_32";
            default:
                return "UNKNOWN OPCODE";
        }
    };
    */

    Opcode opcode = static_cast<Opcode>(static_cast<uint8_t>(instruction & 0x7f));
    uint8_t funct3 = (instruction >> 12) & 0x7;
    uint8_t funct7 = (instruction >> 25) & 0x7f;
    size_t rd = (instruction >> 7) & 0x1f;
    size_t rs1 = (instruction >> 15) & 0x1f;
    size_t rs2 = (instruction >> 20) & 0x1f;
    int64_t immediate = 0;
    bool illegal_instruction = false;
    /*
    if (this->verbose) {
        std::cout << read_opcode(opcode) 
            << " called from PC " << this->pc 
            << " and instruction count " << this->instruction_count 
            << std::endl;
    }
    */
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
            immediate  = static_cast<int32_t>(instruction & 0x80000000) >> 11; 
            immediate |= static_cast<int32_t>(instruction & 0x7fe00000) >> 20;
            immediate |= static_cast<int32_t>(instruction & 0x00100000) >> 9;
            immediate |= static_cast<int32_t>(instruction & 0x000ff000);
            // Store return address in rd & update PC
            this->set_reg(rd, this->pc+4);
            this->pc += immediate;
            break;
        case Opcode::JALR: // JALR
            immediate = immediate_11_0(instruction);
            immediate += this->registers[rs1];
            // Store return address in rd & update PC
            this->set_reg(rd, this->pc+4);
            this->pc = static_cast<uint64_t>(immediate) & 0xfffffffffffffffeULL;
            break;
        case Opcode::BRANCH: // BEQ, BNE, BLT, BGE, BLTU, BGEU
            // Weird immediate encoding needed! 12|10:5, 4:1|11
            immediate  = static_cast<int32_t>(instruction & 0x80000000) >> 19;
            immediate |= static_cast<int32_t>(instruction & 0x7e000000) >> 20;
            immediate |= static_cast<int32_t>(instruction & 0x00000f00) >> 7;
            immediate |= static_cast<int32_t>(instruction & 0x00000080) << 4;
            if (take_branch(funct3, this->registers[rs1], this->registers[rs2], illegal_instruction)) {
                if (illegal_instruction) break;
                this->pc += static_cast<uint64_t>(immediate);
            }
            break;
        case Opcode::LOAD: // LB, LH, LW, LBU, LHU | LWU, LD
            immediate  = static_cast<int32_t>(instruction & 0xfff00000) >> 20;
            this->load(funct3, rd, rs1, immediate);
            break;
        case Opcode::STORE: // SB, SH, SW | SD
            immediate  = static_cast<int32_t>(instruction & 0xfe000000) >> 20;
            immediate |= static_cast<int32_t>(instruction & 0x00000f80) >> 7;
            this->store(funct3, rs2, rs1, immediate);
            break;
        case Opcode::OP_IMM : // ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI | SLLI, SRLI, SRAI
            immediate = static_cast<int32_t>(instruction & 0xfff00000) >> 20;
            immediate = op_imm(funct3, this->registers[rs1], immediate, illegal_instruction);
            if (illegal_instruction) break;
            this->set_reg(rd, immediate);
            break;
        case Opcode::OP: // ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
            immediate = op(funct7, funct3, this->registers[rs1], this->registers[rs2], illegal_instruction);
            if (illegal_instruction) break;
            this->set_reg(rd, immediate);
            break;
        case Opcode::OP_IMM32: // ADDIW, SLLIW, SRLIW, SRAIW
            immediate = static_cast<int32_t>(instruction & 0xfff00000) >> 20;
            immediate = op_imm_32(funct3, this->registers[rs1], immediate, illegal_instruction);
            if (illegal_instruction) break;
            this->set_reg(rd, immediate);
            break;
        case Opcode::OP_32: // ADDW, SUBW, SLLW, SRLW, SRAW
            immediate = op_32(funct7, funct3, this->registers[rs1], this->registers[rs2], illegal_instruction);
            this->set_reg(rd, immediate);
            break;
        case Opcode::MISC_MEM: // FENCE
            break;
        case Opcode::SYSTEM: // ECALL, EBREAK, CSR*
            immediate  = static_cast<uint32_t>(instruction & 0xfff00000) >> 20;
            illegal_instruction = this->system(immediate, rs1, rd, funct3);
            break;
        default:
            illegal_instruction = true;
            break;
    }
    if (illegal_instruction) {
        this->write_csr(CSR::mtval, instruction);
        this->write_csr(CSR::mcause, 2);
        --this->instruction_count;
        this->exception_handler();
        /*
        if (this->verbose) {
            std::cout << "Illegal instruction exception." << std::endl;
        }
        */
    }
}

uint32_t processor::fetch() {
    uint64_t doubleword = this->main_memory->read_doubleword(this->pc);
    return (this->pc & 0x4) ? upper32(doubleword) >> 32 : lower32(doubleword);
}

bool processor::system(uint32_t csr, size_t src, size_t dest, uint8_t funct3) {
    enum class Op_Type : uint8_t {
        ECALL   =   0x00,
        EBREAK  =   0x10,
        MRET    =   0x11,
        CSRRW   =   0x01,
        CSRRS   =   0x02,
        CSRRC   =   0x03,
        CSRRWI  =   0x05,
        CSRRSI  =   0x06,
        CSRRCI  =   0x07,
    };
    if (funct3 != 0) {
        // CSR* calls
        if (!valid_csr(csr)) return true;
        if (this->privilege != Privilege::Machine) return true;
    }
    auto read_only_csr = [](CSR csr){
        switch (csr) {
            case CSR::mvendorid:
            case CSR::marchid:
            case CSR::mimpid:
            case CSR::mhartid:
                return true;
            default:
                return false;
        }
    };
    Op_Type op = static_cast<Op_Type>(funct3);
    if (op == Op_Type::ECALL && csr == 1) op = Op_Type::EBREAK;
    if (op == Op_Type::ECALL && csr == 0x302) op = Op_Type::MRET;
    if (op == Op_Type::ECALL && csr != 0) return true;
    uint64_t rs1 = this->registers[src];
    uint64_t csr_val = this->read_csr(csr);
    uint64_t uimm = src;
    CSR csr_encoded = static_cast<CSR>(csr);
    switch (op) {
        case Op_Type::ECALL:
            // Causes environment-call-from-?-mode-exception
            switch (this->get_prv()) {
                case Privilege::Machine:
                    this->write_csr(CSR::mcause, 11);
                    break;
                case Privilege::User:
                    this->write_csr(CSR::mcause, 8);
                    break;
            }
            this->write_csr(CSR::mtval, 0);
            --this->instruction_count;
            this->exception_handler();
            break;
        case Op_Type::EBREAK:
            // Causes breakpoint exception
            this->write_csr(CSR::mcause, 3);
            this->write_csr(CSR::mtval, this->pc);
            --this->instruction_count;
            this->exception_handler();
            break;
        case Op_Type::MRET:
            // Requires M privilege, sets pc to value in mepc register
            switch (this->get_prv()) {
                case Privilege::Machine:
                    this->pc = this->read_csr(static_cast<uint32_t>(CSR::mepc));
                    this->update_privilege(true);
                    break;
                default:
                    return true;
            }
            break;
        case Op_Type::CSRRW:
            if (read_only_csr(csr_encoded)) return true;
            this->set_reg(dest, csr_val);
            this->write_csr(csr_encoded, rs1);
            break;
        case Op_Type::CSRRS:
            if (src != 0 && read_only_csr(csr_encoded)) return true;
            this->set_reg(dest, csr_val);
            if (src != 0) this->write_csr(csr_encoded, csr_val | rs1);
            break;
        case Op_Type::CSRRC:
            if (src != 0 && read_only_csr(csr_encoded)) return true;
            this->set_reg(dest, csr_val);
            if (src != 0) this->write_csr(csr_encoded, csr_val & ~rs1);
            break;
        case Op_Type::CSRRWI:
            if (read_only_csr(csr_encoded)) return true;
            this->set_reg(dest, csr_val);
            this->write_csr(csr_encoded, uimm);
            break;
        case Op_Type::CSRRSI:
            if (uimm != 0 && read_only_csr(csr_encoded)) return true;
            this->set_reg(dest, csr_val);
            if (uimm != 0) this->write_csr(csr_encoded, csr_val | uimm);
            break;
        case Op_Type::CSRRCI:
            if (uimm != 0 && read_only_csr(csr_encoded)) return true;
            this->set_reg(dest, csr_val);
            if (uimm != 0) this->write_csr(csr_encoded, csr_val & ~uimm);
            break;
    }
    return false;
}

void processor::update_privilege(bool mret) {
    uint64_t l_mstatus = this->read_csr(static_cast<uint32_t>(CSR::mstatus));
    uint32_t mpp = (l_mstatus >> 11) & 0x3ULL;
    uint32_t mie = (l_mstatus >> 3) & 0x1ULL;
    uint32_t mpie = (l_mstatus >> 7) & 0x1ULL;
    if (mret) {
        mie = mpie;
        this->privilege = static_cast<Privilege>(mpp);
        mpie = 1;
        mpp = static_cast<uint32_t>(Privilege::User);
    } else {
        mpie = mie;
        mie = 0;
        mpp = static_cast<uint32_t>(this->privilege);
        this->privilege = Privilege::Machine;
    }
    l_mstatus = (mpp << 11) | (mie << 3) | (mpie << 7);
    this->write_csr(CSR::mstatus, l_mstatus);
}

void processor::exception_handler() {
    /*
    if (this->verbose) {
        std::cout << "Exception called, cause = " << this->mcause << std::endl;
    }
    */
    // Set privilege to machine mode
    this->update_privilege(false);
    // Store address in mepc
    this->write_csr(CSR::mepc, this->pc);
    uint64_t base = this->mtvec & ~0x3ULL;
    if ((this->mtvec & 1) && (this->mcause >> 63ULL)) {
        // Vectored mode
        uint64_t cause = this->mcause << 2;
        base &= ~0xfULL;
        /*
        if (this->verbose) {
            std::cout << "Vectored mode: " << base << " + " << cause << std::endl;
        }
        */
        this->pc = base + cause;
    } else {
        // Direct mode
        this->pc = base;
    }
}

// Consructor
processor::processor (memory* main_memory, bool verbose, bool stage2): 
    verbose(verbose),
    instruction_count(0), 
    pc(0), 
    has_breakpoint(false),
    registers({0}),
    main_memory(main_memory),
    mstatus(0x200000000ULL),
    mie(0),
    mtvec(0),
    mscratch(0),
    mepc(0),
    mcause(0),
    mtval(0),
    mip(0),
    privilege(Privilege::Machine)
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
{
    switch (this->get_prv()) {
        case processor::Privilege::User:
            std::cout << "0 (user)" << std::endl;
            break;
        case processor::Privilege::Machine:
            std::cout << "3 (machine)" << std::endl;
            break;
    }
}

// Set privilege level
// Empty implementation for stage 1, required for stage 2
void processor::set_prv(unsigned int prv_num) 
{
    // TODO: Error handle incorrect privilege number
    if (prv_num != 0 && prv_num != 3) return;
    this->privilege = static_cast<Privilege>(prv_num);
}

processor::Privilege processor::get_prv()
{
    // This can be inferred from context. But that's too much work for me.
    return this->privilege;
}

// Display CSR value
// Empty implementation for stage 1, required for stage 2
void processor::show_csr(unsigned int csr_num)
{
    if (valid_csr(csr_num)) {
        std::cout << std::setw(16) << std::setfill('0') << std::hex << this->read_csr(csr_num) << '\n';
    } else {
        std::cout << "Illegal CSR number" << std::endl;
    }
}

void processor::write_csr(CSR csr, uint64_t new_value) {
    // Mask inputs
    switch (csr) {
        case CSR::mip:
            new_value = (new_value & 0x111ULL) | (this->mip & 0x888ULL);
            break;
        default:
            break;
    }
    this->set_csr(static_cast<uint32_t>(csr), new_value);
}

// Set CSR to new value
// Empty implementation for stage 1, required for stage 2
void processor::set_csr(unsigned int csr_num, uint64_t new_value) 
{
    // WPRI = Writes Preserve, Reads Ignore
    // WLRL = Write Legal, Read Legal
    // WARL = Write Any, Read Legal
    uint64_t mask;
    uint64_t fixed;
    CSR csr = static_cast<CSR>(csr_num);
    switch (csr) {
        case CSR::mvendorid:
        case CSR::marchid:
        case CSR::mimpid:
        case CSR::mhartid:
            std::cout << "Illegal write to read-only CSR" << std::endl;
            break;
        case CSR::mstatus:
            // mie, mpie, mpp implemented
            // uxl fixed at 2
            // all others fixed at 0
            fixed   = 2ULL << 32ULL;
            mask    = 0x1888ULL | fixed;
            //mask  = 0b1000000000000000000001100010001000ULL;
            //fixed = 0b1000000000000000000000000000000000ULL;
            this->mstatus = (new_value & mask) | fixed;
            break;
        case CSR::misa:
            // Legal to write to, but value remains fixed
            break;
        case CSR::mie:
            // usie, msie, utie, mtie, ueie, meie implemented
            // all others fixed at 0
            mask    = 0x999ULL;
            //mask  = 0b100110011001ULL;
            this->mie = new_value & mask;
            break;
        case CSR::mtvec:
            fixed = new_value & 1;
            mask = fixed ? ~0xfe : ~0x2;
            this->mtvec = (new_value & mask) | fixed;
            break;
        case CSR::mscratch:
            this->mscratch = new_value;
            break;
        case CSR::mepc:
            mask    = ~0x3ULL;
            //mask  = ~0b11ULL;
            this->mepc = new_value & mask;
            break;
        case CSR::mcause:
            // interrupt bit, 4-bit exception code field implemented
            // all others fixed at 0
            mask    = 0x800000000000000fULL;
            this->mcause = new_value & mask;
            break;
        case CSR::mtval:
            this->mtval = new_value;
            break;
        case CSR::mip:
            // usip, msip, utip, mtip, ueip, meip implemented
            // all others fixed at 0
            mask  = 0x999ULL;
            //mask  = 0b100110011001ULL;
            this->mip = new_value & mask;
            break;
    };
}

uint64_t processor::read_csr(uint32_t csr) {
    switch (static_cast<CSR>(csr)) {
        case CSR::mvendorid:
            return 0ULL;
            break;
        case CSR::marchid:
            return 0ULL;
            break;
        case CSR::mimpid:
            return 0x2024020000000000ULL;
            break;
        case CSR::mhartid:
            return 0ULL;
            break;
        case CSR::mstatus:
            return this->mstatus;
            break;
        case CSR::misa:
            return 0x8000000000100100ULL;
            break;
        case CSR::mie:
            return this->mie;
            break;
        case CSR::mtvec:
            return this->mtvec;
            break;
        case CSR::mscratch:
            return this->mscratch;
            break;
        case CSR::mepc:
            return this->mepc;
            break;
        case CSR::mcause:
            return this->mcause;
            break;
        case CSR::mtval:
            return this->mtval;
            break;
        case CSR::mip:
            return this->mip;
            break;
        default:
            return 0ULL;
    };
}

uint64_t processor::get_instruction_count() {
    return instruction_count;
}

// Used for Postgraduate assignment. Undergraduate assignment can return 0.
uint64_t processor::get_cycle_count() {
    return 0;
}
