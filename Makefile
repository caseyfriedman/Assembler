CC = g++
CCFLAGS = -g


asm: ASM.cpp ASMParser.cpp Instruction.cpp Opcode.cpp RegisterTable.cpp
	$(CC) $(CCFLAGS) $^ -o $@

