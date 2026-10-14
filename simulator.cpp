// RISC-V 5-stage pipeline simulator
// Canonical source: v1 (Final.cpp) plus v2 correctness fixes (per-stage PC,
// forwarding priority, sub-word memory, compressed halfword fetch).
#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include <cstdint>
#include <sstream>
#include <cmath>

struct Instruction {
    uint32_t raw_data;
    uint32_t operation_code;
    uint32_t destination;
    uint32_t function3;
    uint32_t source1;
    uint32_t source2;
    uint32_t function7;
    int32_t immediate;
    std::string instruction_type;
    std::string instruction_name;

    uint32_t operand_a = 0;
    uint32_t operand_b = 0;
    float operand_fa = 0.0f;
    float operand_fb = 0.0f;

    Instruction(uint32_t data = 0) : raw_data(data) {
        decode_instruction();
    }

    void decode_instruction() {
        operation_code = raw_data & 0x7F;
        destination = (raw_data >> 7) & 0x1F;
        function3 = (raw_data >> 12) & 0x7;
        source1 = (raw_data >> 15) & 0x1F;
        source2 = (raw_data >> 20) & 0x1F;
        function7 = (raw_data >> 25) & 0x7F;

        if ((raw_data & 0x3) != 0x3) {
            instruction_type = "Compressed";
            switch (operation_code) {
                case 0x0:
                    if (function3 == 0x2) {
                        instruction_name = "CompressedLoadWord";
                        immediate = ((raw_data >> 5) & 0x1) << 6 | ((raw_data >> 10) & 0x7) << 3 | ((raw_data >> 6) & 0x1) << 2;
                        destination = 8 + ((raw_data >> 2) & 0x7);
                        source1 = 8 + ((raw_data >> 7) & 0x7);
                    } else if (function3 == 0x6) {
                        instruction_name = "CompressedStoreWord";
                        immediate = ((raw_data >> 5) & 0x1) << 6 | ((raw_data >> 10) & 0x7) << 3 | ((raw_data >> 6) & 0x1) << 2;
                        source2 = 8 + ((raw_data >> 2) & 0x7);
                        source1 = 8 + ((raw_data >> 7) & 0x7);
                    }
                    break;
                case 0x1:
                    if (function3 == 0x0) {
                        instruction_name = "CompressedAddImmediate";
                        immediate = ((raw_data >> 12) & 0x1) << 5 | ((raw_data >> 2) & 0x1F);
                        if (immediate & 0x20) immediate |= 0xFFFFFFC0;
                        destination = (raw_data >> 7) & 0x1F;
                        source1 = destination;
                    } else if (function3 == 0x6) {
                        instruction_name = "CompressedBranchEqualZero";
                        immediate = ((raw_data >> 12) & 0x1) << 8 | ((raw_data >> 5) & 0x3) << 3 | 
                                    ((raw_data >> 2) & 0x1) << 6 | ((raw_data >> 10) & 0x3) << 1;
                        if (immediate & 0x100) immediate |= 0xFFFFFE00;
                        source1 = 8 + ((raw_data >> 7) & 0x7);
                    }
                    break;
                default:
                    instruction_name = "CompressedUnknown";
                    immediate = 0;
            }
            return;
        }

        switch (operation_code) {
            case 0x33:
                instruction_type = "Register";
                if (function3 == 0x0 && function7 == 0x00) instruction_name = "Add";
                else if (function3 == 0x0 && function7 == 0x20) instruction_name = "Subtract";
                else if (function3 == 0x1) instruction_name = "ShiftLeftLogical";
                else if (function3 == 2) instruction_name = "SetLessThan";
                else if (function3 == 3) instruction_name = "SetLessThanUnsigned";
                else if (function3 == 4) instruction_name = "Xor";
                else if (function3 == 5 && function7 == 0x00) instruction_name = "ShiftRightLogical";
                else if (function3 == 5 && function7 == 0x20) instruction_name = "ShiftRightArithmetic";
                else if (function3 == 6) instruction_name = "Or";
                else if (function3 == 7) instruction_name = "And";
                else instruction_name = "RegisterType";
                immediate = 0;
                break;

            case 0x13:
                instruction_type = "Immediate";
                if (function3 == 0x0) instruction_name = "AddImmediate";
                else if (function3 == 2) instruction_name = "SetLessThanImmediate";
                else if (function3 == 3) instruction_name = "SetLessThanImmediateUnsigned";
                else if (function3 == 4) instruction_name = "XorImmediate";
                else if (function3 == 6) instruction_name = "OrImmediate";
                else if (function3 == 7) instruction_name = "AndImmediate";
                else if (function3 == 1) instruction_name = "ShiftLeftLogicalImmediate";
                else if (function3 == 5 && function7 == 0x00) instruction_name = "ShiftRightLogicalImmediate";
                else if (function3 == 5 && function7 == 0x20) instruction_name = "ShiftRightArithmeticImmediate";
                else instruction_name = "ImmediateType";
                immediate = ((int32_t)(raw_data & 0xFFF00000)) >> 20;
                break;

            case 0x03:
                instruction_type = "Immediate";
                if (function3 == 2) instruction_name = "LoadWord";
                else if (function3 == 1) instruction_name = "LoadHalf";
                else if (function3 == 0) instruction_name = "LoadByte";
                else if (function3 == 3) instruction_name = "LoadDouble";
                else if (function3 == 4) instruction_name = "LoadByteUnsigned";
                else if (function3 == 5) instruction_name = "LoadHalfUnsigned";
                else if (function3 == 6) instruction_name = "LoadWordUnsigned";
                else instruction_name = "Load";
                immediate = ((int32_t)(raw_data & 0xFFF00000)) >> 20;
                break;

            case 0x23:
                instruction_type = "Store";
                if (function3 == 2) instruction_name = "StoreWord";
                else if (function3 == 1) instruction_name = "StoreHalf";
                else if (function3 == 0) instruction_name = "StoreByte";
                else if (function3 == 3) instruction_name = "StoreDouble";
                else instruction_name = "Store";
                immediate = ((raw_data >> 25) & 0x7F) << 5 | ((raw_data >> 7) & 0x1F);
                if (immediate & 0x800) immediate |= 0xFFFFF000;
                break;

            case 0x63:
                instruction_type = "Branch";
                if (function3 == 0) instruction_name = "BranchEqual";
                else if (function3 == 1) instruction_name = "BranchNotEqual";
                else if (function3 == 4) instruction_name = "BranchLessThan";
                else if (function3 == 5) instruction_name = "BranchGreaterEqual";
                else if (function3 == 6) instruction_name = "BranchLessThanUnsigned";
                else if (function3 == 7) instruction_name = "BranchGreaterEqualUnsigned";
                else instruction_name = "Branch";
                immediate = ((raw_data >> 31) & 0x1) << 12 | ((raw_data >> 7) & 0x1) << 11 |
                            ((raw_data >> 25) & 0x3F) << 5 | ((raw_data >> 8) & 0xF) << 1;
                if (immediate & 0x1000) immediate |= 0xFFFFE000;
                break;

            case 0x6F:
                instruction_type = "Jump";
                instruction_name = "JumpAndLink";
                immediate = ((raw_data >> 31) & 0x1) << 20 | ((raw_data >> 12) & 0xFF) << 12 |
                            ((raw_data >> 20) & 0x1) << 11 | ((raw_data >> 21) & 0x3FF) << 1;
                if (immediate & 0x100000) immediate |= 0xFFF00000;
                break;

            case 0x67:
                instruction_type = "Immediate";
                instruction_name = "JumpAndLinkRegister";
                immediate = ((int32_t)(raw_data & 0xFFF00000)) >> 20;
                break;

            case 0x37:
                instruction_type = "Upper";
                instruction_name = "LoadUpperImmediate";
                immediate = raw_data & 0xFFFFF000;
                break;

            case 0x17:
                instruction_type = "Upper";
                instruction_name = "AddUpperImmediatePC";
                immediate = raw_data & 0xFFFFF000;
                break;

            case 0x73:
                instruction_type = "Immediate";
                if (function3 == 0x0 && (raw_data >> 20) == 0x0) instruction_name = "EnvironmentCall";
                else if (function3 == 0x0 && (raw_data >> 20) == 0x1) instruction_name = "EnvironmentBreak";
                else instruction_name = "System";
                immediate = 0;
                break;

            case 0x53:
                instruction_type = "Float";
                if (function7 == 0x00) instruction_name = "FloatAdd";
                else if (function7 == 0x04) instruction_name = "FloatSubtract";
                else if (function7 == 0x08) instruction_name = "FloatMultiply";
                else if (function7 == 0x0C) instruction_name = "FloatDivide";
                else instruction_name = "FloatType";
                immediate = 0;
                break;

            case 0x07:
                instruction_type = "Immediate";
                instruction_name = "FloatLoadWord";
                immediate = ((int32_t)(raw_data & 0xFFF00000)) >> 20;
                break;

            case 0x27:
                instruction_type = "Store";
                instruction_name = "FloatStoreWord";
                immediate = ((raw_data >> 25) & 0x7F) << 5 | ((raw_data >> 7) & 0x1F);
                if (immediate & 0x800) immediate |= 0xFFFFF000;
                break;

            default:
                instruction_type = "Unknown";
                instruction_name = "Unknown";
                immediate = 0;
        }
    }

