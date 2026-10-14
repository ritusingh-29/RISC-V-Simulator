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
    uint32_t program_start_address, program_end_address;
    uint32_t forward_value_a, forward_value_b;
    float forward_float_a, forward_float_b;

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
    CPU() : program_counter(0), is_stalled(false), is_flushed(false), stall_next_cycle(false), is_halted(false), program_start_address(0), program_end_address(0) {
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
            std::cout << "Simulation halted due to system call or invalid program counter\n";
        }
    }

    void perform_fetch() {
        if (is_halted || program_counter < program_start_address || program_counter >= program_end_address) {
            fetch_stage = Instruction(0);
            std::cout << "Fetch: None (halted or invalid PC 0x" << std::hex << program_counter << std::dec << ")\n";
            is_halted = true;
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
                fetch_stage = Instruction(0);
                std::cout << "Fetch: None (memory error)\n";
                return;
            }
            uint32_t word = memory[index];
            if (word == 0x0) {
                std::cerr << "Error: Zero instruction at PC 0x" << std::hex << program_counter << std::dec << "\n";
                is_halted = true;
                fetch_stage = Instruction(0);
                std::cout << "Fetch: None (invalid instruction)\n";
                return;
            }
            bool is_compressed = (word & 0x3) != 0x3;
            uint32_t instruction;
    
            if (is_compressed) {
                uint16_t compressed = (program_counter % 4 == 0) ? (word & 0xFFFF) : (word >> 16);
                instruction = decompress_instruction(compressed);
                if (instruction == 0) {
                    is_halted = true;
                    fetch_stage = Instruction(0);
                    std::cout << "Fetch: None (invalid compressed instruction)\n";
                    return;
                }
                program_counter += 2;
            } else {
                instruction = word;
                program_counter += 4;
            }
            fetch_stage = Instruction(instruction);
            std::cout << "Fetch: " << fetch_stage.to_string() << " (0x" << std::hex << instruction << std::dec << ") at PC 0x" << std::hex << (program_counter - (is_compressed ? 2 : 4)) << std::dec << "\n";
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
        
        if (execute_stage.is_load() && execute_stage.destination != 0 && execute_stage.raw_data != 0) {
            if (execute_stage.destination == decode_stage.source1 || execute_stage.destination == decode_stage.source2) {
                is_stalled = true;
                std::cout << "Decode: Detected load-use hazard, stalling\n";
            }
        }
        
        std::cout << "Decode: " << (decode_stage.raw_data != 0 ? decode_stage.instruction_name : "None") << "\n";
    }

    void perform_execution() {
        if (is_halted || is_flushed || stall_next_cycle) {
            execute_stage = Instruction(0);
            std::cout << "Execute: None\n";
            return;
        }
        execute_stage = decode_stage;

        forward_value_a = registers[execute_stage.source1];
        forward_value_b = registers[execute_stage.source2];
        forward_float_a = float_registers[execute_stage.source1];
        forward_float_b = float_registers[execute_stage.source2];

        if (memory_stage.raw_data != 0 && memory_stage.destination != 0) {
            if (memory_stage.destination == execute_stage.source1) {
                if (memory_stage.instruction_name == "Add") forward_value_a = registers[memory_stage.source1] + registers[memory_stage.source2];
                else if (memory_stage.instruction_name == "Subtract") forward_value_a = registers[memory_stage.source1] - registers[memory_stage.source2];
                else if (memory_stage.instruction_name == "AddImmediate" || memory_stage.instruction_name == "CompressedAddImmediate") forward_value_a = registers[memory_stage.source1] + memory_stage.immediate;
                else if (memory_stage.instruction_name == "LoadUpperImmediate") forward_value_a = memory_stage.immediate;
                else if (memory_stage.instruction_name == "LoadWord" || memory_stage.instruction_name == "CompressedLoadWord") {
                    uint32_t addr = registers[memory_stage.source1] + memory_stage.immediate;
                    if (addr/4 < memory.size()) forward_value_a = memory[addr/4];
                }
                else if (memory_stage.instruction_name == "FloatAdd") forward_float_a = float_registers[memory_stage.source1] + float_registers[memory_stage.source2];
                else if (memory_stage.instruction_name == "FloatSubtract") forward_float_a = float_registers[memory_stage.source1] - float_registers[memory_stage.source2];
                else if (memory_stage.instruction_name == "FloatMultiply") forward_float_a = float_registers[memory_stage.source1] * float_registers[memory_stage.source2];
                else if (memory_stage.instruction_name == "FloatDivide") forward_float_a = float_registers[memory_stage.source1] / float_registers[memory_stage.source2];
            }
            if (memory_stage.destination == execute_stage.source2) {
                if (memory_stage.instruction_name == "Add") forward_value_b = registers[memory_stage.source1] + registers[memory_stage.source2];
                else if (memory_stage.instruction_name == "Subtract") forward_value_b = registers[memory_stage.source1] - registers[memory_stage.source2];
                else if (memory_stage.instruction_name == "AddImmediate" || memory_stage.instruction_name == "CompressedAddImmediate") forward_value_b = registers[memory_stage.source1] + memory_stage.immediate;
                else if (memory_stage.instruction_name == "LoadUpperImmediate") forward_value_b = memory_stage.immediate;
                else if (memory_stage.instruction_name == "LoadWord" || memory_stage.instruction_name == "CompressedLoadWord") {
                    uint32_t addr = registers[memory_stage.source1] + memory_stage.immediate;
                    if (addr/4 < memory.size()) forward_value_b = memory[addr/4];
                }
                else if (memory_stage.instruction_name == "FloatAdd") forward_float_b = float_registers[memory_stage.source1] + float_registers[memory_stage.source2];
                else if (memory_stage.instruction_name == "FloatSubtract") forward_float_b = float_registers[memory_stage.source1] - float_registers[memory_stage.source2];
                else if (memory_stage.instruction_name == "FloatMultiply") forward_float_b = float_registers[memory_stage.source1] * float_registers[memory_stage.source2];
                else if (memory_stage.instruction_name == "FloatDivide") forward_float_b = float_registers[memory_stage.source1] / float_registers[memory_stage.source2];
            }
        }

        if (writeback_stage.raw_data != 0 && writeback_stage.destination != 0) {
            if (writeback_stage.destination == execute_stage.source1) {
                if (writeback_stage.instruction_name == "Add") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "Subtract") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "AddImmediate" || writeback_stage.instruction_name == "CompressedAddImmediate") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "LoadUpperImmediate") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "LoadWord" || writeback_stage.instruction_name == "CompressedLoadWord") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftRightLogicalImmediate") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftRightArithmeticImmediate") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftLeftLogicalImmediate") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "SetLessThan") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "SetLessThanUnsigned") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "SetLessThanImmediate") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "SetLessThanImmediateUnsigned") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "XorImmediate") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "OrImmediate") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "AndImmediate") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "Xor") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "Or") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "And") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftLeftLogical") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftRightLogical") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftRightArithmetic") forward_value_a = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "FloatAdd") forward_float_a = float_registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "FloatSubtract") forward_float_a = float_registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "FloatMultiply") forward_float_a = float_registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "FloatDivide") forward_float_a = float_registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "FloatLoadWord") forward_float_a = float_registers[writeback_stage.destination];
            }
            if (writeback_stage.destination == execute_stage.source2) {
                if (writeback_stage.instruction_name == "Add") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "Subtract") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "AddImmediate" || writeback_stage.instruction_name == "CompressedAddImmediate") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "LoadUpperImmediate") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "LoadWord" || writeback_stage.instruction_name == "CompressedLoadWord") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftRightLogicalImmediate") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftRightArithmeticImmediate") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftLeftLogicalImmediate") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "SetLessThan") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "SetLessThanUnsigned") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "SetLessThanImmediate") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "SetLessThanImmediateUnsigned") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "XorImmediate") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "OrImmediate") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "AndImmediate") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "Xor") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "Or") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "And") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftLeftLogical") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftRightLogical") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "ShiftRightArithmetic") forward_value_b = registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "FloatAdd") forward_float_b = float_registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "FloatSubtract") forward_float_b = float_registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "FloatMultiply") forward_float_b = float_registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "FloatDivide") forward_float_b = float_registers[writeback_stage.destination];
                else if (writeback_stage.instruction_name == "FloatLoadWord") forward_float_b = float_registers[writeback_stage.destination];
            }
        }

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
                uint32_t target_pc = program_counter - 4 + execute_stage.immediate;
                if (target_pc < program_start_address || target_pc >= program_end_address) {
                    std::cerr << "Error: Branch to invalid PC 0x" << std::hex << target_pc << std::dec << "\n";
                    is_halted = true;
                } else {
                    program_counter = target_pc;
                    is_flushed = true;
                }
            }
        } else if (execute_stage.instruction_name == "JumpAndLink") {
            if (execute_stage.destination != 0) registers[execute_stage.destination] = program_counter;
            uint32_t target_pc = program_counter - 4 + execute_stage.immediate;
            if (target_pc < program_start_address || target_pc >= program_end_address) {
                std::cerr << "Error: Jump to invalid PC 0x" << std::hex << target_pc << std::dec << "\n";
                is_halted = true;
            } else {
                program_counter = target_pc;
                is_flushed = true;
            }
        } else if (execute_stage.instruction_name == "JumpAndLinkRegister") {
            if (execute_stage.destination != 0) registers[execute_stage.destination] = program_counter;
            uint32_t target_pc = (forward_value_a + execute_stage.immediate) & ~1;
            if (target_pc < program_start_address || target_pc >= program_end_address) {
                std::cerr << "Error: Jump register to invalid PC 0x" << std::hex << target_pc << std::dec << "\n";
                is_halted = true;
            } else {
                program_counter = target_pc;
                is_flushed = true;
            }
        } else if (execute_stage.instruction_name == "EnvironmentCall") {
            is_halted = true;
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

        if (memory_stage.is_load()) {
            uint32_t address = forward_value_a + memory_stage.immediate;
            if ((memory_stage.instruction_name != "LoadByteUnsigned" && address % 4 != 0) || address/4 >= memory.size()) {
                std::cerr << "Error: Invalid memory access at address 0x" << std::hex << address << std::dec << "\n";
                is_halted = true;
                std::cout << "Memory: Invalid access\n";
                return;
            }
            if (memory_stage.instruction_name == "LoadWord" || memory_stage.instruction_name == "CompressedLoadWord") {
                registers[memory_stage.destination] = memory[address/4];
                std::cout << "Memory: Load x" << memory_stage.destination << " = 0x" << std::hex << memory[address/4] << std::dec << " from address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "FloatLoadWord") {
                uint32_t value = memory[address/4];
                float_registers[memory_stage.destination] = *reinterpret_cast<float*>(&value);
                std::cout << "Memory: Load f" << memory_stage.destination << " = " << float_registers[memory_stage.destination] << " from address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "LoadByteUnsigned") {
                registers[memory_stage.destination] = memory[address/4] & 0xFF;
                std::cout << "Memory: Load x" << memory_stage.destination << " = 0x" << std::hex << registers[memory_stage.destination] << std::dec << " from address 0x" << std::hex << address << std::dec << "\n";
            }
        } else if (memory_stage.is_store()) {
            uint32_t address = forward_value_a + memory_stage.immediate;
            if (address % 4 != 0 || address/4 >= memory.size()) {
                std::cerr << "Error: Invalid memory access at address 0x" << std::hex << address << std::dec << "\n";
                is_halted = true;
                std::cout << "Memory: Invalid access\n";
                return;
            }
            if (memory_stage.instruction_name == "StoreWord" || memory_stage.instruction_name == "CompressedStoreWord") {
                memory[address/4] = forward_value_b;
                std::cout << "Memory: Store x" << memory_stage.source2 << " = 0x" << std::hex << forward_value_b << std::dec << " to address 0x" << std::hex << address << std::dec << "\n";
            } else if (memory_stage.instruction_name == "FloatStoreWord") {
                uint32_t value = *reinterpret_cast<uint32_t*>(&forward_float_b);
                memory[address/4] = value;
                std::cout << "Memory: Store f" << memory_stage.source2 << " = " << forward_float_b << " to address 0x" << std::hex << address << std::dec << "\n";
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

        if (writeback_stage.raw_data != 0 && writeback_stage.destination != 0) {
            if (writeback_stage.instruction_name == "Add") {
                registers[writeback_stage.destination] = forward_value_a + forward_value_b;
                std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
            } else if (writeback_stage.instruction_name == "Subtract") {
                registers[writeback_stage.destination] = forward_value_a - forward_value_b;
                std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
            } else if (writeback_stage.instruction_name == "AddImmediate" || writeback_stage.instruction_name == "CompressedAddImmediate") {
                registers[writeback_stage.destination] = forward_value_a + writeback_stage.immediate;
                std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
            } else if (writeback_stage.instruction_name == "LoadUpperImmediate") {
                registers[writeback_stage.destination] = writeback_stage.immediate;
                std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
            } else if (writeback_stage.instruction_name == "LoadWord" || writeback_stage.instruction_name == "CompressedLoadWord") {
                std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
            } else if (writeback_stage.instruction_name == "LoadByteUnsigned") {
                std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
            } else if (writeback_stage.instruction_name == "ShiftRightLogicalImmediate") {
                registers[writeback_stage.destination] = forward_value_a >> (writeback_stage.immediate & 0x1F);
                std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
            } else if (writeback_stage.instruction_name == "ShiftRightArithmeticImmediate") {
                registers[writeback_stage.destination] = (int32_t)forward_value_a >> (writeback_stage.immediate & 0x1F);
                std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
            } else if (writeback_stage.instruction_name == "ShiftLeftLogicalImmediate") {
                registers[writeback_stage.destination] = forward_value_a << (writeback_stage.immediate & 0x1F);
                std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
            } else if (writeback_stage.instruction_name == "SetLessThan") {
                registers[writeback_stage.destination] = (int32_t)forward_value_a < (int32_t)forward_value_b ? 1 : 0;
                std::cout << "Writeback: Write x" << writeback_stage.destination << " = 0x" << std::hex << registers[writeback_stage.destination] << std::dec << "\n";
            } else {
                std::cout << "Writeback: None\n";
            }
        } else {
            std::cout << "Writeback: None\n";
        }
        registers[0] = 0;
    }
};
int main() {
    std::vector<uint32_t> program = {
        0x10000093, 0x00500513, 0x10000537, 0x00A00593, 0x00A0A023, 0x00A0A223, 0x3F800093, 0x0010A623,
        0x10000093, 0x00C0A507, 0x00B50433, 0x40B504B3, 0x00B51533, 0x00B52633, 0x00B53733, 0x00B54833,
        0x00B55933, 0x40B55A33, 0x00B56B33, 0x00B57C33, 0x00A58593, 0x00A5A613, 0x00A5B693, 0x00A5C703,
        0x00A5D783, 0x00A5E803, 0x00159593, 0x0015D613, 0x4015D693, 0x0040A103, 0x0080A183, 0x0080A203,
        0x0080A303, 0x0080A383, 0x0080A403, 0x0080A483, 0x00008567, 0x00A0A823, 0x00A09A23, 0x00A08B23,
        0x00A0AC23, 0x00A0AEA7, 0x00B50663, 0x00B516E3, 0x00B52763, 0x00B537E3, 0x00B54863, 0x00B558E3,
        0x008000EF, 0x10000537, 0x10000597, 0x00A50553, 0x00A515D3, 0x00A52653, 0x00A536D3, 0x00004104,
        0x00002408, 0x0000640C, 0x0000C410, 0x00000073, 0x00100073
    };

    
    CPU processor;
    processor.load_program(program, 0x1000);
    processor.run_simulation(60);

    return 0;
}