# RISC-V Pipeline Simulator

A C++ implementation of a **5-stage RISC-V pipeline simulator** supporting the **RV32I Base Integer ISA**, **Compressed (C) Extension**, and partial **Floating-Point (F) Extension**. The simulator performs instruction decoding, pipeline execution, hazard detection, forwarding, and branch handling while providing human-readable instruction disassembly.

---

## Features

- RV32I instruction decoding
- Support for the RISC-V Compressed (C) Extension
- Partial support for Floating-Point (F) instructions
- Five-stage pipeline simulation
  - Instruction Fetch (IF)
  - Instruction Decode (ID)
  - Execute (EX)
  - Memory Access (MEM)
  - Write Back (WB)
- Pipeline hazard detection
- Data forwarding
- Pipeline stalls and flushes
- Branch and jump handling
- Human-readable assembly generation
- Instruction execution visualization

---

## Supported Instruction Formats

### RV32I

- **R-Type**
  - ADD
  - SUB
  - SLL
  - SLT
  - SLTU
  - XOR
  - SRL
  - SRA
  - OR
  - AND

- **I-Type**
  - ADDI
  - SLTI
  - SLTIU
  - XORI
  - ORI
  - ANDI
  - SLLI
  - SRLI
  - SRAI
  - JALR
  - LB
  - LH
  - LW
  - LBU
  - LHU

- **S-Type**
  - SB
  - SH
  - SW

- **B-Type**
  - BEQ
  - BNE
  - BLT
  - BGE
  - BLTU
  - BGEU

- **U-Type**
  - LUI
  - AUIPC

- **J-Type**
  - JAL

---

### Compressed (C) Extension

Includes decoding support for common compressed instructions such as

- C.ADDI
- C.LW
- C.SW
- C.BEQZ
- C.BNEZ

---

### Floating-Point (Partial)

Basic floating-point register support and instruction decoding.

---

## Pipeline Architecture

```
                +-----------+
                |    IF     |
                +-----------+
                      |
                      v
                +-----------+
                |    ID     |
                +-----------+
                      |
                      v
                +-----------+
                |    EX     |
                +-----------+
                      |
                      v
                +-----------+
                |   MEM     |
                +-----------+
                      |
                      v
                +-----------+
                |    WB     |
                +-----------+
```

The simulator models a classic 5-stage RISC-V pipeline while handling

- Data hazards
- Control hazards
- Pipeline forwarding
- Pipeline stalls
- Pipeline flushing after branches

---

## Instruction Decoding

Each instruction is decoded by extracting

- Opcode
- Destination Register (rd)
- Source Registers (rs1, rs2)
- funct3
- funct7
- Immediate values

The decoder reconstructs immediates according to the instruction format and generates a readable assembly representation.

Example:

Machine Code

```
0x00A50533
```

Decoded Assembly

```
ADD x10, x10, x5
```

---

## Register File

- 32 Integer Registers
- 32 Floating-Point Registers

---

## Hazard Handling

The simulator implements

- RAW hazard detection
- Forwarding from later pipeline stages
- Load-use stalls
- Pipeline flushing for branches and jumps

---

## Building

Compile using any C++17 compatible compiler.

```bash
g++ Final.cpp -o riscv_simulator
```

Run

```bash
./riscv_simulator
```

---

## Future Improvements

- Complete RV32F implementation
- RV32M Extension (Multiplication and Division)
- Cache simulation
- Branch prediction
- Memory hierarchy simulation
- Performance statistics
- Pipeline visualization

---

## Learning Outcomes

This project demonstrates concepts in

- Computer Architecture
- Processor Design
- Instruction Set Architecture (ISA)
- RTL-inspired datapath modeling
- Pipeline execution
- Hazard detection
- Instruction decoding
- Computer Organization

---

## Changes to V2
Branch/Jump/Link addresses now use per‑stage PC values
Branches, jal, and jalr calculate targets and return addresses correctly, instead of using a later fetch PC that was off by 4 bytes.

Forwarding priority is fixed
Newer results from MEM stage correctly override older results from WB stage, so instructions always see the most recent producer’s value.

Byte and halfword loads/stores now use the correct byte offset
They shift by (address % 4) * 8, and stores preserve the other bytes with proper masks.

Compressed instruction detection checks the correct halfword
The code now selects the correct halfword based on PC % 4 before testing the low bits, avoiding misclassification.

Alignment checks are added for halfword access
Halfword loads/stores now require 2‑byte alignment, just like real RISC‑V.