    bool is_branch() const {
        return instruction_type == "Branch" || instruction_name == "CompressedBranchEqualZero";
    }

    bool is_jump() const {
        return instruction_name == "JumpAndLink" || instruction_name == "JumpAndLinkRegister";
    }

    bool is_load() const {
        return instruction_name == "LoadWord" || instruction_name == "FloatLoadWord" || instruction_name == "CompressedLoadWord" || 
               instruction_name == "LoadHalf" || instruction_name == "LoadByte" || instruction_name == "LoadDouble" || 
               instruction_name == "LoadByteUnsigned" || instruction_name == "LoadHalfUnsigned" || instruction_name == "LoadWordUnsigned";
    }

    bool is_store() const {
        return instruction_name == "StoreWord" || instruction_name == "FloatStoreWord" || instruction_name == "CompressedStoreWord" || 
               instruction_name == "StoreHalf" || instruction_name == "StoreByte" || instruction_name == "StoreDouble";
    }

    std::string to_string() const {
        std::stringstream stream;
        stream << instruction_name << " ";
        
        if (instruction_type == "Register") {
            stream << "x" << destination << ", x" << source1 << ", x" << source2;
        } else if (instruction_type == "Immediate" && (is_load() || instruction_name == "JumpAndLinkRegister")) {
            if (instruction_name == "FloatLoadWord") stream << "f" << destination << ", " << immediate << "(x" << source1 << ")";
            else if (instruction_name == "JumpAndLinkRegister") stream << "x" << destination << ", " << immediate << "(x" << source1 << ")";
            else stream << "x" << destination << ", " << immediate << "(x" << source1 << ")";
        } else if (instruction_type == "Store" || is_store()) {
            if (instruction_name == "FloatStoreWord") stream << "f" << source2 << ", " << immediate << "(x" << source1 << ")";
            else stream << "x" << source2 << ", " << immediate << "(x" << source1 << ")";
        } else if (instruction_type == "Branch") {
            stream << "x" << source1 << ", x" << source2 << ", " << immediate;
        } else if (instruction_type == "Jump") {
            stream << "x" << destination << ", " << immediate;
        } else if (instruction_type == "Upper") {
            stream << "x" << destination << ", 0x" << std::hex << immediate << std::dec;
        } else if (instruction_type == "Immediate") {
            stream << "x" << destination << ", x" << source1 << ", " << immediate;
        } else if (instruction_type == "Float") {
            stream << "f" << destination << ", f" << source1 << ", f" << source2;
        } else if (instruction_type == "Compressed") {
            if (instruction_name == "CompressedAddImmediate") stream << "x" << destination << ", " << immediate;
            else if (instruction_name == "CompressedLoadWord") stream << "x" << destination << ", " << immediate << "(x" << source1 << ")";
            else if (instruction_name == "CompressedStoreWord") stream << "x" << source2 << ", " << immediate << "(x" << source1 << ")";
            else if (instruction_name == "CompressedBranchEqualZero") stream << "x" << source1 << ", " << immediate;
            else stream << instruction_name;
        } else {
            stream << "(unknown format)";
        }
        
        return stream.str();
    }

    std::string get_execution_result() const {
        std::stringstream stream;
        
        if (instruction_name == "Add") stream << "x" << source1 << " + x" << source2;
        else if (instruction_name == "Subtract") stream << "x" << source1 << " - x" << source2;
        else if (instruction_name == "ShiftLeftLogical") stream << "x" << source1 << " << x" << source2;
        else if (instruction_name == "SetLessThan") stream << "x" << source1 << " < x" << source2;
        else if (instruction_name == "SetLessThanUnsigned") stream << "x" << source1 << " < x" << source2 << " (unsigned)";
        else if (instruction_name == "Xor") stream << "x" << source1 << " ^ x" << source2;
        else if (instruction_name == "ShiftRightLogical") stream << "x" << source1 << " >> x" << source2;
        else if (instruction_name == "ShiftRightArithmetic") stream << "x" << source1 << " >> x" << source2 << " (arithmetic)";
        else if (instruction_name == "Or") stream << "x" << source1 << " | x" << source2;
        else if (instruction_name == "And") stream << "x" << source1 << " & x" << source2;
        else if (instruction_name == "AddImmediate" || instruction_name == "CompressedAddImmediate") stream << "x" << source1 << " + " << immediate;
        else if (instruction_name == "SetLessThanImmediate") stream << "x" << source1 << " < " << immediate;
        else if (instruction_name == "SetLessThanImmediateUnsigned") stream << "x" << source1 << " < " << immediate << " (unsigned)";
        else if (instruction_name == "XorImmediate") stream << "x" << source1 << " ^ " << immediate;
        else if (instruction_name == "OrImmediate") stream << "x" << source1 << " | " << immediate;
        else if (instruction_name == "AndImmediate") stream << "x" << source1 << " & " << immediate;
        else if (instruction_name == "ShiftLeftLogicalImmediate") stream << "x" << source1 << " << " << immediate;
        else if (instruction_name == "ShiftRightLogicalImmediate") stream << "x" << source1 << " >> " << immediate;
        else if (instruction_name == "ShiftRightArithmeticImmediate") stream << "x" << source1 << " >> " << immediate << " (arithmetic)";
        else if (instruction_name == "LoadWord" || instruction_name == "CompressedLoadWord") stream << "MEM[x" << source1 << " + " << immediate << "]";
        else if (instruction_name == "StoreWord" || instruction_name == "CompressedStoreWord") stream << "MEM[x" << source1 << " + " << immediate << "] = x" << source2;
        else if (instruction_name == "BranchEqual") stream << "if (x" << source1 << " == x" << source2 << ") PC += " << immediate;
        else if (instruction_name == "BranchNotEqual") stream << "if (x" << source1 << " != x" << source2 << ") PC += " << immediate;
        else if (instruction_name == "BranchLessThan") stream << "if (x" << source1 << " < x" << source2 << ") PC += " << immediate;
        else if (instruction_name == "BranchGreaterEqual") stream << "if (x" << source1 << " >= x" << source2 << ") PC += " << immediate;
        else if (instruction_name == "BranchLessThanUnsigned") stream << "if (x" << source1 << " < x" << source2 << ") PC += " << immediate << " (unsigned)";
        else if (instruction_name == "BranchGreaterEqualUnsigned") stream << "if (x" << source1 << " >= x" << source2 << ") PC += " << immediate << " (unsigned)";
        else if (instruction_name == "CompressedBranchEqualZero") stream << "if (x" << source1 << " == 0) PC += " << immediate;
        else if (instruction_name == "JumpAndLink") stream << "PC += " << immediate;
        else if (instruction_name == "JumpAndLinkRegister") stream << "PC = x" << source1 << " + " << immediate;
        else if (instruction_name == "LoadUpperImmediate") stream << "x" << destination << " = " << immediate;
        else if (instruction_name == "FloatAdd") stream << "f" << source1 << " + f" << source2;
        else if (instruction_name == "FloatSubtract") stream << "f" << source1 << " - f" << source2;
        else if (instruction_name == "FloatMultiply") stream << "f" << source1 << " * f" << source2;
        else if (instruction_name == "FloatDivide") stream << "f" << source1 << " / f" << source2;
        else if (instruction_name == "FloatLoadWord") stream << "MEM[x" << source1 << " + " << immediate << "]";
        else if (instruction_name == "FloatStoreWord") stream << "MEM[x" << source1 << " + " << immediate << "] = f" << source2;
        else if (instruction_name == "EnvironmentCall") stream << "system call";
        else stream << "execute " << instruction_name;
        
        return stream.str();
    }
};

class CPU {
private:
    uint32_t program_counter;
    std::vector<uint32_t> memory;
    uint32_t registers[32];
    float float_registers[32];
    Instruction fetch_stage, decode_stage, execute_stage, memory_stage, writeback_stage;
    bool is_stalled, is_flushed, stall_next_cycle, is_halted;
    std::string halt_reason;
    uint32_t program_start_address, program_end_address;
    uint32_t forward_value_a, forward_value_b;
    float forward_float_a, forward_float_b;

    // Per-stage PC tracking for correct branch/jump/link calculations
    uint32_t fetch_pc, decode_pc, execute_pc, memory_pc, writeback_pc;

    uint32_t decompress_instruction(uint16_t instr) {
        if (instr == 0x0) {
            std::cerr << "Error: Zero instruction detected, likely invalid\n";
            return 0;
        }
        uint32_t result = 0;
        uint8_t opcode = instr & 0x3;
        uint8_t funct3 = (instr >> 13) & 0x7;
    
        if (opcode == 0x1 && funct3 == 0x0) {
            uint8_t rd = (instr >> 7) & 0x1F;
            int32_t imm = ((instr >> 12) & 0x1) << 5 | ((instr >> 2) & 0x1F);
            if (imm & 0x20) imm |= 0xFFFFFFC0;
            result = (imm << 20) | (rd << 15) | (0x0 << 12) | (rd << 7) | 0x13;
        } else if (opcode == 0x0 && funct3 == 0x2) {
            uint8_t rd_prime = 8 + ((instr >> 2) & 0x7);
            uint8_t rs1_prime = 8 + ((instr >> 7) & 0x7);
            uint32_t imm = ((instr >> 5) & 0x1) << 6 | ((instr >> 10) & 0x7) << 3 | ((instr >> 6) & 0x1) << 2;
            result = (imm << 20) | (rs1_prime << 15) | (0x2 << 12) | (rd_prime << 7) | 0x3;
        } else if (opcode == 0x0 && funct3 == 0x6) {
            uint8_t rs2_prime = 8 + ((instr >> 2) & 0x7);
            uint8_t rs1_prime = 8 + ((instr >> 7) & 0x7);
            uint32_t imm = ((instr >> 5) & 0x1) << 6 | ((instr >> 10) & 0x7) << 3 | ((instr >> 6) & 0x1) << 2;
            result = ((imm & 0x7C) << 25) | (rs2_prime << 20) | (rs1_prime << 15) | (0x2 << 12) |
                     ((imm & 0x3) << 7) | 0x23;
        } else if (opcode == 0x1 && funct3 == 0x6) {
            uint8_t rs1_prime = 8 + ((instr >> 7) & 0x7);
            int32_t imm = ((instr >> 12) & 0x1) << 8 | ((instr >> 2) & 0x1) << 3 |
                          ((instr >> 3) & 0x3) << 6 | ((instr >> 5) & 0x3) << 1 |
                          ((instr >> 10) & 0x3) << 4;
            if (imm & 0x100) imm |= 0xFFFFFE00;
            result = ((imm & 0x1000) << 19) | ((imm & 0x10) << 20) | (0 << 20) |
                     (rs1_prime << 15) | (0x0 << 12) | ((imm & 0xC00) >> 2) |
                     ((imm & 0x3E) << 7) | 0x63;
        } else {
            std::cerr << "Unsupported compressed instruction: 0x" << std::hex << instr << std::dec << std::endl;
            return 0;
        }
        return result;
    }

public:
    CPU() : program_counter(0), is_stalled(false), is_flushed(false), stall_next_cycle(false), is_halted(false), program_start_address(0), program_end_address(0),
            fetch_pc(0), decode_pc(0), execute_pc(0), memory_pc(0), writeback_pc(0) {
        memory.resize(4096, 0);
        for (int i = 0; i < 32; i++) {
            registers[i] = 0;
            float_registers[i] = 0.0f;
        }
        registers[0] = 0;
    }

    void load_program(const std::vector<uint32_t>& program, uint32_t address) {
        program_start_address = address;
        program_end_address = address + program.size() * 4;
        program_counter = address;
        if (program_end_address/4 > memory.size()) {
            std::cerr << "Error: Program size exceeds memory capacity\n";
            is_halted = true;
            halt_reason = "program size exceeds memory capacity";
            return;
        }
        for (size_t i = 0; i < program.size(); i++) {
            memory[address/4 + i] = program[i];
        }
    }

    void run_simulation(int max_cycles) {
        std::cout << "Cycle end:\n";
        for (int i = 0; i < max_cycles && !is_halted; i++) {
            std::cout << "Cycle start: // Cycle " << (i+1) << "\n";
            perform_writeback();
            perform_memory_access();
            perform_execution();
            perform_decode();
            perform_fetch();
            std::cout << "Cycle end:\n";
        }
        if (is_halted) {
            std::cout << "Simulation halted";
            if (!halt_reason.empty()) std::cout << ": " << halt_reason;
            std::cout << "\n";
        }
    }

    bool pipeline_empty() const {
        return fetch_stage.raw_data == 0 && decode_stage.raw_data == 0 &&
               execute_stage.raw_data == 0 && memory_stage.raw_data == 0 &&
               writeback_stage.raw_data == 0;
    }

    void perform_fetch() {
        if (is_halted) {
            fetch_stage = Instruction(0);
            std::cout << "Fetch: None (halted)\n";
            return;
        }
        if (program_counter < program_start_address) {
            fetch_stage = Instruction(0);
            std::cout << "Fetch: None (invalid PC 0x" << std::hex << program_counter << std::dec << ")\n";
            is_halted = true;
            if (halt_reason.empty()) halt_reason = "program counter ran outside the loaded program";
            return;
        }
        // Past the last instruction: insert a bubble so in-flight ops (jalr, ecall) can finish.
        if (program_counter >= program_end_address) {
            fetch_stage = Instruction(0);
            std::cout << "Fetch: None (past end of program, PC 0x" << std::hex << program_counter << std::dec << ")\n";
            if (pipeline_empty()) {
                is_halted = true;
                if (halt_reason.empty()) halt_reason = "reached end of program";
            }
            return;
        }
        if (is_stalled || stall_next_cycle) {
            fetch_stage = Instruction(0);
            std::cout << "Fetch: None (stalled)\n";
            is_stalled = false;
            stall_next_cycle = false;
        } else if (is_flushed) {
            fetch_stage = Instruction(0);
            std::cout << "Fetch: None (flushed)\n";
            is_flushed = false;
        } else {
            uint32_t index = program_counter / 4;
            if (index >= memory.size()) {
                std::cerr << "Error: Program counter 0x" << std::hex << program_counter << " out of memory bounds\n";
                is_halted = true;
                halt_reason = "program counter out of memory bounds";
                fetch_stage = Instruction(0);
                std::cout << "Fetch: None (memory error)\n";
                return;
            }
            uint32_t word = memory[index];
            if (word == 0x0) {
                std::cerr << "Error: Zero instruction at PC 0x" << std::hex << program_counter << std::dec << "\n";
                is_halted = true;
                halt_reason = "fetched a zero/invalid instruction";
                fetch_stage = Instruction(0);
                std::cout << "Fetch: None (invalid instruction)\n";
                return;
            }

            // Correct compressed detection: check the halfword that will be used
            uint16_t half = (program_counter % 4 == 0) ? (word & 0xFFFF) : (word >> 16);
            bool is_compressed = (half & 0x3) != 0x3;
            uint32_t instruction;

            if (is_compressed) {
                instruction = decompress_instruction(half);
                if (instruction == 0) {
                    is_halted = true;
                    halt_reason = "unsupported/invalid compressed instruction";
                    fetch_stage = Instruction(0);
                    std::cout << "Fetch: None (invalid compressed instruction)\n";
                    return;
                }
                fetch_pc = program_counter;   // PC of this compressed instruction
                program_counter += 2;
            } else {
                instruction = word;
                fetch_pc = program_counter;   // PC of this 32-bit instruction
                program_counter += 4;
            }
            fetch_stage = Instruction(instruction);
            std::cout << "Fetch: " << fetch_stage.to_string() << " (0x" << std::hex << instruction << std::dec << ") at PC 0x" << std::hex << fetch_pc << std::dec << "\n";
        }
    }

    void perform_decode() {
        if (is_halted) {
            decode_stage = Instruction(0);
            std::cout << "Decode: None (halted)\n";
            return;
        }
        if (is_flushed || stall_next_cycle) {
            decode_stage = Instruction(0);
            std::cout << "Decode: None (flushed)\n";
            return;
        }
        if (is_stalled) {
            std::cout << "Decode: " << decode_stage.instruction_name << "\n";
            stall_next_cycle = true;
            return;
        }
        decode_stage = fetch_stage;
        decode_pc = fetch_pc;   // propagate PC

        // Informational only; functional simulation handles hazards via stage ordering
        if (execute_stage.is_load() && execute_stage.destination != 0 && execute_stage.raw_data != 0) {
            if (execute_stage.destination == decode_stage.source1 || execute_stage.destination == decode_stage.source2) {
                std::cout << "Decode: Detected load-use hazard (handled by data forwarding/ordering)\n";
            }
        }
        
        std::cout << "Decode: " << (decode_stage.raw_data != 0 ? decode_stage.instruction_name : "None") << "\n";
    }

    // Single source of truth for "what integer value does this instruction write".
    bool compute_integer_result(const Instruction& instr, uint32_t& result) {
        const std::string& name = instr.instruction_name;
        uint32_t a = instr.operand_a;
        uint32_t b = instr.operand_b;
        uint32_t shamt_imm = instr.immediate & 0x1F;
        uint32_t shamt_reg = b & 0x1F;

        if (name == "Add") { result = a + b; return true; }
        if (name == "Subtract") { result = a - b; return true; }
        if (name == "AddImmediate" || name == "CompressedAddImmediate") { result = a + instr.immediate; return true; }
        if (name == "LoadUpperImmediate") { result = instr.immediate; return true; }
        if (name == "ShiftLeftLogicalImmediate") { result = a << shamt_imm; return true; }
        if (name == "ShiftRightLogicalImmediate") { result = a >> shamt_imm; return true; }
        if (name == "ShiftRightArithmeticImmediate") { result = (uint32_t)((int32_t)a >> shamt_imm); return true; }
        if (name == "ShiftLeftLogical") { result = a << shamt_reg; return true; }
        if (name == "ShiftRightLogical") { result = a >> shamt_reg; return true; }
        if (name == "ShiftRightArithmetic") { result = (uint32_t)((int32_t)a >> shamt_reg); return true; }
        if (name == "SetLessThan") { result = (int32_t)a < (int32_t)b ? 1 : 0; return true; }
        if (name == "SetLessThanUnsigned") { result = a < b ? 1 : 0; return true; }
        if (name == "SetLessThanImmediate") { result = (int32_t)a < instr.immediate ? 1 : 0; return true; }
        if (name == "SetLessThanImmediateUnsigned") { result = a < (uint32_t)instr.immediate ? 1 : 0; return true; }
        if (name == "Xor") { result = a ^ b; return true; }
        if (name == "Or") { result = a | b; return true; }
        if (name == "And") { result = a & b; return true; }
        if (name == "XorImmediate") { result = a ^ (uint32_t)instr.immediate; return true; }
        if (name == "OrImmediate") { result = a | (uint32_t)instr.immediate; return true; }
        if (name == "AndImmediate") { result = a & (uint32_t)instr.immediate; return true; }

        static const std::string live_from_regfile[] = {
            "LoadWord", "CompressedLoadWord", "LoadByteUnsigned", "LoadHalfUnsigned",
            "LoadWordUnsigned", "LoadByte", "LoadHalf", "LoadDouble",
            "JumpAndLink", "JumpAndLinkRegister"
        };
        for (const auto& n : live_from_regfile) {
            if (name == n) { result = registers[instr.destination]; return true; }
        }
        return false;
    }

    bool compute_float_result(const Instruction& instr, float& result) {
        const std::string& name = instr.instruction_name;
        if (name == "FloatAdd") { result = instr.operand_fa + instr.operand_fb; return true; }
        if (name == "FloatSubtract") { result = instr.operand_fa - instr.operand_fb; return true; }
        if (name == "FloatMultiply") { result = instr.operand_fa * instr.operand_fb; return true; }
        if (name == "FloatDivide") { result = instr.operand_fa / instr.operand_fb; return true; }
        if (name == "FloatLoadWord") { result = float_registers[instr.destination]; return true; }
        return false;
    }

    void perform_execution() {
        if (is_halted || is_flushed || stall_next_cycle) {
            execute_stage = Instruction(0);
            std::cout << "Execute: None\n";
            return;
        }
        execute_stage = decode_stage;
        execute_pc = decode_pc;

        forward_value_a = registers[execute_stage.source1];
        forward_value_b = registers[execute_stage.source2];
        forward_float_a = float_registers[execute_stage.source1];
        forward_float_b = float_registers[execute_stage.source2];

        // Forwarding: apply older WB stage first, then newer MEM stage so MEM wins
        if (writeback_stage.raw_data != 0) {
            uint32_t int_result;
            if (compute_integer_result(writeback_stage, int_result) && writeback_stage.destination != 0) {
                if (writeback_stage.destination == execute_stage.source1) forward_value_a = int_result;
                if (writeback_stage.destination == execute_stage.source2) forward_value_b = int_result;
            }
            float float_result;
            if (compute_float_result(writeback_stage, float_result)) {
                if (writeback_stage.destination == execute_stage.source1) forward_float_a = float_result;
                if (writeback_stage.destination == execute_stage.source2) forward_float_b = float_result;
            }
        }

        if (memory_stage.raw_data != 0) {
            uint32_t int_result;
            if (compute_integer_result(memory_stage, int_result) && memory_stage.destination != 0) {
                if (memory_stage.destination == execute_stage.source1) forward_value_a = int_result;
                if (memory_stage.destination == execute_stage.source2) forward_value_b = int_result;
            }
            float float_result;
            if (compute_float_result(memory_stage, float_result)) {
                if (memory_stage.destination == execute_stage.source1) forward_float_a = float_result;
                if (memory_stage.destination == execute_stage.source2) forward_float_b = float_result;
            }
        }

        execute_stage.operand_a = forward_value_a;
        execute_stage.operand_b = forward_value_b;
        execute_stage.operand_fa = forward_float_a;
        execute_stage.operand_fb = forward_float_b;

        bool branch_taken = false;
        if (execute_stage.is_branch()) {
            int32_t value1 = (int32_t)forward_value_a;
            uint32_t unsigned_value1 = forward_value_a;
            int32_t value2 = (int32_t)forward_value_b;
            uint32_t unsigned_value2 = forward_value_b;
            if (execute_stage.instruction_name == "BranchEqual" && value1 == value2) branch_taken = true;
            else if (execute_stage.instruction_name == "BranchNotEqual" && value1 != value2) branch_taken = true;
            else if (execute_stage.instruction_name == "BranchLessThan" && value1 < value2) branch_taken = true;
            else if (execute_stage.instruction_name == "BranchGreaterEqual" && value1 >= value2) branch_taken = true;
            else if (execute_stage.instruction_name == "BranchLessThanUnsigned" && unsigned_value1 < unsigned_value2) branch_taken = true;
            else if (execute_stage.instruction_name == "BranchGreaterEqualUnsigned" && unsigned_value1 >= unsigned_value2) branch_taken = true;
            else if (execute_stage.instruction_name == "CompressedBranchEqualZero" && value1 == 0) branch_taken = true;
            if (branch_taken) {
                uint32_t target_pc = execute_pc + execute_stage.immediate;   // correct target
                if (target_pc < program_start_address || target_pc >= program_end_address) {
                    std::cerr << "Error: Branch to invalid PC 0x" << std::hex << target_pc << std::dec << "\n";
                    is_halted = true;
                    halt_reason = "branch target outside the loaded program";
                } else {
                    program_counter = target_pc;
                    is_flushed = true;
                }
            }
        } else if (execute_stage.instruction_name == "JumpAndLink") {
            if (execute_stage.destination != 0)
                registers[execute_stage.destination] = execute_pc + 4;   // link = PC+4
            uint32_t target_pc = execute_pc + execute_stage.immediate;
            if (target_pc < program_start_address || target_pc >= program_end_address) {
                std::cerr << "Error: Jump to invalid PC 0x" << std::hex << target_pc << std::dec << "\n";
                is_halted = true;
                halt_reason = "jump target outside the loaded program";
            } else {
                program_counter = target_pc;
                is_flushed = true;
            }
        } else if (execute_stage.instruction_name == "JumpAndLinkRegister") {
            if (execute_stage.destination != 0)
                registers[execute_stage.destination] = execute_pc + 4;
            uint32_t target_pc = (forward_value_a + execute_stage.immediate) & ~1;
            if (target_pc < program_start_address || target_pc >= program_end_address) {
                std::cerr << "Error: Jump register to invalid PC 0x" << std::hex << target_pc << std::dec << "\n";
                is_halted = true;
                halt_reason = "jalr target outside the loaded program";
            } else {
                program_counter = target_pc;
                is_flushed = true;
            }
        } else if (execute_stage.instruction_name == "EnvironmentCall") {
            is_halted = true;
            halt_reason = "ecall (system call) executed";
        }
        
        std::cout << "Execute: " << (execute_stage.raw_data != 0 ? execute_stage.get_execution_result() : "None") << "\n";
    }

    void perform_memory_access() {
        if (is_halted || is_flushed || stall_next_cycle) {
            memory_stage = Instruction(0);
            std::cout << "Memory: No operation\n";
            return;
        }
        memory_stage = execute_stage;
        memory_pc = execute_pc;

        if (memory_stage.is_load()) {
            uint32_t address = memory_stage.operand_a + memory_stage.immediate;
            bool is_byte_access = memory_stage.instruction_name == "LoadByteUnsigned" || memory_stage.instruction_name == "LoadByte";
            bool is_half_access = memory_stage.instruction_name == "LoadHalfUnsigned" || memory_stage.instruction_name == "LoadHalf";
            bool is_word_access = !is_byte_access && !is_half_access;

            // Alignment checks
            if ((is_word_access && address % 4 != 0) || (is_half_access && address % 2 != 0) || address/4 >= memory.size()) {
                std::cerr << "Error: Invalid memory access at address 0x" << std::hex << address << std::dec << "\n";
                is_halted = true;
                halt_reason = (address/4 >= memory.size())
                    ? "load address out of bounds"
                    : "misaligned load address";
                std::cout << "Memory: Invalid access\n";
                return;
            }

            uint32_t word = memory[address/4];
            uint32_t shift = (address % 4) * 8;   // byte offset within word

            if (memory_stage.instruction_name == "LoadWord" || memory_stage.instruction_name == "CompressedLoadWord" ||
                memory_stage.instruction_name == "LoadWordUnsigned" || memory_stage.instruction_name == "LoadDouble") {
                registers[memory_stage.destination] = word;
                std::cout << "Memory: Load x" << memory_stage.destination << " = 0x" << std::hex << word << std::dec << " from address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "FloatLoadWord") {
                float_registers[memory_stage.destination] = *reinterpret_cast<float*>(&word);
                std::cout << "Memory: Load f" << memory_stage.destination << " = " << float_registers[memory_stage.destination] << " from address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "LoadByteUnsigned") {
                registers[memory_stage.destination] = (word >> shift) & 0xFF;
                std::cout << "Memory: Load x" << memory_stage.destination << " = 0x" << std::hex << registers[memory_stage.destination] << std::dec << " from address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "LoadHalfUnsigned") {
                registers[memory_stage.destination] = (word >> shift) & 0xFFFF;
                std::cout << "Memory: Load x" << memory_stage.destination << " = 0x" << std::hex << registers[memory_stage.destination] << std::dec << " from address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "LoadByte") {
                int32_t value = (int8_t)((word >> shift) & 0xFF);
                registers[memory_stage.destination] = (uint32_t)value;
                std::cout << "Memory: Load x" << memory_stage.destination << " = 0x" << std::hex << registers[memory_stage.destination] << std::dec << " from address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "LoadHalf") {
                int32_t value = (int16_t)((word >> shift) & 0xFFFF);
                registers[memory_stage.destination] = (uint32_t)value;
                std::cout << "Memory: Load x" << memory_stage.destination << " = 0x" << std::hex << registers[memory_stage.destination] << std::dec << " from address 0x" << std::hex << address << std::dec << "\n";
            }
        } else if (memory_stage.is_store()) {
            uint32_t address = memory_stage.operand_a + memory_stage.immediate;
            bool is_byte_access = memory_stage.instruction_name == "StoreByte";
            bool is_half_access = memory_stage.instruction_name == "StoreHalf";
            bool is_word_access = !is_byte_access && !is_half_access;

            if ((is_word_access && address % 4 != 0) || (is_half_access && address % 2 != 0) || address/4 >= memory.size()) {
                std::cerr << "Error: Invalid memory access at address 0x" << std::hex << address << std::dec << "\n";
                is_halted = true;
                halt_reason = (address/4 >= memory.size())
                    ? "store address out of bounds"
                    : "misaligned store address";
                std::cout << "Memory: Invalid access\n";
                return;
            }

            uint32_t shift = (address % 4) * 8;
            uint32_t old_word = memory[address/4];

            if (memory_stage.instruction_name == "StoreWord" || memory_stage.instruction_name == "CompressedStoreWord" ||
                memory_stage.instruction_name == "StoreDouble") {
                memory[address/4] = memory_stage.operand_b;
                std::cout << "Memory: Store x" << memory_stage.source2 << " = 0x" << std::hex << memory_stage.operand_b << std::dec << " to address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "StoreHalf") {
                uint32_t mask = 0xFFFF << shift;
                uint32_t value = (memory_stage.operand_b & 0xFFFF) << shift;
                memory[address/4] = (old_word & ~mask) | value;
                std::cout << "Memory: Store x" << memory_stage.source2 << " = 0x" << std::hex << (memory_stage.operand_b & 0xFFFF) << std::dec << " to address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "StoreByte") {
                uint32_t mask = 0xFF << shift;
                uint32_t value = (memory_stage.operand_b & 0xFF) << shift;
                memory[address/4] = (old_word & ~mask) | value;
                std::cout << "Memory: Store x" << memory_stage.source2 << " = 0x" << std::hex << (memory_stage.operand_b & 0xFF) << std::dec << " to address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "FloatStoreWord") {
                float fb = memory_stage.operand_fb;
                uint32_t value = *reinterpret_cast<uint32_t*>(&fb);
                memory[address/4] = value;
                std::cout << "Memory: Store f" << memory_stage.source2 << " = " << fb << " to address 0x" << std::hex << address << std::dec << "\n";
            }
        } else {
            std::cout << "Memory: No operation\n";
        }
    }

    void perform_writeback() {
        if (is_halted || is_flushed || stall_next_cycle) {
            writeback_stage = Instruction(0);
            std::cout << "Writeback: None\n";
            return;
        }
        writeback_stage = memory_stage;
        writeback_pc = memory_pc;

        uint32_t int_result;
        float float_result;
        bool wrote_int = false, wrote_float = false;

        if (writeback_stage.raw_data != 0) {
            if (compute_integer_result(writeback_stage, int_result) && writeback_stage.destination != 0) {
                registers[writeback_stage.destination] = int_result;
                wrote_int = true;
            }
            if (compute_float_result(writeback_stage, float_result)) {
                float_registers[writeback_stage.destination] = float_result;
                wrote_float = true;
            }
        }

        if (wrote_int) {
            std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
        } else if (wrote_float) {
            std::cout << "Writeback: Write f" << writeback_stage.destination << " = " << float_registers[writeback_stage.destination] << "\n";
        } else {
            std::cout << "Writeback: None\n";
        }
        registers[0] = 0;
    }
};

int main() {
    std::vector<uint32_t> program = {
    // Initialize registers
    0x00000093,   // addi x1, x0, 0        ; x1 = 0
    0x00000113,   // addi x2, x0, 0        ; x2 = sum = 0
    0x00000193,   // addi x3, x0, 0        ; x3 = counter = 0
    0x00500213,   // addi x4, x0, 5        ; x4 = loop limit = 5

    // Loop: sum += counter; counter++
    0x00118193,   // addi x3, x3, 1        ; counter++
    0x00310133,   // add  x2, x2, x3       ; sum += counter
    0xFE41CCE3,   // blt  x3, x4, -8       ; CORRECTED: if counter < 5, loop back

    // Store sum to memory at 0x1100
    0x000012B7,   // lui  x5, 0x1          ; x5 = 0x1000
    0x10028293,   // addi x5, x5, 0x100    ; x5 = 0x1100
    0x0022A023,   // sw   x2, 0(x5)        ; mem[0x1100] = sum

    // Load it back
    0x0002A303,   // lw   x6, 0(x5)        ; x6 = mem[0x1100]

    // Call subroutine that adds 10 to x6
    0x00C000EF,   // jal  x1, 12           ; CORRECTED: jump to 0x1038, return addr 0x1030
    0x0062A223,   // sw   x6, 4(x5)        ; CORRECTED: store x6 (sum+10) to 0x1104
    0x00000073,   // ecall                  ; halt

    // Subroutine: add 10 to x6
    0x00A30313,   // addi x6, x6, 10       ; x6 += 10
    0x00008067    // jalr x0, x1, 0        ; return to caller
};

    CPU processor;
    processor.load_program(program, 0x1000);
    processor.run_simulation(60);

    return 0;
}